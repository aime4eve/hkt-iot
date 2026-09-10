# 参数配置页（P_config）— 规格卡（从冻结原型逐条提取）

> 事实源：`docs/ios/design/prototype/index.html`（已确认·冻结 2026-09-08）。
> 提取位置：`P_config()` L1313-1367；`cfgInit()` L1188；`cfgValidate()` L1369-1387；`cfgSummary()` L1388-1396；`cfgWrite()` L1406-1421；`numRow/segRow/selRow/svcChoice/svcPortAction/svcPortControls/svcStableControls/svcNumRow/cfgSection` L1225-1311；`tzGroups/tzLabel/phoneTzEnc` L1194-1221；CSS `.cfg-section/.cfg-title/.cfg-caption/.cfg-state/.cfg-label/.choice-grid/.choice/.port-grid/.port-set/.mode-set/.mode-option/.cfg-control/.cfg-combo/.cfg-input-row/.savebar/.input`；i18n ZH L341-349 / EN L443-451。
> 原型变更时同一次提交更新本卡（§9 第 4 条）。

---

## 1. 通用结构

```
navbar.small：back「‹ 返回」+「参数配置」（无首页按钮，二级页裁决）
body（padding 0 16 24）
  ├─ [横幅] 保存结果：ok=绿（✓ 配置已保存（设备已确认）+ 右侧「完成」→重置并回正常态）
  │          / fail=红（✕ 设备未确认配置…+ 右侧「重试」→清横幅）
  ├─ 家族表单（见 §2/§3/§4）
  └─ 「保存配置」按钮（.btn 全宽 primary）+ 载荷说明（kv 11px：0x02 · 载荷 {9|4|8} B）
     [SVC 专属] 保存区为吸底 savebar（margin 12 -16 -24；padding 11 16 24；bg=--bg 上边框 line 68%）
```

## 2. UDS100 表单（数字行 .card：label 12px text2 → 输入行[input + 单位] → hint(11px) → error(12px err)）

| 行 | label | hint | 校验 | 单位 |
|---|---|---|---|---|
| 上报周期 | 上报周期 | — | 1–1440 | 分钟 |
| GPS 周期 | GPS 周期 | 0 = 关闭 GPS 定位 | 0 或 10–1440 | 分钟 |
| 低阈值 | 低阈值 | — | 30–4500 | mm |
| 高阈值 | 高阈值 | 0 = 关闭高阈值告警 | 0 或 30–4500 | mm |
- 底注（kv 11px）：固件校验任一参数非法时整包静默拒绝（无 ACK），App 侧已前置同规则校验。

## 3. DC200Family 表单

| 行 | 说明 | 校验 |
|---|---|---|
| 上报周期（数字行）| hint：0 为固件接受的合法值（开放策略见需求 §7.4） | 0–1440，单位分钟 |
| 工作模式（分段芯片卡）| 融合模式 / 仅地磁 / 雷达优先（单选，选中=info 底白字，未选=card2 底 text2，r9 padding 9 4）| — |

## 4. SVC100 表单（两个 cfg-section + 吸底保存）

### cfg-section 结构：标题(15/700)+caption(11px text2)+右上状态胶囊(10px/700，card2 底 line 描边；on=ok 色系)
1. **输出与端口**（caption「选择供电档位与两路端口动作」，状态=派生端口 0xNN）
   - choice-grid 3 列：12V / 9V / 5V（.choice：padding 10 6，r8，card2 底，选中=info 10%底 info字 info描边）
   - caption：固件映射 0=12V / 1=9V / 2=5V
   - 端口功能 label + port-grid 2 列（.port-set：card2 底 r8）：
     - 阀 1（位 0x01）/ 阀 2（位 0x02）各两组 mode-option（开关控制 / PWM 控制，选中=info 10%底 info字）
     - 位运算：`derived(port,bit,mode)=(port&3&~bit)|(mode?bit:0)` 再并 `port&0x80`
   - caption：合成端口值: 0xNN
   - cfg-combo：含稳定时长 switch（切换 0x80 位）+ 稳定时长输入行 s（非稳定模式禁用 opacity .5；err=超出范围 1–255；hint=1–255 秒 / 仅在带稳定时间的端口模式下可编辑）
   - cfg-control：自动开关机 switch + caption smartHint（开启后：阀插入自动开机，阀拔出自动关机）
