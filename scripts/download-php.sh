#!/usr/bin/env bash
# ============================================================================
# download-php.sh
#
# Downloads a self-contained static PHP 8.3 CLI binary for macOS and places
# it in OpenVetSim/build/bin/PHP8.0/ where the simulation engine looks first
# when searching for PHP (see WebSrv.cpp findPhpPath()).
#
# Uses pre-built static binaries from pmmp/PHP-Binaries (GitHub Releases).
# The GitHub API is queried at runtime so the script always finds the correct
# asset name without needing to hardcode it.
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

ARCH="$(uname -m)"   # arm64 (Apple Silicon) or x86_64 (Intel)

echo "Looking for PHP 8.3 macOS ${ARCH} binary via GitHub API..."

# Use Python3 (always available on macOS) to query the GitHub releases API
# and find the correct download URL for our platform.
DOWNLOAD_URL="$(python3 - "$ARCH" <<'PYEOF'
import sys, json
from urllib.request import urlopen

arch = sys.argv[1]          # arm64 or x86_64
php_minor = "8.3"

url = "https://api.github.com/repos/pmmp/PHP-Binaries/releases"
with urlopen(url) as resp:
    releases = json.load(resp)

for release in releases:
    assets = release.get("assets", [])
    for asset in assets:
        name = asset["name"]
        # Match e.g. "PHP-8.3.x-MacOS-arm64-PM5.tar.gz"
        if (php_minor in name
                and "MacOS" in name
                and arch in name
                and name.endswith(".tar.gz")):
            print(asset["browser_download_url"])
            sys.exit(0)

print("NOT_FOUND")
PYEOF
)"

if [ "$DOWNLOAD_URL" = "NOT_FOUND" ] || [ -z "$DOWNLOAD_URL" ]; then
  echo "ERROR: No PHP 8.3 macOS ${ARCH} asset found in pmmp/PHP-Binaries releases." >&2
  echo "       Check https://github.com/pmmp/PHP-Binaries/releases manually." >&2
  exit 1
fi

echo "Found: $DOWNLOAD_URL"
echo ""

mkdir -p "$DEST"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
ASSET="$(basename "$DOWNLOAD_URL")"

echo "Downloading $ASSET ..."
curl -fL --progress-bar "$DOWNLOAD_URL" -o "$TMP/$ASSET"

echo ""
echo "Archive contents:"
tar -tzf "$TMP/$ASSET"
echo ""

tar -xzf "$TMP/$ASSET" -C "$DEST"

# Find the php binary — may be nested anywhere under $DEST
PHP_BIN="$(find "$DEST" -type f -name "php" | grep -v '/php\.ini$' | head -1)"
if [ -z "$PHP_BIN" ]; then
  echo "ERROR: could not find php binary after extraction." >&2
  echo "Files extracted:" >&2
  find "$DEST" >&2
  exit 1
fi

chmod +x "$PHP_BIN"
PHP_BIN_DIR="$(dirname "$PHP_BIN")"

# Disable opcache in whichever php.ini lives next to the real binary.
# The pmmp build has a hardcoded CI path for opcache.so that doesn't exist
# on end-user machines. Opcache is optional (performance only).
PHP_INI="$PHP_BIN_DIR/php.ini"
printf '\n[opcache]\nopcache.enable=0\nopcache.enable_cli=0\n' >> "$PHP_INI"
echo "Patched $PHP_INI (disabled opcache)"

# Create a wrapper script at $DEST/php.
# The wrapper sets PHPRC to the directory containing the real binary so PHP
# finds the patched php.ini regardless of the compiled-in default path.
rm -f "$DEST/php"
PHP_BIN_REL="$(python3 -c "import os,sys; print(os.path.relpath(sys.argv[1], sys.argv[2]))" "$PHP_BIN" "$DEST")"
cat > "$DEST/php" <<WRAPPER
#!/usr/bin/env bash
# Wrapper: sets PHPRC so PHP finds the patched php.ini (suppresses opcache warning)
DIR="\$(cd "\$(dirname "\$0")" && pwd)"
export PHPRC="\$DIR/${PHP_BIN_REL%/*}"
exec "\$DIR/${PHP_BIN_REL}" "\$@"
WRAPPER
chmod +x "$DEST/php"
echo "Created wrapper $DEST/php → $PHP_BIN_REL"

echo ""
echo "Done. PHP is ready at:"
echo "  $DEST/php"
echo ""
echo "Verify with:"
echo "  $DEST/php --version"
