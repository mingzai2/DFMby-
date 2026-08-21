# DRM ID Cleaner

通过 Zygisk Next Hook 技术修改 DRM 设备唯一标识，解决设备标记/黑号问题。

## 作者

铭

## 模块信息

- **ID**: mingdfm
- **版本**: 1.0.0
- **最低要求**: Android 8.0+ / KernelSU / Zygisk Next

## 功能

- Hook MediaDrm.getPropertyByteArray() 返回伪造的 deviceUniqueId
- Hook MediaDrm.getUniqueId() 返回一致的伪随机ID
- Hook Build.getSerial() 返回伪造序列号
- Hook Settings.Secure.ANDROID_ID 返回包名隔离的伪ID
- Hook TelephonyManager 的 IMEI/DeviceId/SubscriberId
- 每次刷入自动生成新种子，确保所有ID刷新
- 同一应用始终获得相同ID（稳定性）
- 不同应用获得不同ID（隔离性）

## 依赖

- KernelSU 或 Magisk
- **Zygisk Next**（必须，模块无法独立工作）
- Android 8.0+

## 安装

1. 确保已安装 Zygisk Next 并启用
2. 在 KernelSU Manager 中选择"从存储安装"
3. 选择 ming_drm_cleaner_v1.0.0.zip
4. 等待安装完成，重启设备

## 验证

重启后检查 `/cache/mingdfm.log` 确认模块已加载：

```
[DRM ID Cleaner] === Ming Module Loaded (by 铭) ===
[DRM ID Cleaner] ABI: arm64-v8a | PID: 1
[DRM ID Cleaner] Generated new DRM seed: a3f2b8c1...
```

## 重置设备指纹

删除种子文件后重启即可生成全新身份：

```bash
su
rm /data/adb/modules/mingdfm/tools/drm_seed
reboot
```

## 卸载

在 KernelSU Manager 中禁用或删除本模块后重启。

## 注意事项

- 本模块仅修改应用层读取的DRM相关标识
- 不影响系统完整性校验
- 不会修改/损坏 Widevine L1 认证
- 游戏启动前确保模块已加载完成
- 如遇兼容性问题，检查 logcat 中 DRMCleaner 标签

## 编译

### 方式一：GitHub Actions（推荐）

推送代码到 GitHub 仓库，自动编译并打包。

### 方式二：本地编译

```bash
export ANDROID_NDK_HOME=/path/to/ndk
./build.sh
./pack.sh
```

### 方式三：Docker

```bash
docker build -f Dockerfile.builder -t mingdfm-builder .
docker run --rm -v $(pwd):/out mingdfm-builder
```

## 架构

```
mingdfm/
├── module.prop
├── customize.sh
├── post-fs-data.sh
├── service.sh
├── boot-completed.sh
├── zn_modules.txt
├── sepolicy.rule
├── build.sh
├── pack.sh
├── README.md
├── tools/
│   ├── drm_seed
│   └── device_props
└── zygisk/
    ├── zygisk.hpp
    ├── config_store.hpp
    ├── sha256_util.hpp
    ├── zygisk.cpp
    ├── config_store.cpp
    ├── CMakeLists.txt
    ├── arm64-v8a/libmingdfm.so
    └── armeabi-v7a/libmingdfm.so
```