2. **时间与上报**（caption「用于 0x06 对时换算与定时任务触发」，状态=当前 tz）
   - 时区 select（分组：西半球 UTC−12~−01 / UTC+00:00 / 东半球 +01~+12 / 半小时时区 +03:30 +05:30）
   - caption：设备当前: UTC±HH:MM
   - 按钮（.btn secondary 自适应）「与手机时区一致」：手机偏移按半小时取整编码（±:45 不支持）；+00..+12→编码值；−1..−12→12−h；+03:30→25、+05:30→26；不支持→置 0 并提示
   - 上报周期（svcNumRow，hintPeriod0，单位分钟，0–1440）

### 保存流
1. 点保存配置 → 前置校验：非法 → 各字段行内 error，并滚动到首个错误字段；
2. 通过 → 确认对话框：标题「写入以下配置？」+ 摘要表（table.props 13px，label 列 text2 42% / 值粗体）+ 取消 / 确认写入；
3. 写入中对话框（不可关）：「正在写入配置…」+ 正文居中 `0x02 → {设备} …后4位`（900ms）；
4. 结果：ACK → ok 横幅；无 ACK → fail 横幅（重试）。

## 5. 文案表（新增键；其余复用前卡）

| key | ZH | EN |
|---|---|---|
| cfgConfirmT / confirmWrite | 写入以下配置？ / 确认写入 | Write these values? / Write Values |
| cfgSent | 正在写入配置… | Writing configuration… |
| cfgSave | 保存配置 | Save |
| cfgOk / cfgFail | 配置已保存（设备已确认）/ 设备未确认配置（可能被固件拒绝），请检查参数范围后重试 | Configuration saved (device acknowledged) / Device did not acknowledge (possibly rejected). Check ranges and retry. |
| cfgOutput(Hint) | 输出与端口 / 选择供电档位与两路端口动作 | Output & port / Choose supply level and both port actions |
| cfgTimeReport(Hint) | 时间与上报 / 用于 0x06 对时换算与定时任务触发 | Time & reporting / Used for 0x06 time sync and schedule timing |
| cfgFirmwareMap / cfgDerivedPort | 固件映射 0=12V / 1=9V / 2=5V / 合成端口值 | firmware: 0=12V / 1=9V / 2=5V / Derived port value |
| cfgStableRange / portStableOff | 1–255 秒 / 仅在带稳定时间的端口模式下可编辑 | 1–255 s / Only editable in stable-time port modes |
| smartHint | 开启后：阀插入自动开机，阀拔出自动关机 | When on: the valve powers on when inserted and off when removed |
| hintGps / hintHigh / hintPeriod0 | 0 = 关闭 GPS 定位 / 0 = 关闭高阈值告警 / 0 为固件接受的合法值（开放策略见需求 §7.4） | 0 = GPS positioning off / 0 = high-threshold alarm off / 0 is accepted by firmware (see req §7.4) |
| errRange / errGps / errHigh | 超出范围 {a}–{b} / 合法值：0 或 10–1440 / 合法值：0 或 30–4500 | Out of range {a}–{b} / valid: 0 or 10–1440 / valid: 0 or 30–4500 |
| 时区分组 | 西半球时区（UTC−12:00 ~ UTC−01:00）/ 东半球时区（UTC+01:00 ~ UTC+12:00）/ 半小时时区 | Western… / Eastern… / Half-hour time zones |
| 与手机时区一致 + 提示 | 与手机时区一致（不支持时置 UTC+00:00 并提示） | Match phone timezone |
| portF 7 值 | 端口1+2 脉冲 / 端口1 开关 · 端口2 脉冲 / 端口1 脉冲 · 端口2 开关 / 端口1+2 开关 / 端口1 开关 · 端口2 脉冲（稳定时间）/ 端口1 脉冲 · 端口2 开关（稳定时间）/ 端口1+2 开关（稳定时间） | Port 1 + Port 2 pulse / … / Port 1 + Port 2 switch (stable time) |

## 6. iOS 状态与裁定

1. 草稿初值来自轮询快照（period/gps/low/high/modeL/vol/port/stable/smart/tz）；保存成功横幅确认后「完成」回到详情页（真实 0x02 写入随协议里程碑接入；当前与原型演示一致为 900ms 模拟 ACK，详情页数值以设备后续上报为准）。
2. 稳定时长输入在非稳定模式（port&0x80=0）禁用并跳过校验（Android updateStableTimeEditor 同款）。
3. 校验规则与固件一致：整包前置校验，任一非法即拦截保存并定位首个错误字段。
