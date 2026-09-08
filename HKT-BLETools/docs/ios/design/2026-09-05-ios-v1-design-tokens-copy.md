# iOS v1 设计 Token 与中英文文案表（D5 批次）

| 项 | 内容 |
| --- | --- |
| 文档版本 | v0.1 Review Draft |
| 日期 | 2026-09-05 |
| 输入基线 | [UI 设计计划](2026-09-03-ios-v1-ui-design-plan.md) §6/§7、[线框](2026-09-05-ios-v1-wireframes.md) |
| 性质 | SwiftUI 实现依据：Token 以代码常量/Asset Catalog 语义名表达，文案以 String Catalog key 表达；两表均随评审迭代 |
| 本地化机制 | String Catalog（`.xcstrings`），默认语言 zh-Hans，全量补 en；应用内语言切换通过自定义 locale 覆写生效（F-07） |

## 1. 设计 Token

### 1.1 色彩（浅/深双外观，语义命名）

| Token | 浅色 | 深色 | 用途 |
| --- | --- | --- | --- |
| `color/ble.ok` | System Green | 同 | 已连接/轮询正常/成功 |
| `color/ble.warn` | System Orange | 同 | 轮询无响应/弱信号/警告 |
| `color/ble.error` | System Red | 同 | 断开/失败/数据异常 |
| `color/ble.info` | System Blue | 同 | 扫描中/进行中阶段 |
| `color/ota.progress` | System Blue | 同 | 进度条 |
| `color/bg.primary` | System Background | 同 | 页面背景 |
| `color/bg.card` | Secondary System Grouped Background | 同 | 设备卡/字段卡 |
| `color/text.primary` | Label | 同 | 主文本 |
| `color/text.secondary` | Secondary Label | 同 | 辅助文本/单位 |

色值全部走系统语义色保证深色模式；品牌强调色后续由视觉稿定稿补充，不阻塞组件开发。

### 1.2 字体（Dynamic Type 全量支持）

| Token | TextStyle | 用途 |
| --- | --- | --- |
| `font/pageTitle` | largeTitle | 导航标题 |
| `font/sectionHeader` | title3 | 分组头 |
| `font/fieldLabel` | subheadline | 字段标签 |
| `font/fieldValue` | body | 字段值 |
| `font/caption` | footnote | 时间戳/单位/辅助 |

### 1.3 间距/圆角/图标

| Token | 值 | 用途 |
| --- | --- | --- |
| `space/s` · `/m` · `/l` | 8 · 16 · 24 pt | 4pt 网格 |
| `radius/card` · `/dialog` | 12 · 20 pt | 卡片/弹窗 |
| `icon/rssi.0~3` · `icon/ble.state` | SF Symbols（`wifi` 系列不适用，RSSI 用自定义四档或 `cellularbars`） | 信号/状态 |

### 1.4 组件状态矩阵（UI 验收标准 1）

| 组件 | normal | disabled | loading | error |
| --- | --- | --- | --- | --- |
| PrimaryButton | 强调色填充 | 灰化+`disabled`文案 | 转圈+禁点 | — |
| DeviceCard | 可点 | — | 名称位"发现中…" | — |
| FieldRow | 值正常 | 置灰（能力未开放） | 值位"…" | 值位"——"+行内错误角标 |
| OTAStageList | 当前态高亮 | 未达态置灰 | 当前态转圈 | 失败态红色+阶段名 |
| ConfirmDialog/ErrorBanner/EmptyState | 见线框对应态 | — | — | 必含建议动作按钮 |

## 2. 中英文文案表（String Catalog key 草案）

命名 `<feature>.<screen>.<element>`。下表为 v1 核心集；设备字段名（需求 §7.1–7.3 全集）按 `field.<name>` 规则补全，随高保真稿定稿。

### 2.1 通用

| Key | zh-Hans | en |
| --- | --- | --- |
| common.retry | 重试 | Retry |
| common.cancel | 取消 | Cancel |
| common.confirm | 确认 | Confirm |
| common.done | 完成 | Done |
| common.back | 返回 | Back |
| common.exportLog | 导出日志 | Export Log |
| common.comingSoon | 即将开放 | Coming soon |

### 2.2 扫描/连接（P-01/P-02/P-08）

| Key | zh-Hans | en |
| --- | --- | --- |
| scan.state.scanning | 扫描中… 已发现 %lld 台 | Scanning… %lld found |
| scan.state.done | 扫描完成 · %lld 台 | Scan finished · %lld found |
| scan.state.empty | 未发现支持设备 | No supported devices found |
| scan.recent | 上次连接：%@" | Last connected: %@" |
| connect.stage.connecting | 正在连接… | Connecting… |
| connect.stage.discovering | 正在发现服务… | Discovering services… |
| connect.stage.subscribing | 正在建立通知… | Subscribing to notifications… |
| connect.fail.reason | 无法连接到设备 | Couldn't connect to the device |
| permission.bluetooth.denied | 需要蓝牙权限才能查找和连接附近设备 | Bluetooth permission is required to find and connect devices |
| permission.bt.off | 蓝牙已关闭，请开启后重试 | Bluetooth is off. Turn it on and try again |
| permission.ble.unsupported | 此设备不支持蓝牙低功耗 | This device doesn't support Bluetooth Low Energy |

