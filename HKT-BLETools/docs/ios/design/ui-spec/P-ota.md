# OTA 升级页（P_ota）— 规格卡（从冻结原型逐条提取）

> 事实源：`docs/ios/design/prototype/index.html`（已确认·冻结 2026-09-08）。
> 提取位置：`P_ota()` L1056-1100；`stageList()` L1116；`pickFile/fileinfo/confirmDialog/runOTA/failOTA/forceAbort` L1122-1177；`PKTS=1284`；演示 tick=380ms；CSS `.progress/.steps/.banner/.row`；i18n OTA 段（ZH L292-309 / EN L393-408）。
> 原型变更时同一次提交更新本卡（§9 第 4 条）。

---

## 1. 阶段流（演示引擎=tick 380ms，真实 OTAEngine 随协议里程碑接入）

```
stage 0 选择固件 → 1 确认对话框 → 2 数据传输(0→1284 包, 每16包页写延迟) → 3 传输完成
→ 4 等待重启(4 tick) → 5 重连(3 tick) → 6 版本确认 → 7 完成 → 结果(成功/失败)
```

## 2. 各阶段 UI

### stage 0 选择固件
- .row：📄 选择固件文件… ▸（点击=picked 演示文件）
- [已选] 文件卡：📄 **mps100_v1.28_full.bin** / 164,352 B · CRC ✓ / 当前版本 v1.26 → 期望版本: **1.28**
- 「开始升级」.btn（未选文件 disabled）
- kv：⚠︎ 升级期间请保持 App 前台、勿锁屏

### stage 1 确认对话框（296/r18）
- 标题「确认升级？」；正文 confirmB（设备 …9C01 将从 v1.26 升级到 v1.28，保持前台/蓝牙/勿离页）
- 取消(secondary) / 确认升级(primary)

### stage 2–7 进行中（用户 2026-09-10 裁决：标注目标设备 + 内容居中 + 取消贴底）
- 整列 flex 纵向：**stageList 顶部** → 中段卡片+横幅（`margin:auto 0` 垂直居中）→ **「取消升级」贴底全宽**
- **stageList** 六段（选择固件/确认/数据传输/等待重启/重连/版本确认）：每段 上 4px 圆角条（active=info/已过=ok/未到=fill）+ 下 10px 名称（active=info/已过=ok/未到=text2）
- 当前阶段卡（**text-align:center 居中**）：
  - 首行小字(11px text2)：`正在升级设备 {设备名} {ID后4位}`（targetDev，用户裁决 1）
  - 阶段文案（数据传输中/数据发送完成/等待设备重启…/等待设备重新广播…/正在重新连接设备…/正在读取新版本…）
    - stage≥4 追加 warn badge「尚未完成」；stage==2 页写入时追加「· 页写入中…」
  - 进度条（stage≥4 满，否则 pkt%）
  - 居中 kv：`包 X / 1284 · NN%`（百分比仅 stage≤3；用户裁决 2 由左右两端对齐改居中合并）
- stage≤3：warn banner「⚠︎ 升级期间请保持 App 前台、勿锁屏」（居中）
- 「取消升级」.btn danger **贴底** → guard 对话框：⚠︎ 升级正在进行 / leaveB / 继续升级(secondary) / 仍然离开(danger→fail)

### 成功（result ok）
- .center：✓(48px ok) + 「升级成功 1.26 → 1.28」(17/600) + ⏱ 总耗时(13px text2)
- 「查看升级报告」(secondary 自适应) → 报告对话框：rpTitle 各行（设备/升级前版本/期望版本/实际版本/阶段耗时/传输包数/设备复位重传/结果/总耗时）
- 「完成」(primary 自适应 padding 12 40) → 回详情

### 失败（result fail）
- .center：✕(48px err) + 「升级失败」+ 原因: xxx + 「重新升级」+ 「导出日志」(secondary 占位) + linkbtn「返回」

## 3. iOS 状态与裁定

1. 演示引擎：Timer 380ms tick 复刻原型推进（无 Scene 复位/版本不符演示分支；失败分支经 guard 仍离开/引擎超时触发，文案就绪）。
2. 真实 OTAEngine（分页传输、设备复位重传、版本校验）随协议里程碑接入；UI 不变。
3. 入口：详情页会话条「固件升级」（此前占位，本批接通）。
