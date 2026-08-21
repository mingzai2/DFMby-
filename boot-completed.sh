#!/bin/bash

MODDIR="/data/adb/modules/mingdfm"

log() {
  echo "[DRM Cleaner] $1" >> /cache/mingdfm.log 2>/dev/null
}

log "boot-completed triggered"

sleep 10

ZYPID=$(pgrep -x zygote 2>/dev/null | head -1)
ZYPID64=$(pgrep -x zygote64 2>/dev/null | head -1)

if [ -n "$ZYPID" ] || [ -n "$ZYPID64" ]; then
  log "Zygote running - hooks should be active"
else
  log "WARNING: Zygote not found!"
fi

HOOK_STATUS=$(logcat -d -s "DRMCleaner" --pid 2>/dev/null | tail -5)
if [ -n "$HOOK_STATUS" ]; then
  log "Hook status: $HOOK_STATUS"
fi

if [ -f /cache/mingdfm.log ]; then
  tail -20 /cache/mingdfm.log > /cache/mingdfm.log.tmp 2>/dev/null
  mv /cache/mingdfm.log.tmp /cache/mingdfm.log 2>/dev/null
fi

log "All checks done. Module active."
