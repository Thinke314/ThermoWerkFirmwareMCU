#include "wifi_manager.h"

#include <string.h>

#include "app_state.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define THERMOWORK_AP_SSID "ThermoWerk-Setup"
#define THERMOWORK_AP_PASS "thermowerk"
#define THERMOWORK_AP_CHANNEL 6
#define THERMOWORK_AP_MAX_CONN 4

static const char *TAG = "tw_wifi";
static bool g_started = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "Client connected to local setup AP");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "Client disconnected from local setup AP");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        thermowork_app_state_set_wifi_connected(true);
        ESP_LOGI(TAG, "STA connected");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        thermowork_app_state_set_wifi_connected(false);
        ESP_LOGW(TAG, "STA disconnected");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        thermowork_app_state_set_wifi_connected(true);
        ESP_LOGI(TAG, "STA got IP");
    }
}

void thermowork_wifi_manager_start(void)
{
    if (g_started) {
        return;
    }

    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(nvs_result);
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t ap_config = {0};
    strncpy((char *)ap_config.ap.ssid, THERMOWORK_AP_SSID, sizeof(ap_config.ap.ssid));
    strncpy((char *)ap_config.ap.password, THERMOWORK_AP_PASS, sizeof(ap_config.ap.password));
    ap_config.ap.ssid_len = strlen(THERMOWORK_AP_SSID);
    ap_config.ap.channel = THERMOWORK_AP_CHANNEL;
    ap_config.ap.max_connection = THERMOWORK_AP_MAX_CONN;
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    if (strlen(THERMOWORK_AP_PASS) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    g_started = true;
    ESP_LOGI(TAG, "Local AP started: SSID=%s password=%s", THERMOWORK_AP_SSID, THERMOWORK_AP_PASS);
}
