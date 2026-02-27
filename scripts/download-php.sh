#!/usr/bin/env bash
# ============================================================================
# download-php.sh
#
# Downloads a self-contained static PHP 8.x CLI binary for macOS and places
# it in OpenVetSim/build/bin/PHP8.0/ where the simulation engine looks first
# when searching for PHP (see WebSrv.cpp findPhpPath()).
#
# Uses pre-built static binaries from the static-php-cli project:
#   https://github.com/crazywhalecc/static-php-cli
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
#   npm start          (development mode, if PHP is not installed system-wide)
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DEST="$REPO_ROOT/OpenVetSim/build/bin/PHP8.0"

# PHP version to bundle — update this when a new release is needed.
PHP_VERSION="8.3.11"
SPC_RELEASE="2.4.0"   # static-php-cli release tag

ARCH="$(uname -m)"   # arm64 (Apple Silicon) or x86_64 (Intel)

case "$ARCH" in
  arm64)
    ASSET="php-${PHP_VERSION}-cli-macos-aarch64.tar.gz"
    ;;
  x86_64)
    ASSET="php-${PHP_VERSION}-cli-macos-x86_64.tar.gz"
    ;;
  *)
    echo "ERROR: Unsupported architecture: $ARCH" >&2
    exit 1
    ;;
esac

URL="https://github.com/crazywhalecc/static-php-cli/releases/download/${SPC_RELEASE}/${ASSET}"

echo "Downloading PHP ${PHP_VERSION} for ${ARCH}..."
echo "  Source : $URL"
echo "  Dest   : $DEST"
echo ""

mkdir -p "$DEST"

# Download and extract — the archive contains a single 'php' binary
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

curl -fL --progress-bar "$URL" -o "$TMP/$ASSET"
tar -xzf "$TMP/$ASSET" -C "$DEST"

# Ensure the binary is executable
chmod +x "$DEST/php"

echo ""
echo "Done. PHP is ready at:"
echo "  $DEST/php"
echo ""
echo "You can verify it with:"
echo "  $DEST/php --version"
