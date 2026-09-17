# 迁移对照表（2026-09-17 P0 规范化迁移）

来源：`98-hkt-iot/HKT-Firmwares`（分支 main @ 41ad0d9）。来源脚本自此降级为迁移素材，正确性判定见仓库 README「正确性条款」。

## 判定规则摘要

- 版本：来源文件带版本号沿用（v1.3→v1.3.0）；无版本名一律定基线 v1.0.0；按权威依据重写的升 v2.0.0（破坏性变更）。
- 入口：历史 `Decoder(bytes, port)` 统一追加 `decodeUplink(input)` 适配层；已有 `decodeUplink` 的不动。
- 平台：来源文件名 `_chirpstack`/`decodeUplink` 风格→chirpstack；`_ttn`→ttn；`-cn`/`-en`→语言限定符。

## 自研（7 项）

| 旧路径 | 新路径/文件 | 版本判定 | 备注 |
| --- | --- | --- | --- |
| LoRaWAN_People_Counter/…_Recv/Compents/Decode/people_couter.js | in-house/irc01/irc01_chirpstack_v1.0.1.js | 沿用修复后状态 1.0.1 | 含 2026-09-04 八项修复；固件权威 communicate.c（USER/Drive/）；**verified** |
| LoRaWAN_Air_Switch/Compents/Decode/door_sensor.js | in-house/air-switch/dms01_chirpstack_v1.0.0.js | 基线 1.0.0 | 头注释 DMS01 vs 官方索引 SCB100，待定名 |
| LoRaWAN_Air_Switch_CL/Compents/Decode/door_sensor.js | in-house/air-switch-cl/dms01_chirpstack_v1.0.0.js | 基线 1.0.0 | 与 air-switch 逐字节相同；CL100 硬件运行空开软件 |
| LoRaWAN_Animals_Track/Compents/Decode/cattle_sheep_track.js | in-house/ct02/ct02_chirpstack_v1.0.0.js | 基线 1.0.0 | 头注释 CT02 vs 官方索引 AT100/GAT100 |
| LoRaWAN_IAQ1000/Compents/Decode/Air_Sensor.js | in-house/aqs1000/aqs1000_chirpstack_v1.0.0.js | 基线 1.0.0 | 官方索引：IAQ300/600/900 共用 AQS1000 协议 |
| LoRaWAN_Smart_Rumen_Bolus/Compents/Decode/rbc-100_ttn.js | in-house/rbc-100/rbc-100_ttn_v1.0.0.js | 基线 1.0.0 | 官方索引型号 SRB100 |
| LoRaWAN_THTB_Sensor/Compents/Decode/THTB_Sensor.js | in-house/gt30/gt30_chirpstack_v1.0.0.js | 基线 1.0.0 | 头注释 GT-30 |

## 外购（18 项）

| 旧路径 | 新路径/文件 | 版本判定 | 备注 |
| --- | --- | --- | --- |
| 欧孚-手环/chirpstack-decode.js | out-sourced/oufu/b2315l/b2315l_chirpstack_v1.0.0.js | 基线 1.0.0 | 无版本名 |
| 欧孚-手环/chirpstack-decode-b2315l-v1.3.js | out-sourced/oufu/b2315l/b2315l_chirpstack_v1.3.0.js | 沿用 1.3.0 | current |
| PIR100-人存感应器/decode/pir-100_chirpstack.js | out-sourced/oem/pir-100/pir-100_chirpstack_v1.0.0.js | 基线 1.0.0（存档勿用） | 与手册不符（KB 2026-09-04 已论证） |
| PIR100-人存感应器/decode/pir-100_ttn.js | out-sourced/oem/pir-100/pir-100_ttn_v1.0.0.js | 基线 1.0.0（存档勿用） | 同上 |
| —（按手册 §5 重写） | pir-100_chirpstack_v2.0.0.js / pir-100_ttn_v2.0.0.js | v2.0.0 current | 手册原文核验+黄金样例；**verified**。原 feature 分支上的同主题修复因分支不可达，按手册重做 |
| gat-100…/gat-100_chirpstack.js | out-sourced/oem/gat-100/gat-100_chirpstack_v1.0.0.js | 基线 1.0.0 | 无文档 |
| gat-100…/gat-100_ttn.js | out-sourced/oem/gat-100/gat-100_ttn_v1.0.0.js | 基线 1.0.0 | 无文档 |
| 气表/gm-100 Gas meter decode/gm-100_chirpstack.js | out-sourced/oem/gm-100/gm-100_chirpstack_v1.0.0.js | 基线 1.0.0 | |
| 气表/gm-100 Gas meter decode/gm-100_ttn.js | out-sourced/oem/gm-100/gm-100_ttn_v1.0.0.js | 基线 1.0.0 | |
| 气表/gm100-decode-cn.js | out-sourced/oem/gm-100/gm-100_chirpstack_cn_v1.0.0.js | 基线 1.0.0 | 语言变体 cn |
| 水表/汇中/uni-decode-en.js | out-sourced/huizhong/uni-water/uni-water_chirpstack_en_v1.0.0.js | 基线 1.0.0 | README 标注“当前推荐”→current |
| 水表/汇中/uni-decode-cn.js | out-sourced/huizhong/uni-water/uni-water_chirpstack_cn_v1.0.0.js | 基线 1.0.0 | 语言变体 cn |
| 水表/中配/hkt_water_meter_decoder.js | out-sourced/oem/hkt-water-meter/hkt-water-meter_chirpstack_cn_v1.0.0.js | 基线 1.0.0 | 迁移中去除 Buffer 依赖（生产运行时无 Buffer） |
| 水表/中配/hkt_water_meter_decoder_en.js | out-sourced/oem/hkt-water-meter/hkt-water-meter_chirpstack_en_v1.0.0.js | 基线 1.0.0 | 同上 |
| 烟感/sd-300…/sd-300_chirpstack.js | out-sourced/oem/sd-300/sd-300_chirpstack_v1.0.0.js | 基线 1.0.0 | |
| 烟感/sd-300…/sd-300_ttn.js | out-sourced/oem/sd-300/sd-300_ttn_v1.0.0.js | 基线 1.0.0 | |
| 耳标/Ear tag decode/eartag_chirpstack.js | out-sourced/oem/eartag/eartag_chirpstack_v1.0.0.js | 基线 1.0.0 | |
| 耳标/Ear tag decode/eartag_ttn.js | out-sourced/oem/eartag/eartag_ttn_v1.0.0.js | 基线 1.0.0 | |
| 水表/汇中/Sample_chirpstack.js | out-sourced/oem/liquid-level/liquid-level_chirpstack_v1.0.0.js | 基线 1.0.0 | **实为液位传感器**（@product Liquid level sensor），与汇中水表无关，独立立户 |

## 文档随迁

PIR-100 手册 PDF、欧孚 B2315L 协议 DOCX ×2、GM-100 手册 MD、SD-300 手册 PDF、耳标协议 PDF、中配水表协议 MD → 各设备 `docs/`；欧孚真机历史数据 XLSX ×2 → `out-sourced/oufu/b2315l/data/`。

## 刻意不迁移

- `in-house/LoRaWAN_People_Counter/…/Decode/pir-100 PIR Sensor/`：out-sourced 区已有同内容副本，避免双真相，仅迁 out-sourced 一份。
- `汇中/test-node.js`、`气表/run-decode-sn.js`、`LoRaWAN-tools.html`、`encode-test.txt` 等：工具/数据资产，不是解码器；其产出已提炼进黄金样例。如需可在旧位置查阅。
- 各 Decode 目录的 `.vscode/`：编辑器配置，无迁移价值。
