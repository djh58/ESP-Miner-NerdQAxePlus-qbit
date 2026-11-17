#pragma once

#include <pthread.h>
#include <string>

#include "esp_err.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "network_stats.h"

class APIsFetcher {
private:
    const char *TAG = "APIsFetcher";

    // Largest response body accepted; qbit's hashrate response is about 2 KB.
    static constexpr size_t MAX_BODY = 16 * 1024;
    // Readings older than this are shown as unknown.
    static constexpr int64_t MAX_AGE_US = 15LL * 60 * 1000000;

    enum ApiType {
        APItype_PRICE,
        APItype_BLOCK_HEIGHT,
        APItype_HASHRATE,
        APItype_FEES
    };

    NetworkStats m_stats;
    int64_t m_updatedUs[4] = {0, 0, 0, 0}; // per ApiType, esp_timer time of the last success
    std::string m_source;                  // explorer base URL the stats came from
    uint32_t m_generation = 0;             // bumped whenever m_source changes

    char *m_body = nullptr; // response body being received
    size_t m_bodyLen = 0;
    size_t m_bodyCap = 0;
    bool m_bodyOverflow = false;

    bool m_enabled = false; // Flag indicating whether fetching is enabled
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;

    // HTTP event handler to process HTTP responses
    static esp_err_t http_event_handler(esp_http_client_event_t *evt);
    bool appendBody(const char *data, int len);

    bool fetchData(const char *apiUrl, ApiType type, uint32_t generation);
    void publish(ApiType type, const NetworkStats &parsed, uint32_t generation);
    void fetchAll();

public:
    APIsFetcher();

    static void taskWrapper(void *pvParameters);

    // Main FreeRTOS task function
    void task();

    // Enables APIs fetching
    void enableFetching();

    // Disables APIs fetching
    void disableFetching();

    // A consistent copy of the latest readings, with stale ones marked unknown.
    NetworkStats getStats();

    // Call after the explorer settings change: clears readings from the old
    // explorer at once and, while fetching is enabled, fetches from the new one.
    void sourceChanged();
};
