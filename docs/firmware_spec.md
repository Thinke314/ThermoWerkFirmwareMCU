# ThermoWerk MCU Firmware Specification

## Goal

This repository is the ESP32-S3 firmware implementation of the ThermoWerk3p PV-surplus heater controller / Leistungssteller.

The product direction is local-first ESP32-only:

- no Linux runtime on the device
- no Node.js runtime on the device
- no SQLite runtime on the device
- no Docker runtime on the device
- ESP32-S3 owns control, safety, local interface, SSR output scheduling and optional cloud telemetry

ThermoWerk3p remains the reference/prototype repository. This repository is the firmware product line for the ESP32.

## Implemented now

- ESP-IDF firmware project
- ESP32-S3 default target
- UART0 JSON protocol at 115200 baud
- local control loop at 20 ms
- SSR GPIO output driver
- default SSR GPIO17
- full-wave / burst-fire output scheduler
- Bresenham-style distribution of ON waves over a burst window
- PV-surplus mode
- manual power mode
- direct burst-percent mode
- test mode
- multi-channel temperature safety
- UART/process-value timeout
- emergency stop flag
- local Wi-Fi setup AP
- embedded local web interface
- local REST API
- local RAM history ring buffer
- optional HTTP cloud telemetry client
- JSON status telemetry every 500 ms over UART

## Local interface

The firmware starts a fallback Wi-Fi access point:

```text
SSID: ThermoWerk-Setup
Password: thermowerk
```

The ESP32 serves a local UI and API on port 80:

```text
GET  /
GET  /api/status
GET  /api/history
POST /api/config
POST /api/inputs
POST /api/command
POST /api/cloud
```

The local interface is not a separate backend. It is compiled into the ESP32 firmware and directly shares runtime state with the control loop.

## Cloud behavior

Cloud is optional telemetry only in the current implementation.

- Cloud failure must not stop local control.
- Cloud status is reflected in runtime status.
- Telemetry is HTTP POST to a configured endpoint.
- No cloud command execution is implemented yet.
- Future cloud commands must pass through the same local safety and config validation path as local API and UART commands.

## Runtime responsibilities on ESP32

The ESP32 owns:

- Wi-Fi setup AP
- local web UI
- local REST API
- runtime state
- local history buffer
- process-value supervision
- local control mode selection
- PV-surplus power calculation
- power limiting
- temperature safety
- emergency stop handling
- timeout supervision
- final output enable decision
- SSR GPIO command
- burst-fire/full-wave scheduling
- optional cloud telemetry
- status reporting

External systems may provide:

- Modbus-derived meter values
- cloud telemetry receiver
- future mobile app or dashboard
- production test commands

External systems must not be required for the fast realtime output decision once valid inputs are available.

## Grid power convention

Use this sign convention everywhere:

```text
grid_power_w > 0  => import from grid
grid_power_w < 0  => export / PV surplus
```

PV-surplus formula:

```text
requested_power_w = -(grid_power_w - target_grid_power_w)
```

Example:

- `grid_power_w = -1200 W`
- `target_grid_power_w = 0 W`
- `requested_power_w = 1200 W`

The final request is clamped to `min_power_w ... max_power_w`.

## Control modes

### disabled

All outputs OFF.

### pv_surplus / auto

Use surplus power based on received grid power and target grid power.

### manual_power / manual

Use configured `manual_power_w`.

### burst_percent / burst

Directly command duty using either:

- `target_power_percent` from 0 to 100
- or `burst_target_permille` from 0 to 1000

This is useful for oscilloscope testing, SSR output verification and UI integration.

### test

10% output test mode. This must never be used as a replacement for safety validation.

## Output scheduler

The current scheduler assumes 50 Hz mains and a 20 ms full-wave period.

For zero-cross SSRs, the firmware commands the SSR input according to a distributed packet pattern:

- `1000 permille` = all full waves ON
- `500 permille` = approximately every second full wave ON
- `100 permille` = approximately 5 full waves per second ON

The scheduler uses a Bresenham-style distribution so the ON waves are spread over the configured burst window rather than grouped into one block.

## Safety behavior

Outputs must default to OFF.

Outputs are OFF when:

- `enable` is false
- control mode is `disabled`
- `emergency_stop` is true
- UART/process-value timeout occurs
- `temp_valid` is false
- any main temperature channel reaches `temperature_limit_c`
- invalid configuration is detected
- internal fallback fault occurs

Future safety additions:

- zero-cross sync fault
- hardware overtemperature input
- welded SSR detection concept
- independent safety limiter integration
- relay/contactor de-energize output

## Current limitations

- Wi-Fi STA credential storage is not implemented yet.
- NVS persistent product config is not implemented yet.
- Real sensor drivers are not implemented yet.
- Direct Modbus master is not implemented yet.
- Cloud command downlink is intentionally not implemented yet.
- OTA update is not implemented yet.
- True hardware zero-cross capture is not implemented yet.

## First target hardware assumption

- ESP32-S3
- UART0 debug/control transport
- local AP for setup
- SSR command output on GPIO17 by default
- external isolated zero-cross SSR/opto input stage
- no direct mains connection to ESP32 GPIO

## Recommended next modules

```text
main/zero_cross.c/.h       true zero-cross capture and sync-loss detection
main/temperature.c/.h      real sensor inputs instead of UART-only values
main/nvs_config.c/.h       persistent config storage and Wi-Fi credentials
main/modbus_master.c/.h    optional direct meter polling
main/ota_update.c/.h       product update path
main/cloud_commands.c/.h   safe cloud downlink path if required
docs/hardware_pinout.md    fixed pinout and electrical assumptions
```

## Product constraints

This firmware is not safety-certified. For a sold product, independent hardware safety elements are required, for example:

- thermal cutoff
- fuse / overcurrent protection
- correct creepage and clearance
- enclosure and thermal design
- EMC strategy
- external certified temperature limiter where required
- watchdog/failsafe output stage
- production test procedure
