# ThermoWerk ESP32-S3 Product Requirements

## 1. Product vision

ThermoWerk is a local-first ESP32-S3 based PV-surplus power controller for resistive loads, primarily heating rods / Heizstab systems.

The device shall replace a Linux/Node prototype with a compact embedded product architecture:

- one ESP32-S3 as the main device controller
- local realtime control on the ESP32
- local web interface directly from the ESP32
- optional cloud telemetry and remote access
- OTA firmware update capability
- safe SSR/burst-fire output for zero-cross SSRs
- modular future support for Modbus, temperature sensors, zero-cross detection and hardware safety inputs

The product shall feel closer to a polished consumer energy device than a hobby controller: simple setup, beautiful interface, robust local operation and clear safety behavior.

## 2. Hard architecture decisions

### 2.1 ESP32-only device runtime

The device firmware must run directly on ESP32-S3 using ESP-IDF.

Do not require on-device:

- Linux
- Docker
- Node.js
- SQLite
- Raspberry Pi
- external gateway for basic operation

External systems may exist, but the core device must work locally without cloud.

### 2.2 Local-first operation

The device must remain functional when:

- internet is offline
- cloud is offline
- app/server is unavailable
- Wi-Fi router is unavailable, if fallback AP is active

Local control, safety and SSR output must never depend on cloud availability.

### 2.3 Cloud is optional

Cloud functions are allowed for:

- telemetry
- device overview
- remote dashboard
- remote support
- OTA distribution metadata
- fleet management later

Cloud functions must not bypass local safety logic.

## 3. Main product modes

### 3.1 PV surplus mode

Use measured grid import/export to consume exported PV surplus.

Sign convention:

```text
grid_power_w > 0 = import from grid
grid_power_w < 0 = export / surplus
```

Formula:

```text
requested_power_w = -(grid_power_w - target_grid_power_w)
```

Clamp:

```text
0 <= requested_power_w <= max_power_w
```

### 3.2 Manual power mode

User directly sets a watt setpoint.

Use cases:

- testing
- commissioning
- fallback
- manual heating

### 3.3 Burst percent mode

User directly sets a percentage / permille output command.

Use cases:

- oscilloscope testing
- SSR verification
- UI demo
- production test

### 3.4 Disabled mode

All outputs OFF.

### 3.5 Test mode

Low power test mode for commissioning only. Must be clearly labeled as test mode.

## 4. Power output requirements

### 4.1 Output concept

Initial output concept:

- zero-cross SSR command output
- full-wave / burst-fire packet control
- default output GPIO17
- active-high by default
- 50 Hz mains assumption: 20 ms full-wave period

### 4.2 Burst scheduling

The output shall distribute ON waves over the configured burst window instead of grouping all ON waves together.

Example:

- 1000 permille = all full waves ON
- 500 permille = roughly every second full wave ON
- 100 permille = roughly 5 full waves per second ON at 50 Hz

### 4.3 Future zero-cross detection

A true hardware zero-cross input shall be added.

Requirements:

- detect missing zero-cross pulses
- detect implausible mains frequency
- force output OFF on sync loss
- optionally align SSR command with detected mains wave

## 5. Safety requirements

### 5.1 Default state

Outputs must default to OFF after boot, reset, crash, invalid config or communication timeout.

### 5.2 Output must be OFF when

- mode is disabled
- enable flag is false
- emergency stop is active
- process values are stale
- temperature values are invalid
- any configured temperature exceeds limit
- configuration is invalid
- internal fault exists
- future zero-cross sync fault exists
- future hardware fault input is active

### 5.3 Independent hardware safety

Firmware safety is not enough for a sold mains-voltage product.

Future hardware must include or support:

- fuse / overcurrent protection
- thermal cutoff
- independent safety temperature limiter where required
- correct creepage and clearance
- EMC concept
- watchdog-safe output stage
- optional contactor/relay off path
- safe enclosure and cooling concept

## 6. Local web interface requirements

The ESP32 shall serve a beautiful local web interface.

### 6.1 UI goals

The UI should feel modern, clean and product-ready:

- mobile-first
- dark premium design
- clear cards
- large values
- responsive layout
- no technical clutter on the home screen
- smooth status feedback
- clear emergency stop state
- clear output-active state
- setup wizard instead of raw config fields where possible

### 6.2 Main UI pages

#### Home

Show:

- current mode
- current output power
- current duty percentage
- grid import/export
- PV power
- tank temperature
- fault state
- Wi-Fi/cloud state
- output enabled/disabled

Actions:

- enable/disable
- emergency stop
- reset fault
- change mode quickly

#### Control

Show/edit:

- PV surplus target grid power
- nominal heater power
- manual power
- burst percentage
- temperature limit
- burst window

#### Setup wizard

