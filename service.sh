#!/bin/bash

MODDIR="/data/adb/modules/mingdfm"

log() {
  echo "[DRM Cleaner] $1" >> /cache/mingdfm.log 2>/dev/null
}

sleep 5

log "service.sh triggered"

check_zygisk() {
  local zn_pid=$(pgrep -f "zygisk" 2>/dev/null | head -1)
  if [ -n "$zn_pid" ]; then
    log "Zygisk Next is running (PID: $zn_pid)"
    return 0
  fi
  log "Zygisk Next not detected in process list"
  return 1
}

check_drm_service() {
  local drm_pid=$(pgrep -f "drm@.*service" 2>/dev/null | head -1)
  if [ -n "$drm_pid" ]; then
    log "DRM service detected (PID: $drm_pid)"
    return 0
  fi
  return 1
}

verify_module() {
  local so_path="$MODDIR/zygisk/arm64-v8a.so"
  if [ -f "$so_path" ]; then
    local size=$(stat -c%s "$so_path" 2>/dev/null || echo "0")
    log "Native lib present: ${size} bytes"
  else
    log "Native lib not found at $so_path"
  fi
}

check_zygisk
check_drm_service
verify_module

if [ -f "$MODDIR/sepolicy.rule" ]; then
  log "sepolicy.rule found, will be applied by KSU"
fi

log "service.sh completed"
