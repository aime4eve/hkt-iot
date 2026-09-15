# HKT BLETools iOS 编译与装机操作说明

> 更新：2026-09-15 · 适用：HKT-BLETools/ios（原生 SwiftUI 工程，Xcode 16+ 文件系统同步组）
> 签名方式：免费个人团队开发签名（Team `J62PU4Y357`，工程内已配置自动签名，无需手工管理证书）
> 一句话装机：`HKT-BLETools/ios/Tools/build_install.sh`

## 1. 前提条件

| 项目 | 要求 | 检查方法 |
|---|---|---|
| Mac | 装有 Xcode 16+，且已登录团队 Apple ID（sales@hktlora.com） | Xcode → Settings → Accounts |
| iPhone | 已与 Mac 配对（首次需 USB 连接并点「信任」），之后可在同 Wi-Fi 下远程安装 | `xcrun devicectl list devices` 能看到该机且状态含 `available` |
| 网络 | 普通网络即可；签名刷新需要访问 Apple 服务器 | — |

`xcrun devicectl list devices` 输出示例（记下 Identifier 列）：

```
Name       Hostname                    Identifier                             State                Model
myiPhone   myiPhone.coredevice.local   E3F9E013-0CAB-5D57-B9F1-88487CFB5AA0   available (paired)   iPhone XR (iPhone11,8)
```

## 2. 一条命令装机（最常用）

```bash
HKT-BLETools/ios/Tools/build_install.sh
```

自动完成四步：**Release 编译 → 打 IPA → 装到第一台可用的 iPhone → 启动**。
成功标志：输出 `安装到设备` 段有 `App installed:`，最后打印 `完成 ✓`。

产物位置：`HKT-BLETools/ios/build/ipa/HKTBLETools-v<版本>-b<构建号>-<日期时间>.ipa`
（如 `HKTBLETools-v1.0.0-b1-20260915-0843.ipa`，约 1.7M；`build/` 已在 .gitignore，不会误提交。）

## 3. 参数说明

```bash
./Tools/build_install.sh [-c Debug|Release] [-d <设备ID|auto|none>] [-o <输出目录>] [--no-launch]
```

| 参数 | 默认 | 说明 |
|---|---|---|
| `-c` | `Release` | 构建配置。日常装机用 Release；要验证演示钩子（MOCK_*）行为时用 Debug（两类构建可并存） |
| `-d` | `auto` | `auto`=第一台状态 available 的 iPhone；`-d <Identifier>`=指定设备；`-d none`=只编译打包不装机 |
| `-o` | `ios/build/ipa` | IPA 输出目录 |
| `--no-launch` | — | 装完不自动启动 |

## 4. 常见场景操作卡

### 4.1 新手机首次装机

1. USB 连接 Mac，手机上点「信任此电脑」。
2. 确认 `xcrun devicectl list devices` 能看到（状态 available）。
3. 直接跑 `./Tools/build_install.sh`——自动签名会把这台手机的 UDID 注册进描述文件（`-allowProvisioningDeviceRegistration`），无需去开发者网站手工注册。
4. 之后拔掉 USB，同 Wi-Fi 下仍可继续远程安装/续命。

注意：免费账号的描述文件只包含「构建时连着（或已注册）的设备」，**IPA 拷给别人装是装不上的**（对方设备 UDID 不在 profile 里），每台新手机都必须过一遍第 1–3 步。

### 4.2 多台手机批量装机

前提：每台手机都按 §4.1 完成过首次注册（连一次 Mac）。自动签名生成的描述文件**包含团队下全部已注册设备**，不限于构建当时连着的那台，所以注册过的手机随时可装。

批量安装就是换着设备 ID 循环跑脚本（每次运行会重新构建，IPA 文件名带时间戳互不覆盖）：

```bash
for d in <设备ID-A> <设备ID-B> <设备ID-C>; do
  HKT-BLETools/ios/Tools/build_install.sh -d "$d" --no-launch
done
```

规模参考见 §7：三四台演示机免费路线够用；十台以上或不想每周续命，开付费账号走 Ad Hoc。

### 4.3 7 天过期续命（免费签名日常成本）

免费签名的描述文件 **7 天过期**，过期后手机上 App 打不开（点图标闪退/无法验证）。处理：

```bash
HKT-BLETools/ios/Tools/build_install.sh        # 重跑即续命，数据不丢
```

已配对手机在同 Wi-Fi 下即可远程续命，不必插线。覆盖安装保留 App 数据。

### 4.4 只编译打包（不装机）

```bash
./Tools/build_install.sh -d none
```

产出 IPA 留在 `ios/build/ipa/`，可用于：存档、给已注册设备补装（`xcrun devicectl device install app --device <ID> <ipa路径>`）、或付费账号后改签分发。

