# AGENTS.md — BLETools 本机实操指南

> **文档分工**
> - [CLAUDE.md](CLAUDE.md) 是权威：项目概览、架构、BLE 协议、`Communicate.kt` 详解、代码约定、openspec 流程。改动协议/UI/设备类型前必读。
> - **本文件（AGENTS.md）** 聚焦**本机环境实操**：构建、测试、模拟器的具体命令，以及已踩过并解决的环境坑。执行构建/测试/仿真任务时优先看这里。

---

## 1. 构建环境（本机已部署，2026-08-08 验证通过）

本机（Windows，用户 `hkt`）原本无任何 JDK/Android SDK，已手动从零部署完成，**后续直接用即可**：

| 组件 | 位置 / 状态 | 备注 |
|------|------------|------|
| JDK 17 | `D:\Java\jdk-17`（Oracle OpenJDK 17.0.12） | 匹配 `gradle.properties` 的 `org.gradle.java.home` |
| Android SDK | `D:\Android\sdk` | 即 `android/local.properties` 的 `sdk.dir`（该文件**不在 VCS**，丢失需重建） |
| SDK 组件 | `platforms;android-34`、`build-tools;34.0.0`(+30.0.3 自动补)、`platform-tools` | `cmdline-tools/latest` 下有 `sdkmanager`/`avdmanager` |
| emulator | `D:\Android\sdk\emulator\emulator.exe` | 已装 |
| AVD | `ble_test`（android-34, google_apis, x86_64） | 在 `C:\Users\hkt\.android\avd\`，持久 |
| AEHD 加速 | 已安装（v2.2） | 模拟器开机只需十几秒，**无需重装** |

**重建 `android/local.properties`（若丢失）**：
```properties
sdk.dir=D\:\\Android\\sdk
```

---

## 2. 构建 APK

**首选根目录 `build.ps1`**（内部调用 `scripts/android/build.ps1`，自动设 `JAVA_HOME=D:\Java\jdk-17` 后调 `android/gradlew.bat`，规避 PATH 上无效的 jdk-21）：

```bash
./build.ps1 assembleDevDebug         # dev flavor debug（可装）
./build.ps1 assembleProdDebug        # prod flavor debug（可装，推荐分发）
./build.ps1 assembleProdRelease      # prod release（minify+shrink，未签名）
./build.ps1 clean                    # 清理
./build.ps1 lint                     # 代码检查
./build.ps1 testDevDebugUnitTest --tests "com.hkt.ble.bletools.BluetoothScanFilterTest"
```

变体：flavor × buildType = `{dev,prod} × {debug,release}`

**产物路径**：`android/app/build/outputs/apk/<flavor>/<buildType>/`
- `app-prod-debug.apk` ≈ 15 MB，**debug 签名，可直接安装**（默认发布包）
- `app-prod-release-unsigned.apk` ≈ 7.6 MB（R8 优化瘦身），**未签名，需自行签名才能装**

**签名现状**：`android/key.jks` 存在（`alias=hkt`），但**密码未知**，`android/app/build.gradle.kts` 未配 `signingConfigs` → release 产物恒为 `-unsigned.apk`。如需正式签名，获取 `storePassword`/`keyPassword` 后用 `apksigner`（见 [docs/安装包制作说明.md](docs/安装包制作说明.md)，注意该文档版本号/SDK 信息偏旧，以 `android/app/build.gradle.kts` 为准）。

---

## 3. 关键环境坑与对策（必读，省大量时间）

### 坑 1：项目路径含中文 → AGP 中止构建
项目位于 `d:\智能体软件\BLETools`，AGP 会抛 `StopExecutionException: non-ASCII path`。
**已解决**：`gradle.properties` 已加 `android.overridePathCheck=true`。**不要删这行**。

### 坑 2：单元测试因中文路径失败（ClassNotFoundException）
`overridePathCheck` 只绕过 AGP 检查，**管不到 Gradle 测试 worker 的 classloader**——测试类 `.class` 已编译产出，但运行时 `ClassNotFoundException`。
**对策**：把项目复制到 **ASCII 物理路径**再跑测试（junction 无效，因 Java `getCanonicalPath()` 会还原中文真实路径）：
```bash
powershell.exe -Command "robocopy 'D:\智能体软件\BLETools' 'D:\BLETools' /E /XD build /XF *.apk" 
cd /d/BLETools && ./gradlew.bat testDevDebugUnitTest --console=plain
```

### 坑 3：国内网络下载超时
默认 `google()`/`mavenCentral()`/`services.gradle.org` 在国内极慢/超时（GitHub release 同样慢；`dl.google.com` 反而快）。
**已解决**：
- `android/settings.gradle.kts` 已加**阿里云 Maven 镜像**（`google`/`public`/`gradle-plugin`，官方源作 fallback）
- `android/gradle/wrapper/gradle-wrapper.properties` 的 `distributionUrl` 已改为**腾讯云 gradle 镜像**

### 坑 4：Git Bash 的 `tar` 不支持 zip
GNU tar 解 zip 报 `numeric off_t value expected`。
**对策**：用 Windows 自带 bsdtar（`C:\Windows\System32\tar.exe`）或 PowerShell `Expand-Archive`。

### 坑 5：adb 在 Git Bash 下路径转换
`adb pull /sdcard/x.xml` 会被 MSYS 转成 `F:/Git/sdcard/x.xml`。
**对策**：用 `MSYS_NO_PATHCONV=1`，或 `adb exec-out cat /sdcard/x.xml > local.xml`（二进制安全）。

---

## 4. 运行测试

| 测试类型 | 命令 | 前提 |
|---------|------|------|
| 单元测试 | `./gradlew.bat testDevDebugUnitTest` | **必须在 ASCII 路径跑**（坑 2） |
| instrumented | `./gradlew.bat connectedDevDebugAndroidTest` | 需模拟器或真机在线 |
| BLE 功能验证 | — | **只能真机 + 真实设备**（UDS100/DC200 等） |

现有单元测试：`BluetoothScanFilterTest`（BLE 扫描过滤逻辑）、`ExampleUnitTest`。

---

## 5. 模拟器（UI 仿真）

**一键启动**：双击根目录 [启动模拟器.bat](启动模拟器.bat)（内部调用 `android/start-emulator.bat`，自动启动→等开机→装最新 prod-debug APK→启动 app）。

**手动启动**（任选其一，**必须先设 `JAVA_HOME`**）：
```cmd
:: CMD
set JAVA_HOME=D:\Java\jdk-17
"D:\Android\sdk\emulator\emulator.exe" -avd ble_test
```
```powershell
# PowerShell
$env:JAVA_HOME="D:\Java\jdk-17"
& "D:\Android\sdk\emulator\emulator.exe" -avd ble_test
```

**自动化用无窗口模式**（截图等）：加 `-no-window -no-audio -gpu swiftshader_indirect`。

**已知限制（重要）**：模拟器**无真实蓝牙硬件**，能验证 app 启动/UI/权限流程/不崩溃，**但 BLE 扫描、GATT 连接、协议收发、OTA 全部测不了** → 这些必须真机。

---

## 6. 命令速查

```bash
# 构建（Git Bash / agent 默认环境）
JAVA_HOME=D:/Java/jdk-17 ./build.ps1 assembleProdDebug

