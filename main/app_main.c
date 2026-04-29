#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_config.h"
#include "config_store.h"
#include "state_manager.h"
#include "ssr_output.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "control_loop.h"
#include "cloud_client.h"
#include "ota_update.h"
static const char *TAG="ThermoWerk";
void app_main(void){ thermowork_config_t c; ESP_LOGI(TAG,"ThermoWerk ESP32-S3 starting..."); ESP_ERROR_CHECK(config_store_init()); ESP_ERROR_CHECK(config_store_load(&c)); ESP_ERROR_CHECK(state_manager_init(&c)); ESP_ERROR_CHECK(ssr_output_init(c.ssr_gpio)); ssr_output_set_window_ms(c.ssr_window_ms); ESP_LOGI(TAG,"GPIO17 initialized as SSR output"); wifi_manager_start(); ESP_LOGI(TAG,"WiFi started"); web_server_start(); ESP_LOGI(TAG,"Web server started"); ota_update_init(); xTaskCreate(control_loop_task,"control_loop",4096,NULL,5,NULL); xTaskCreate(ssr_output_task,"ssr_output",4096,NULL,6,NULL); xTaskCreate(cloud_client_task,"cloud",4096,NULL,3,NULL); ESP_LOGI(TAG,"Control loop started"); ESP_LOGI(TAG,"ThermoWerk ready"); }
