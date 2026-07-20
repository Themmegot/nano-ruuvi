# nano-ruuvi

A Ruuvi tag emulator for the **Arduino Nano 33 BLE Sense (rev1)**.

Reads the board's onboard sensors — HTS221 (temperature/humidity), LPS22HB
(pressure), LSM9DS1 (movement counter) — and broadcasts them as **Ruuvi RAWv2
(data format 5)** BLE manufacturer advertisements at Ruuvi's standard 1.285 s
cadence. Anything that speaks Ruuvi adopts it as a real tag, verified with the
official Ruuvi Station app. Built to feed a **Victron Cerbo GX** (Venus OS →
Settings → I/O → Bluetooth sensors), which shows it on the GX display and VRM
portal as a native wireless climate sensor — no WiFi or MQTT required.

## Build & flash

Requires arduino-cli with the `arduino:mbed_nano` core and libraries:
ArduinoBLE, Arduino_HTS221, Arduino_LPS22HB, Arduino_LSM9DS1.

```sh
arduino-cli compile --fqbn arduino:mbed_nano:nano33ble .
arduino-cli upload  --fqbn arduino:mbed_nano:nano33ble -p /dev/cu.usbmodemXXXX .
```

Serial debug at 115200 prints each broadcast (open with DTR asserted).

## Notes

- **Rev1 only**: the Nano 33 BLE Sense rev2 replaced these sensors
  (HS3003/BMP390/BMI270); init fails cleanly with an error on serial.
- The HTS221 reads ~2–3 °C high from nRF52 self-heating; an offset
  correction is planned once calibrated against a reference thermometer.
- Runs continuously on USB power; battery operation would want a
  deep-sleep/advertise duty cycle (not yet implemented).
- Battery level in the payload is hardcoded to 3300 mV (USB-powered).

CAVEAT: unofficial, not affiliated with Ruuvi Innovations, emulates their public RAWv2 broadcast format for compatibility with Victron/VRM.
