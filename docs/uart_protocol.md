# UART Protocol

ThermoWerkFirmwareMCU uses UART line-based JSON frames. This is the initial bridge between host/debug tooling and the ESP32-S3 firmware. One JSON object is sent per line.

## Transport

```text
Baud: 115200
Format: 8N1
Line ending: \n or \r\n
```

## Sign convention

```text
grid_power_w > 0  => import from grid
grid_power_w < 0  => export / PV surplus
```

## Host to MCU: config

Preferred current format:

```json
{"type":"config","control_mode":"pv_surplus","power_nominal_w":3500,"target_grid_power_w":0,"burst_window_ms":1000,"ssr_gpio_pin":17,"output_active_high":true,"temperature_limit_c":85,"uart_timeout_ms":3000}
```

Supported aliases:

- `control_mode` or `mode`
- `power_nominal_w` or `max_power_w`
- `window_ms` or `burst_window_ms`
- `target_power_percent` or `burst_target_permille`

Supported `control_mode` values:

| Mode | Meaning |
|---|---|
| `disabled` | Output always OFF. |
| `pv_surplus` / `auto` | Consume exported PV surplus. |
| `manual_power` / `manual` | Use `manual_power_w`. |
| `burst_percent` / `burst` | Direct burst duty command. |
| `test` | 10% output for commissioning. |

Important config fields:

| Field | Type | Description |
|---|---:|---|
| `power_nominal_w` / `max_power_w` | int | Nominal heater power. Used to convert W to duty. |
| `min_power_w` | int | Minimum active power. Usually `0`. |
| `manual_power_w` | int | Manual watt setpoint. |
| `target_grid_power_w` | int | Desired grid import/export target. `0` means zero export/import target. |
| `target_power_percent` | number | Direct burst command, 0 to 100. |
| `burst_target_permille` | int | Direct burst command, 0 to 1000. |
| `burst_window_ms` | int | Burst window. Default 1000 ms. |
| `ssr_gpio_pin` | int | ESP32 GPIO for SSR command. Default GPIO17. |
| `output_active_high` | bool | `true` means GPIO high commands SSR ON. |
| `temperature_limit_c` | float | Output is disabled at or above this temperature. |
| `uart_timeout_ms` | int | Process-value timeout. Default 3000 ms. |

## Host to MCU: process values / inputs

Preferred format:

```json
{"type":"process_values","grid_power_w":-1200,"pv_power_w":5400,"load_power_w":0,"tank_top_c":50,"tank_mid_c":45,"tank_bottom_c":40,"flow_line_c":38,"return_line_c":35,"ambient_c":22,"temp_valid":true,"enable":true}
```

Supported aliases:

- `type:"inputs"` is accepted as alternative to `process_values`.
- `site_import_export_w` is accepted as alternative to `grid_power_w`.
- `pv_surplus_w` is accepted as alternative to `pv_power_w`.
- `temp_1_c` maps to `tank_top_c`.
- `temp_2_c` maps to `tank_mid_c`.

Important input fields:

| Field | Type | Description |
|---|---:|---|
| `grid_power_w` | int | Positive import, negative export. |
| `pv_power_w` | int | PV production or external PV value. |
| `load_power_w` | int | Current heater/load power if available. |
| `tank_top_c` | float | Tank top temperature. |
| `tank_mid_c` | float | Tank middle temperature. |
| `tank_bottom_c` | float | Tank bottom temperature. |
| `flow_line_c` | float | Flow line temperature. |
| `return_line_c` | float | Return line temperature. |
| `ambient_c` | float | Ambient temperature. |
| `temp_valid` | bool | Must be `true`, otherwise output is OFF. |
| `enable` | bool | Must be `true`, otherwise output is OFF. |
| `emergency_stop` | bool | Forces output OFF. |

## Host to MCU: command

Emergency stop:

```json
{"type":"command","emergency_stop":true}
```

Reset emergency stop flag:

```json
{"type":"command","reset_fault":true}
```

Enable output flag:

```json
{"type":"command","enable_outputs":true}
```

## MCU to Host: status

Example:

```json
{"type":"status","mode":3,"power_setpoint_w":1050,"duty_permille":300,"outputs_enabled":true,"gpio_level":true,"ssr_gpio_pin":17,"grid_power_w":-1200,"pv_power_w":5400,"rx_age_ms":120,"fault":"none"}
```

Fault values:

| Fault | Meaning |
|---|---|
| `none` | Output allowed. |
| `disabled` | Disabled by mode or enable flag. |
| `emergency_stop` | Emergency stop active. |
| `uart_timeout` | No fresh process values. |
| `sensor_invalid` | Temperature values not valid. |
| `overtemperature` | At least one monitored channel exceeded limit. |
| `invalid_config` | Config rejected by safety layer. |
| `internal` | Internal fallback fault. |

## Quick tests

Direct burst test, about 30%:

```json
{"type":"config","control_mode":"burst_percent","power_nominal_w":3500,"target_power_percent":30,"burst_window_ms":1000,"ssr_gpio_pin":17,"temperature_limit_c":85,"uart_timeout_ms":3000}
```

```json
{"type":"process_values","grid_power_w":-1200,"pv_power_w":5400,"tank_top_c":50,"tank_mid_c":45,"tank_bottom_c":40,"flow_line_c":38,"return_line_c":35,"temp_valid":true,"enable":true}
```

PV surplus test:

```json
{"type":"config","control_mode":"pv_surplus","power_nominal_w":3500,"target_grid_power_w":0,"burst_window_ms":1000,"ssr_gpio_pin":17,"temperature_limit_c":85,"uart_timeout_ms":3000}
```

```json
{"type":"process_values","grid_power_w":-900,"pv_power_w":4500,"tank_top_c":48,"tank_mid_c":42,"tank_bottom_c":37,"flow_line_c":35,"return_line_c":32,"temp_valid":true,"enable":true}
```

Expected: `power_setpoint_w` around `900`, `duty_permille` around `257` for a 3500 W heater, `outputs_enabled:true`, `fault:"none"`.
