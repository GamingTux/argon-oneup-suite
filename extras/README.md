# Argon ONE UP Suite — Extras (Desktop Widgets & Panel Applets)

This directory contains optional, environment-specific desktop widgets and taskbar plugins for the **Argon ONE UP Laptop** and **Raspberry Pi 5 / Compute Module 5 (CM5)**.

Keeping these packages in `extras/` ensures that the core hardware driver suite (`src/`) remains lightweight and desktop-agnostic.

---

## 📦 Available Extra Packages

| Package | Target Desktop | Architecture | Description |
| :--- | :---: | :---: | :--- |
| [**`argon-wfplug-batt`**](argon-wfplug-batt) | **Raspberry Pi OS** (`rpd-labwc` / `Wayfire` / `wf-panel-pi`) | `arm64` | **Enhanced Battery Telemetry Taskbar Widget:** Drop-in `dpkg-divert` upgrade for `/usr/lib/aarch64-linux-gnu/wf-panel-pi/libbatt.so`. Displays all available Linux `/sys/class/power_supply/BAT*` attributes on mouseover in a clean, structured order (status, smoothed remaining runtime, power in Watts, current in Amps, voltage + design bounds, energy/capacity in Wh & mAh, battery health %, cycle count, temperature, and battery model/technology), automatically hiding unsupported attributes. Also fixes upstream Cairo surface memory leaks. |
| [**`argon-oneup-plasma-widget`**](argon-oneup-plasma-widget) | **KDE Plasma 6** (`org.argon.oneup.dashboard`) | `arm64` | **KDE Plasma 6 Hardware Telemetry Dashboard:** Real-time desktop widget showing per-core Cortex-A76 CPU load & frequency, VideoCore VII (V3D) GPU utilization & clock, CW2217 battery telemetry (Watts, voltage, health, cycles), RAM/NVMe/USB storage usage, temperatures (SoC, Battery, NVMe), PWM fan RPM, and one-click power profile switching (*Eco*, *Balanced*, *Turbo*) powered by a zero-overhead native C backend. |

---

## 🚀 Installation

### Raspberry Pi OS (`wf-panel-pi` Enhanced Battery Widget)
```bash
sudo apt install ./argon-wfplug-batt_1.2.5_arm64.deb
```
*(Uninstalling `argon-wfplug-batt` automatically restores the original `wfplug-batt` plugin via `dpkg-divert`.)*

### KDE Plasma 6 (Desktop Telemetry Dashboard)
```bash
sudo apt install ./argon-oneup-plasma-widget_1.1.9_arm64.deb
```
