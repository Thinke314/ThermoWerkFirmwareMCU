#include "cloud_client.h"

#include <stdio.h>
#include <string.h>

#include "app_state.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "tw_cloud";
static SemaphoreHandle_t g_lock;
static thermowork_cloud_config_t g_config;
static bool g_started = false;

void thermowork_cloud_client_init(void)
{
    g_lock = xSemaphoreCreateMutex();
    memset(&g_config, 0, sizeof(g_config));
    g_config.enabled = false;
    g_config.publish_interval_s = 30;
    snprintf(g_config.device_id, sizeof(g_config.device_id), "thermowerk-esp32");
    thermowork_app_state_set_cloud_state(false, false);
}

void thermowork_cloud_client_set_config(const thermowork_cloud_config_t *config)
{
    if (config == NULL) {
        return;
    }
    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_config = *config;
    if (g_config.publish_interval_s < 5) {
        g_config.publish_interval_s = 5;
    }
    xSemaphoreGive(g_lock);
    thermowork_app_state_set_cloud_state(g_config.enabled, false);
}

thermowork_cloud_config_t thermowork_cloud_client_get_config(void)
{
    thermowork_cloud_config_t copy;
    xSemaphoreTake(g_lock, portMAX_DELAY);
    copy = g_config;
    xSemaphoreGive(g_lock);
    return copy;
}

static void cloud_task(void *arg)
{
    (void)arg;
    char json[1024];

    while (true) {
        thermowork_cloud_config_t cfg = thermowork_cloud_client_get_config();
        if (!cfg.enabled || cfg.endpoint_url[0] == '\0') {
            thermowork_app_state_set_cloud_state(cfg.enabled, false);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        int len = thermowork_app_state_format_json(json, sizeof(json));
        if (len <= 0) {
            thermowork_app_state_set_cloud_state(cfg.enabled, false);
            vTaskDelay(pdMS_TO_TICKS(cfg.publish_interval_s * 1000));
            continue;
        }

        esp_http_client_config_t http_cfg = {
            .url = cfg.endpoint_url,
            .method = HTTP_METHOD_POST,
            .timeout_ms = 5000,
        };
        esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
        if (client == NULL) {
            thermowork_app_state_set_cloud_state(cfg.enabled, false);
            vTaskDelay(pdMS_TO_TICKS(cfg.publish_interval_s * 1000));
            continue;
        }

        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "X-ThermoWerk-Device", cfg.device_id);
        esp_http_client_set_post_field(client, json, len);
        esp_err_t err = esp_http_client_perform(client);
        int status = esp_http_client_get_status_code(client);
        esp_http_client_cleanup(client);

        bool ok = (err == ESP_OK && status >= 200 && status < 300);
        thermowork_app_state_set_cloud_state(cfg.enabled, ok);
        if (!ok) {
            ESP_LOGW(TAG, "Cloud publish failed err=%s http=%d", esp_err_to_name(err), status);
        }

        vTaskDelay(pdMS_TO_TICKS(cfg.publish_interval_s * 1000));
    }
}

void thermowork_cloud_client_start(void)
{
    if (g_started) {
        return;
    }
    g_started = true;
    xTaskCreate(cloud_task, "thermowork_cloud", 6144, NULL, 5, NULL);
    ESP_LOGI(TAG, "Cloud client task started");
}
