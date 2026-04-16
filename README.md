# Embedded Solar Monitoring System

An ESP32-based solar panel monitor that measures bus voltage, current, and power of a small PV module, shows live readings on an OLED, and publishes JSON telemetry to an MQTT broker over Wi-Fi. Measurements were validated against a bench multimeter and a lab power supply.

![Circuit diagram](https://github.com/ghulam420-sarwar/Solar-Monitoring-System/blob/main/circuit_diagram.png)

## Highlights

- **High-side current sensing** with INA219 (V-shunt + V-bus in one chip)
- **Live OLED readout** (V, I, P, cumulative Wh)
- **MQTT telemetry** every 5 s — ready for Home Assistant / Grafana
- **Accuracy** validated against a Fluke 117 and a calibrated lab PSU
- **Designed for a small 6 V / 12 V panel** (up to ~400 mA)

## Hardware

| Component     | Purpose                   | Interface |
| ------------- | ------------------------- | --------- |
| ESP32 DevKit  | MCU + Wi-Fi               | —         |
| INA219        | V / I / P measurement     | I²C 0x40  |
| SSD1306 OLED  | 128×64 display            | I²C 0x3C  |
| Solar panel   | 6–12 V PV                 | —         |
| Load          | 12 V fan / LED array      | —         |

### Wiring

The INA219 is placed **in series with the panel's positive lead**. Vin+ connects to the panel, Vin− goes to the load; the chip senses the voltage drop across its internal 0.1 Ω shunt.

## Build

```bash
pio run -t upload
pio device monitor
```

Edit `WIFI_SSID`, `WIFI_PASS`, and `MQTT_HOST` at the top of `src/main.cpp` before flashing.

## Validation

| Quantity  | INA219 reading | Fluke 117 | Error |
| --------- | -------------- | --------- | ----- |
| Vbus      | 11.92 V        | 11.94 V   | 0.17% |
| I         | 245 mA         | 243 mA    | 0.82% |
| P (calc)  | 2.92 W         | 2.90 W    | 0.69% |

Errors stayed well within the INA219's ±1% spec.

## MQTT Payload

```json
{"v":11.924,"i_mA":245.12,"p_mW":2923.5,"e_Wh":0.812,"t":60025}
```

## What I learned

- Correct calibration range selection on the INA219 to avoid saturation
- Non-blocking energy integration using `millis()` deltas
- Proper decoupling at the INA219 input — a 100 nF + 10 µF pair was needed to avoid spikes from the switching load
- MQTT reconnection handling in firmware

## License

MIT © Ghulam Sarwar
