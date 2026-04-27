# UART Protocol

## Transport

Initial firmware uses UART line-based JSON frames.

Default settings:

```text
Baud: 115200
Format: 8N1
Line ending: \n or \r\n
```

Each frame is one JSON object per line.

## Sign convention

```text
grid_power_w > 0  => import from grid
grid_power_w < 0  => export / surplus
```

## Host to MCU: process values

```json
{"type":"process_values","grid_power_w":-1200,"pv_power_w":5400,"load_power_w":800,"temp_1_c":54.2,"temp_2_c":38.7,"enable":true}
```

Fields:

| Field | Type | Description |
|---|---:|---|
| type | string | `process_values` |
| grid_power_w | int | Grid import/export power. Positive import, negative export. |
| pv_power_w | int | PV generation power. Optional for first control loop. |
| load_power_w | int | Current controlled load power. Optional. |
| temp_1_c | float | Temperature channel 1. |
| temp_2_c | float | Temperature channel 2. |
| enable | bool | Host enable flag. False forces outputs OFF. |

## Host to MCU: config

```json
{"type":"config","control_mode":"pv_surplus","max_power_w":3500,"min_power_w":0,"manual_power_w":0,"target_grid_power_w":0,"temperature_limit_c":85,"uart_timeout_ms":3000}
```

Fields:

| Field | Type | Description |
|---|---:|---|
| type | string | `config` |
| control_mode | string | `disabled`, `pv_surplus`, `manual_power`, `test` |
| max_power_w | int | Maximum allowed output power. |
| min_power_w | int | Minimum output power if output is active. Usually 0 for burst-fire. |
| manual_power_w | int | Manual power setpoint. |
| target_grid_power_w | int | Desired grid power. 0 = zero export/import target. |
| temperature_limit_c | float | Output is disabled at or above this temperature. |
| uart_timeout_ms | int | Timeout for process values. |

## Host to MCU: command

```json
{"type":"command","enable_outputs":true}
```

Currently parsed but not yet used as separate latch. Later this can become a local arming command.

## MCU to Host: status

```json
{"type":"status","power_setpoint_w":1200,"duty_permille":343,"outputs_enabled":true,"fault":"none"}
```

Fields:

| Field | Type | Description |
|---|---:|---|
| type | string | `status` |
| power_setpoint_w | int | Calculated output setpoint. |
| duty_permille | int | Output duty in permille, 0 to 1000. |
| outputs_enabled | bool | Final output permission after control and safety. |
| fault | string | `none`, `disabled`, `uart_timeout`, `overtemperature`, `invalid_config`, `internal` |

## Test examples

After flashing, send these lines via serial terminal.

Set config:

```json
{"type":"config","control_mode":"pv_surplus","max_power_w":3500,"min_power_w":0,"target_grid_power_w":0,"temperature_limit_c":85,"uart_timeout_ms":3000}
```

Send surplus:

```json
{"type":"process_values","grid_power_w":-1200,"pv_power_w":5400,"load_power_w":0,"temp_1_c":30,"temp_2_c":31,"enable":true}
```

Expected status contains approximately:

```json
{"power_setpoint_w":1200,"duty_permille":342,"outputs_enabled":true,"fault":"none"}
```

Send import:

```json
{"type":"process_values","grid_power_w":500,"pv_power_w":1200,"load_power_w":0,"temp_1_c":30,"temp_2_c":31,"enable":true}
```

Expected output OFF / zero power.
