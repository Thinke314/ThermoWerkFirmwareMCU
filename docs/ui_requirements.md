# ThermoWerk Local UI Requirements

## Goal

The ESP32 shall serve a local web interface that looks and feels like a polished product UI, not an engineering debug page.

The UI must work directly from the ESP32 without external assets, internet or cloud.

## Design principles

- premium dark interface
- mobile-first layout
- large readable values
- clear operating state
- clear fault visibility
- fast feedback on every user action
- safe controls separated from dangerous controls
- simple normal mode, advanced expert settings hidden
- no dependency on CDN assets
- no external JavaScript framework required for first version

## Visual direction

Style target:

- modern energy product
- Apple/Tesla-like simplicity
- dark background
- soft cards
- clear typography
- subtle gradients allowed
- green/blue for healthy operation
- orange for warnings
- red for faults/emergency

## Main navigation

Suggested pages:

```text
Home
Control
History
Setup
Settings
Update
Advanced
```

For early firmware, this can be a single-page app with sections.

## Home page

The Home page is the normal user page.

Show cards for:

- operating mode
- output power
- output percentage
- grid import/export
- PV power
- tank top temperature
- fault status
- Wi-Fi state
- cloud state

Primary actions:

- enable / disable
- emergency stop
- reset fault
- quick mode change

Home page must avoid showing raw technical config unless needed.

## Control page

Controls:

- mode selection
- PV surplus target grid power
- nominal heater power
- manual power setpoint
- burst percentage
- temperature limit
- burst window

Requirements:

- input validation in UI
- show resulting estimated duty
- show warning if output would be disabled by safety
- apply button with success/failure feedback

## History page

Initial history may come from RAM ring buffer.

Show:

- output power trend
- grid power trend
- tank temperature trend
- duty trend
- fault markers

First version may use simple SVG/canvas or a minimal table. Future version should use charts.

## Setup wizard

The setup wizard should guide a non-expert user.

Steps:

1. welcome and safety warning
2. language/region
3. Wi-Fi setup
4. heater nominal power
5. hardware variant / SSR output
6. meter source
7. temperature source
8. safety limit
9. cloud optional setup
10. output test
11. finish

The wizard must allow skipping cloud.

## Settings page

Settings categories:

- Wi-Fi
- device name
- cloud telemetry
- local API
- OTA update
- Modbus/meter
- temperature sensors
- output hardware
- factory reset

## Update page

Show:

- current firmware version
- build type
- build date
- current partition
- update availability
- progress
- last result

Actions:

- check update
- start update
- reboot
- rollback

## Fault display

Faults must be displayed in plain language.

Mapping examples:

```text
disabled          -> Output disabled
emergency_stop    -> Emergency stop active
uart_timeout      -> No fresh input values
sensor_invalid    -> Temperature sensor values invalid
overtemperature   -> Temperature limit reached
invalid_config    -> Invalid configuration
internal          -> Internal firmware fault
```

Each fault should show:

- what happened
- whether output is off
- what user can do

## UI API contract

The UI should use only local API endpoints:

```text
GET  /api/status
GET  /api/history
POST /api/config
POST /api/inputs
POST /api/command
POST /api/cloud
GET  /api/ota/status
POST /api/ota/start
```

The UI must not directly manipulate firmware globals.

## Offline behavior

The UI must work when:

- no internet is available
- cloud endpoint is unreachable
- ESP32 is only in setup AP mode

## Performance constraints

Because the UI is served by ESP32:

- keep HTML/CSS/JS compact
- avoid large libraries
- avoid remote fonts
- avoid remote icons
- compress/minify later if needed
- avoid high-frequency polling below 1 s unless necessary

## First UI milestone

The first polished UI milestone is done when:

- Home dashboard looks clean on phone
- mode/config can be changed
- emergency stop works
- simulated inputs can be sent
- status updates every 1 s
- history endpoint is visible
- cloud telemetry can be configured
- clear fault display exists
- no external web assets are required
