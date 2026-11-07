# Argon ONE UP Linux Driver & Power Management Suite (Raspberry Pi CM5 / Pi 5)

[![Release](https://img.shields.io/github/v/release/GamingTux/argon-oneup-suite?style=flat-square&color=00e5ff)](https://github.com/GamingTux/argon-oneup-suite/releases/latest)
[![CI & Package Build](https://img.shields.io/github/actions/workflow/status/GamingTux/argon-oneup-suite/ci.yml?branch=main&style=flat-square&label=CI%20Build)](https://github.com/GamingTux/argon-oneup-suite/actions)
[![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi%20CM5%20%2F%20Pi%205%20(ARM64)-blue?style=flat-square)](https://www.raspberrypi.com/)
[![OS Support](https://img.shields.io/badge/OS-LMPE%20%7C%20Raspberry%20Pi%20OS%20%7C%20Debian%20%7C%20Ubuntu-green?style=flat-square)](https://github.com/GamingTux/argon-oneup-suite)
[![License: MIT](https://img.shields.io/badge/License-MIT-green?style=flat-square)](LICENSE)

**Complete Linux hardware driver, power management, sleep/standby, and desktop integration suite for the [Argon ONE UP Laptop](https://argon40.com/) powered by the Raspberry Pi Compute Module 5 (CM5 / CM5 Lite).**

Out of the box, standard Linux distributions (LMPE / Linux Mint Pi Edition, Raspberry Pi OS 64-bit Trixie/Bookworm, LMDE 7 Gigi, Debian 12/13, Ubuntu 24.04/24.10, Cinnamon, KDE Plasma, GNOME, XFCE) lack native kernel drivers and hardware definitions for the Argon ONE UP laptop's custom embedded components. **Argon ONE UP Suite** bridges every hardware component directly into standard Linux kernel subsystems (`/sys/class/power_supply`, `/sys/class/backlight`, `EV_SW`/`SW_LID`, `hwmon`/`lm-sensors`, `UPower`, `systemd-logind`, and `power-profiles-daemon`)—with zero proprietary background bloat and lightning-fast **~5.8 second boot times**.

![Argon ONE UP KDE Plasma 6 Hardware Telemetry Dashboard](assets/dashboard-screenshot.png)

---

## ✨ Key Features & Hardware Support

* **🔋 Native Kernel Battery & AC Driver (`CW2217` Fuel Gauge at `0x64`):** Exposes `BAT0` and `AC0` via the standard Linux `power_supply` subsystem using atomic 16-bit SMBus word transfers (`i2c_smbus_read_word_swapped`). Reports state of charge (`%`), EMA-smoothed remaining runtime (`time_to_empty_avg`), real-time power draw (`W`), current (`A`, `current_now` & `current_avg`), pack voltage (`V`), energy (`53.28 Wh`) and charge capacity (`4800 mAh`), battery temperature (`°C`), state of health (`%`), and charge cycle count (`cycle_count`), with configurable critical low-battery auto-shutdown.
* **💡 Hardware Display Backlight (`DDC/CI` on I2C Bus 14):** Maps the internal laptop panel's DDC/CI brightness register (`0x10`) to `/sys/class/backlight/ddcci*`, enabling native brightness hotkeys (`Fn` + Brightness Up/Down), desktop sliders, and OSD notifications across Labwc, Wayfire, Cinnamon, KDE Plasma, and GNOME.
* **💻 Magnetic Hall-Effect Lid Switch (`GPIO 27`):** Device Tree overlay (`argon-oneup-lid.dtbo`) binds the physical lid sensor to `gpio-keys` (`EV_SW` / `SW_LID`), triggering native `systemd-logind` lid-close actions and instant lid-open wakeup.
* **📡 Hardware Essentials & APT Repository (`argon-oneup-essentials`):**
  * **External Display-Lid Wi-Fi/BT Antenna (`dtparam=ant2`):** Switches the CM5 wireless module from the internal PCB trace antenna (shielded inside the aluminum chassis) to the high-gain external antenna inside the display lid.
  * **PCIe Gen 3 NVMe Speed (`dtparam=nvme`, `pciex1_gen=3`):** Unlocks full ~900 MB/s M.2 NVMe SSD throughput (Speed 8 GT/s, Width x1).
  * **Hardware I2C Enablement (`dtparam=i2c_arm=on`):** Enables the SoC I2C bus required for the battery gauge (CW2217) and display DDC/CI backlight.
  * **Quiet 4-Stage Blower Fan Curve:** Configures a quiet 4-stage active blower fan curve (`40 / 50 / 60 / 70 °C` with `5 °C` hysteresis), idling at a cool ~39 °C.
  * **Official APT Repository:** Automatically installs the official repository sources and signing keyring for updates.
  * **Single Hardware Configuration:** Cleanly managed via `/etc/argon-oneup.conf`.
* **⚡ Dynamic ARM64 Power Profiles (`argon-oneup-power-profiles`):** Drop-in D-Bus replacement for `power-profiles-daemon` (`power-saver` @ 1.5 GHz, `balanced` @ dynamic `schedutil` 2.8 GHz, `performance` @ locked 2.8 GHz) with dynamic PCIe ASPM switching, automatic AC/battery profile switching, and per-profile display brightness memory.
* **💤 Low-Power Standby / Sleep ([`rpi-suspend`](https://github.com/GamingTux/rpi-suspend)):** Universal userspace sleep manager for Raspberry Pi 5 / CM5 where ACPI S3 is unavailable. Freezes user processes via `cgroups v2`, parks CPU cores C1–C3 in hardware `WFI` on `CPU0`, puts the internal USB 2.0 host tree and unused Ethernet PHY into power-down, blanks display via DRM DPMS, and wakes in **< 1 ms** on lid open or keypress.
* **🛠️ Raspberry Pi CM5 Lite Shutdown Hang Fix (`cm5-mmc-fix` in Extras):** Eliminates the 30–80 second shutdown/reboot hang caused by the BCM2712 SDHCI 100 kHz hardware timeout when the microSD slot is empty.

---

## 📦 Modular Package Architecture

The project is cleanly divided into **Core Hardware Packages** ([`src/`](src)) and optional **Desktop Extras** ([`extras/`](extras)).

### Core Hardware & Power Packages ([`src/`](src))

| Package | Arch | Description |
| :--- | :---: | :--- |
| [**`argon-oneup-essentials`**](src/argon-oneup-essentials) | `all` | **Hardware Setup & APT Repository:** Configures external lid antenna (`ant2`), PCIe Gen 3 NVMe, hardware I2C (`i2c_arm=on`), 4-stage quiet blower fan curve, and official APT repository keyring and sources list. Managed cleanly via `/etc/argon-oneup.conf`. |
| [**`argon-oneup-battery`**](src/argon-oneup-battery) | `all` | **Kernel Battery & AC Driver (`oneUpPower` DKMS) & Power Benchmark:** Native Linux `power_supply` driver for the CW2217 fuel gauge (`0x64`). Exports `BAT0` and `AC0` with atomic 16-bit SMBus reads, EMA-smoothed runtime estimation, 53.28 Wh & 4800 mAh capacity, cycle count, SOH %, temperature, and critical low-battery shutdown. Includes the `argon-power-bench` hardware power benchmark utility. |
| [**`argon-oneup-backlight`**](src/argon-oneup-backlight) | `all` | **DDC/CI Display Backlight Bridge:** Exposes the internal display on I2C bus 14 as `/sys/class/backlight/ddcci*` for universal brightness keys and desktop sliders. |
| [**`argon-oneup-lid`**](src/argon-oneup-lid) | `all` | **Magnetic Lid Switch Overlay:** Configures GPIO 27 as a native Linux `SW_LID` input device via Device Tree overlay. |
| [**`argon-oneup-sensors`**](src/argon-oneup-sensors) | `all` | **Hardware Sensors Mapping:** Standardizes `lm-sensors` physical hardware definitions (`/etc/sensors.d/argon-oneup.conf`) for CPU thermal, RP1, PWM Fan RPM, NVMe SSD temperature, and CW2217 battery telemetry across all Linux distributions. |
| [**`argon-oneup-power-profiles`**](src/argon-oneup-power-profiles) | `all` | **ARM64 Power Profiles Daemon:** Implements `org.freedesktop.UPower.PowerProfiles` / `net.hadess.PowerProfiles` D-Bus APIs to manage `schedutil`/`performance`/`powersave` CPU governors, PCIe ASPM, Wi-Fi power saving, and display backlight states. |
| [**`rpi-suspend`**](src/rpi-suspend) | `all` | **Universal Standby & Sleep Manager:** Deep userspace suspend (`cgroups v2` freeze, single-core WFI + IRQ parking on `CPU0`, USB autosuspend, DPMS off) with instant lid-open & keyboard wake. Drop-in replacement for legacy `rpi-fake-sleep`. |
| [**`argon-oneup-suite`**](src/argon-oneup-suite) | `all` | **Core Metapackage:** Installs all 7 core packages above with a single command. 100% system-agnostic and clean: modifies no third-party system components. |

### Extras: Optional Standby, Hardware Fixes & Desktop Widgets ([`extras/`](extras))

| Package | Target System / Desktop | Arch | Description |
| :--- | :---: | :---: | :--- |
| [**`rpi-vc4-edp`**](extras/rpi-vc4-edp) | **Raspberry Pi CM4 / CM5 Laptops** | `all` | **Automatic Kernel eDP-1 Display Patcher:** Automatically patches and builds the `vc4` DRM driver across every kernel upgrade to expose internal HDMI laptop panels as native `eDP-1`. Fixes clamshell suspend, eliminates Wayland crashes, and auto-retires once upstream PR #7732 is released. |
| [**`cm5-mmc-fix`**](extras/cm5-mmc-fix) | **Raspberry Pi CM5 Lite** | `all` | **CM5 Empty MicroSD Slot Shutdown Fix:** Patches the BCM2712 SDHCI rescan timeout table (`mmc_rescan freqs[]`) and applies `cm5-sdcard-fast.dtbo` across kernel upgrades to eliminate the 30–80s shutdown hang. |
| [**`ksystemstats-rpi5-fix`**](extras/ksystemstats-rpi5-fix) | **KDE Plasma 6** | `arm64` | **KDE Plasma 6 Systemmonitor ksystemstats Fix:** Resolves the KDE Plasma 6 `ksystemstats` ARM64 `/proc/cpuinfo` deadlock ([KDE Bug #493093](https://bugs.kde.org/show_bug.cgi?id=493093)) via private mount namespace and maps `cpu_thermal` to CPU sensor cards. |
| [**`rpi-pishutdown`**](extras/rpi-pishutdown) | **Raspberry Pi OS** (`rpd-labwc` / `LXDE`) | `all` | **Standby-Enabled Shutdown Dialog:** Drop-in `dpkg-divert` upgrade for `/usr/bin/pishutdown` on Raspberry Pi OS. Adds a dedicated "Standby (Ruhezustand)" button that triggers `systemctl suspend` via [**`rpi-suspend`**](https://github.com/GamingTux/rpi-suspend). |
| [**`argon-wfplug-batt`**](extras/argon-wfplug-batt) | **Raspberry Pi OS** (`rpd-labwc` / `wf-panel-pi`) | `arm64` | **Enhanced Battery Telemetry Widget:** Drop-in upgrade for `wf-panel-pi`'s `libbatt.so`. Displays complete `/sys/class/power_supply/BAT*` telemetry on mouseover (status, smoothed runtime, Watts, Amps, Voltage + bounds, Wh & mAh, Health %, Cycle Count, Temperature, Model) and fixes upstream Cairo surface memory leaks. |
| [**`argon-oneup-plasma-widget`**](extras/argon-oneup-plasma-widget) | **KDE Plasma 6** | `arm64` | **KDE Plasma 6 Desktop Dashboard (`org.argon.oneup.dashboard`):** Real-time CPU/GPU/Battery/Storage/Thermal/Fan telemetry widget and one-click power profile switcher with a compiled Cortex-A76 C backend. |

---

## ⚙️ Hardware Configuration (`/etc/argon-oneup.conf`)

All laptop hardware features are cleanly managed through a single configuration file:

```bash
sudo nano /etc/argon-oneup.conf
```

```ini
# Use external Wi-Fi/Bluetooth antenna in the display lid (dtparam=ant2)
ENABLE_ANT2=1

# Enable M.2 NVMe slot at PCIe Gen 3 speed (8 GT/s, ~900 MB/s)
ENABLE_NVME=1

# Enable hardware I2C bus (dtparam=i2c_arm=on) for battery gauge & display backlight
ENABLE_I2C=1

# Apply Argon ONE UP 4-stage quiet blower fan curve (40C/50C/60C/70C with 5C hysteresis)
ENABLE_FAN_CURVE=1
```

Apply any changes instantly without rebooting:
```bash
sudo argon-essentials-apply
```

---

## ⚡ Power Consumption & Battery Runtime Benchmarks

Measured on the **Argon ONE UP Laptop** (Raspberry Pi Compute Module 5 Lite, 8 GB LPDDR4X, 256 GB NVMe Gen 3 SSD, 53.28 Wh / 4800 mAh 3S1P Li-Ion battery, Kernel `6.18+rpt-rpi-2712`) using `sudo argon-power-bench`:

### 1. Standard Raspberry Pi OS (64-bit Debian 13 Trixie — `rpd-labwc` Desktop)

| Operating State | Avg Power (3S Pack) | Min .. Max | Avg Current (`I`) | SoC Temp | Real-World Battery Runtime |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Desktop Idle** *(Labwc active, 70% Display, Wi-Fi connected)* | **8.39 W** | `8.26 .. 8.85 W` | `-712 mA` | `40.5 – 42.1 °C` | **~ 6.4 – 6.6 Hours** |
| **Full CPU Stress Load** *(4× Cortex-A76 @ 100%, 2.80 GHz)* | **16.57 W** | `14.12 .. 17.56 W` | `-1438 mA` | `52.0 – 61.4 °C` | **~ 3.2 – 3.4 Hours** |
| **Fake-Sleep Standby (Steady-State)** *(Display off, C1–C3 WFI, USB & ETH PHY suspended)* | **3.34 W** | **`3.10 .. 3.55 W`** | **`-278 mA`** | `36.6 – 38.8 °C` | **~ 16.0 – 17.2 Hours** |

### 2. Debian 13 Trixie with KDE Plasma 6 (Wayland)

| Operating State | Avg Power (3S Pack) | Min .. Max | Avg Current (`I`) | SoC Temp | Real-World Battery Runtime |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Desktop Idle** *(KDE Plasma 6 active, 70% Display)* | **9.54 W** | `9.41 .. 9.88 W` | `-767 mA` | `33.6 – 36.4 °C` | **~ 5.6 – 5.8 Hours** |
| **Full CPU Stress Load** *(4× Cortex-A76 @ 100%, 2.80 GHz)* | **15.24 W** | `14.70 .. 16.40 W` | `-1240 mA` | `44.1 – 52.4 °C` | **~ 3.5 – 3.6 Hours** |
| **Fake-Sleep Standby (Steady-State)** | **3.46 W** | **`3.28 .. 3.59 W`** | **`-275 mA`** | `34.2 – 37.5 °C` | **~ 15.5 – 16.8 Hours** |

* **60–65% Standby Power Reduction:** In steady-state Fake-Sleep (`3.10 – 3.34 W`), total laptop power consumption drops by **over 5 to 6 Watts** compared to desktop idle, with instant **< 1 ms** wakeup on lid open or key press and only **118 IRQs/s** across the entire SoC.
* **Geekbench 7.1.0 ARM64 Performance (@ 2.80 GHz):**
  * **Desktop Active:** **902 Single-Core / 2,206 Multi-Core** ([View Result](https://browser.geekbench.com/v7/cpu/740896))
  * **Console (No GUI):** **919 Single-Core / 2,316 Multi-Core** ([View Result](https://browser.geekbench.com/v7/cpu/740978))

---

## 🚀 Quick Start & Installation

### Option A: Official APT Repository (Recommended — Automatic Updates)

Once `argon-oneup-essentials` is installed, the APT repository and GPG signing key are automatically configured. To set it up manually on any fresh LMPE, Raspberry Pi OS, Debian, or Ubuntu installation:

```bash
# 1. Install the official repository GPG keyring
sudo mkdir -p /usr/share/keyrings
curl -fsSL https://gamingtux.github.io/argon-oneup-suite/argon-oneup-archive-keyring.gpg | sudo tee /usr/share/keyrings/argon-oneup-archive-keyring.gpg > /dev/null

# 2. Add the repository sources list
sudo tee /etc/apt/sources.list.d/argon-oneup.sources << 'EOF'
Types: deb
URIs: https://gamingtux.github.io/argon-oneup-suite/
Suites: stable
Components: main
Signed-By: /usr/share/keyrings/argon-oneup-archive-keyring.gpg
EOF

# 3. Update and install the complete hardware suite
sudo apt update
sudo apt install argon-oneup-suite
```

### Option B: Offline Installation via `.deb` Packages

Download the pre-built `.deb` packages from the [**Latest GitHub Release**](https://github.com/GamingTux/argon-oneup-suite/releases/latest) or clone and build directly from source:

```bash
sudo apt update
sudo apt install ./argon-oneup-essentials_*.deb \
                 ./argon-oneup-battery_*.deb \
                 ./argon-oneup-backlight_*.deb \
                 ./argon-oneup-lid_*.deb \
                 ./argon-oneup-sensors_*.deb \
                 ./argon-oneup-power-profiles_*.deb \
                 ./rpi-suspend_*.deb \
                 ./argon-oneup-suite_*.deb
```

---

## 🛠️ Building from Source

```bash
git clone https://github.com/GamingTux/argon-oneup-suite.git
cd argon-oneup-suite

# Build all core packages (dist/*.deb) and extra widgets (dist/extras/*.deb)
./build_packages.sh
```

* Core packages are generated in `dist/*.deb`.
* Optional desktop widgets and standby managers are generated in `dist/extras/*.deb`.

---

## 🤝 Upstream Open-Source Contributions

Issues discovered and fixed during the development of this suite have been contributed back upstream to the Raspberry Pi and Argon ONE UP ecosystem:
* **Raspberry Pi Linux Kernel (`vc4` Internal eDP Panel Support for CM4/CM5):** [Kernel PR #7732](https://github.com/raspberrypi/linux/pull/7732) (`drm/vc4: hdmi: Add Device Tree override for internal eDP panels on CM4/CM5`). Exposes internal HDMI laptop displays natively as `eDP-1` in KMS, fixing clamshell suspend, Wayland desktop crashes, and display brightness association.
* **Raspberry Pi Linux Kernel (`vc4` HDMI HPD IRQ Storm Fix):** [Issue #7724](https://github.com/raspberrypi/linux/issues/7724) / [Kernel PR #7727](https://github.com/raspberrypi/linux/pull/7727) (`drm/vc4: hdmi: Only ignore hotplug interrupts on forced connectors`).
* **Raspberry Pi OS Taskbar Battery Plugin (`pplug-batt`):** [PR #16](https://github.com/raspberrypi-ui/pplug-batt/pull/16) (Fix Cairo surface memory leak, read `capacity` and `time_to_empty_avg` sysfs nodes, add extended battery telemetry & German localization).
* **Argon ONE UP Battery Kernel Driver (`oneUpPower`):** [PR #59](https://github.com/JeffCurless/argon-oneup/pull/59) (`oneUpPower: Add hardware cycle count, dynamic SOH, fractional SOC, and Linux-compliant health monitoring`). Brings the CW2217 fuel gauge driver into full compliance with Linux `power_supply` subsystem conventions, exposing hardware cycle counts, dynamic SOH cell degradation scaling, sub-percent SOC precision, and true 3S pack voltage.

---

## 📄 License

Released under the [MIT License](LICENSE) (kernel modules under `GPL-2.0-only`). Free and open-source for the Raspberry Pi and Linux community.
