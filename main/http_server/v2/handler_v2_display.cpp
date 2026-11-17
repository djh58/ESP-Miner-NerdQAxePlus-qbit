#include "handler_v2_display.h"

#include <cstring>

#include "global_state.h"
#include "http_cors.h"
#include "http_utils.h"
#include "macros.h"

static void putU16(uint8_t *p, uint16_t v)
{
    p[0] = v & 0xff;
    p[1] = v >> 8;
}

static void putU32(uint8_t *p, uint32_t v)
{
    putU16(p, v & 0xffff);
    putU16(p + 2, v >> 16);
}

esp_err_t GET_V2_display_screenshot(httpd_req_t *req)
{
    ConGuard g(http_server, req);

    if (is_network_allowed(req) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
    }
    if (set_cors_headers(req) != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    DisplayDriver *display = SYSTEM_MODULE.getDisplay();
    uint16_t w = 0, h = 0;
    lv_color_t *pixels = display ? display->snapshot(&w, &h) : nullptr;
    if (!pixels) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Display snapshot unavailable");
    }

    // 24-bit BMP, rows bottom-up, each padded to four bytes.
    const uint32_t rowBytes = ((uint32_t) w * 3 + 3) & ~3u;
    uint8_t header[54] = {'B', 'M'};
    putU32(header + 2, 54 + rowBytes * h);
    putU32(header + 10, 54);
    putU32(header + 14, 40);
    putU32(header + 18, w);
    putU32(header + 22, h);
    putU16(header + 26, 1);
    putU16(header + 28, 24);
    putU32(header + 34, rowBytes * h);

    uint8_t *row = (uint8_t *) MALLOC(rowBytes);
    if (!row) {
        FREE(pixels);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
    }
    httpd_resp_set_type(req, "image/bmp");
    esp_err_t err = httpd_resp_send_chunk(req, (const char *) header, sizeof(header));
    for (int y = h - 1; y >= 0 && err == ESP_OK; y--) {
        memset(row, 0, rowBytes);
        for (uint16_t x = 0; x < w; x++) {
            uint32_t c = lv_color_to32(pixels[(uint32_t) y * w + x]);
            row[x * 3 + 0] = c & 0xff;         // blue
            row[x * 3 + 1] = (c >> 8) & 0xff;  // green
            row[x * 3 + 2] = (c >> 16) & 0xff; // red
        }
        err = httpd_resp_send_chunk(req, (const char *) row, rowBytes);
    }
    FREE(row);
    FREE(pixels);
    if (err == ESP_OK) {
        err = httpd_resp_send_chunk(req, nullptr, 0);
    }
    return err;
}

esp_err_t POST_V2_display_button(httpd_req_t *req)
{
    ConGuard g(http_server, req);

    if (is_network_allowed(req) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
    }
    if (set_cors_headers(req) != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    DisplayDriver *display = SYSTEM_MODULE.getDisplay();
    if (!display) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No display");
    }
    display->pressButton1Remotely();
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"pressed\":1}");
}
