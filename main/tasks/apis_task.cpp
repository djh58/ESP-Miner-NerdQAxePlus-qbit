#include "apis_task.h"

#include <cstring>
#include <time.h>

#include "crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "explorer_parse.h"
#include "macros.h"
#include "nvs_config.h"

static const char *TAG = "APIsFetcher";

// API paths (appended to the configurable base URL)
#define API_PATH_BTCPRICE    "/api/v1/prices"
#define API_PATH_BLOCKHEIGHT "/api/blocks/tip/height"
#define API_PATH_GLOBALHASH  "/api/v1/mining/hashrate/3d"
#define API_PATH_GETFEES     "/api/v1/fees/recommended"

#define HTTP_TIMEOUT_MS 10000

// Constructor
APIsFetcher::APIsFetcher() {
    // Initialize mutex and condition variable
    pthread_mutex_init(&m_mutex, nullptr);
    pthread_cond_init(&m_cond, nullptr);
}

// Enable fetching - Wakes up the fetcher thread immediately
void APIsFetcher::enableFetching()
{
    pthread_mutex_lock(&m_mutex);
    m_enabled = true;
    pthread_cond_signal(&m_cond); // Wake up thread immediately
    pthread_mutex_unlock(&m_mutex);
}

// Disable fetching - Stops the fetching process
void APIsFetcher::disableFetching()
{
    m_enabled = false;
}

static std::string configuredBaseUrl()
{
    char *baseUrl = Config::isMempoolCustom() ? Config::getMempoolUrl() : strdup(CONFIG_MEMPOOL_URL);
    std::string base = baseUrl ? baseUrl : "";
    free(baseUrl);
    return base;
}

void APIsFetcher::sourceChanged()
{
    std::string base = configuredBaseUrl();
    pthread_mutex_lock(&m_mutex);
    if (base != m_source) {
        // Readings from the previous explorer must never show as the new one's.
        m_source = base;
        m_generation++;
        m_stats = NetworkStats();
        memset(m_updatedUs, 0, sizeof(m_updatedUs));
        // A qbit explorer is also recognized by its per-lane mining data.
        m_stats.qbit = base.find("qbit") != std::string::npos;
    }
    if (m_enabled) {
        pthread_cond_signal(&m_cond); // fetch from the new explorer now
    }
    pthread_mutex_unlock(&m_mutex);
}

NetworkStats APIsFetcher::getStats()
{
    pthread_mutex_lock(&m_mutex);
    NetworkStats s = m_stats;
    int64_t updated[4];
    memcpy(updated, m_updatedUs, sizeof(updated));
    pthread_mutex_unlock(&m_mutex);

    int64_t now = esp_timer_get_time();
    auto fresh = [&](ApiType t) { return updated[t] != 0 && now - updated[t] <= MAX_AGE_US; };
    s.priceKnown = s.priceKnown && fresh(APItype_PRICE);
    s.heightKnown = s.heightKnown && fresh(APItype_BLOCK_HEIGHT);
    s.hashrateKnown = s.hashrateKnown && fresh(APItype_HASHRATE);
    s.feesKnown = s.feesKnown && fresh(APItype_FEES);
    return s;
}

bool APIsFetcher::appendBody(const char *data, int len)
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
        size_t cap = m_bodyCap ? m_bodyCap : 1024;
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

// HTTP event handler
esp_err_t APIsFetcher::http_event_handler(esp_http_client_event_t *evt) {
    APIsFetcher *instance = static_cast<APIsFetcher *>(evt->user_data);
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
        case HTTP_EVENT_DISCONNECTED: {
            int errnum = esp_http_client_get_errno(evt->client);
            if (errnum) {
                ESP_LOGE("APIsFetcher", "HTTP connection error/disconnected, errno=%d, transport=%s",
                    errnum,
                    esp_http_client_get_transport_type(evt->client)==HTTP_TRANSPORT_OVER_SSL?"SSL":"TCP");
            }
            break;
        }
        case HTTP_EVENT_ON_DATA:
            // Chunked and fixed-length bodies alike; the body is dechunked here.
            instance->appendBody((const char *) evt->data, evt->data_len);
            break;
        default:
            break;
    }
    return ESP_OK;
}