### 2.3 设备详情（P-03）

| Key | zh-Hans | en |
| --- | --- | --- |
| device.state.ready | 已连接 · 轮询正常 | Connected · polling |
| device.state.stale | 最后更新 %lld 秒前 · 正在重试 | Updated %lld s ago · retrying |
| device.state.dataAbnormal | 数据格式异常，已显示上次有效值 | Data format abnormal; showing last valid values |
| device.state.disconnected | 连接已断开 | Disconnected |
| device.state.reconnecting | 重连中（第 %lld/%lld 次） | Reconnecting (attempt %lld of %lld) |
| device.state.upgradeMode | 设备处于升级模式：上次升级未完成，请重新执行固件升级 | Device is in upgrade mode: the last update didn't finish. Please run the firmware update again |
| device.action.gotoUpgrade | 去升级 | Update Firmware |

### 2.4 OTA 六阶段与结果（P-04）

| Key | zh-Hans | en |
| --- | --- | --- |
| ota.stage.pick.title | 选择固件文件 | Select firmware file |
| ota.stage.confirm.title | 确认升级？ | Start update? |
| ota.stage.confirm.body | 升级中请保持 App 前台，勿断开设备 | Keep the app in the foreground and don't disconnect the device |
| ota.stage.transferring | 数据传输中 | Transferring data |
| ota.stage.finalPadding | 完成数据发送 | Finishing transfer |
| ota.stage.waitingReboot | 等待设备重启…（尚未完成） | Waiting for the device to restart… (not done yet) |
| ota.stage.waitingAdvertisement | 等待设备重新广播…（尚未完成） | Waiting for the device to re-advertise… (not done yet) |
| ota.stage.reconnecting | 正在重新连接…（尚未完成） | Reconnecting… (not done yet) |
| ota.stage.verifyingVersion | 正在确认新版本…（尚未完成） | Verifying the new version… (not done yet) |
| ota.result.success | 升级成功 %@ → %@ · 全程 %@ | Update succeeded %@ → %@ · took %@ |
| ota.result.report | 查看升级报告 | View update report |
| ota.leaveguard.title | 升级正在进行 | Update in progress |
| ota.leaveguard.body | 中断可能导致设备无法正常工作。确定要离开吗？ | Interrupting may leave the device unusable. Leave anyway? |
| ota.file.mismatch | 固件文件与设备型号可能不匹配，升级失败可能导致设备无法使用 | This firmware may not match the device; a failed update can leave it unusable |
| ota.mtu.blocked | 当前连接带宽不足以传输固件，请重新靠近设备后再试 | The connection can't carry firmware data. Move closer and try again |

### 2.5 错误十分类（需求 §10.2 → UI 计划 §5.1）

| Key | zh-Hans | en |
| --- | --- | --- |
| error.input | 输入超出有效范围（%@） | Value out of range (%@) |
| error.permission | 缺少必要权限 | Required permission missing |
| error.bluetoothUnavailable | 蓝牙不可用 | Bluetooth unavailable |
| error.connection | 连接失败 | Connection failed |
| error.serviceMissing | 设备服务异常，请重试或检查固件 | Device services missing; retry or check firmware |
| error.writeFailed | 命令发送失败 | Failed to send the command |
| error.crc | 数据校验失败，链路质量差 | Data check failed; poor link quality |
| error.responseFormat | 设备返回的数据无法识别 | Unrecognized data from the device |
| error.otaFile | 固件文件无效或已损坏 | Invalid or corrupted firmware file |
| error.otaTimeout | 设备长时间无响应 | The device stopped responding |

### 2.6 设置/关于（P-07）

| Key | zh-Hans | en |
| --- | --- | --- |
| settings.language | 语言 | Language |
| settings.language.system | 跟随系统 | Follow System |
| settings.diagnosticLog | 诊断日志 | Diagnostic Log |
| settings.scanFilter.rssi | 信号强度阈值 | Signal strength threshold |
| settings.scanFilter.hideUnnamed | 过滤无名称设备 | Hide devices without a name |
| about.version | 版本 | Version |
| debug.enableHint | 已开启调试模式 | Debug mode enabled |

## 3. 评审核对

1. Key 命名与 UI 计划 §7 规范一致；无 view 内硬编码路径可走（实现期以构建检查兜底）。
2. OTA"尚未完成"后缀与需求 §8.4/ADR-006 一致。
3. 十类错误文案与架构错误树一一对应（spec §错误体系）。
4. 英文文案为初稿，随高保真稿（D3）人工润色。