### 4.5 装到模拟器（日常开发调试）

真机 IPA 与模拟器切片**不通用**（`simctl install` 装得上但启动被拒），模拟器要走独立构建：

```bash
cd HKT-BLETools/ios
xcodebuild -project HKTBLETools.xcodeproj -scheme HKTBLETools \
  -destination 'platform=iOS Simulator,name=iPhone 17 Pro' build
xcrun simctl install <模拟器UDID> <DerivedData产物>/Debug-iphonesimulator/HKTBLETools.app
```

演示模式（无蓝牙硬件时全链路可用）：

```bash
SIMCTL_CHILD_MOCK_RT_AUTOFIRE=1:1 SIMCTL_CHILD_MOCK_RT_DUR=12 \
xcrun simctl launch <UDID> com.hkt.ble.bletools.ios -mockble -demo-flow SVC -demo-page tasks
```

## 5. 故障排查 FAQ

| 现象 | 原因与处理 |
|---|---|
| `未找到可用 iPhone` | 手机锁屏/未信任/未配对。USB 重连并解锁，跑 `xcrun devicectl list devices` 确认状态含 `available`；只显示 `unavailable` 就插线重新配对 |
| 构建报签名错误（Signing / provisioning） | Xcode 未登录团队账号，或免费账号一周 10 个 App ID 上限用满（等额度刷新）。Settings → Accounts 确认 sales@hktlora.com 在列 |
| 安装成功但手机上点不开 | 免费签名 7 天过期 → 重跑脚本续命（§4.3）；刚装完系统弹「不受信任的开发者」→ 手机 设置 → 通用 → VPN与设备管理 → 信任该开发者证书 |
| `device install` 报 0x…/install 失败 | 手机存储不足，或手机上已有**不同签名来源**的同名 App——先删掉手机上的旧 HKTBLETools 再装 |
| 改了代码/图标但手机没变化 | 脚本必须重跑（IPA 不会热更新）；确认输出里的 IPA 文件名时间戳是新的 |
| 构建慢/卡在签名 | 首次构建或 profile 刷新需联网，正常 1–5 分钟；反复失败看 `ios/build/last_build.log` 尾部 |

## 6. 相关文件与原理

脚本四步流水线（了解即可，日常不用关心）：

1. `xcodebuild -destination 'generic/platform=iOS' -allowProvisioningUpdates -allowProvisioningDeviceRegistration build`
   —— 自动签名 + 静默刷新免费团队 profile（绕开 ExportOptions 里 `$(TEAM)` 变量不展开的坑）。
2. 读产物 Info.plist 真实版本号，`Payload` 直压成 IPA（xcodebuild 产物已签名，无需 exportArchive）。
3. `xcrun devicectl device install app` 安装。
4. `xcrun devicectl device process launch` 启动，并提示 7 天续命事项。

| 文件 | 作用 |
|---|---|
| `ios/Tools/build_install.sh` | 编译+装机一条龙（本文档主角） |
| `ios/Tools/make_appicon.swift` | 图标生成器（改设计 → 跑它 → 重跑装机脚本生效） |
| `ios/Resources/Assets.xcassets/` | 图标资产目录（单尺寸 1024 AppIcon） |
| `ios/build/last_build.log` | 最近一次构建完整日志（排查构建失败先看这里） |
| `ios/build/DerivedData/` | 构建中间产物（可随时删除，下次构建重建） |

## 7. 团队规模化路线（何时升级签名方式）

| 路线 | 成本 | 适合 | 关键差异 |
|---|---|---|---|
| 免费个人签名（现状） | ¥0 | 1–3 台演示机 | 7 天过期需续命；每台新机必须连 Mac 注册；装机脚本照用 |
| **付费 Ad Hoc**（推荐） | ¥688/年 | 内部工具，100 台内 | 描述文件**一年有效**；设备 UDID 注册进后台，装机流程与脚本基本不变 |
| TestFlight | ¥688/年 | 人多、不想管 UDID | 邮件邀请即可装；需过 Beta 审核，中国区涉及 App 备案，内部工具反而绕远 |

超过三台使用者在用，建议直接开付费账号转 Ad Hoc——一次性消灭 7 天续命和逐台注册两个运维负担。

## 8. 附：App 基本信息

- Bundle ID：`com.hkt.ble.bletools.ios`（与 Android/其他 App 无冲突）
- 当前版本：1.0.0 (1)，`MARKETING_VERSION`/`CURRENT_PROJECT_VERSION` 在 Xcode 工程设置里改
- 最低系统：iOS 17.0 · 仅竖屏 iPhone（iPad 竖屏兼容）
- 蓝牙权限文案等 Info 键：工程 Build Settings 的 `INFOPLIST_KEY_*`（生成式 Info.plist，无独立文件）
