# P-07 设置页 + 日志页 — 规格卡（从冻结原型逐条提取）

> 事实源：`docs/ios/design/prototype/index.html`（已确认·冻结 2026-09-08）。
> 提取位置：`P_settings()` L1579-1602；`cycleLang()/langLabel()` L1605-1611；`taps()` L1612-1618；`P_log()` L1615-1621；CSS `.navbar.small/.row/.card/.kv/.slider/.logline`；i18n ZH L281-287 / EN L383-392。
> 原型变更时同一次提交更新本卡（§9 第 4 条）。

---

## 1. 设置页结构（navbar.small + body padding 0 16 24）

```
navbar.small：back 胶囊「‹ 返回」+ 标题「设置」（无首页按钮，用户 2026-09-10 裁决：二级页不显示）
body
  ├─ section「语言」
  │    └─ row：label「语言」 + value「跟随系统 ▸」(点击循环 跟随系统→简体中文→English)
  ├─ section「诊断日志」
  │    └─ row：label「诊断日志」 + value「查看 ▸」(→ 日志页)
  ├─ section「扫描过滤」
  │    ├─ row：label「信号强度阈值」 + value：滑杆(min -95 max -40 step 5，accent=info，宽 110px) + 「{v} dBm」
  │    └─ card：k「设备名称前缀过滤」(12px text2) + 4 枚芯片 MPS/SVC/UDS/EPS + hint(11px text2)
  ├─ section「关于」
  │    ├─ row：label「版本」 + value「1.0.0 (123)」(点击 7 次=开启调试)
  │    └─ row：label「隐私说明」 + value「▸」(→ 隐私页，随下一批接入)
  └─ taps 提示行（居中，13px；点版本行出现：还差 N 次=text2 / 已开启=ok）
```

### 1.1 前缀芯片
- 4 枚等宽横排 gap 6；padding `8 0`、圆角 9、13px/600、居中。
- 选中=info 底白字；未选中=card2 底 text2 字。点击切换（R-1 扫描过滤：名称前 3 字符命中任一勾选前缀才入列）。

### 1.2 语言模式
- 三态：跟随系统 / 简体中文 / English，点击循环；切换立即生效（全 App 中英文案随之切换）。

### 1.3 7 击调试
- 点版本行累计 7 次：<7 次显示「再点 {N} 次开启调试模式」(text2)；第 7 次显示「已开启调试模式」(ok) 并保持。

## 2. 日志页结构

```
navbar.small：back 胶囊 + 标题「诊断日志」+ navright linkbtn「导出」（无首页按钮，同 §1 裁决）
body：日志行 ×N（或 空态「暂无日志」kv 居中 padding 30）
```

### 2.1 日志行（.logline）
- 等宽字体 11px、行高 1.8、底边 1px line、padding 2 0。
- 结构：时间戳（text2，右距 6）+ 级别（宽 44、粗体：ERR=err / WARN=warn / 其余=info）+ 事件+详情。

### 2.2 导出
- 原型为 toast 假动作「导出日志 ✓ (txt)」→ iOS v1 同为占位（真实导出随诊断里程碑接入），按钮按原型渲染。

## 3. 文案表

| key | ZH | EN |
|---|---|---|
| settings | 设置 | Settings |
| lang / sys | 语言 / 跟随系统 | Language / Follow System |
| 语言值 | 简体中文 / English | 简体中文 / English |
| diag / view | 诊断日志 / 查看 ▸ | Diagnostic Log / View ▸ |
| rssiTh | 信号强度阈值 | Signal threshold |
| prefixF / prefixHint | 设备名称前缀过滤 / 仅显示名称以所选前缀开头的设备；未命名设备自动排除 | Name prefix filter / Only devices whose name starts with a selected prefix; unnamed devices are excluded |
| about / ver / privacy | 关于 / 版本 / 隐私说明 | About / Version / Privacy |
| taps(n) / dbgOn | 再点 {n} 次开启调试模式 / 已开启调试模式 | {n} more taps to enable Debug / Debug mode enabled |
| logT / logExport / logEmpty | 诊断日志 / 导出 / 暂无日志 | Diagnostic Log / Export / No entries yet |

## 4. iOS 状态与数据

| 项 | 载体 |
|---|---|
| 语言模式 | LanguageStore（@Observable 单例）：system/zh/en；isZh 供全 App `zh` 计算（视图经 @Environment 观察，切换即时生效） |
| 信号阈值 | ScanModel.rssiThreshold（既有，-80 默认） |
| 前缀过滤 | ScanModel.allowedPrefixes（既有）+ togglePrefix |
| 版本 | Bundle CFBundleShortVersionString + build |
| 调试计数 | AppSettings.debugTaps / debugOn |
| 日志 | LogStore（@Observable 单例，环形上限 500 条）：扫描启停/发现设备/连接成功/失败/断开等 App 层事件 |

## 5. 一致性裁定

1. **隐私说明行**：入口按原型渲染；隐私页（P_privacy 全文）属「设置/日志/隐私」批次的最后一项，到货前点击为占位。
2. **导出**：原型即假动作；iOS 保留按钮占位，真实 txt 导出随诊断里程碑接入。
3. **调试模式**：计数与开启态照原型；调试页本身不在 v1 原型页面清单（后续里程碑）。
