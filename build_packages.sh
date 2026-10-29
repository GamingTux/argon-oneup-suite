#!/usr/bin/env bash
set -e

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${REPO_DIR}/src"
EXTRAS_SRC_DIR="${REPO_DIR}/extras"
DIST_DIR="${REPO_DIR}/dist"
EXTRAS_DIST_DIR="${DIST_DIR}/extras"

mkdir -p "${DIST_DIR}" "${EXTRAS_DIST_DIR}"
rm -f "${DIST_DIR}"/*.deb "${EXTRAS_DIST_DIR}"/*.deb

echo "=== Cleaning temporary files ==="
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -type f -name "*.pyc" -delete 2>/dev/null || true
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -type f -name "*~" -delete 2>/dev/null || true

echo "=== Building Argon ONE UP Core Debian Packages ==="

if [ "$(uname -m)" = "aarch64" ]; then
    CC_BIN="gcc"
    STRIP_BIN="strip"
else
    CC_BIN="aarch64-linux-gnu-gcc"
    STRIP_BIN="aarch64-linux-gnu-strip"
fi

if [ -f "${EXTRAS_SRC_DIR}/argon-oneup-plasma-widget/usr/src/argon-oneup-telemetry.c" ]; then
    echo "Compiling argon-oneup-telemetry binary (C / Cortex-A76)..."
    "${CC_BIN}" -O3 -pipe -mcpu=cortex-a76+crypto+dotprod "${EXTRAS_SRC_DIR}/argon-oneup-plasma-widget/usr/src/argon-oneup-telemetry.c" -o "${EXTRAS_SRC_DIR}/argon-oneup-plasma-widget/usr/bin/argon-oneup-telemetry"
    "${STRIP_BIN}" "${EXTRAS_SRC_DIR}/argon-oneup-plasma-widget/usr/bin/argon-oneup-telemetry"
fi

if [ -f "${SRC_DIR}/argon-oneup-sensors/usr/src/ksystemstats-ns-helper.c" ]; then
    echo "Compiling ksystemstats-ns-helper binary (C / Cortex-A76)..."
    "${CC_BIN}" -O2 -pipe -Wall -Wextra -mcpu=cortex-a76 "${SRC_DIR}/argon-oneup-sensors/usr/src/ksystemstats-ns-helper.c" -o "${SRC_DIR}/argon-oneup-sensors/usr/bin/ksystemstats-ns-helper"
    "${STRIP_BIN}" "${SRC_DIR}/argon-oneup-sensors/usr/bin/ksystemstats-ns-helper"
fi

if command -v dtc >/dev/null 2>&1; then
    echo "Compiling Device Tree Overlays (.dts -> .dtbo)..."
    dtc -@ -I dts -O dtb -Wno-gpios_property \
        -o "${SRC_DIR}/argon-oneup-lid/usr/share/argon-oneup/overlays/argon-oneup-lid.dtbo" \
        "${SRC_DIR}/argon-oneup-lid/usr/share/argon-oneup/dts/argon-oneup-lid.dts"
    dtc -@ -I dts -O dtb -Wno-gpios_property \
        -o "${SRC_DIR}/argon-oneup-battery/usr/share/argon-oneup/overlays/argon-oneup-battery.dtbo" \
        "${SRC_DIR}/argon-oneup-battery/usr/share/argon-oneup/dts/argon-oneup-battery.dts"
    dtc -@ -I dts -O dtb -Wno-gpios_property \
        -o "${SRC_DIR}/cm5-mmc-fix/usr/share/cm5-mmc-fix/overlays/cm5-sdcard-fast.dtbo" \
        "${SRC_DIR}/cm5-mmc-fix/usr/share/cm5-mmc-fix/cm5-sdcard-fast.dts"
fi

echo "Normalizing file and directory permissions (0755 / 0644)..."
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -type d -exec chmod 0755 {} +
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -type f -perm -0100 -exec chmod 0755 {} +
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -type f -not -perm -0100 -exec chmod 0644 {} +

for pkg in argon-oneup-essentials argon-oneup-backlight argon-oneup-battery argon-oneup-lid argon-oneup-sensors argon-power-profiles cm5-mmc-fix rpi-fake-sleep argon-oneup-suite; do
    PKG_PATH="${SRC_DIR}/${pkg}"
    if [ -d "${PKG_PATH}" ]; then
        echo "Building ${pkg}..."
        ARCH=$(grep "^Architecture:" "${PKG_PATH}/DEBIAN/control" | awk '{print $2}')
        VER=$(grep "^Version:" "${PKG_PATH}/DEBIAN/control" | awk '{print $2}')
        DEB_NAME="${pkg}_${VER}_${ARCH}.deb"
        dpkg-deb --root-owner-group -b "${PKG_PATH}" "${DIST_DIR}/${DEB_NAME}"
    fi
done

echo "=== Building Argon ONE UP Extras (Desktop Widgets & Applets) ==="

for pkg in argon-oneup-plasma-widget argon-wfplug-batt; do
    PKG_PATH="${EXTRAS_SRC_DIR}/${pkg}"
    if [ -d "${PKG_PATH}" ]; then
        echo "Building extra: ${pkg}..."
        ARCH=$(grep "^Architecture:" "${PKG_PATH}/DEBIAN/control" | awk '{print $2}')
        VER=$(grep "^Version:" "${PKG_PATH}/DEBIAN/control" | awk '{print $2}')
        DEB_NAME="${pkg}_${VER}_${ARCH}.deb"
        dpkg-deb --root-owner-group -b "${PKG_PATH}" "${DIST_DIR}/extras/${DEB_NAME}"
    fi
done

cd "${DIST_DIR}"
sha256sum *.deb extras/*.deb > SHA256SUMS
echo "=== All packages built successfully into ${DIST_DIR} and ${EXTRAS_DIST_DIR} ==="
ls -lh "${DIST_DIR}"/*.deb "${EXTRAS_DIST_DIR}"/*.deb
