# HKT-Decoders — 设备负载解码器仓库与调试平台

**解码器的唯一权威存放地是调试平台**（团队服务器上运行的 `platform/`，其数据目录保存全部解码器代码与台账），通过平台界面统一管理：新增设备、上传/更新解码器版本、编辑台账、管理黄金样例与协议文档、zip 批量导入/全量备份导出。

本仓库的职责（2026-09-17 起）：
1. **规范文档**：命名/版本/元数据/样例/核验的规范（下文），录入平台时照此执行；
2. **初始数据源**：`decoders/` 目录内容已通过平台「导入」功能录入服务器（zip 包形态，不是目录部署），此后平台为准；
3. **历史存档**：git 历史保留迁移轨迹。

> 正确性条款：解码器的正确性依据按设备来源二选一，没有第三种。
> - **自研设备**：正确性来自**固件代码**（通信/组帧源码），元数据 `authority.type = "firmware"`；
> - **采购设备**：正确性来自**协议文档**（用户手册/协议规格），元数据 `authority.type = "protocol-doc"`。
> 2026-09-17 前散落在 HKT-Firmwares 各目录中的解码脚本只是迁移素材，不作为正确性来源；其原位置已降级为指路牌。

## 目录规范

```
decoders/
  in-house/<device>/            自研设备，无厂商层
  out-sourced/<vendor>/<device>/  采购设备；厂商未知/贴牌时 vendor 用 oem
```

每个设备目录内：

| 条目 | 必填 | 说明 |
| --- | --- | --- |
| `decoder.json` | ✅ | 设备与解码器元数据（见下） |
| `{model}_{platform}[_{lang}]_v{x.y.z}.js` | ✅ | 解码器本体，一平台一文件多版本并存 |
| `docs/` | 采购设备必填 | 协议文档权威副本（PDF/DOCX/MD），自研可选（参考资料） |
| `samples/samples.json` | 核验前必配 | 黄金样例（见下） |

## 命名与版本规范

- 文件名：`{model}_{platform}[_{lang}]_v{major}.{minor}.{patch}.js`
  - `model`：小写，连字符分隔（如 `pir-100`、`irc01`、`b2315l`）；
  - `platform`：`chirpstack` / `ttn` / `thingsboard`；
  - `lang`（可选）：输出语言限定 `cn` / `en`，仅当中文版/英文版输出内容确实不同时使用；
  - 版本号三段式数字，同平台不允许重号，旧版本永久保留。
- **双轨版本号**：
  - `decoderVersion`（进文件名）：解码器自己的版本，任何修改（修 bug、加字段、破坏性变更）都递增；破坏性输出变更升 major；
  - `firmwareVersion`（进元数据）：该解码器对应的固件/协议版本，固件协议变了才动它。
- 入口契约：所有解码器统一导出 `decodeUplink(input)`：
  - 入参 `input = { bytes: number[], fPort: number | null }`；
  - 返回 `{ data: object }`；失败返回 `{ data: {}, errors: string[] }`（可附 `warnings: string[]`）；
  - 迁移的历史脚本若仅有 `Decoder(bytes, port)`，由平台适配层包装调用，不影响线上粘贴使用。

## 元数据 `decoder.json`

```jsonc
{
  "model": "IRC01",                       // 展示型号（以官方设备索引/手册封面为准）
  "modelKey": "irc01",                    // 文件名用键
  "aliases": ["PC100"],                   // 其他叫法（检索用，历史口径不一致时必填）
  "name": "红外人流量计数器（接收端）",
  "origin": "in-house",                   // in-house | out-sourced
  "vendor": "HKT",                        // 采购设备填厂商名；未知/贴牌填 "OEM（待补）"
  "category": "人员感知传感器",
  "authority": {                          // 正确性依据（按 origin 二选一）
    "type": "firmware",                   // 自研：固件代码
    "path": "HKT-Firmwares/in-house/.../communicate.c",
    "ref": "41ad0d9"                      // 核验时的固件 commit/版本号，保证可复现
  },
  "authority": {
    "type": "protocol-doc",               // 采购：协议文档
    "file": "docs/xxx用户手册v1.2.pdf",
    "section": "5. 上行协议"
  },
  "fPortDefault": 210,
  "codecs": [{
    "platform": "chirpstack",
    "lang": null,                          // cn/en 或 null
    "file": "irc01_chirpstack_v1.0.1.js",
    "decoderVersion": "1.0.1",
    "firmwareVersion": "1.2",
    "current": true,                       // 每平台+语言组合至多一个 current
    "status": "verified",                  // verified | unverified
    "basis": ["固件 communicate.c 组帧函数", "黄金样例 samples/samples.json"],
    "verifiedAt": "2026-09-17",
    "changelog": "修复截断帧死循环等 5 项缺陷（2026-09-04）"
  }],
  "deviations": [],                        // 实测帧与协议文档冲突的记录（文档为准，待厂商确认）
  "notes": ""
}
```