Steps:

1. language / region
2. Wi-Fi setup
3. heater nominal power
4. SSR GPIO / hardware variant
5. meter source
6. temperature source
7. safety limits
8. cloud enable / skip
9. test output

#### History

Show:

- grid power
- output power
- duty
- temperature
- fault events

Initial firmware may use RAM history. Future firmware should persist summary history in flash with wear protection.

#### Settings

Show/edit:

- Wi-Fi
- cloud endpoint
- device name
- OTA update
- Modbus settings
- hardware pinout
- advanced safety settings
- factory reset

### 6.3 UI behavior requirements

- Every button press must give visible feedback.
- Dangerous actions require clear visual distinction.
- Emergency stop must be red and prominent.
- Faults must be understandable in plain language.
- Advanced settings must be hidden from normal user flow.
- UI must remain usable on phone screen.

## 7. Local API requirements

The ESP32 local REST API shall support:

```text
GET  /api/status
GET  /api/history
POST /api/config
POST /api/inputs
POST /api/command
POST /api/cloud
GET  /api/ota/status
POST /api/ota/start
POST /api/ota/rollback
```

Future APIs:

```text
GET  /api/settings
POST /api/settings
GET  /api/wifi/scan
POST /api/wifi/connect
GET  /api/modbus/test
POST /api/factory-reset
```

## 8. OTA update requirements

OTA is required for the product direction.

### 8.1 OTA goals

- firmware can be updated without opening enclosure
- update can be triggered from local UI
- update can be offered through cloud later
- device must survive failed update
- rollback must be possible

### 8.2 ESP-IDF OTA partition concept

Future partition table should move from single factory app to OTA-capable layout:

```text
nvs
phy_init
otadata
ota_0
ota_1
storage
```

### 8.3 OTA safety rules

- never update while output is active
- force output OFF before OTA
- reject OTA if supply/power state is unsafe
- verify image before switching boot partition
- mark app valid only after successful boot and self-test
- rollback on failed boot
- keep local control disabled during update

### 8.4 OTA UI requirements

The UI must show:

- current firmware version
- available firmware version
- update progress
- reboot required state
- last update result
- rollback option if supported

### 8.5 OTA security requirements

Product version should eventually support:

- HTTPS OTA
- signed firmware images
- secure boot option
- flash encryption option
- device-specific credentials / provisioning

## 9. Cloud requirements

### 9.1 Initial cloud telemetry

Initial cloud telemetry may be simple HTTP POST.

Telemetry payload should include:

- device id
- firmware version
- uptime
- mode
- fault
- grid power
- PV power
- output power
- duty
- temperatures
- Wi-Fi/cloud state

### 9.2 Future cloud features

Possible future features:

- device fleet overview
- remote diagnostics
- OTA campaign management
- remote support mode
- data export
- user account / device claim

### 9.3 Cloud command safety

Cloud commands must not directly drive GPIO.

Cloud commands must go through:

1. authentication
2. config validation
3. safety layer
4. local control layer
5. output driver

## 10. Storage requirements

### 10.1 Current

RAM history is acceptable for early firmware.

### 10.2 Future

Persistent storage should use NVS and/or wear-leveled flash.

Persist:

- Wi-Fi credentials
- device name
- cloud settings
- hardware config
- safety config
- last known operating mode
- calibration values
- firmware update status

Do not persist high-frequency raw history directly to flash without wear strategy.

## 11. Modbus / meter requirements

The ESP32 should eventually support direct meter integration.

Possible inputs:

- Modbus TCP meter
- Modbus RTU meter via UART/RS485
- external gateway via UART/JSON
- cloud or local API for simulation/test

Values needed:

- grid import/export power
- PV power if available
- load power if available
- voltage / phase data later
- meter freshness timestamp

## 12. Hardware abstraction requirements

Firmware must remain modular.

Recommended modules:

```text
app_state
control
safety
output_driver
zero_cross
temperature
modbus_master
nvs_config
wifi_manager
local_api
cloud_client
ota_update
history_store
```

## 13. Product polish requirements

- clear boot logs
- clear fault names
- sane defaults
- factory reset path
- setup AP fallback
- mDNS name later, e.g. thermowerk.local
- no hidden dependency on external server
- responsive UI
- safe defaults after reset
- clear documentation for flashing and recovery

## 14. Definition of done for next firmware milestone

The next milestone is considered done when:

- repo builds with `idf.py build`
- firmware flashes to ESP32-S3
- local AP starts
- local UI opens at `192.168.4.1`
- `/api/status` returns valid JSON
- config can be changed from UI
- simulated inputs can enable output
- GPIO17 burst signal can be measured
- emergency stop forces output OFF
- OTA requirements are represented in code stubs or module interfaces
- README has exact flash/test instructions
