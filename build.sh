#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/zygisk"
BUILD_DIR="$SCRIPT_DIR/build"

NDK_PATH="${ANDROID_NDK_HOME:-$ANDROID_NDK_ROOT}"
if [ -z "$NDK_PATH" ]; then
  for p in /opt/android-ndk /usr/local/android-ndk $HOME/Android/Sdk/ndk/*; do
    if [ -d "$p" ] && [ -f "$p/ndk-build" ]; then
      NDK_PATH="$p"
      break
    fi
  done
fi

if [ -z "$NDK_PATH" ] || [ ! -d "$NDK_PATH" ]; then
  echo "[!] Android NDK not found!"
  echo "    Set ANDROID_NDK_HOME or ANDROID_NDK_ROOT environment variable"
  echo "    Or install NDK to /opt/android-ndk"
  exit 1
fi

echo "[*] Using NDK: $NDK_PATH"

API_LEVEL="${API_LEVEL:-24}"
TOOLCHAIN="$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64"
TARGETS=("aarch64-linux-android" "armv7a-linux-androideabi" "x86_64-linux-android" "i686-linux-android")
ABIS=("arm64-v8a" "armeabi-v7a" "x86_64" "x86")

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

CFLAGS="-O2 -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections -Wl,--gc-sections"
CFLAGS="$CFLAGS -fomit-frame-pointer -Wno-unused-function -Wno-unused-variable"
CFLAGS="$CFLAGS -D__ANDROID_API__=$API_LEVEL"

for i in "${!TARGETS[@]}"; do
  TARGET="${TARGETS[$i]}"
  ABI="${ABIS[$i]}"

  echo ""
  echo "[*] Building for $ABI ($TARGET)..."

  mkdir -p "$BUILD_DIR/$ABI"

  if [[ "$TARGET" == "armv7a-linux-androideabi" ]]; then
    CC="$TOOLCHAIN/bin/armv7a-linux-androideabi$API_LEVEL-clang"
  else
    CC="$TOOLCHAIN/bin/${TARGET}${API_LEVEL}-clang"
  fi

  if [ ! -f "$CC" ]; then
    echo "[!] Compiler not found: $CC"
    echo "[!] Falling back to generic clang..."
    CC="$TOOLCHAIN/bin/clang"
  fi

  $CC $CFLAGS -c \
    "$SCRIPT_DIR/zygisk/config_store.cpp" \
    -I"$SCRIPT_DIR/zygisk" \
    -o "$BUILD_DIR/$ABI/config_store.o"

  $CC $CFLAGS -c \
    "$SCRIPT_DIR/zygisk/zygisk.cpp" \
    -I"$SCRIPT_DIR/zygisk" \
    -o "$BUILD_DIR/$ABI/zygisk.o"

  $CC $CFLAGS -shared -Wl,-soname,libmingdfm.so \
    "$BUILD_DIR/$ABI/config_store.o" \
    "$BUILD_DIR/$ABI/zygisk.o" \
    -llog -ldl \
    -o "$BUILD_DIR/$ABI/libmingdfm.so"

  "$TOOLCHAIN/bin/llvm-strip" --strip-all "$BUILD_DIR/$ABI/libmingdfm.so" 2>/dev/null || true

  mkdir -p "$OUTPUT_DIR/$ABI"
  cp "$BUILD_DIR/$ABI/libmingdfm.so" "$OUTPUT_DIR/$ABI/libmingdfm.so"

  SIZE=$(stat -c%s "$OUTPUT_DIR/$ABI/libmingdfm.so" 2>/dev/null || echo "?")
  echo "[+] $ABI: libmingdfm.so ($SIZE bytes) -> $OUTPUT_DIR/$ABI/"
done

echo ""
echo "======================================"
echo "  Build Complete!"
echo "======================================"
echo ""
echo "Output structure:"
find "$OUTPUT_DIR" -name "*.so" -exec ls -lh {} \;
echo ""
echo "Next: run pack.sh to create flashable ZIP"
