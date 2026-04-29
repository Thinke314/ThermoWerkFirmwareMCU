# ThermoWerkFirmwareMCU
## Ziel
ESP32-S3 ESP-IDF C MVP mit lokaler Regelung/WebApp ohne Cloud-Pflicht.
## Produktarchitektur
Ein ESP32-S3 übernimmt WLAN, API, UI, Regelung, SSR, NVS, OTA-Vorbereitung.
## Nur ein ESP32-S3 als Steuerhardware
Keine zusätzliche Raspberry-Pi-/Gateway-Steuerhardware.
## Hardware
SSR GPIO17, Zero-Cross GPIO16 (vorbereitet).
## ESP-IDF Version
ESP-IDF v5.x (VS Code ESP-IDF Extension).
## VS Code Setup
ESP-IDF Extension installieren, Projekt öffnen, Target `esp32s3` setzen.
## Build
`idf.py set-target esp32s3 && idf.py fullclean && idf.py build`
## Flash
`idf.py flash monitor`
## Monitor
Erwartete Logs: starting, GPIO17 initialized, WiFi started, Web server started, Control loop started, ready.
## WLAN
MVP mit AP-Fallback `ThermoWerk-Setup` / `thermowerk`, UI unter `http://192.168.4.1/`.
## WebApp
Lokale dark UI (embedded HTML/CSS/JS), pollt `/api/status`.
## API
`/api/status`, `/api/input`, `/api/test`, `/api/gpio_test` (MVP), weitere Endpunkte als nächste Schritte.
## GPIO17 Test
Buttons für HIGH/LOW/Blink vorhanden. Mit Oszi/Multimeter gegen GND messen.
## Regelungslogik
AUTO nutzt `-grid_power_w` als Überschuss, MANUAL nutzt `manual_power_w`, OFF/DISABLED/Fehler => SSR LOW.
## OTA
OTA-fähige Partitionstabelle und Stub-Modul vorhanden.
## Cloud-Roadmap
Cloud-Stub inkl. enabled/connected/device_id vorbereitet; lokale Regelung bleibt autark.
## Modbus Registermap
`modbus_map.*` vorbereitet (MVP-Stub).
## UART Vorbereitung
`uart_gateway.*` vorbereitet (MVP-Stub für JSON-Line-Protokoll).
## Troubleshooting
Bei Build-Problemen: `idf.py fullclean`, Target neu setzen, Port prüfen.
## Sicherheitshinweise
Achtung: Dieses MVP ist nur für Niederspannungs-/Labortests am GPIO-Ausgang gedacht.
## Bekannte Einschränkungen
STA-WLAN, vollständige OTA-Uploadroute und vollständige UI/API sind als nächste Iteration vorgesehen.
## Nächste Schritte
WLAN-STA+NVS Credentials, vollständige API-Routen, OTA Upload, Modbus TCP, UART JSON-Line Runtime.
