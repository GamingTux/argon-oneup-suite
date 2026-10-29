# Argon ONE UP Linux Driver & Power Management Suite (Raspberry Pi CM5 / Pi 5)

[![Release](https://img.shields.io/github/v/release/GamingTux/argon-oneup-suite?style=flat-square&color=00e5ff)](https://github.com/GamingTux/argon-oneup-suite/releases/latest)
[![CI & Package Build](https://img.shields.io/github/actions/workflow/status/GamingTux/argon-oneup-suite/ci.yml?branch=main&style=flat-square&label=CI%20Build)](https://github.com/GamingTux/argon-oneup-suite/actions)
[![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi%20CM5%20%2F%20Pi%205%20(ARM64)-blue?style=flat-square)](https://www.raspberrypi.com/)
[![OS Support](https://img.shields.io/badge/OS-Raspberry%20Pi%20OS%20%7C%20Debian%2013%20%7C%20Ubuntu-red?style=flat-square)](https://www.debian.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green?style=flat-square)](LICENSE)

**Complete Linux hardware driver, power management, sleep/standby, and desktop integration suite for the [Argon ONE UP Laptop](https://argon40.com/) powered by the Raspberry Pi Compute Module 5 (CM5 / CM5 Lite).**

Out of the box, standard Linux distributions (Raspberry Pi OS 64-bit Trixie/Bookworm, Debian 13, Ubuntu, KDE Plasma, GNOME) lack native kernel drivers and hardware definitions for the Argon ONE UP laptop's custom embedded components. **Argon ONE UP Suite** bridges every hardware component directly into standard Linux kernel subsystems (`/sys/class/power_supply`, `/sys/class/backlight`, `EV_SW`/`SW_LID`, `hwmon`/`lm-sensors`, `UPower`, `systemd-logind`, and `power-profiles-daemon`)—with zero proprietary background bloat.

![Argon ONE UP KDE Plasma 6 Hardware Telemetry Dashboard](assets/dashboard-screenshot.png)

---

## ✨ Key Features & Hardware Support

* **🔋 Native Kernel Battery & AC Driver (`CW2217` Fuel Gauge at `0x64`):** Exposes `BAT0` and `AC0` via the standard Linux `power_supply` subsystem using atomic 16-bit SMBus word transfers (`i2c_smbus_read_word_swapped`). Reports state of charge (`%`), EMA-smoothed remaining runtime (`time_to_empty_avg`), real-time power draw (`W`), current (`A`, `current_now` & `current_avg`), pack voltage (`V`), energy (`53.28 Wh`) and charge capacity (`4800 mAh`), battery temperature (`°C`), state of health (`%`), and charge cycle count (`cycle_count`), with configurable critical low-battery auto-shutdown.
* **💤 True Low-Power Standby / Sleep (`3.10 – 3.34 W`):** Universal userspace sleep manager (`rpi-fake-sleep`) for Raspberry Pi 5 / CM5 where ACPI S3 / hardware `s2idle` is unavailable. Freezes user processes via `cgroups v2`, parks CPU cores C1–C3 in hardware `WFI` via single-core `cpuset` + workqueue + GICv2 Level-IRQ pinning to `CPU0`, puts the internal USB 2.0 host tree (`dwc2`, `snd-usb-audio`, `usbhid`) and unused Broadcom `BCM54213PE` Ethernet PHY into hardware power-down, blanks the display via sysfs/Wayland/X11/DRM DPMS, and wakes in **< 1 ms** on lid open or keypress.
* **💡 Hardware Display Backlight (`DDC/CI` on I2C Bus 14):** Maps the internal laptop panel's DDC/CI brightness register (`0x10`) to `/sys/class/backlight/ddcci*`, enabling native brightness hotkeys (`Fn` + Brightness Up/Down), desktop sliders, and OSD notifications across Labwc, Wayfire, KDE Plasma, and GNOME.
* **💻 Magnetic Hall-Effect Lid Switch (`GPIO 27`):** Device Tree overlay (`argon-oneup-lid.dtbo`) binds the physical lid sensor to `gpio-keys` (`EV_SW` / `SW_LID`), triggering native `systemd-logind` lid-close suspend and lid-open wakeup.
* **📡 Hardware Essentials & Performance Tuning (`argon-oneup-essentials`):**
  * **External Display-Lid Wi-Fi/BT Antenna (`dtparam=ant2`):** Switches the CM5 wireless module from the internal PCB trace antenna (shielded inside the aluminum chassis) to the external display-lid antenna.
  * **PCIe Gen 3 NVMe Speed (`dtparam=nvme`, `pciex1_gen=3`):** Unlocks full ~900 MB/s M.2 NVMe SSD throughput with `rq_affinity=2`.
  * **USB 2.0 Host Bus & Bootloader EEPROM Tuning (`PSU_MAX_CURRENT=5000`, `POWER_OFF_ON_HALT=1`, `SDRAM_BANKLOW=1`):** Enables the internal keyboard, touchpad, webcam, and C-Media USB audio controller with 5A PMIC profile, true PMIC rail halt power-off, and BCM2712 8-bank LPDDR4X memory interleaving (+15–25% RAM bandwidth).
  * **2.8 GHz CPU Boost, `schedutil` Governor & Quiet 4-Stage Fan Curve (`arm_freq=2800`):** Configures 2.8 GHz Cortex-A76 boost clock, low-latency `schedutil` governor (`rate_limit_us=1000`), and a quiet 4-stage PWM blower fan curve (`40 / 50 / 60 / 70 °C` with `5 °C` hysteresis).
  * **Multi-Core GICv2 Interrupt Spreading & 4-Core Network RPS:** Distributes heavy ARM64 `Level` interrupts across all 4 cores (`CPU0`: `rp1`/`xhci`, `CPU1`: `dwc2`, `CPU2`: `v3d`, `CPU3`: `vc4`/`hvs`/`mmc`) protected via `/usr/bin/argon-oneup-irq-policy`, paired with 4-core Receive Packet Steering (`rps_cpus=f`) and TCP BBR.
  * **`lz4` ZRAM, VM Sysctl & Desktop Polish:** Configures ultra-fast `lz4` ZRAM with `vm.swappiness=15` and a 64 MB DMA reserve (`vm.min_free_kbytes=65536`), Subpixel RGB font rendering, touchpad tap-to-click, and USB webcam/audio quirks.
* **⚡ Dynamic ARM64 Power Profiles (`argon-power-profiles`):** Drop-in D-Bus replacement for `power-profiles-daemon` (`power-saver` @ 1.5 GHz, `balanced` @ dynamic `schedutil` 2.8 GHz, `performance` @ locked 2.8 GHz) with dynamic PCIe ASPM switching, automatic AC/battery profile switching, and per-profile display brightness memory.
* **🛠️ Raspberry Pi CM5 Lite Shutdown Hang Fix (`cm5-mmc-fix`):** Eliminates the 30–80 second shutdown/reboot hang caused by the BCM2712 SDHCI 100 kHz hardware timeout when the microSD slot is empty.

---

## 📦 Modular Package Architecture

The project is cleanly divided into **9 Core Hardware & System Packages** ([`src/`](src)) and optional **Desktop Extras** ([`extras/`](extras)).

### Core Hardware & Power Packages ([`src/`](src))

| Package | Arch | Description |
| :--- | :---: | :--- |
| [**`argon-oneup-essentials`**](src/argon-oneup-essentials) | `all` | **Hardware Setup & System Optimization:** Configures external lid antenna (`ant2`), PCIe Gen 3 NVMe, internal `dwc2` USB 2.0 host bus, EEPROM tuning (`PSU_MAX_CURRENT=5000`, `POWER_OFF_ON_HALT=1`, `SDRAM_BANKLOW=1`), 2.8 GHz boost, `schedutil` governor, GICv2 multi-core IRQ spreading (`argon-oneup-irq-policy`), 4-core network RPS, `lz4` ZRAM (`swappiness=15`), 4-stage fan curve, `eth0` MDIO PHY power-down, USB webcam/audio quirks, Subpixel RGB font rendering, and touchpad tap-to-click. |
| [**`argon-oneup-battery`**](src/argon-oneup-battery) | `all` | **Kernel Battery & AC Driver (`oneUpPower` DKMS v1.1.0):** Native Linux `power_supply` driver for the CW2217 fuel gauge (`0x64`). Exports `BAT0` and `AC0` with atomic 16-bit SMBus reads, EMA-smoothed runtime estimation, 53.28 Wh & 4800 mAh capacity, cycle count, SOH %, temperature, and critical low-battery shutdown. |
| [**`argon-oneup-backlight`**](src/argon-oneup-backlight) | `all` | **DDC/CI Display Backlight Bridge:** Exposes the internal display on I2C bus 14 as `/sys/class/backlight/ddcci*` for universal brightness keys and desktop sliders. |
| [**`argon-oneup-lid`**](src/argon-oneup-lid) | `all` | **Magnetic Lid Switch Overlay:** Configures GPIO 27 as a native Linux `SW_LID` input device via Device Tree overlay. |
| [**`argon-oneup-sensors`**](src/argon-oneup-sensors) | `arm64` | **Hardware Sensors & KDE Plasma 6 Fix:** Standardizes `lm-sensors` labels (CPU, RP1, PWM Fan, NVMe, Battery) and resolves the KDE Plasma 6 `ksystemstats` ARM64 `/proc/cpuinfo` deadlock ([KDE Bug #493093](https://bugs.kde.org/show_bug.cgi?id=493093)). |
| [**`argon-power-profiles`**](src/argon-power-profiles) | `all` | **ARM64 Power Profiles Daemon:** Implements `org.freedesktop.UPower.PowerProfiles` / `net.hadess.PowerProfiles` D-Bus APIs to manage `schedutil`/`performance`/`powersave` CPU governors, PCIe ASPM, Wi-Fi power saving, and display backlight states. |
| [**`cm5-mmc-fix`**](src/cm5-mmc-fix) | `all` | **CM5 Empty MicroSD Slot Shutdown Fix:** Patches the BCM2712 SDHCI rescan timeout and applies `cm5-sdcard-fast.dtbo` across kernel upgrades while preserving microSD hotplugging. |
| [**`rpi-fake-sleep`**](src/rpi-fake-sleep) | `all` | **Universal Standby & Sleep Manager:** Deep userspace suspend (`cgroups v2` freeze, single-core WFI + IRQ parking on `CPU0`, USB 2.0 host/HID/audio autosuspend, DPMS off) + `argon-power-bench` hardware power benchmark tool. |
| [**`argon-oneup-suite`**](src/argon-oneup-suite) | `all` | **Metapackage:** Installs all 8 core hardware and power management packages above with a single command. |

### Extras: Optional Desktop Widgets & Applets ([`extras/`](extras))

| Package | Target Desktop | Arch | Description |
| :--- | :---: | :---: | :--- |
| [**`argon-wfplug-batt`**](extras/argon-wfplug-batt) | **Raspberry Pi OS** (`rpd-labwc` / `wf-panel-pi`) | `arm64` | **Enhanced Battery Telemetry Widget:** Drop-in upgrade for `wf-panel-pi`'s `libbatt.so`. Displays complete `/sys/class/power_supply/BAT*` telemetry on mouseover (status, smoothed runtime, Watts, Amps, Voltage + bounds, Wh & mAh, Health %, Cycle Count, Temperature, Model) and fixes upstream Cairo surface memory leaks. |
| [**`argon-oneup-plasma-widget`**](extras/argon-oneup-plasma-widget) | **KDE Plasma 6** | `arm64` | **KDE Plasma 6 Desktop Dashboard (`org.argon.oneup.dashboard`):** Real-time CPU/GPU/Battery/Storage/Thermal/Fan telemetry widget and one-click power profile switcher with a compiled Cortex-A76 C backend. |

---

## ⚡ Power Consumption & Battery Runtime Benchmarks (`v1.2.5`)

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

Download the pre-built `.deb` packages from the [**Latest GitHub Release**](https://github.com/GamingTux/argon-oneup-suite/releases/latest) or clone and build directly from source.

### 1. Install the Core Hardware Suite (Recommended for All Desktops)

```bash
sudo apt update
sudo apt install ./argon-oneup-essentials_*.deb \
                 ./argon-oneup-battery_*.deb \
                 ./argon-oneup-backlight_*.deb \
                 ./argon-oneup-lid_*.deb \
                 ./argon-oneup-sensors_*.deb \
                 ./argon-power-profiles_*.deb \
                 ./cm5-mmc-fix_*.deb \
                 ./rpi-fake-sleep_*.deb \
                 ./argon-oneup-suite_*.deb
```

### 2. Install Optional Desktop Extras ([`extras/`](extras))

* **For Raspberry Pi OS (`rpd-labwc` / `wf-panel-pi` Taskbar):**
  ```bash
  sudo apt install ./argon-wfplug-batt_*.deb
  ```
* **For KDE Plasma 6 Desktop:**
  ```bash
  sudo apt install ./argon-oneup-plasma-widget_*.deb
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
* Optional desktop widgets are generated in `dist/extras/*.deb`.

---

## 🤝 Upstream Open-Source Contributions

Issues discovered and fixed during the development of this suite have been contributed back upstream to the Raspberry Pi ecosystem:
* **Raspberry Pi Linux Kernel (`vc4` HDMI HPD IRQ Storm Fix):** [Issue #7724](https://github.com/raspberrypi/linux/issues/7724) / [Kernel PR #7727](https://github.com/raspberrypi/linux/pull/7727) (`drm/vc4: hdmi: Only ignore hotplug interrupts on forced connectors`).
* **Raspberry Pi OS Taskbar Battery Plugin (`pplug-batt`):** [PR #16](https://github.com/raspberrypi-ui/pplug-batt/pull/16) (Fix Cairo surface memory leak, read `capacity` and `time_to_empty_avg` sysfs nodes, add extended battery telemetry & German localization).

---

## 📄 License

Released under the [MIT License](LICENSE) (kernel modules under `GPL-2.0-only`). Free and open-source for the Raspberry Pi and Linux community.
