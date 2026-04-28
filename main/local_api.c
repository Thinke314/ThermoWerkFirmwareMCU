#include "local_api.h"

#include <stdio.h>
#include <string.h>

#include "app_state.h"
#include "control.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "uart_protocol.h"

static const char *TAG = "tw_local_api";
static httpd_handle_t g_server = NULL;

static const char INDEX_HTML[] =
"<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>ThermoWerk</title><style>body{font-family:system-ui;background:#0b0f14;color:#e8eef5;margin:0;padding:24px}"
".card{background:#151b23;border:1px solid #263241;border-radius:18px;padding:18px;margin:0 0 16px;box-shadow:0 10px 30px #0005}"
"h1{margin:0 0 10px;font-size:28px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}"
".v{font-size:26px;font-weight:700}.k{color:#9fb0c3;font-size:13px}button,input,select{font:inherit;border-radius:12px;border:1px solid #334155;background:#0f1720;color:#e8eef5;padding:10px;width:100%;box-sizing:border-box}"
"button{background:#1f6feb;border:0;font-weight:700;cursor:pointer}.row{display:grid;grid-template-columns:1fr 1fr;gap:10px}@media(max-width:700px){.row{grid-template-columns:1fr}}"
"</style></head><body><h1>ThermoWerk ESP32</h1><div class='card'><div class='grid' id='status'></div></div>"
"<div class='card'><h2>Control</h2><div class='row'><select id='mode'><option value='pv_surplus'>PV Surplus</option><option value='manual_power'>Manual Power</option><option value='burst_percent'>Burst Percent</option><option value='disabled'>Disabled</option></select><input id='power' type='number' value='3500' placeholder='Nominal W'></div><br>"
"<div class='row'><input id='manual' type='number' value='0' placeholder='Manual W'><input id='percent' type='number' value='0' placeholder='Burst %'></div><br><button onclick='sendConfig()'>Apply config</button></div>"
"<div class='card'><h2>Inputs simulation</h2><div class='row'><input id='grid' type='number' value='-1000' placeholder='Grid W'><input id='pv' type='number' value='5000' placeholder='PV W'></div><br>"
"<div class='row'><input id='temp' type='number' value='45' placeholder='Tank top °C'><button onclick='sendInputs()'>Send inputs + enable</button></div><br><button onclick='estop()'>Emergency stop</button></div>"
"<script>async function j(u,o){let r=await fetch(u,o);return r.json()}function b(v){return v?'true':'false'}async function load(){let s=await j('/api/status');let e=document.getElementById('status');e.innerHTML='';for(let [k,v] of Object.entries(s)){let d=document.createElement('div');d.innerHTML='<div class=k>'+k+'</div><div class=v>'+v+'</div>';e.appendChild(d)}}"
"async function sendConfig(){await j('/api/config',{method:'POST',body:JSON.stringify({type:'config',control_mode:mode.value,power_nominal_w:+power.value,manual_power_w:+manual.value,target_power_percent:+percent.value,burst_window_ms:1000,ssr_gpio_pin:17,temperature_limit_c:85,uart_timeout_ms:3000})});load()}"
"async function sendInputs(){await j('/api/inputs',{method:'POST',body:JSON.stringify({type:'process_values',grid_power_w:+grid.value,pv_power_w:+pv.value,tank_top_c:+temp.value,tank_mid_c:+temp.value,tank_bottom_c:+temp.value,flow_line_c:+temp.value,return_line_c:+temp.value,temp_valid:true,enable:true})});load()}"
"async function estop(){await j('/api/command',{method:'POST',body:JSON.stringify({type:'command',emergency_stop:true})});load()}setInterval(load,1000);load()</script></body></html>";

static esp_err_t send_json(httpd_req_t *req, const char *json)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t index_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    char buffer[1024];
    thermowork_app_state_format_json(buffer, sizeof(buffer));
    return send_json(req, buffer);
}

static int read_body(httpd_req_t *req, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return -1;
    }
    size_t total = 0;
    while (total < req->content_len && total < buffer_size - 1) {
        int r = httpd_req_recv(req, buffer + total, buffer_size - 1 - total);
        if (r <= 0) {
            return -1;
        }
        total += (size_t)r;
    }
    buffer[total] = '\0';
    return (int)total;
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
    char body[768];
    if (read_body(req, body, sizeof(body)) < 0) {
        return send_json(req, "{\"ok\":false,\"error\":\"read_failed\"}");
    }

    thermowork_uart_message_t message;
    if (thermowork_uart_parse_line(body, &message) != THERMOWORK_UART_MSG_CONFIG) {
        return send_json(req, "{\"ok\":false,\"error\":\"invalid_config\"}");
    }

    thermowork_app_state_set_config(&message.config);
    return send_json(req, "{\"ok\":true}");
}

static esp_err_t inputs_post_handler(httpd_req_t *req)
{
    char body[768];
    if (read_body(req, body, sizeof(body)) < 0) {
        return send_json(req, "{\"ok\":false,\"error\":\"read_failed\"}");
    }

    thermowork_uart_message_t message;
    if (thermowork_uart_parse_line(body, &message) != THERMOWORK_UART_MSG_PROCESS_VALUES) {
        return send_json(req, "{\"ok\":false,\"error\":\"invalid_inputs\"}");
    }

    thermowork_app_state_set_process_values(&message.process_values);
    return send_json(req, "{\"ok\":true}");
}

static esp_err_t command_post_handler(httpd_req_t *req)
{
    char body[512];
    if (read_body(req, body, sizeof(body)) < 0) {
        return send_json(req, "{\"ok\":false,\"error\":\"read_failed\"}");
    }

    thermowork_uart_message_t message;
    if (thermowork_uart_parse_line(body, &message) != THERMOWORK_UART_MSG_COMMAND) {
        return send_json(req, "{\"ok\":false,\"error\":\"invalid_command\"}");
    }

    thermowork_process_values_t values = thermowork_app_state_get_process_values();
    if (message.command_emergency_stop) {
        values.emergency_stop = true;
        values.enable = false;
    }
    if (message.command_reset_fault) {
        values.emergency_stop = false;
    }
    if (message.command_enable_outputs) {
        values.enable = true;
    }
    thermowork_app_state_set_process_values(&values);
    return send_json(req, "{\"ok\":true}");
}

void thermowork_local_api_start(void)
{
    if (g_server != NULL) {
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&g_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start local HTTP API");
        return;
    }

    httpd_uri_t index_uri = {.uri = "/", .method = HTTP_GET, .handler = index_get_handler};
    httpd_uri_t status_uri = {.uri = "/api/status", .method = HTTP_GET, .handler = status_get_handler};
    httpd_uri_t config_uri = {.uri = "/api/config", .method = HTTP_POST, .handler = config_post_handler};
    httpd_uri_t inputs_uri = {.uri = "/api/inputs", .method = HTTP_POST, .handler = inputs_post_handler};
    httpd_uri_t command_uri = {.uri = "/api/command", .method = HTTP_POST, .handler = command_post_handler};

    httpd_register_uri_handler(g_server, &index_uri);
    httpd_register_uri_handler(g_server, &status_uri);
    httpd_register_uri_handler(g_server, &config_uri);
    httpd_register_uri_handler(g_server, &inputs_uri);
    httpd_register_uri_handler(g_server, &command_uri);

    ESP_LOGI(TAG, "Local HTTP API started on port 80");
}
