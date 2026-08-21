#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT="$SCRIPT_DIR/../ming_drm_cleaner_v1.0.0.zip"

echo "======================================"
echo "  DRM ID Cleaner - Pack Tool"
echo "  Author: 铭"
echo "======================================"
echo ""

check_file() {
  if [ ! -f "$1" ]; then
    echo "[!] Missing: $1"
    return 1
  fi
  return 0
}

echo "[*] Checking module files..."

FILES_OK=true
check_file "$SCRIPT_DIR/module.prop" || FILES_OK=false
check_file "$SCRIPT_DIR/customize.sh" || FILES_OK=false
check_file "$SCRIPT_DIR/post-fs-data.sh" || FILES_OK=false
check_file "$SCRIPT_DIR/service.sh" || FILES_OK=false
check_file "$SCRIPT_DIR/boot-completed.sh" || FILES_OK=false
check_file "$SCRIPT_DIR/zn_modules.txt" || FILES_OK=false
check_file "$SCRIPT_DIR/sepolicy.rule" || FILES_OK=false

SO_COUNT=$(find "$SCRIPT_DIR/zygisk" -name "*.so" 2>/dev/null | wc -l)
if [ "$SO_COUNT" -eq 0 ]; then
  echo "[!] Warning: No compiled .so files found in zygisk/"
  echo "[!] Module will still work for shell-based spoofing"
  echo "[!] For full DRM hook, compile with build.sh first"
fi

if [ "$FILES_OK" = false ]; then
  echo ""
  echo "[!] Missing critical files. Aborting."
  exit 1
fi

echo "[+] All core files present"
echo "[+] Native libraries: $SO_COUNT .so files"
echo ""

chmod 0755 "$SCRIPT_DIR/customize.sh"
chmod 0755 "$SCRIPT_DIR/post-fs-data.sh"
chmod 0755 "$SCRIPT_DIR/service.sh"
chmod 0755 "$SCRIPT_DIR/boot-completed.sh"
chmod 0755 "$SCRIPT_DIR/build.sh" 2>/dev/null || true
chmod 0755 "$SCRIPT_DIR/pack.sh" 2>/dev/null || true
chmod 0644 "$SCRIPT_DIR/module.prop"
chmod 0644 "$SCRIPT_DIR/zn_modules.txt"
chmod 0644 "$SCRIPT_DIR/sepolicy.rule"

find "$SCRIPT_DIR/zygisk" -name "*.so" -exec chmod 0755 {} \; 2>/dev/null || true

echo "[*] Creating ZIP package..."

rm -f "$OUTPUT"

cd "$SCRIPT_DIR"
zip -r "$OUTPUT" \
  module.prop \
  customize.sh \
  post-fs-data.sh \
  service.sh \
  boot-completed.sh \
  zn_modules.txt \
  sepolicy.rule \
  build.sh \
  pack.sh \
  README.md \
  tools/ \
  zygisk/ \
  -x "*.git*" "*.DS_Store" "build/*" "*.o" 2>/dev/null || true

if [ -f "$OUTPUT" ]; then
  SIZE=$(stat -c%s "$OUTPUT" 2>/dev/null || echo "?")
  echo ""
  echo "[+] Package created successfully!"
  echo "[+] Output: $OUTPUT"
  echo "[+] Size: $SIZE bytes"
  echo ""
  echo "Contents:"
  zip -l "$OUTPUT" 2>/dev/null | tail -n +3 | head -20
  echo ""
  echo "======================================"
  echo "  Flash via KernelSU Manager:"
  echo "  1. Open KernelSU app"
  echo "  2. Modules -> Install from storage"
  echo "  3. Select ming_drm_cleaner_v1.0.0.zip"
  echo "  4. Reboot"
  echo "======================================"
else
  echo "[!] Failed to create ZIP"
  exit 1
fi