平台强制校验：新增/升级版本时 `changelog` 必填；`authority` 完整性按 origin 校验（自研必须有固件 path+ref，采购必须挂实际存在的文档文件）；`current` 只有黄金样例全过才允许标。

## 黄金样例 `samples/samples.json`

```jsonc
[{
  "name": "手册示例帧：红外告警+防拆",
  "source": "manual",                     // manual | firmware | field | regression
  "payload": "000104040c857d0077011701",  // hex，可含空格
  "fPort": 210,
  "expectData": { "batteryVoltage": 3.205 },  // 与 result.data 深比较；null=只要求不报错
  "expectError": false,                   // true=必须报错（坏帧用例）；null=不关心
  "note": ""
}]
```

规则：采购设备**必须**含至少一条 `source:"manual"` 的文档示例帧；自研设备用 `source:"firmware"`（按固件逻辑构造）与 `source:"field"`（真机实测）。
实机与文档冲突时：**以文档为准**判解码错误，把偏差记入 `deviations`，不许私改解码器凑实机。

## 核验状态

- `unverified`：默认。迁移件一律从此起步，平台列表黄标提示。
- `verified`：authority 齐备 + 黄金样例全过 + 人工确认后才能标，必须记录 `basis` 与 `verifiedAt`。
- 固件协议升级（自研）或文档换版（采购）后，已核验状态失效，需重新核验。

## 调试平台（platform/）

团队内网部署的单服务管理系统，解码器数据保存在平台专属数据目录（`DATA_DIR`，服务器上为 `~/hkt-decoder-data`，**不随代码目录分发**）。

- **检索**：设备/解码器多维检索（自研/采购、平台、核验状态、关键词、别名）
- **测试**：批量解码（hex 含空格/base64 自动识别、逐条隔离、单条硬超时熔断）
- **回归**：黄金样例一键回归（期望深比较+偏差定位）；**对比**：双版本输出 diff
- **管理**：新增设备、上传新版本（changelog 强制）、台账编辑、切换 current、标记核验（basis 强制）、样例编辑、文档上传、删除版本（移入回收站）
- **导入/备份**：zip 批量导入（增量安全：已存在的文件与台账不覆盖）；全量 zip 备份导出
- 技术栈 Node.js + Fastify + Vue3（无数据库，文件系统存储）；部署见 `platform/README-部署.md`

**新增/更新解码器的日常流程**（全部在平台界面完成）：
1. 新设备：顶栏「＋ 新增设备」→ 填台账与正确性依据 → 在「管理」页上传解码器版本、文档、样例；
2. 更新：「管理」页新增版本（版本号递增、changelog 必填）→ 版本对比确认差异 → 样例回归通过 → 设 current、标核验；
3. 备份：顶栏「⬇ 导出备份」定期下载全量 zip。

## 迁移债务台账（P0 遗留，按此清偿）

| 债务 | 涉及设备 | 说明 |
| --- | --- | --- |
| 缺协议文档 | gat-100、liquid-level | 无任何依据文件，永远无法核验，需向厂商索取 |
| 缺黄金样例 | 绝大多数设备 | 仅 irc01、pir-100、uni-water、hkt-water-meter、gm-100 首批配置 |
| 型号口径核对 | irc01(=PC100?)、ct02(=AT100/GAT100?)、rbc-100(=SRB100?)、hkt-water-meter(=中配?) | 解码器头注释与官方设备索引型号不一致，已进 aliases，待定名 |
|ThingsBoard 变体缺失 | 全部 | 现有脚本仅 chirpstack/ttn 两类，TB 变体待按设备补齐 |

## 迁移对照表

见 `docs/migration-map.md`（旧路径 → 新路径 → 版本判定的完整映射，含判定理由）。
