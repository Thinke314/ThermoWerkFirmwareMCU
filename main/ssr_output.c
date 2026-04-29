#include "ssr_output.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
static gpio_num_t g_gpio=GPIO_NUM_NC; static uint8_t g_d; static uint32_t g_w=1000; static bool g_s;
esp_err_t ssr_output_init(gpio_num_t gpio){g_gpio=gpio; gpio_config_t c={.pin_bit_mask=1ULL<<gpio,.mode=GPIO_MODE_OUTPUT}; ESP_ERROR_CHECK(gpio_config(&c)); gpio_set_level(gpio,0); g_s=false; g_d=0; return ESP_OK;}
void ssr_output_set_duty(uint8_t d){g_d=d>100?100:d;} void ssr_output_set_window_ms(uint32_t w){g_w=w?w:1000;} bool ssr_output_get_state(void){return g_s;} uint8_t ssr_output_get_duty(void){return g_d;}
void ssr_output_force_off(void){g_d=0; gpio_set_level(g_gpio,0); g_s=false;} esp_err_t ssr_output_set_raw_level(bool l){g_s=l; return gpio_set_level(g_gpio,l?1:0);} esp_err_t ssr_output_blink(uint8_t c,uint32_t p){for(uint8_t i=0;i<c;i++){ssr_output_set_raw_level(true); vTaskDelay(pdMS_TO_TICKS(p/2)); ssr_output_set_raw_level(false); vTaskDelay(pdMS_TO_TICKS(p/2));} return ESP_OK;}
void ssr_output_task(void *p){(void)p; while(1){ uint32_t pos=(esp_timer_get_time()/1000)%g_w; uint32_t on=(g_w*g_d)/100; bool lv=(g_d==100)||((g_d>0)&&pos<on); ssr_output_set_raw_level(lv); vTaskDelay(pdMS_TO_TICKS(10)); }}
