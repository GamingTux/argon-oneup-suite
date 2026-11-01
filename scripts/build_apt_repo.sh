#!/usr/bin/env bash
set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST_DIR="${REPO_ROOT}/dist"
EXTRAS_DIST_DIR="${DIST_DIR}/extras"
OUTPUT_DIR="${REPO_ROOT}/public"

echo "=== Building APT Repository Structure ==="
rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}/pool/main"
mkdir -p "${OUTPUT_DIR}/dists/stable/main/binary-all"
mkdir -p "${OUTPUT_DIR}/dists/stable/main/binary-arm64"

# 1. Copy all Debian packages to pool/main
cp -f "${DIST_DIR}"/*.deb "${OUTPUT_DIR}/pool/main/" 2>/dev/null || true
cp -f "${EXTRAS_DIST_DIR}"/*.deb "${OUTPUT_DIR}/pool/main/" 2>/dev/null || true

# 2. Copy the public archive keyring to the repository root
KEYRING_SRC="${REPO_ROOT}/src/argon-oneup-essentials/usr/share/keyrings"
if [ -f "${KEYRING_SRC}/argon-oneup-archive-keyring.gpg" ]; then
    cp -f "${KEYRING_SRC}/argon-oneup-archive-keyring.gpg" "${OUTPUT_DIR}/"
    cp -f "${KEYRING_SRC}/argon-oneup-archive-keyring.asc" "${OUTPUT_DIR}/"
fi

# 3. Generate Packages and Packages.gz for all and arm64
cd "${OUTPUT_DIR}"
dpkg-scanpackages --arch all pool/main > dists/stable/main/binary-all/Packages
gzip -9fk dists/stable/main/binary-all/Packages

dpkg-scanpackages --arch arm64 pool/main > dists/stable/main/binary-arm64/Packages
gzip -9fk dists/stable/main/binary-arm64/Packages

# 4. Generate dists/stable/Release
apt-ftparchive \
  -o APT::FTPArchive::Release::Origin="Argon ONE UP" \
  -o APT::FTPArchive::Release::Label="Argon ONE UP Suite" \
  -o APT::FTPArchive::Release::Suite="stable" \
  -o APT::FTPArchive::Release::Codename="stable" \
  -o APT::FTPArchive::Release::Architectures="all arm64" \
  -o APT::FTPArchive::Release::Components="main" \
  -o APT::FTPArchive::Release::Description="Argon ONE UP Debian Package Repository" \
  release dists/stable > dists/stable/Release

# 5. Sign Release if GPG key is present
if [ -n "${GPG_KEY_ID:-}" ] || gpg --list-secret-keys "Argon ONE UP" >/dev/null 2>&1; then
    echo "Signing Release with GPG..."
    gpg --batch --yes --armor --detach-sign -o dists/stable/Release.gpg dists/stable/Release
    gpg --batch --yes --clearsign -o dists/stable/InRelease dists/stable/Release
    echo "Release signed successfully (Release.gpg and InRelease created)."
else
    echo "WARNING: No GPG signing key found in environment, repository will be unsigned."
fi

# 6. Generate a landing page index.html
cat > index.html << 'HTMLEOF'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>Argon ONE UP APT Repository</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; max-width: 800px; margin: 40px auto; padding: 0 20px; line-height: 1.6; color: #333; background: #fdfdfd; }
    h1 { color: #111; border-bottom: 2px solid #00e5ff; padding-bottom: 10px; }
    code, pre { background: #f4f4f4; border-radius: 4px; padding: 2px 6px; font-family: "JetBrains Mono", Consolas, monospace; }
    pre { padding: 15px; overflow-x: auto; border-left: 4px solid #00e5ff; }
    .badge { display: inline-block; background: #00e5ff; color: #000; font-weight: bold; padding: 2px 8px; border-radius: 3px; font-size: 0.85em; }
  </style>
</head>
<body>
  <h1>⚡ Argon ONE UP Debian / APT Repository</h1>
  <p><span class="badge">Official</span> Hardware driver and power management suite repository for the <strong>Argon ONE UP</strong> Raspberry Pi CM5 laptop.</p>
  
  <h2>📦 Automated Installation via Essentials</h2>
  <p>Installing <code>argon-oneup-essentials</code> automatically configures this repository and its GPG keyring.</p>

  <h2>🔧 Manual Repository Setup</h2>
  <p>To manually add this repository to any Debian / Raspberry Pi OS / Ubuntu system:</p>
  <pre><code># 1. Download and install the GPG signing key
sudo mkdir -p /usr/share/keyrings
curl -fsSL https://gamingtux.github.io/argon-oneup-suite/argon-oneup-archive-keyring.gpg | sudo tee /usr/share/keyrings/argon-oneup-archive-keyring.gpg > /dev/null

# 2. Add the repository sources list
sudo tee /etc/apt/sources.list.d/argon-oneup.sources &lt;&lt; 'EOF'
Types: deb
URIs: https://gamingtux.github.io/argon-oneup-suite/
Suites: stable
Components: main
Signed-By: /usr/share/keyrings/argon-oneup-archive-keyring.gpg
EOF

# 3. Update APT and install the suite
sudo apt update
sudo apt install argon-oneup-suite</code></pre>
</body>
</html>
HTMLEOF

echo "=== APT repository built successfully in ${OUTPUT_DIR} ==="
