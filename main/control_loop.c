#include "control_loop.h"
#include <string.h>
#include "state_manager.h"
#include "ssr_output.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
void control_loop_task(void *p){(void)p; while(1){ thermowork_inputs_t in; thermowork_config_t c; thermowork_status_t s={0}; state_manager_get_inputs(&in); state_manager_get_config(&c); if(!c.enabled||c.mode==THERMOWORK_MODE_OFF){s.fault=true; s.fault_code=THERMOWORK_FAULT_DISABLED; strcpy(s.fault_reason,"disabled");}
else if(in.temperature_c>=c.max_temperature_c){s.fault=true; s.fault_code=THERMOWORK_FAULT_OVER_TEMPERATURE; strcpy(s.fault_reason,"over_temperature");}
else if(((esp_timer_get_time()/1000)-in.last_update_ms)>(int64_t)c.input_timeout_ms){s.fault=true; s.fault_code=THERMOWORK_FAULT_INPUT_TIMEOUT; strcpy(s.fault_reason,"input_timeout");}
if(!s.fault){ if(c.mode==THERMOWORK_MODE_MANUAL) s.heater_setpoint_w=c.manual_power_w; else if(c.mode==THERMOWORK_MODE_AUTO){ s.surplus_power_w=in.grid_power_w<0?-in.grid_power_w:0; s.heater_setpoint_w=s.surplus_power_w; } if(s.heater_setpoint_w<0) s.heater_setpoint_w=0; if(s.heater_setpoint_w>c.heater_max_w) s.heater_setpoint_w=c.heater_max_w; }
uint8_t d=(c.heater_max_w>0)?(uint8_t)((s.heater_setpoint_w*100)/c.heater_max_w):0; if(c.mode==THERMOWORK_MODE_TEST)d=ssr_output_get_duty(); else ssr_output_set_duty(s.fault?0:d); s.ssr_duty_percent=ssr_output_get_duty(); s.ssr_gpio_state=ssr_output_get_state(); s.ssr_active=s.ssr_gpio_state; state_manager_set_status(&s); vTaskDelay(pdMS_TO_TICKS(500)); }}
