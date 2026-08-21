#!/bin/bash

SKIPUNZIP=0

ui_print() {
  echo "$1"
}

require_zn() {
  local zn_dir="/data/adb/modules/zygisk_next"
  local zn_alt="/data/adb/modules/zygisk-next"
  if [ -d "$zn_dir" ] || [ -d "$zn_alt" ]; then
    return 0
  fi
  return 1
}

ui_print "======================================"
ui_print "  DRM ID Cleaner - by 铭"
ui_print "======================================"
ui_print ""

API=$(getprop ro.build.version.sdk 2>/dev/null || echo "unknown")
ARCH=$(uname -m 2>/dev/null || echo "unknown")

ui_print "[*] Android API: $API"
ui_print "[*] Architecture: $ARCH"
ui_print ""

if ! require_zn; then
  ui_print "[!] 警告: 未检测到 Zygisk Next 模块"
  ui_print "[!] 本模块依赖 Zygisk Next 才能正常工作"
  ui_print "[!] 请先安装 Zygisk Next 后重启再刷入本模块"
  ui_print ""
  ui_print "[!] Warning: Zygisk Next not found"
  ui_print "[!] This module requires Zygisk Next to function"
fi

ui_print "[*] 检查设备环境..."
ui_print "[*] 创建必要目录..."

mkdir -p "$MODPATH/tools"
mkdir -p "$MODPATH/zygisk"

ui_print "[*] 设置文件权限..."
chmod 0755 "$MODPATH/service.sh" 2>/dev/null
chmod 0755 "$MODPATH/post-fs-data.sh" 2>/dev/null
chmod 0644 "$MODPATH/module.prop"
chmod 0644 "$MODPATH/zn_modules.txt" 2>/dev/null

ui_print ""
ui_print "[*] 安装完成"
ui_print "[*] 请重启设备使修改生效"
ui_print "[*] 重启后DRM ID将被自动修改"
ui_print ""
ui_print "======================================"

exit 0
