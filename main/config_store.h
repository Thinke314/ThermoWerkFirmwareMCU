#pragma once
#include "esp_err.h"
#include "thermo_types.h"
esp_err_t config_store_init(void); esp_err_t config_store_load(thermowork_config_t *c); esp_err_t config_store_save(const thermowork_config_t *c); void config_store_set_defaults(thermowork_config_t *c);
