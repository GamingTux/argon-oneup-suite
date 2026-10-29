#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""
argononeup-eepromconfig.py — Ensures optimal Raspberry Pi 5 / CM5 bootloader EEPROM
settings (PSU_MAX_CURRENT=5000, POWER_OFF_ON_HALT=1, SDRAM_BANKLOW=1) are configured
using the system's rpi-eeprom-config utility.
"""
import os
import shutil
import subprocess
import sys
import tempfile

DESIRED_SETTINGS = {
    "PSU_MAX_CURRENT": "5000",
    "POWER_OFF_ON_HALT": "1",
    "SDRAM_BANKLOW": "1",
}


def is_bcm2712() -> bool:
    compat_path = "/sys/firmware/devicetree/base/compatible"
    try:
        with open(compat_path, "rb") as f:
            return b"bcm2712" in f.read()
    except OSError:
        return False


def main() -> int:
    if os.geteuid() != 0:
        sys.stderr.write("ERROR: Please run as root\n")
        return 1

    if not is_bcm2712() or not shutil.which("rpi-eeprom-config"):
        return 0

    try:
        res = subprocess.run(
            ["rpi-eeprom-config"],
            capture_output=True,
            text=True,
            timeout=15,
            check=True,
        )
        current_config = res.stdout.replace("\r", "")
    except Exception as e:
        sys.stderr.write(f"WARNING: Could not read EEPROM config: {e}\n")
        return 0

    lines = current_config.splitlines()
    found_keys = set()
    needs_update = False
    new_lines = []

    for line in lines:
        stripped = line.strip()
        if "=" in stripped and not stripped.startswith("#"):
            key, val = stripped.split("=", 1)
            key = key.strip()
            if key in DESIRED_SETTINGS:
                found_keys.add(key)
                target_line = f"{key}={DESIRED_SETTINGS[key]}"
                if stripped != target_line:
                    new_lines.append(target_line)
                    needs_update = True
                else:
                    new_lines.append(line)
                continue
        new_lines.append(line)

    missing_lines = [
        f"{k}={v}" for k, v in DESIRED_SETTINGS.items() if k not in found_keys
    ]
    if missing_lines:
        needs_update = True
        inserted = False
        for i, line in enumerate(new_lines):
            if line.strip() == "[all]":
                for offset, mline in enumerate(missing_lines):
                    new_lines.insert(i + 1 + offset, mline)
                inserted = True
                break
        if not inserted:
            new_lines.append("[all]")
            new_lines.extend(missing_lines)

    if not needs_update:
        print("EEPROM settings up to date")
        return 0

    fd, tmp_conf = tempfile.mkstemp(prefix="argon-eeprom-", suffix=".conf")
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write("\n".join(new_lines) + "\n")
        subprocess.run(
            ["rpi-eeprom-config", "--apply", tmp_conf],
            timeout=30,
            check=True,
        )
    except Exception as e:
        sys.stderr.write(f"WARNING: Failed to apply EEPROM config: {e}\n")
        return 1
    finally:
        try:
            os.remove(tmp_conf)
        except OSError:
            pass

    return 0


if __name__ == "__main__":
    sys.exit(main())
