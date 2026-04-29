#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
esp_err_t ssr_output_init(gpio_num_t gpio); void ssr_output_set_duty(uint8_t d); void ssr_output_set_window_ms(uint32_t w); void ssr_output_task(void *p); bool ssr_output_get_state(void); uint8_t ssr_output_get_duty(void); void ssr_output_force_off(void); esp_err_t ssr_output_set_raw_level(bool l); esp_err_t ssr_output_blink(uint8_t c,uint32_t p);
