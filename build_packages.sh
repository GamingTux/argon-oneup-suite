#!/usr/bin/env bash
set -e

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${REPO_DIR}/src"
DIST_DIR="${REPO_DIR}/dist"

mkdir -p "${DIST_DIR}"
rm -f "${DIST_DIR}"/*.deb

echo "=== Cleaning temporary files ==="
find "${SRC_DIR}" -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
find "${SRC_DIR}" -type f -name "*.pyc" -delete 2>/dev/null || true
find "${SRC_DIR}" -type f -name "*~" -delete 2>/dev/null || true

echo "=== Building Argon ONE UP Debian Packages ==="

if [ -f "${SRC_DIR}/argon-oneup-plasma-widget/argon-oneup-telemetry.c" ]; then
    echo "Compiling argon-oneup-telemetry binary (C / Cortex-A76)..."
    if [ "$(uname -m)" = "aarch64" ]; then
        CC_BIN="gcc"
        STRIP_BIN="strip"
    else
        CC_BIN="aarch64-linux-gnu-gcc"
        STRIP_BIN="aarch64-linux-gnu-strip"
    fi
    "${CC_BIN}" -O3 -pipe -mcpu=cortex-a76+crypto+dotprod "${SRC_DIR}/argon-oneup-plasma-widget/argon-oneup-telemetry.c" -o "${SRC_DIR}/argon-oneup-plasma-widget/usr/bin/argon-oneup-telemetry"
    "${STRIP_BIN}" "${SRC_DIR}/argon-oneup-plasma-widget/usr/bin/argon-oneup-telemetry"
fi

for pkg in argon-oneup-backlight argon-oneup-battery argon-oneup-lid argon-oneup-sensors argon-oneup-plasma-widget argon-power-profiles cm5-mmc-fix rpi-fake-sleep argon-oneup-suite; do
    PKG_PATH="${SRC_DIR}/${pkg}"
    if [ -d "${PKG_PATH}" ]; then
        echo "Building ${pkg}..."
        ARCH=$(grep "^Architecture:" "${PKG_PATH}/DEBIAN/control" | awk '{print $2}')
        VER=$(grep "^Version:" "${PKG_PATH}/DEBIAN/control" | awk '{print $2}')
        DEB_NAME="${pkg}_${VER}_${ARCH}.deb"
        dpkg-deb --root-owner-group -b "${PKG_PATH}" "${DIST_DIR}/${DEB_NAME}"
    fi
done

cd "${DIST_DIR}"
sha256sum *.deb > SHA256SUMS
echo "=== All packages built successfully into ${DIST_DIR} ==="
ls -lh "${DIST_DIR}"/*.deb