// Fetch Data - Performs an HTTP request and parses the response
bool APIsFetcher::fetchData(const char* apiUrl, ApiType type, uint32_t generation)
{
    m_bodyLen = 0;
    m_bodyOverflow = false;

    esp_http_client_config_t config = {};
    config.url = apiUrl;
    config.event_handler = http_event_handler;
    config.crt_bundle_attach = crt_bundle_attach;
    config.user_data = this;
    config.timeout_ms = HTTP_TIMEOUT_MS;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client.");
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP GET %s failed: %s, errno=%d", apiUrl, esp_err_to_name(err), esp_http_client_get_errno(client));
    }
    esp_http_client_cleanup(client);
    if (err != ESP_OK) {
        return false;
    }
    if (status != 200) {
        ESP_LOGW(TAG, "HTTP GET %s returned %d", apiUrl, status);
        return false;
    }
    if (m_bodyOverflow) {
        ESP_LOGW(TAG, "HTTP GET %s: response larger than %u bytes", apiUrl, (unsigned) MAX_BODY);
        return false;
    }
    if (m_bodyLen == 0) {
        ESP_LOGE(TAG, "Empty response received!");
        return false;
    }

    NetworkStats parsed;
    bool ok = false;
    switch (type) {
        case APItype_PRICE:
            ok = explorer::parse_price_usd(m_body, m_bodyLen, &parsed.priceUsd);
            break;
        case APItype_BLOCK_HEIGHT:
            ok = explorer::parse_block_height(m_body, m_bodyLen, &parsed.blockHeight);
            break;
        case APItype_HASHRATE: {
            explorer::NetworkHashrate hr;
            ok = explorer::parse_network_hashrate(m_body, m_bodyLen, &hr);
            parsed.hashrate = hr.hashrate;
            parsed.difficulty = hr.difficulty;
            parsed.qbit = hr.qbit_lanes;
            break;
        }
        case APItype_FEES: {
            explorer::Fees fees;
            ok = explorer::parse_fees(m_body, m_bodyLen, &fees);
            parsed.feeHour = fees.hour;
            parsed.feeHalfHour = fees.half_hour;
            parsed.feeFastest = fees.fastest;
            break;
        }
    }
    if (!ok) {
        // Includes a qbit explorer's price, which has no feed (USD -1).
        ESP_LOGI(TAG, "HTTP GET %s: no usable value in the response", apiUrl);
        return false;
    }
    publish(type, parsed, generation);
    return true;
}

void APIsFetcher::publish(ApiType type, const NetworkStats &p, uint32_t generation)
{
    pthread_mutex_lock(&m_mutex);
    // Drop results from a fetch that began before the explorer changed.
    if (generation == m_generation) {
        switch (type) {
            case APItype_PRICE:
                m_stats.priceUsd = p.priceUsd;
                m_stats.priceKnown = true;
                break;
            case APItype_BLOCK_HEIGHT:
                m_stats.blockHeight = p.blockHeight;
                m_stats.heightKnown = true;
                break;
            case APItype_HASHRATE:
                m_stats.hashrate = p.hashrate;
                m_stats.difficulty = p.difficulty;
                m_stats.qbit = m_stats.qbit || p.qbit;
                m_stats.hashrateKnown = true;
                break;
            case APItype_FEES:
                m_stats.feeHour = p.feeHour;
                m_stats.feeHalfHour = p.feeHalfHour;
                m_stats.feeFastest = p.feeFastest;
                m_stats.feesKnown = true;
                break;
        }
        m_updatedUs[type] = esp_timer_get_time();
    }
    pthread_mutex_unlock(&m_mutex);
}

// FreeRTOS task wrapper (must be static)
void APIsFetcher::taskWrapper(void *pvParameters) {
    APIsFetcher *instance = static_cast<APIsFetcher *>(pvParameters);
    instance->task();
}

// Build full URL from base + path. Caller must free() the result.
static char* buildUrl(const char* base, const char* path)
{
    // Strip trailing slash from base if present
    size_t baseLen = strlen(base);
    while (baseLen > 0 && base[baseLen - 1] == '/') baseLen--;

    size_t len = baseLen + strlen(path) + 1;
    char* url = (char*) MALLOC(len);
    if (url) {
        snprintf(url, len, "%.*s%s", (int) baseLen, base, path);
    }
    return url;
}

void APIsFetcher::fetchAll()
{
    // Pick up an explorer change made outside the settings API, too.
    sourceChanged();

    pthread_mutex_lock(&m_mutex);
    std::string base = m_source;
    uint32_t generation = m_generation;
    pthread_mutex_unlock(&m_mutex);

    if (base.empty()) {
        ESP_LOGI(TAG, "Mempool URL not configured, skipping fetch");
        return;
    }

    const char* paths[] = { API_PATH_BTCPRICE, API_PATH_BLOCKHEIGHT, API_PATH_GLOBALHASH, API_PATH_GETFEES };
    const ApiType types[] = { APItype_PRICE, APItype_BLOCK_HEIGHT, APItype_HASHRATE, APItype_FEES };

    for (int i = 0; i < 4; i++) {
        char* url = buildUrl(base.c_str(), paths[i]);
        if (url) {
            fetchData(url, types[i], generation);
            free(url);
        }
    }
}

// FreeRTOS task function
void APIsFetcher::task() {
    ESP_LOGI(TAG, "APIs Fetcher started...");

    // initial fetching
    fetchAll();

    while (true) {
        pthread_mutex_lock(&m_mutex);
        pthread_cond_wait(&m_cond, &m_mutex); // Wait for enable signal
        pthread_mutex_unlock(&m_mutex);

        do{
            fetchAll();
#if 0
            UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGI(TAG, "Stack high watermark: %u bytes", watermark);
#endif
            // Wait a minute, or less when sourceChanged() asks for a fetch.
            struct timespec until;
            clock_gettime(CLOCK_REALTIME, &until);
            until.tv_sec += 60;
            pthread_mutex_lock(&m_mutex);
            pthread_cond_timedwait(&m_cond, &m_mutex, &until);
            pthread_mutex_unlock(&m_mutex);
        }while (m_enabled);
    }
}
