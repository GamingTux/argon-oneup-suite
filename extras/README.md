# Argon ONE UP Suite — Extras (Optional Fixes, Standby & Desktop Extensions)

This directory contains optional, environment-specific desktop widgets, standby managers, and hardware workarounds for the **Argon ONE UP Laptop** and **Raspberry Pi 5 / Compute Module 5 (CM5)**.

Keeping these packages in `extras/` ensures that the core hardware driver suite (`src/`) remains 100% lightweight, desktop-agnostic, and does not alter third-party system components.

---

## 📦 Available Extra Packages

| Package | Target System / Desktop | Architecture | Description |
| :--- | :---: | :---: | :--- |
| [**`rpi-vc4-edp`**](rpi-vc4-edp) | **Raspberry Pi CM4 / CM5 Laptops** | `all` | **Automatic Kernel eDP-1 Display Patcher:** Automatically patches and builds the `vc4` DRM driver across every kernel upgrade to expose internal HDMI laptop panels as native `eDP-1`. Fixes clamshell suspend, eliminates Wayland crashes, and auto-retires once upstream PR #7732 is released. |
| [**`cm5-mmc-fix`**](cm5-mmc-fix) | **Raspberry Pi CM5 Lite** | `all` | **Empty MicroSD Slot Shutdown Delay Fix:** Patches the BCM2712 SDHCI rescan timeout table (`mmc_rescan freqs[]`) in kernel images and applies `cm5-sdcard-fast.dtbo` across kernel upgrades to eliminate the 30–80s shutdown/reboot hang when the microSD slot is empty. |
| [**`ksystemstats-rpi5-fix`**](ksystemstats-rpi5-fix) | **KDE Plasma 6** | `arm64` | **KDE Plasma 6 Systemmonitor ksystemstats Fix:** Resolves the KDE Plasma 6 ARM64 `/proc/cpuinfo` deadlock ([KDE Bug #493093](https://bugs.kde.org/show_bug.cgi?id=493093)) via private mount namespace and maps `cpu_thermal` to CPU sensor cards. |
| [**`rpi-pishutdown`**](rpi-pishutdown) | **Raspberry Pi OS** (`rpd-labwc` / `LXDE`) | `all` | **Standby-Enabled Shutdown Dialog:** Drop-in `dpkg-divert` upgrade for `/usr/bin/pishutdown` on Raspberry Pi OS. Adds a dedicated "Standby (Ruhezustand)" button that triggers `systemctl suspend` via [**`rpi-suspend`**](https://github.com/GamingTux/rpi-suspend). |
| [**`argon-oneup-plasma-widget`**](argon-oneup-plasma-widget) | **KDE Plasma 6** (`org.argon.oneup.dashboard`) | `arm64` | **KDE Plasma 6 Hardware Telemetry Dashboard:** Real-time desktop widget showing per-core Cortex-A76 CPU load & frequency, VideoCore VII (V3D) GPU utilization & clock, CW2217 battery telemetry (Watts, voltage, health, cycles), RAM/NVMe/USB storage usage, temperatures (SoC, Battery, NVMe), PWM fan RPM, and one-click power profile switching (*Eco*, *Balanced*, *Turbo*) powered by a zero-overhead native C backend. |
| [**`argon-wfplug-batt`**](argon-wfplug-batt) | **Raspberry Pi OS** (`rpd-labwc` / `Wayfire` / `wf-panel-pi`) | `arm64` | **Enhanced Battery Telemetry Taskbar Widget:** Drop-in `dpkg-divert` upgrade for `/usr/lib/aarch64-linux-gnu/wf-panel-pi/libbatt.so`. Displays all available Linux `/sys/class/power_supply/BAT*` attributes on mouseover in a clean, structured order (status, smoothed remaining runtime, power in Watts, current in Amps, voltage + design bounds, energy/capacity in Wh & mAh, battery health %, cycle count, temperature, and battery model/technology), automatically hiding unsupported attributes. Also fixes upstream Cairo surface memory leaks. |

> [!NOTE]
> **Standby / Suspend (`rpi-suspend`):**
> Sleep and standby management for Raspberry Pi 5 / CM5 is now integrated as a core component of the suite in [`src/rpi-suspend`](../src/rpi-suspend) and installed automatically by the metapackage `argon-oneup-suite`. It directly replaces legacy `rpi-fake-sleep`.
> The benchmark utility `argon-power-bench` is included in [`argon-oneup-battery`](../src/argon-oneup-battery).

---

## 🚀 Installation Examples

### Raspberry Pi CM4/CM5 Laptop Internal Display eDP Patcher
```bash
sudo apt install ./rpi-vc4-edp_*.deb
```

### Raspberry Pi CM5 Empty MicroSD Slot Fix
```bash
sudo apt install ./cm5-mmc-fix_*.deb
```

### KDE Plasma 6 System Monitor Sensors Fix
```bash
sudo apt install ./ksystemstats-rpi5-fix_*.deb
```

### Raspberry Pi OS (`wf-panel-pi` Enhanced Battery Widget & Standby Dialog)
```bash
sudo apt install ./argon-wfplug-batt_*.deb ./rpi-pishutdown_*.deb
```

### KDE Plasma 6 (Desktop Telemetry Dashboard)
```bash
sudo apt install ./argon-oneup-plasma-widget_*.deb
```
