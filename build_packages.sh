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
    echo "Compiling argon-oneup-telemetry binary (C / Cortex-A76 + LTO + Vectorize)..."
    "${CC_BIN}" -O3 -pipe -march=armv8.2-a+crypto+fp16+dotprod -mtune=cortex-a76 -ftree-vectorize -flto=auto -ffat-lto-objects "${EXTRAS_SRC_DIR}/argon-oneup-plasma-widget/usr/src/argon-oneup-telemetry.c" -o "${EXTRAS_SRC_DIR}/argon-oneup-plasma-widget/usr/bin/argon-oneup-telemetry"
    "${STRIP_BIN}" "${EXTRAS_SRC_DIR}/argon-oneup-plasma-widget/usr/bin/argon-oneup-telemetry"
fi

if [ -f "${EXTRAS_SRC_DIR}/ksystemstats-rpi5-fix/usr/src/ksystemstats-ns-helper.c" ]; then
    echo "Compiling ksystemstats-ns-helper binary (C / Cortex-A76 + LTO + Vectorize)..."
    "${CC_BIN}" -O2 -pipe -Wall -Wextra -march=armv8.2-a+crypto+fp16+dotprod -mtune=cortex-a76 -ftree-vectorize -flto=auto -ffat-lto-objects "${EXTRAS_SRC_DIR}/ksystemstats-rpi5-fix/usr/src/ksystemstats-ns-helper.c" -o "${EXTRAS_SRC_DIR}/ksystemstats-rpi5-fix/usr/bin/ksystemstats-ns-helper"
    "${STRIP_BIN}" "${EXTRAS_SRC_DIR}/ksystemstats-rpi5-fix/usr/bin/ksystemstats-ns-helper"
fi

if command -v dtc >/dev/null 2>&1; then
    echo "Compiling Device Tree Overlays (.dts -> .dtbo)..."
    dtc -@ -I dts -O dtb -Wno-gpios_property \
        -o "${SRC_DIR}/argon-oneup-lid/usr/share/argon-oneup/overlays/argon-oneup-lid.dtbo" \
        "${SRC_DIR}/argon-oneup-lid/usr/share/argon-oneup/dts/argon-oneup-lid.dts"
    dtc -@ -I dts -O dtb -Wno-gpios_property \
        -o "${SRC_DIR}/argon-oneup-battery/usr/share/argon-oneup/overlays/argon-oneup-battery.dtbo" \
        "${SRC_DIR}/argon-oneup-battery/usr/share/argon-oneup/dts/argon-oneup-battery.dts"
    if [ -f "${EXTRAS_SRC_DIR}/cm5-mmc-fix/usr/share/cm5-mmc-fix/cm5-sdcard-fast.dts" ]; then
        dtc -@ -I dts -O dtb -Wno-gpios_property \
            -o "${EXTRAS_SRC_DIR}/cm5-mmc-fix/usr/share/cm5-mmc-fix/overlays/cm5-sdcard-fast.dtbo" \
            "${EXTRAS_SRC_DIR}/cm5-mmc-fix/usr/share/cm5-mmc-fix/cm5-sdcard-fast.dts"
    fi
    if [ -f "${EXTRAS_SRC_DIR}/rpi-vc4-edp/usr/share/rpi-vc4-edp/dts/argon-oneup-display.dts" ]; then
        dtc -@ -I dts -O dtb -Wno-gpios_property \
            -o "${EXTRAS_SRC_DIR}/rpi-vc4-edp/usr/share/rpi-vc4-edp/overlays/argon-oneup-display.dtbo" \
            "${EXTRAS_SRC_DIR}/rpi-vc4-edp/usr/share/rpi-vc4-edp/dts/argon-oneup-display.dts"
    fi
fi

