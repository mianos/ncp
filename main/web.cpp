#include "web.h"
#include <cstring>
#include "esp_random.h"

static const char *TAG = "WebServer";

QueueHandle_t WebServer::async_req_queue = nullptr;
SemaphoreHandle_t WebServer::worker_ready_count = nullptr;
TaskHandle_t WebServer::worker_handles[MAX_ASYNC_REQUESTS] = {nullptr};

WebServer::WebServer() : server(nullptr) {
    start_async_req_workers();
}

WebServer::~WebServer() {
    stop();
}

esp_err_t WebServer::start() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.server_port = 80;

    // Set max_open_sockets > MAX_ASYNC_REQUESTS to allow for synchronous requests
    config.max_open_sockets = MAX_ASYNC_REQUESTS + 1;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting server!");
        return ret;
    }

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = this
    };

    httpd_uri_t long_uri = {
        .uri       = "/long",
        .method    = HTTP_GET,
        .handler   = long_async_handler,
        .user_ctx  = this
    };

    httpd_uri_t quick_uri = {
        .uri       = "/quick",
        .method    = HTTP_GET,
        .handler   = quick_handler,
        .user_ctx  = this
    };

    httpd_register_uri_handler(server, &index_uri);
    httpd_register_uri_handler(server, &long_uri);
    httpd_register_uri_handler(server, &quick_uri);

    return ESP_OK;
}

esp_err_t WebServer::stop() {
    if (server != nullptr) {
        esp_err_t ret = httpd_stop(server);
        if (ret == ESP_OK) {
            server = nullptr;
        }
        return ret;
    }
    return ESP_OK;
}

bool WebServer::is_on_async_worker_thread() {
    TaskHandle_t handle = xTaskGetCurrentTaskHandle();
    for (int i = 0; i < MAX_ASYNC_REQUESTS; ++i) {
        if (worker_handles[i] == handle) {
            return true;
        }
    }
    return false;
}

esp_err_t WebServer::submit_async_req(httpd_req_t *req, httpd_req_handler_t handler) {
    // Create a copy of the request
    httpd_req_t* copy = nullptr;
    esp_err_t err = httpd_req_async_handler_begin(req, &copy);
    if (err != ESP_OK) {
        return err;
    }

    httpd_async_req_t async_req = {
        .req = copy,
        .handler = handler,
    };

    // Check for available workers
    if (xSemaphoreTake(worker_ready_count, 0) == pdFALSE) {
        ESP_LOGE(TAG, "No workers are available");
        httpd_req_async_handler_complete(copy);
        return ESP_FAIL;
    }

    // Send the request to the worker queue
    if (xQueueSend(async_req_queue, &async_req, pdMS_TO_TICKS(100)) == pdFALSE) {
        ESP_LOGE(TAG, "Worker queue is full");
        httpd_req_async_handler_complete(copy);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t WebServer::long_async_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "uri: /long");

    if (!is_on_async_worker_thread()) {
        if (submit_async_req(req, long_async_handler) == ESP_OK) {
            return ESP_OK;
        } else {
            httpd_resp_set_status(req, "503 Service Unavailable");
            httpd_resp_sendstr(req, "<div>No workers available. Server busy.</div>");
            return ESP_OK;
        }
    }

    // Track the number of long requests
    static uint8_t req_count = 0;
    req_count++;

    // Send initial response
    char s[100];
    snprintf(s, sizeof(s), "<div>req: %u</div>\n", req_count);
    httpd_resp_sendstr_chunk(req, s);

    // Simulate long-running task
    for (int i = 0; i < 10; ++i) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        snprintf(s, sizeof(s), "<div>%u</div>\n", i);
        httpd_resp_sendstr_chunk(req, s);
    }
    httpd_resp_sendstr_chunk(req, nullptr);
    return ESP_OK;
}

void WebServer::async_req_worker_task(void *arg) {
    ESP_LOGI(TAG, "Starting async request worker task");

    while (true) {
        // Signal that a worker is ready
        xSemaphoreGive(worker_ready_count);

        // Wait for a request
        httpd_async_req_t async_req;
        if (xQueueReceive(async_req_queue, &async_req, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Invoking %s", async_req.req->uri);

            // Call the handler
            async_req.handler(async_req.req);

            // Complete the asynchronous request
            if (httpd_req_async_handler_complete(async_req.req) != ESP_OK) {
                ESP_LOGE(TAG, "Failed to complete async request");
            }
        }
    }

    vTaskDelete(nullptr);
}

void WebServer::start_async_req_workers() {
    // Create counting semaphore
    worker_ready_count = xSemaphoreCreateCounting(MAX_ASYNC_REQUESTS, 0);
    if (worker_ready_count == nullptr) {
        ESP_LOGE(TAG, "Failed to create workers counting semaphore");
        return;
    }
    async_req_queue = xQueueCreate(1, sizeof(httpd_async_req_t));
    if (async_req_queue == nullptr) {
        ESP_LOGE(TAG, "Failed to create async request queue");
        vSemaphoreDelete(worker_ready_count);
        return;
    }

    // Start worker tasks
    for (int i = 0; i < MAX_ASYNC_REQUESTS; ++i) {
        BaseType_t success = xTaskCreate(
            async_req_worker_task,
            "async_req_worker",
            ASYNC_WORKER_TASK_STACK_SIZE,
            nullptr,
            ASYNC_WORKER_TASK_PRIORITY,
            &worker_handles[i]
        );

        if (success != pdPASS) {
            ESP_LOGE(TAG, "Failed to start async request worker");
            continue;
        }
    }
}



esp_err_t WebServer::quick_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "uri: /quick");
    char s[100];
    uint32_t random_number = 0;
    esp_fill_random(&random_number, sizeof(random_number));
    snprintf(s, sizeof(s), "random: %lu\n", random_number);
    httpd_resp_sendstr(req, s);
    return ESP_OK;
}
esp_err_t WebServer::index_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "uri: /");
    const char* html = "<div><a href=\"/long\">long</a></div>"
                       "<div><a href=\"/quick\">quick</a></div>";
    httpd_resp_sendstr(req, html);
    return ESP_OK;
}

