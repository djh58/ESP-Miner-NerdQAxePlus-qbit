#include "qbit_pool_task.h"

#include <cstring>

#include "crt_bundle.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "macros.h"
#include "nvs_config.h"
#include "qbit_address.h"

static const char *TAG = "qbit_pool";

// The qbit pool's public dashboard API (mainnet).
#define QBIT_POOL_API_URL "https://www.prismpool.io/public/v1"
#define HTTP_TIMEOUT_MS 10000
#define POLL_INTERVAL_MS 60000

QbitPoolFetcher::QbitPoolFetcher()
{
    pthread_mutex_init(&m_mutex, nullptr);
}

void QbitPoolFetcher::taskWrapper(void *pvParameters)
{
    static_cast<QbitPoolFetcher *>(pvParameters)->task();
}

bool QbitPoolFetcher::active()
{
    pthread_mutex_lock(&m_mutex);
    bool a = m_active;
    pthread_mutex_unlock(&m_mutex);
    return a;
}

QbitPoolView QbitPoolFetcher::getView()
{
    pthread_mutex_lock(&m_mutex);
    QbitPoolView v = m_view;
    int64_t poolUpdated = m_poolUpdatedUs, minerUpdated = m_minerUpdatedUs;
    pthread_mutex_unlock(&m_mutex);

    int64_t now = esp_timer_get_time();
    v.poolKnown = v.poolKnown && poolUpdated && now - poolUpdated <= MAX_AGE_US;
    v.minerKnown = v.minerKnown && minerUpdated && now - minerUpdated <= MAX_AGE_US;
    v.sinceFetchSeconds = poolUpdated ? (uint32_t) ((now - poolUpdated) / 1000000) : 0;
    return v;
}

bool QbitPoolFetcher::takeNewBlock(QbitPoolBlock *out)
{
    pthread_mutex_lock(&m_mutex);
    bool pending = m_blockPending;
    if (pending) {
        *out = m_block;
        m_blockPending = false;
    }
    pthread_mutex_unlock(&m_mutex);
    return pending;
}

bool QbitPoolFetcher::appendBody(const char *data, int len)
{
    if (len <= 0 || m_bodyOverflow) {
        return !m_bodyOverflow;
    }
    size_t needed = m_bodyLen + (size_t) len + 1;
    if (needed > MAX_BODY) {
        m_bodyOverflow = true;
        return false;
    }
    if (needed > m_bodyCap) {
        size_t cap = m_bodyCap ? m_bodyCap : 2048;
        while (cap < needed) {
            cap *= 2;
        }
        if (cap > MAX_BODY) {
            cap = MAX_BODY;
        }
        char *grown = (char *) REALLOC(m_body, cap);
        if (!grown) {
            m_bodyOverflow = true;
            return false;
        }
        m_body = grown;
        m_bodyCap = cap;
    }
    memcpy(m_body + m_bodyLen, data, len);
    m_bodyLen += len;
    m_body[m_bodyLen] = '\0';
    return true;
}

esp_err_t QbitPoolFetcher::httpEvent(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        static_cast<QbitPoolFetcher *>(evt->user_data)->appendBody((const char *) evt->data, evt->data_len);
    }
    return ESP_OK;
}

bool QbitPoolFetcher::get(const char *url)
{
    m_bodyLen = 0;
    m_bodyOverflow = false;

    esp_http_client_config_t config = {};
    config.url = url;
    config.event_handler = httpEvent;
    config.crt_bundle_attach = crt_bundle_attach;
    config.user_data = this;
    config.timeout_ms = HTTP_TIMEOUT_MS;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return false;
    }
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200 || m_bodyOverflow || m_bodyLen == 0) {
        ESP_LOGW(TAG, "GET %s failed: %s, status %d%s", url, esp_err_to_name(err), status,
                 m_bodyOverflow ? ", response too large" : "");
        return false;
    }
    return true;
}

void QbitPoolFetcher::poll()
{
    char *user = Config::getStratumUser();
    char *host = Config::getStratumURL();
    uint16_t port = Config::getStratumPortNumber();
    qbit::PayoutCheck check = qbit::check_stratum_user(user, host, port);
    bool active = check.status == qbit::PayoutStatus::Ok && check.network == qbit::Network::Mainnet &&
                  qbit::pool_network(host, port) == qbit::Network::Mainnet;

    char address[sizeof(m_view.address)] = "";
    char worker[sizeof(m_view.worker)] = "";
    if (active) {
        size_t base = qbit::stratum_user_base_len(user);
        snprintf(address, sizeof(address), "%.*s", (int) base, user);
        if (user[base] == '.') {
            snprintf(worker, sizeof(worker), "%s", user + base + 1);
        }
    }
    free(user);
    free(host);

    pthread_mutex_lock(&m_mutex);
    bool changed = m_active != active || strcmp(m_view.address, address) != 0 || strcmp(m_view.worker, worker) != 0;
    if (changed) {
        // Readings for another address must never show as this one's.
        m_view = QbitPoolView();
        snprintf(m_view.address, sizeof(m_view.address), "%s", address);
        snprintf(m_view.worker, sizeof(m_view.worker), "%s", worker);
        m_poolUpdatedUs = m_minerUpdatedUs = 0;
        m_lastBlockHeight = 0;
        m_blockPending = false;
    }
    m_active = active;
    pthread_mutex_unlock(&m_mutex);

    if (!active) {
        return;
    }

    char url[192];
    qbitpool::PoolSummary pool;
    snprintf(url, sizeof(url), "%s/pool-summary", QBIT_POOL_API_URL);
    bool poolOk = get(url) && qbitpool::parse_pool_summary(m_body, m_bodyLen, &pool);

    qbitpool::MinerStats miner;
    snprintf(url, sizeof(url), "%s/miners/%s", QBIT_POOL_API_URL, address);
    bool minerOk = get(url) && qbitpool::parse_miner(m_body, m_bodyLen, worker, &miner);

    int64_t now = esp_timer_get_time();
    pthread_mutex_lock(&m_mutex);
    if (strcmp(m_view.address, address) == 0) {
        if (poolOk) {
            if (m_lastBlockHeight && pool.latestBlockHeight > m_lastBlockHeight) {
                m_block = QbitPoolBlock();
                m_block.height = pool.latestBlockHeight;
                if (minerOk) {
                    m_block.sharePercent = miner.windowPercent;
                    m_block.paid = miner.hasLastPayout && miner.lastPayoutHeight == pool.latestBlockHeight;
                    m_block.paidBits = m_block.paid ? miner.lastPayoutBits : 0;
                }
                m_blockPending = true;
            }
            m_lastBlockHeight = pool.latestBlockHeight;
            m_view.pool = pool;
            m_view.poolKnown = true;
            m_poolUpdatedUs = now;
        }
        if (minerOk) {
            m_view.miner = miner;
            m_view.minerKnown = true;
            m_minerUpdatedUs = now;
        }
    }
    pthread_mutex_unlock(&m_mutex);
}

void QbitPoolFetcher::task()
{
    ESP_LOGI(TAG, "qbit pool fetcher started");
    while (true) {
        poll();
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}
