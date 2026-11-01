# Argon ONE UP Suite — Extras (Optional Fixes, Standby & Desktop Extensions)

This directory contains optional, environment-specific desktop widgets, standby managers, and hardware workarounds for the **Argon ONE UP Laptop** and **Raspberry Pi 5 / Compute Module 5 (CM5)**.

Keeping these packages in `extras/` ensures that the core hardware driver suite (`src/`) remains 100% lightweight, desktop-agnostic, and does not alter third-party system components.

---

## 📦 Available Extra Packages

| Package | Target System / Desktop | Architecture | Description |
| :--- | :---: | :---: | :--- |
| [**`rpi-fake-sleep`**](rpi-fake-sleep) | **Universal Linux** (systemd) | `all` | **Universal Userspace Standby / Sleep Manager:** Deep userspace suspend (`cgroups v2` freeze, single-core WFI + IRQ parking on `CPU0`, USB 2.0 host/HID/audio autosuspend, DPMS off) + `argon-power-bench` hardware power benchmark tool. |
| [**`cm5-mmc-fix`**](cm5-mmc-fix) | **Raspberry Pi CM5 Lite** | `all` | **Empty MicroSD Slot Shutdown Delay Fix:** Patches the BCM2712 SDHCI rescan timeout table (`mmc_rescan freqs[]`) in kernel images and applies `cm5-sdcard-fast.dtbo` across kernel upgrades to eliminate the 30–80s shutdown/reboot hang when the microSD slot is empty. |
| [**`ksystemstats-rpi5-fix`**](ksystemstats-rpi5-fix) | **KDE Plasma 6** | `arm64` | **KDE Plasma 6 Systemmonitor ksystemstats Fix:** Resolves the KDE Plasma 6 ARM64 `/proc/cpuinfo` deadlock ([KDE Bug #493093](https://bugs.kde.org/show_bug.cgi?id=493093)) via private mount namespace and maps `cpu_thermal` to CPU sensor cards. |
| [**`rpi-pishutdown`**](rpi-pishutdown) | **Raspberry Pi OS** (`rpd-labwc` / `LXDE`) | `all` | **Standby-Enabled Shutdown Dialog:** Drop-in `dpkg-divert` upgrade for `/usr/bin/pishutdown` on Raspberry Pi OS. Adds a dedicated "Standby (Ruhezustand)" button that triggers `systemctl suspend` (`rpi-fake-sleep`). |
| [**`argon-oneup-plasma-widget`**](argon-oneup-plasma-widget) | **KDE Plasma 6** (`org.argon.oneup.dashboard`) | `arm64` | **KDE Plasma 6 Hardware Telemetry Dashboard:** Real-time desktop widget showing per-core Cortex-A76 CPU load & frequency, VideoCore VII (V3D) GPU utilization & clock, CW2217 battery telemetry (Watts, voltage, health, cycles), RAM/NVMe/USB storage usage, temperatures (SoC, Battery, NVMe), PWM fan RPM, and one-click power profile switching (*Eco*, *Balanced*, *Turbo*) powered by a zero-overhead native C backend. |
| [**`argon-wfplug-batt`**](argon-wfplug-batt) | **Raspberry Pi OS** (`rpd-labwc` / `Wayfire` / `wf-panel-pi`) | `arm64` | **Enhanced Battery Telemetry Taskbar Widget:** Drop-in `dpkg-divert` upgrade for `/usr/lib/aarch64-linux-gnu/wf-panel-pi/libbatt.so`. Displays all available Linux `/sys/class/power_supply/BAT*` attributes on mouseover in a clean, structured order (status, smoothed remaining runtime, power in Watts, current in Amps, voltage + design bounds, energy/capacity in Wh & mAh, battery health %, cycle count, temperature, and battery model/technology), automatically hiding unsupported attributes. Also fixes upstream Cairo surface memory leaks. |

---

## 🚀 Installation Examples

### Userspace Sleep / Standby
```bash
sudo apt install ./rpi-fake-sleep_1.2.5_all.deb
```

### Raspberry Pi CM5 Empty MicroSD Slot Fix
```bash
sudo apt install ./cm5-mmc-fix_1.2.5_all.deb
```

### KDE Plasma 6 System Monitor Sensors Fix
```bash
sudo apt install ./ksystemstats-rpi5-fix_1.2.5_arm64.deb
```

### Raspberry Pi OS (`wf-panel-pi` Enhanced Battery Widget & Standby Dialog)
```bash
sudo apt install ./argon-wfplug-batt_1.2.5_arm64.deb ./rpi-pishutdown_1.2.5_all.deb
```

### KDE Plasma 6 (Desktop Telemetry Dashboard)
```bash
sudo apt install ./argon-oneup-plasma-widget_1.2.5_arm64.deb
```