echo "Normalizing file and directory permissions (0755 / 0644)..."
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -type d -exec chmod 0755 {} +
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -type f -perm -0100 -exec chmod 0755 {} +
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -type f -not -perm -0100 -exec chmod 0644 {} +
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -path "*/DEBIAN/post*" -exec chmod 0755 {} +
find "${SRC_DIR}" "${EXTRAS_SRC_DIR}" -path "*/DEBIAN/pre*" -exec chmod 0755 {} +

GIT_COMMIT_COUNT=$(git rev-list --count HEAD 2>/dev/null || echo "")
GIT_SHORT_HASH=$(git rev-parse --short HEAD 2>/dev/null || echo "")
EXACT_TAG=$(git describe --tags --exact-match 2>/dev/null || echo "")

GIT_SUFFIX=""
if [ -z "${EXACT_TAG}" ] && [ -n "${GIT_COMMIT_COUNT}" ] && [ -n "${GIT_SHORT_HASH}" ]; then
    GIT_SUFFIX="+git${GIT_COMMIT_COUNT}.${GIT_SHORT_HASH}"
    echo "=== Rolling Continuous Delivery: Version suffix ${GIT_SUFFIX} ==="
else
    echo "=== Release Build: Clean versions from control files (Tag: ${EXACT_TAG:-none}) ==="
fi

package_dir() {
    local src_path="$1"
    local dest_dir="$2"
    local pkg="$3"

    local arch
    arch=$(grep "^Architecture:" "${src_path}/DEBIAN/control" | awk '{print $2}')
    local base_ver
    base_ver=$(grep "^Version:" "${src_path}/DEBIAN/control" | awk '{print $2}')
    local final_ver="${base_ver}${GIT_SUFFIX}"
    local deb_name="${pkg}_${final_ver}_${arch}.deb"

    echo "Building package: ${pkg} (${final_ver} [${arch}])..."

    local stage_dir
    stage_dir=$(mktemp -d)
    cp -a "${src_path}" "${stage_dir}/${pkg}"

    if [ -n "${GIT_SUFFIX}" ]; then
        sed -i "s/^Version: .*/Version: ${final_ver}/" "${stage_dir}/${pkg}/DEBIAN/control"
        sed -i "s/argon-power-profiles (= [^)]*)/argon-power-profiles (= ${final_ver})/" "${stage_dir}/${pkg}/DEBIAN/control" 2>/dev/null || true
        sed -i "s/argon-power-profiles (<< [^)]*)/argon-power-profiles (<< ${final_ver})/" "${stage_dir}/${pkg}/DEBIAN/control" 2>/dev/null || true
    fi

    dpkg-deb --root-owner-group -b "${stage_dir}/${pkg}" "${dest_dir}/${deb_name}"
    rm -rf "${stage_dir}"
}

for pkg in argon-oneup-essentials argon-oneup-backlight argon-oneup-battery argon-oneup-lid argon-oneup-sensors argon-oneup-power-profiles rpi-suspend argon-oneup-suite; do
    PKG_PATH="${SRC_DIR}/${pkg}"
    if [ -d "${PKG_PATH}" ]; then
        package_dir "${PKG_PATH}" "${DIST_DIR}" "${pkg}"
    fi
done

echo "=== Building Argon ONE UP Extras (Optional Fixes, Standby, Widgets & Applets) ==="

for pkg in argon-oneup-plasma-widget argon-wfplug-batt cm5-mmc-fix ksystemstats-rpi5-fix rpi-pishutdown rpi-vc4-edp; do
    PKG_PATH="${EXTRAS_SRC_DIR}/${pkg}"
    if [ -d "${PKG_PATH}" ]; then
        package_dir "${PKG_PATH}" "${EXTRAS_DIST_DIR}" "${pkg}"
    fi
done

cd "${DIST_DIR}"
sha256sum *.deb extras/*.deb > SHA256SUMS
echo "=== All packages built successfully into ${DIST_DIR} and ${EXTRAS_DIST_DIR} ==="
ls -lh "${DIST_DIR}"/*.deb "${EXTRAS_DIST_DIR}"/*.deb
