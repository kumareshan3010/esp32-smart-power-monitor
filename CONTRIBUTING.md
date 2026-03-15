# Contributing to ESP32 Smart AC Power Monitor

Thank you for your interest in contributing! This project welcomes collaborators, reviewers, and anyone with ideas for improvement — whether you're into embedded firmware, power electronics, hardware design, or cloud integration.

---

## 📋 Table of Contents

- [Project Overview](#project-overview)
- [Ways to Contribute](#ways-to-contribute)
- [Getting Started](#getting-started)
- [Project Structure](#project-structure)
- [Development Setup](#development-setup)
- [Coding Guidelines](#coding-guidelines)
- [Submitting Changes](#submitting-changes)
- [Open Issues & Ideas](#open-issues--ideas)
- [Safety Notice](#safety-notice)
- [Contact](#contact)

---

## Project Overview

This is an ESP32-based AC power monitoring and automatic power factor correction system. It measures real-time AC parameters (voltage, current, power, PF, frequency, etc.) and automatically switches a 7-capacitor bank using Zero Voltage Switching (ZVS) to correct poor power factor.

The codebase is currently being migrated from **Arduino** to **bare-metal ESP-IDF C** (register-level ADC, GPIO, timers) — making this a great time to get involved.

---

## Ways to Contribute

### 🔍 Code Review
- Review RMS calculation accuracy and sampling logic
- Review ISR (interrupt service routine) safety and ZVS switching logic
- Review capacitor bank selection algorithm
- Identify race conditions or edge cases in cloud sync

### 🤝 Collaboration (New Features)
| Feature | Difficulty | Notes |
|---|---|---|
| THD (Total Harmonic Distortion) calculation | Hard | Requires FFT on ADC samples |
| Local web server dashboard | Medium | No cloud dependency |
| OTA (Over-The-Air) firmware updates | Medium | ESP-IDF OTA component |
| CT sensor support (alternative to ACS712) | Medium | Hardware + calibration changes |
| Three-phase power monitoring | Hard | Major hardware expansion |
| Telegram / WhatsApp alerts | Easy | API integration |
| SD card data logging | Easy | SPI + fatfs |
| Auto ZMPT calibration routine | Medium | ADC + math |

### 💡 Suggestions
- Open a GitHub Issue with your idea, even if you're not planning to implement it
- Feedback on accuracy, safety design, or documentation is always welcome

### 🐛 Bug Reports
- Open an Issue with your serial monitor output, hardware setup, and what you expected vs. what happened

---

## Getting Started

### Prerequisites

**For the Arduino version (current stable):**
- Arduino IDE 2.x
- ESP32 board support package
- Libraries: Blynk, Adafruit IO Arduino, Preferences, WiFi, time.h

**For the ESP-IDF bare-metal version (in development):**
- ESP-IDF v5.x toolchain
- Windows: Use the ESP-IDF CMD terminal
- Linux/macOS: Standard shell

### Clone the Repo

```bash
git clone https://github.com/kumareshan3010/esp32-smart-power-monitor.git
cd esp32-smart-power-monitor
```

### Configuration

Fill in your credentials before building:

```cpp
#define WIFI_SSID        "your_wifi"
#define WIFI_PASSWORD    "your_password"
#define BLYNK_AUTH_TOKEN "your_blynk_token"
#define AIO_USERNAME     "your_aio_username"
#define AIO_KEY          "your_aio_key"
```

See the [Wiki](https://github.com/kumareshan3010/esp32-smart-power-monitor/wiki) for full setup, wiring, and calibration guides.

---

## Project Structure

```
esp32-smart-power-monitor/
├── esp32_power_monitor_public.ino   # Arduino version (stable)
├── README.md
├── CONTRIBUTING.md
└── LICENSE
```

> The ESP-IDF bare-metal port will be added as a separate folder once the migration is complete.

---

## Development Setup

### Hardware Required

| Component | Spec |
|---|---|
| Microcontroller | ESP32 WROOM-32 |
| Voltage Sensor | ZMPT101B |
| Current Sensor | ACS712-30A |
| Zero Cross Detector | Optocoupler-based ZCD |
| Capacitor Bank | 7 relays: 1, 2, 3, 4, 5, 6, 15 µF |
| Mains | 220V / 50Hz |

Refer to the [Wiring Guide](https://github.com/kumareshan3010/esp32-smart-power-monitor/wiki/Wiring-Guide) for full circuit diagrams.

### Calibration

After wiring, you must calibrate `ZMPT_SCALE` for your specific transformer. See the [Calibration Guide](https://github.com/kumareshan3010/esp32-smart-power-monitor/wiki/Calibration).

---

## Coding Guidelines

- Keep ISRs short — do minimal work inside interrupt handlers, use flags or queues
- Use `volatile` for variables shared between ISR and main loop
- Comment any non-obvious math (RMS, PFC formula, etc.) with the formula reference
- For the ESP-IDF port: prefer register-level access over HAL where performance matters
- Avoid blocking calls (delay, vTaskDelay with long durations) in critical paths
- Follow existing naming conventions: `snake_case` for variables and functions

---

## Submitting Changes

1. **Fork** the repository
2. **Create a branch** for your feature or fix:
   ```bash
   git checkout -b feature/thd-calculation
   ```
3. **Make your changes** and test on real hardware if possible
4. **Commit** with a clear message:
   ```bash
   git commit -m "Add FFT-based THD calculation for voltage waveform"
   ```
5. **Push** your branch and open a **Pull Request**
6. Describe what you changed, why, and how you tested it

For large changes, please open an Issue first to discuss the approach before writing code.

---

## Open Issues & Ideas

Check the [Issues tab](https://github.com/kumareshan3010/esp32-smart-power-monitor/issues) for open tasks. If you're new to the project, look for issues tagged `good first issue`.

---

## Safety Notice

> ⚠️ This project interfaces directly with **220V AC mains voltage**. Incorrect wiring can cause **serious injury or death**.
>
> Only work on the hardware if you are qualified to handle mains electricity. Always use proper isolation, fusing, and enclosed enclosures. Never touch live circuits. When in doubt, ask.

Hardware-related contributions must include appropriate safety notes in code comments and documentation.

---

## Contact

**Author:** Kumareshan V
**GitHub:** [@kumareshan3010](https://github.com/kumareshan3010)

Feel free to open an Issue for questions, or DM on Reddit if you found this project there.
