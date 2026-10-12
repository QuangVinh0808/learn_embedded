#include "ota_update.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"

static const char* TAG = "[OTA]";

static bool reboot_pending = false;
static esp_timer_handle_t reboot_timer = NULL;
// ===================== UI web ==================
static const char ota_page_html[] =
    "<!doctype html>"
    "<html lang='vi'>"
    "<head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>ESP32 OTA</title>"
    "</head>"
    "<body>"
    "<h1>Cap nhat firmware OTA</h1>"
    "<p>Chon application firmware.bin cho ESP32-S3, toi da 3 MiB.</p>"
    "<p>Khong chon bootloader.bin, partitions.bin hoac merged image.</p>"
    "<input id='file' type='file' accept='.bin'>"
    "<button id='upload'>Upload</button>"
    "<pre id='status'></pre>"

    "<script>"
    "const input = document.getElementById('file');"
    "const button = document.getElementById('upload');"
    "const status = document.getElementById('status');"

    "button.onclick = () => {"
        "const file = input.files[0];"

        "if (!file) {"
            "status.textContent = 'Hay chon firmware.bin';"
            "return;"
        "}"

        "if (file.size === 0 || file.size > 3145728) {"
            "status.textContent = 'Kich thuoc firmware khong hop le';"
            "return;"
        "}"

        "button.disabled = true;"

        "const xhr = new XMLHttpRequest();"
        "xhr.open('POST', '/ota');"
        "xhr.setRequestHeader("
            "'Content-Type', 'application/octet-stream'"
        ");"
        "xhr.timeout = 180000;"

        "xhr.upload.onprogress = (event) => {"
            "if (event.lengthComputable) {"
                "const percent = Math.round("
                    "event.loaded / event.total * 100"
                ");"
                "status.textContent = 'Dang gui: ' + percent + '%';"
            "}"
        "};"

        "xhr.onload = () => {"
            "status.textContent = "
                "'HTTP ' + xhr.status + ': ' + xhr.responseText;"
            "button.disabled = false;"
        "};"

        "xhr.onerror = () => {"
            "status.textContent = "
                "'Mat ket noi. Kiem tra board va ket noi lai Wi-Fi.';"
            "button.disabled = false;"
        "};"

        "xhr.ontimeout = () => {"
            "status.textContent = "
                "'Timeout. Kiem tra board truoc khi thu lai.';"
            "button.disabled = false;"
        "};"

        /* Gửi file trực tiếp, không đóng gói multipart/form-data. */
        "xhr.send(file);"
    "};"
    "</script>"

    "</body>"
    "</html>";

//============REBOOT=====================
static void reboot_callback(void *arg) {
    (void)arg;
    esp_restart();
}

//=============== http response ============
static esp_err_t ota_send_error(httpd_req_t *req,const char *status,const char *message) {
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "Connection", "close");

    httpd_resp_send(req, message, HTTPD_RESP_USE_STRLEN);

    return ESP_FAIL;
}


// ===================== GET /ota ===================== 

static esp_err_t ota_page_handler(httpd_req_t *req) {
    esp_err_t err = httpd_resp_set_type(req, "text/html; charset=utf-8");

    if (err != ESP_OK) {
        return err;
    }

    return httpd_resp_send(req, ota_page_html, HTTPD_RESP_USE_STRLEN);
}


// ===================== POST /ota =====================

