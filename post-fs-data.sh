#!/bin/bash

MODDIR="/data/adb/modules/mingdfm"

log() {
  echo "[DRM Cleaner] $1" >> /cache/mingdfm.log 2>/dev/null
}

log "post-fs-data triggered"

mkdir -p "$MODDIR/tools"

if [ ! -f "$MODDIR/tools/drm_seed" ]; then
  SEED=$(cat /proc/sys/kernel/random/boot_id 2>/dev/null | sha256sum | cut -c1-32 | tr -d '\n')
  if [ -z "$SEED" ]; then
    SEED=$(head -c 16 /dev/urandom 2>/dev/null | xxd -p)
  fi
  echo "$SEED" > "$MODDIR/tools/drm_seed"
  chmod 0644 "$MODDIR/tools/drm_seed"
  log "Generated new DRM seed: ${SEED:0:8}..."
fi

if [ ! -f "$MODDIR/tools/device_props" ]; then
  {
    echo "PRODUCT=$(getprop ro.product.name 2>/dev/null)"
    echo "MODEL=$(getprop ro.product.model 2>/dev/null)"
    echo "MANUFACTURER=$(getprop ro.product.manufacturer 2>/dev/null)"
    echo "BRAND=$(getprop ro.product.brand 2>/dev/null)"
    echo "FINGERPRINT=$(getprop ro.build.fingerprint 2>/dev/null)"
    echo "SERIAL=$(head -c 8 /dev/urandom 2>/dev/null | xxd -p)"
  } > "$MODDIR/tools/device_props"
  chmod 0644 "$MODDIR/tools/device_props"
  log "Generated device props template"
fi

log "post-fs-data completed"
