idf_component_register(
    SRCS
        "app_main.c"
        "state_manager.c"
        "wifi_manager.c"
        "web_server.c"
        "control_loop.c"
        "ssr_output.c"
        "config_store.c"
        "ota_update.c"
        "cloud_client.c"
        "modbus_map.c"
        "uart_gateway.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer esp_http_server esp_wifi esp_netif esp_event nvs_flash json
)
