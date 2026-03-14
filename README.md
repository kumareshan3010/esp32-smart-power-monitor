# ⚡ ESP32 Smart AC Power Monitor & Power Factor Corrector

[![Platform](https://img.shields.io/badge/platform-ESP32-blue)](https://www.espressif.com/en/products/socs/esp32)
[![Language](https://img.shields.io/badge/language-C++-orange)](https://isocpp.org/)
[![Docs](https://img.shields.io/badge/docs-Wiki-blue)](https://github.com/kumareshan3010/esp32-smart-power-monitor/wiki)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Status](https://img.shields.io/badge/status-Completed-brightgreen)]()

> A professional-grade AC power monitoring and automatic power factor correction system built on ESP32 WROOM-32, with real-time cloud dashboards on Blynk and Adafruit IO.

**Author:** Kumareshan V — [@kumareshan3010](https://github.com/kumareshan3010)

---

## 📌 Overview

This project turns an ESP32 into a complete AC power quality analyzer. It measures voltage, current, power, power factor, frequency, and waveform metrics in real time — and automatically corrects poor power factor using a 7-capacitor switched bank with **Zero Voltage Switching (ZVS)** to prevent relay stress and voltage spikes.

All data is pushed live to **Blynk** (1s refresh) and **Adafruit IO** (3s refresh), with full NVS persistence so nothing is lost on reboot.

---

## 📊 Interactive Block Diagram
👉 [Click here to view](https://kumareshan3010.github.io/esp32-smart-power-monitor/blob/main/simplified_block_diagram.html)

---

## 📖 Documentation

Detailed explanations and design documentation are available in the Wiki:

👉 https://github.com/kumareshan3010/esp32-smart-power-monitor/wiki

---

## ✨ Features

| Category | Details |
|---|---|
| **Voltage & Current** | RMS, DC component, Peak, Std Deviation |
| **Power** | Real (W), Reactive (VAR), Apparent (VA), Complex Power |
| **Power Factor** | Phase-shift method, Leading/Lagging/Unity label |
| **Impedance** | Resistance (Ω), Reactance (Ω), Complex Impedance |
| **Waveform Quality** | Crest Factor, Form Factor, Std Deviation |
| **Frequency** | Live measurement via zero-cross ISR |
| **Load Detection** | Resistive, Inductive, Capacitive, Non-linear, No Load |
| **Inrush Detection** | Ratio-based detection with Blynk event logging |
| **Energy Monitoring** | kWh accumulation + Monthly estimate |
| **Extremes Tracking** | Min/Max with timestamps for V, I, F, PF |
| **PF Correction** | 7-cap bank, 1–36µF, ZVS switching, 3-cycle hysteresis |
| **Cloud** | Blynk (realtime) + Adafruit IO (logging/history) |
| **Persistence** | NVS storage — survives reboots |
| **Time** | NTP sync, IST timezone, formatted timestamps |
| **Chip Temp** | ESP32 internal temperature sensor |

---

## 🛠 Hardware

| Component | Specification |
|---|---|
| Microcontroller | ESP32 WROOM-32 |
| Voltage Sensor | ZMPT101B |
| Current Sensor | ACS712 — 30A version |
| Zero Cross Detector | Optocoupler-based ZCD circuit |
| Capacitor Bank | 7 relays: 1, 2, 3, 4, 5, 6, 15 µF |
| AC Mains | 220V / 50Hz |

### 📍 Pin Mapping

| Signal | GPIO |
|---|---|
| ZMPT101B Output | 34 |
| ACS712 Output | 35 |
| Zero Cross Detection | 32 |
| Capacitor C1 (1µF) | 25 |
| Capacitor C2 (2µF) | 26 |
| Capacitor C3 (3µF) | 27 |
| Capacitor C4 (4µF) | 14 |
| Capacitor C5 (5µF) | 12 |
| Capacitor C6 (6µF) | 13 |
| Capacitor C7 (15µF) | 33 |

> See the [Wiring Guide](../../wiki/Wiring-Guide) for full circuit description.

---

## 📦 Libraries Required

Install all via Arduino Library Manager or PlatformIO:

| Library | Purpose |
|---|---|
| `Blynk` | Blynk IoT dashboard |
| `Adafruit IO Arduino` | Adafruit IO cloud feeds |
| `WiFi` | Built-in ESP32 WiFi |
| `Preferences` | NVS flash storage |
| `time.h` | NTP time sync |

> See [Library Setup](../../wiki/Library-Setup) for exact installation steps.

---

## 🚀 Quick Start

1. Clone this repository
2. Open `esp32_power_monitor.ino` in Arduino IDE
3. Fill in your credentials in Section 2:
```cpp
#define WIFI_SSID        "your_wifi"
#define WIFI_PASSWORD    "your_password"
#define BLYNK_AUTH_TOKEN "your_blynk_token"
#define AIO_USERNAME     "your_aio_username"
#define AIO_KEY          "your_aio_key"
```
4. Select **ESP32 Dev Module** as board
5. Upload and open Serial Monitor at 115200 baud
6. Calibrate `ZMPT_SCALE` (see [Calibration Guide](../../wiki/Calibration))

---

## 🧠 How the PF Correction Works

The system uses a mathematically optimal 7-capacitor set **{1, 2, 3, 4, 5, 6, 15} µF** that can produce every integer capacitance from 1–36µF through combinations. This is computed using the formula:

```
C = Q / (2π f V²)   where Q = P × tan(φ)
```

Switching only happens at **voltage zero crossings** to eliminate inrush and relay wear. Correction is triggered only after **3 consecutive bad-PF cycles** to avoid hunting.

> See [PF Correction Logic](../../wiki/PF-Correction-Logic) for full details.

---

## 📊 Blynk Virtual Pin Map

| Pin | Data |
|---|---|
| V0 | Voltage RMS |
| V1 | Current RMS |
| V2 | Real Power |
| V3–V9 | Cap Bank C1–C7 State |
| V11 | Energy (kWh) |
| V12 | Apparent Power |
| V13 | Reactive Power |
| V14 | Power Factor |
| V15 | Frequency |
| V16 | Complex Power |
| V17 | Complex Impedance |
| V18 | DC Voltage |
| V19 | DC Current |
| V20–V25 | Crest/Form/StdDev |
| V26 | Load Type |
| V27 | Inrush Ratio |
| V28 | Chip Temp |
| V29 | Monthly Estimate |
| V30–V33 | Min/Max V & I |
| V34 | Reset Extremes |
| V35–V46 | Extreme Timestamps |
| V47 | PF Label |
| V48–V49 | R & X |
| V56 | PF Correction Count |
| V57 | Reset Inrush |
| V58 | Reset PF Count |
| V59 | Max Inrush Ratio |
| V60–V61 | Inrush/PFC Timestamps |
| V62 | Target Capacitance |

---

## 🔭 Future Improvements

- [ ] THD (Total Harmonic Distortion) calculation
- [ ] ESP32 web server for local dashboard (no cloud needed)
- [ ] OTA (Over The Air) firmware updates
- [ ] Current transformer (CT) support as alternative to ACS712
- [ ] Three-phase power monitoring expansion
- [ ] Telegram/WhatsApp alert integration
- [ ] SD card data logging for offline analysis
- [ ] Auto ZMPT calibration routine

---

## ⚠️ Safety Warning

> This project interfaces directly with **220V AC mains voltage**. Incorrect wiring can cause **serious injury or death**. Only attempt this build if you are qualified to work with mains electricity. Always use proper isolation, fusing, and enclosures.

---

## 📄 License

MIT License — free to use, modify, and distribute with attribution.

---

## 🙏 Acknowledgements

- Blynk IoT Platform
- Adafruit IO
- ESP32 Arduino Core by Espressif
