#pragma once

#include <pthread.h>

#include "esp_http_client.h"
#include "qbit_pool_view.h"

// Polls the qbit pool's public dashboard API for the payout address in the
// primary pool's username, for the qbit pool screen and the block toast. It
// only runs while that pool is a mainnet qbit pool and the username holds a
// valid payout address.
class QbitPoolFetcher {
  public:
    QbitPoolFetcher();

    static void taskWrapper(void *pvParameters);
    void task();

    // True while the primary pool is a qbit pool with a valid payout address.
    bool active();

    // A consistent copy of the latest readings; ones older than 10 minutes
    // are marked unknown.
    QbitPoolView getView();

    // A block the pool found since the previous call, if any (each block once).
    bool takeNewBlock(QbitPoolBlock *out);

  private:
    static constexpr size_t MAX_BODY = 16 * 1024;
    static constexpr int64_t MAX_AGE_US = 10LL * 60 * 1000000;

    pthread_mutex_t m_mutex;
    bool m_active = false;
    QbitPoolView m_view;
    int64_t m_poolUpdatedUs = 0;
    int64_t m_minerUpdatedUs = 0;
    uint32_t m_lastBlockHeight = 0; // 0 until the first reading sets the baseline
    bool m_blockPending = false;
    QbitPoolBlock m_block;

    char *m_body = nullptr;
    size_t m_bodyLen = 0;
    size_t m_bodyCap = 0;
    bool m_bodyOverflow = false;

    static esp_err_t httpEvent(esp_http_client_event_t *evt);
    bool appendBody(const char *data, int len);
    bool get(const char *url);
    void poll();
};
