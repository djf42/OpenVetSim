#!/usr/bin/env bash
# ============================================================================
# download-php.sh
#
# Downloads static PHP 8.3 CLI binaries for BOTH macOS arm64 and x86_64,
# then combines them into a single universal (fat) binary using lipo.
#
# This is required for building a universal DMG with electron-builder --universal,
# which uses @electron/universal to merge the two arch builds. That tool rejects
# single-arch binaries that appear identically in both builds.
#
# Uses pre-built static binaries from the static-php-cli project CDN:
#   https://dl.static-php.dev/static-php-cli/common/
#
# ⚠️  The CDN redirects through DigitalOcean Spaces, which is blocked on some
#     university/corporate networks. Run this script from a non-university
#     network (home internet, phone hotspot, VPN, etc.) if you are on campus.
#
# These binaries have no external library dependencies — they work on any
# macOS installation without Homebrew or any other package manager.
#
# Usage:
#   chmod +x scripts/download-php.sh
#   ./scripts/download-php.sh
#
# Run this once before:
#   npm run dist:mac   (packaging a distributable DMG)
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DEST="$REPO_ROOT/OpenVetSim/build/bin/PHP8.0"

PHP_MINOR="8.3"
CANDIDATES=(17 16 15 14 13)
BASE_URL="https://dl.static-php.dev/static-php-cli/common"

# ---------------------------------------------------------------------------
# find_version <spc_arch> — probes the CDN and echos the first available
# version string (to stdout only), with progress to stderr.
# ---------------------------------------------------------------------------
find_version() {
  local SPC_ARCH="$1"
  for PATCH in "${CANDIDATES[@]}"; do
    local VERSION="${PHP_MINOR}.${PATCH}"
    local ASSET="php-${VERSION}-cli-macos-${SPC_ARCH}.tar.gz"
    local URL="${BASE_URL}/${ASSET}"
    printf "  Trying %s (%s) ... " "$VERSION" "$SPC_ARCH" >&2
    HTTP_CODE="$(curl -o /dev/null -w "%{http_code}" \
      --silent --head --location \
      --connect-timeout 8 --max-time 10 \
      "$URL" 2>/dev/null || echo "000")"
    if [ "$HTTP_CODE" = "200" ]; then
      echo "found!" >&2
      echo "$VERSION"   # only this goes to stdout (captured by caller)
      return 0
    else
      echo "not available (HTTP $HTTP_CODE)" >&2
    fi
  done
  echo "" >&2
  echo "ERROR: Could not find a PHP binary for arch: $SPC_ARCH" >&2
  echo "       If you are on a university/corporate network, try running this" >&2
  echo "       script from a home network, phone hotspot, or VPN." >&2
  exit 1
}

# ---------------------------------------------------------------------------
# download_php <spc_arch> <version> <dest_file>
# ---------------------------------------------------------------------------
download_php() {
  local SPC_ARCH="$1"
  local VERSION="$2"
  local OUT="$3"
  local ASSET="php-${VERSION}-cli-macos-${SPC_ARCH}.tar.gz"
  local URL="${BASE_URL}/${ASSET}"
  local TMP
  TMP="$(mktemp -d)"

  echo "Downloading PHP ${VERSION} for macos-${SPC_ARCH}..."
  curl -fL --progress-bar "$URL" -o "$TMP/$ASSET"
  echo ""

  tar -xzf "$TMP/$ASSET" -C "$TMP"
  PHP_BIN="$(find "$TMP" -type f -name "php" | head -1)"
  if [ -z "$PHP_BIN" ]; then
    echo "ERROR: could not find 'php' binary in archive for ${SPC_ARCH}." >&2
    exit 1
  fi
  chmod +x "$PHP_BIN"
  cp "$PHP_BIN" "$OUT"
  rm -rf "$TMP"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
echo "Building a universal (arm64 + x86_64) PHP ${PHP_MINOR}.x binary for macOS..."
echo "(If this hangs, you may be on a network that blocks the CDN — try a VPN or home network)"
echo ""

echo "Probing for arm64 build..."
ARM64_VERSION="$(find_version aarch64)"

echo ""
echo "Probing for x86_64 build..."
X64_VERSION="$(find_version x86_64)"

echo ""

rm -rf "$DEST"
mkdir -p "$DEST"

TMP_ARM64="$(mktemp)"
TMP_X64="$(mktemp)"
trap 'rm -f "$TMP_ARM64" "$TMP_X64"' EXIT

download_php "aarch64" "$ARM64_VERSION" "$TMP_ARM64"
download_php "x86_64"  "$X64_VERSION"  "$TMP_X64"

echo "Creating universal binary with lipo..."
lipo -create "$TMP_ARM64" "$TMP_X64" -output "$DEST/php"
chmod +x "$DEST/php"

echo ""
echo "Verifying universal binary:"
lipo -info "$DEST/php"

echo ""
echo "Done. Universal PHP is ready at:"
echo "  $DEST/php"
echo ""
echo "Verify with:"
echo "  '$DEST/php' --version"
