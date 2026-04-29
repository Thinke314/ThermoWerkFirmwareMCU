#include "config_store.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "app_config.h"
#include <string.h>
void config_store_set_defaults(thermowork_config_t *c){ memset(c,0,sizeof(*c)); c->mode=THERMOWORK_MODE_AUTO;c->enabled=false;c->heater_max_w=3500;c->manual_power_w=0;c->target_temperature_c=60;c->max_temperature_c=75;c->input_timeout_ms=10000;c->ssr_window_ms=1000;c->ssr_gpio=THERMOWORK_SSR_GPIO;c->cloud_enabled=false; }
esp_err_t config_store_init(void){ esp_err_t e=nvs_flash_init(); if(e==ESP_ERR_NVS_NO_FREE_PAGES||e==ESP_ERR_NVS_NEW_VERSION_FOUND){ESP_ERROR_CHECK(nvs_flash_erase()); e=nvs_flash_init();} return e;}
esp_err_t config_store_save(const thermowork_config_t *c){ nvs_handle_t h; ESP_RETURN_ON_ERROR(nvs_open("cfg",NVS_READWRITE,&h),"cfg",""); esp_err_t e=nvs_set_blob(h,"config",c,sizeof(*c)); if(e==ESP_OK)e=nvs_commit(h); nvs_close(h); return e;}
esp_err_t config_store_load(thermowork_config_t *c){ size_t sz=sizeof(*c); nvs_handle_t h; config_store_set_defaults(c); esp_err_t e=nvs_open("cfg",NVS_READWRITE,&h); if(e!=ESP_OK){config_store_save(c); return ESP_OK;} e=nvs_get_blob(h,"config",c,&sz); if(e!=ESP_OK||sz!=sizeof(*c)){config_store_set_defaults(c); config_store_save(c);} nvs_close(h); return ESP_OK;}