static esp_err_t ota_upload_handler(httpd_req_t *req)
{
    /* 1. Không nhận thêm firmware khi chuẩn bị reboot. */
    if (reboot_pending) {
        return ota_send_error( req, "503 Service Unavailable", "Firmware da update. reboot phai cho xu ly");
    }
    char content_type[64]; // chua byte cua firmware

    esp_err_t err = httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type));

    if (err != ESP_OK ||strcmp(content_type, "application/octet-stream") != 0) {
        return ota_send_error(req, "415 Unsupported Media Type", "Send raw application/octet-stream.");
    }

    const esp_partition_t *running = esp_ota_get_running_partition(); // vung dang chay
    const esp_partition_t *update = esp_ota_get_next_update_partition(NULL); // vung nhan update

    if (running == NULL || update == NULL || running->address == update->address) {
        return ota_send_error(req, "500 Internal Server Error", "Khong co phan vung OTA.");
    }

    /* 4. Kiểm tra kích thước trước khi xóa/ghi flash. */
    if (req->content_len == 0 || (size_t)req->content_len > update->size) {
        return ota_send_error(req, "413 Payload Too Large","Firmware size đang bị tràn hoặc lỗi.");
    }

    ESP_LOGI( TAG, "Running: %s, update: %s, size: %u bytes", running->label, update->label, (unsigned int)req->content_len);

    /* 5. Bắt đầu OTA và chuẩn bị vùng flash đích. */
    esp_ota_handle_t ota_handle;

    err = esp_ota_begin(update, req->content_len, &ota_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin: %s", esp_err_to_name(err));

        return ota_send_error(
            req,
            "500 Internal Server Error",
            esp_err_to_name(err)
        );
    }

    /* 6. Nhận từng khối và ghi trực tiếp vào flash. */
    char buffer[1024];
    size_t received = 0;

    /* Giới hạn vòng nhận firmware trong khoảng 120 giây. */
    const int64_t deadline = esp_timer_get_time() + 120000000LL;

    while (received < (size_t)req->content_len) {
        if (esp_timer_get_time() > deadline) {
            esp_ota_abort(ota_handle);
            return ota_send_error(req, "408 Request Timeout", "Upload phai trong khoang 120s.");
        }

        size_t remaining = (size_t)req->content_len - received;

        size_t read_size =
            remaining < sizeof(buffer)
                ? remaining
                : sizeof(buffer);

        int length = httpd_req_recv(req, buffer, read_size);
        if (length == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (length <= 0) {
            esp_ota_abort(ota_handle);
            return ota_send_error(req, "400 Bad Request", "Upload đang bị gián đoạn.");
        }

        err = esp_ota_write(ota_handle, buffer, length);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "OTA write: %s", esp_err_to_name(err));

            esp_ota_abort(ota_handle);

            return ota_send_error(
                req,
                "400 Bad Request",
                esp_err_to_name(err)
            );
        }

        received += (size_t)length;
    }

    /* 7. Kết thúc OTA và kiểm tra image đã nhận. */
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA end: %s", esp_err_to_name(err));

        return ota_send_error(req, "400 Bad Request", esp_err_to_name(err));
    }

    /* 8. Chọn firmware mới cho lần boot tiếp theo. */
    err = esp_ota_set_boot_partition(update);
    if (err != ESP_OK) {
        return ota_send_error(
            req,
            "500 Internal Server Error",
            esp_err_to_name(err)
        );
    }

    reboot_pending = true;

    ESP_LOGI(
        TAG,
        "OTA complete: %u bytes written to %s",
        (unsigned int)received,
        update->label
    );

    /*
     * 9. Hẹn reboot sau 2 giây.
     * Timer vẫn chạy nếu client mất kết nối lúc nhận response.
     */
    ESP_ERROR_CHECK(
        esp_timer_start_once(reboot_timer, 2000000)
    );

    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "Connection", "close");

    return httpd_resp_send(req, "OTA OK. Reboot in 2 seconds. Reconnect Wi-Fi.", HTTPD_RESP_USE_STRLEN);
}


/* ===================== XAC NHAN FIRMWARE ===================== */

esp_err_t ota_confirm_running_app(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running == NULL) return ESP_FAIL;
    ESP_LOGI(TAG, "Running partition: %s", running->label);
    esp_ota_img_states_t state;
    esp_err_t err = esp_ota_get_state_partition(
        running,
        &state
    );
    if (err == ESP_OK &&
        state == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGI(TAG, "Dang xac nhan firmware new");
        return esp_ota_mark_app_valid_cancel_rollback();
    }
    return ESP_OK;
}


/* ===================== DANG KY ROUTES ===================== */

esp_err_t ota_register_routes(httpd_handle_t server)
{
    if (reboot_timer == NULL) {
        const esp_timer_create_args_t timer_config = {
            .callback = reboot_callback,
            .name = "ota_reboot",
        };

        esp_err_t err = esp_timer_create(
            &timer_config,
            &reboot_timer
        );

        if (err != ESP_OK) {
            return err;
        }
    }

    const httpd_uri_t page_route = {
        .uri = "/ota",
        .method = HTTP_GET,
        .handler = ota_page_handler,
        .user_ctx = NULL,
    };

    const httpd_uri_t upload_route = {
        .uri = "/ota",
        .method = HTTP_POST,
        .handler = ota_upload_handler,
        .user_ctx = NULL,
    };

    esp_err_t err = httpd_register_uri_handler(
        server,
        &page_route
    );

    if (err != ESP_OK) {
        return err;
    }

    return httpd_register_uri_handler(
        server,
        &upload_route
    );
}