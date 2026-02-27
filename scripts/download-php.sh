#!/usr/bin/env bash
# ============================================================================
# download-php.sh
#
# Downloads a self-contained static PHP 8.3 CLI binary for macOS and places
# it in OpenVetSim/build/bin/PHP8.0/ where the simulation engine looks first
# when searching for PHP (see WebSrv.cpp findPhpPath()).
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

ARCH="$(uname -m)"

case "$ARCH" in
  arm64)   SPC_ARCH="aarch64" ;;
  x86_64)  SPC_ARCH="x86_64"  ;;
  *)
    echo "ERROR: Unsupported architecture: $ARCH" >&2
    exit 1
    ;;
esac

PHP_MINOR="8.3"
CANDIDATES=(17 16 15 14 13)
BASE_URL="https://dl.static-php.dev/static-php-cli/common"

echo "Looking for PHP ${PHP_MINOR}.x CLI binary for macOS ${ARCH}..."
echo "(If this hangs, you may be on a network that blocks the CDN — try a VPN or home network)"
echo ""

FOUND_URL=""
FOUND_VERSION=""
FOUND_ASSET=""
for PATCH in "${CANDIDATES[@]}"; do
  VERSION="${PHP_MINOR}.${PATCH}"
  ASSET="php-${VERSION}-cli-macos-${SPC_ARCH}.tar.gz"
  URL="${BASE_URL}/${ASSET}"
  printf "  Trying %s ... " "$VERSION"
  HTTP_CODE="$(curl -o /dev/null -w "%{http_code}" \
    --silent --head --location \
    --connect-timeout 8 --max-time 10 \
    "$URL" 2>/dev/null || echo "000")"
  if [ "$HTTP_CODE" = "200" ]; then
    echo "found!"
    FOUND_URL="$URL"
    FOUND_VERSION="$VERSION"
    FOUND_ASSET="$ASSET"
    break
  else
    echo "not available (HTTP $HTTP_CODE)"
  fi
done

if [ -z "$FOUND_URL" ]; then
  echo ""
  echo "ERROR: Could not reach the PHP CDN." >&2
  echo "       If you are on a university/corporate network, try running this" >&2
  echo "       script from a home network, phone hotspot, or VPN." >&2
  exit 1
fi

echo ""
echo "Downloading PHP ${FOUND_VERSION} for macOS ${ARCH}..."
echo "  Source : $FOUND_URL"
echo "  Dest   : $DEST"
echo ""

rm -rf "$DEST"
mkdir -p "$DEST"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

curl -fL --progress-bar "$FOUND_URL" -o "$TMP/$FOUND_ASSET"

echo ""
echo "Archive contents:"
tar -tzf "$TMP/$FOUND_ASSET"
echo ""

tar -xzf "$TMP/$FOUND_ASSET" -C "$DEST"

# Find the php binary
PHP_BIN="$(find "$DEST" -type f -name "php" | head -1)"
if [ -z "$PHP_BIN" ]; then
  echo "ERROR: could not find 'php' binary after extraction." >&2
  find "$DEST" >&2
  exit 1
fi

chmod +x "$PHP_BIN"

# If the binary isn't directly at $DEST/php, symlink it there
if [ "$PHP_BIN" != "$DEST/php" ]; then
  ln -sf "$PHP_BIN" "$DEST/php"
  echo "Linked $DEST/php → $PHP_BIN"
fi

echo ""
echo "Done. PHP is ready at:"
echo "  $DEST/php"
echo ""
echo "Verify with:"
echo "  '$DEST/php' --version"
