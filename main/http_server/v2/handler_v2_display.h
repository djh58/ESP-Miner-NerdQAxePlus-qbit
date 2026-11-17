#pragma once
#include "esp_http_server.h"

// qbit edition: remote view of the miner's display.
//   GET  /api/v2/display/screenshot  the active screen as a 24-bit BMP
//   POST /api/v2/display/button      presses the first button (next screen)
esp_err_t GET_V2_display_screenshot(httpd_req_t *req);
esp_err_t POST_V2_display_button(httpd_req_t *req);