# 列出 AVD
"D:/Android/sdk/emulator/emulator.exe" -list-avds

# 检查模拟器加速（应显示 AEHD usable）
JAVA_HOME=D:/Java/jdk-17 "D:/Android/sdk/emulator/emulator.exe" -accel-check

# adb 常用（路径含中文时先 copy 到 ASCII 再 install）
"D:/Android/sdk/platform-tools/adb.exe" devices
"D:/Android/sdk/platform-tools/adb.exe" install -r <apk>
"D:/Android/sdk/platform-tools/adb.exe" shell monkey -p com.hkt.ble.bletools -c android.intent.category.LAUNCHER 1
"D:/Android/sdk/platform-tools/adb.exe" emu kill          # 关闭模拟器

# 签名 release（需 key.jks 密码）
BT="D:/Android/sdk/build-tools/34.0.0"
"$BT/zipalign.exe" -p -f 4 in.apk aligned.apk
"$BT/apksigner.bat" sign --ks android/key.jks --ks-key-alias hkt --out signed.apk aligned.apk
```

---

## 7. 给 Agent 的工作要点

1. **改协议/UI/加设备类型** → 先读 [CLAUDE.md](CLAUDE.md) 的「The wire protocol」和「Adding a new device type」章节，涉及 `Communicate.kt`/`DeviceActivity.kt`/`MainActivity.parseDeviceType` 多文件协同改动。
2. **构建前**确认 `gradle.properties` 含 `android.overridePathCheck=true`，否则中文路径直接失败。
3. **跑单元测试**记得切到 ASCII 路径副本，否则 `ClassNotFoundException`。
4. **新装 SDK 组件**用 `sdkmanager`（需 `JAVA_HOME=D:\Java\jdk-17`），许可证已接受。
5. **全局可变状态并发**：`StreamThread`（后台线程）与 `streamRev()`（BLE 回调）并发读写 `mDeviceData`/`mDeviceEvent`/`mDeviceDataString`，UI 线程也读——无锁，改动需小心（见 CLAUDE.md「Concurrency hazard」）。
