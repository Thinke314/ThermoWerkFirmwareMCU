#pragma once
#include "thermo_types.h"
#include "esp_err.h"
esp_err_t state_manager_init(const thermowork_config_t *cfg);
void state_manager_get_inputs(thermowork_inputs_t *v);
void state_manager_set_inputs(const thermowork_inputs_t *v);
void state_manager_get_config(thermowork_config_t *v);
void state_manager_set_config(const thermowork_config_t *v);
void state_manager_get_status(thermowork_status_t *v);
void state_manager_set_status(const thermowork_status_t *v);
void state_manager_get_cloud(thermowork_cloud_status_t *v);
void state_manager_set_cloud(const thermowork_cloud_status_t *v);
