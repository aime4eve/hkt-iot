# HKT BLETools iOS v1 设计评审报告 — Part 2：架构 / 测试基线 / 固件追溯 / 协议契约

| 项 | 内容 |
| --- | --- |
| 评审范围 | 架构大纲（2026-09-03-ios-v1-architecture-outline.md，含 §7.1/§8.1/§9.5/ADR）、稳定性与连续性测试用例基线（2026-09-04）、firmware-traceability.md、shared/protocol/{app-frame,bootloader-ota,crc16}.md、需求 §3.4/§10.1/§10.2/§10.4/§12.1/§18 |
| 评审维度 | A 需求覆盖；B 技术正确性；C 固件证据链；D 测试可执行性；E 可演进性；F 文档一致性 |
| 日期 | 2026-09-04 |
| 评审人 | iOS v1 设计评审员（独立评审） |
| 抽查方式 | 固件源码 grep 实证（LoRaWAN_ParkingSensor 当前 checkout）+ Android Communicate.kt 对照 + CRC 数值复算 |

## 总体结论：有条件通过

架构分层、并发模型、OTA 停等语义、验收门（版本确认而非 100%）等主干设计正确且明显优于 Android 反模式；固件证据链总体扎实（50 ms 判帧、128/255 缓冲、停等 ARQ、10 s 复位、2 KB 页写均源码实证吻合）。但存在 4 项 P1：其中"OTA 包号自 0x0002 起"与 CRC golden vector "abc→29B1" 属**协议契约事实错误**（会直接写错实现与共享测试向量），"OTA 超时预算矛盾"使设备复位恢复路径不可达，"5 s 睡眠硬约束"与当前 checkout 源码语义不符却被多篇文档以"已核实"口径引用。上述 P1 必须在 M3（固件协议与测试基线里程碑）关闭前修正，并同步修订需求 §18 S-1 表述。

## 分级发现清单

### P0（阻断性）

无。

### P1（评审通过前必须修正）

**P1-1 OTA 包号起点错误：`0x0002` 是 ACK 命令码，不是包号；实际包号自 0 开始**
- 文件/章节：architecture outline §8 要点 2（"包号自 `0x0002` 起（bootloader 请求驱动）"）；shared/protocol/bootloader-ota.md 第 2 步（"The bootloader requests packet number `0x0002`"）。
- 问题：三份证据一致表明首包号为 0：(a) 固件 `Compents/BootLoader/uart.c` `case 1:` 分支 `InfoUartAck(cmd + 1, flash_write_count)`，此时 `flash_write_count=0`，即 ACK 帧为 **cmd=0x02、包号=0x0000**；后续 `InfoUartAck(cmd, ++flash_write_count)` 递增。(b) 包号仅用于 `flash_write_count == count` 比对，地址由 `FLASH_ONE_PAGE_SIZE * firmware_write_count`（2 KB 页计数）决定，与包号无偏移换算。(c) Android `Communicate.kt` `streamRev()` 解析 ACK 后原样回发 `packNum`，收到 ACK(2,0) 即发包 0；若 App 从 0x0002 起发，`0 == 2` 恒假，设备永远回 `InfoUartAck(2, 0)`，传输死锁。
- 依据：`Compents/BootLoader/uart.c:355-360`（case 1）、`uart.c:401-406`（比对与重请求）、`Communicate.kt:386-390`。
- 建议：两处文档改为"包号自 `0x0000` 起；`0x02` 为 ACK/数据请求命令码"，`OTATransferPlanner` 向量首包号锁定 `0x0000`。

**P1-2 CRC golden vector `abc→29B1` 与自述参数矛盾，错误已固化进共享 fixtures**
- 文件/章节：architecture outline §5.1；shared/protocol/crc16.md；shared/fixtures/crc16.json。
- 问题：crc16.md 与固件 `Compents/BootLoader/crclib.c` `crc16_ccitt()` 均为 KERMIT 参数（init `0x0000`、reflected，poly `0x8408`）。按此参数实测：`abc→0x58E9`，`ABC(41 42 43)→0x59E3`，均非 `0x29B1`。`0x29B1` 是 CRC-16/CCITT-FALSE（init 0xFFFF、不反射）对 `"123456789"` 的标准 check 值，属张冠李戴。fixtures 中该条 `hex:"414243"`（"ABC"）配 `expected:"29B1"`，在任何合理参数下都对不上。
- 依据：crclib.c:382-404（实现）；本评审用独立脚本复算（见"源码抽查记录"）；`hkt→0x77D1`、`empty→0x0000` 两条复算通过。
- 建议：删除或修正 `abc` 条目（KERMIT("abc")=0x58E9），并增加 `"123456789"→0x2189`（KERMIT 标准 check 值，与固件参数一致）作为防回归锚点；双端 fixtures 同步修订。

**P1-3 OTA 超时预算自相矛盾：单包 3 s×2 次=6 s 判失败，先于设备 ~10 s 复位 ACK，"自动从包 0 重传"路径不可达**
- 文件/章节：architecture outline §8.1 表（行 2/行 3）；测试基线 TC-BC-003。
- 问题：§8.1 规定"同一包连续 2 次超时判失败并释放资源"（3 s×2=6 s），同时规定"收到 `InfoUartAck(1,0)` 传输中收到即从包 0 自动重传"。但设备复位信号在静默约 10-11 s 后才发出（`reget_flash_data()` `recv_fail_cnt > 10`，每秒一次 nudge）。App 在 ~6 s 已判失败并"释放资源"，不可能在 ~11 s 收到 `ACK(1,0)`。TC-BC-003 的预期"≈10 s：识别设备复位 ACK(1,0) 自动从包 0 重传"在该预算下无法达成；"5 s 停顿自动恢复"也处于 6 s 判失败边界的竞态。
- 依据：`Compents/BootLoader/uart.c:312-327`（10 次计数复位 + 每秒 nudge）；§8.1 与 TC-BC-003 文本对比。
- 建议：明确区分两种静默：(a) App 已发包、等 ACK 超时 → 重发同包（不计"失败"）；(b) 会话级传输静默看门狗设 8~10 s（<10 s 复位阈值留余量或对齐 12 s），超限才判失败。同时规定：packet 超时后保持会话监听至会话级超时，使 `ACK(1,0)` 恢复路径可达；TC-BC-003 三个档位预期随之改写。另修数值不一致："传输间隙必须 <8 s" 与 "2×3 s 判失败"（6 s）矛盾，前者形同虚设（并入本条或列 P3-7）。

**P1-4 "连接后 5 s 无交互即进睡眠、链路可能中断"与当前 checkout 源码语义不符，多篇文档以"已核实"口径引用**
- 文件/章节：architecture outline §7.1 行 6；需求 §18 S-1；firmware-traceability.md §7 "Sleep / keepalive semantics"；测试基线 §1 固件依据、TC-ST-001/003/004。
- 问题：源码 `USER/Drive/control_center.c` `systemLowPowerMode_Process()` 首行即 `if (device_t.ble_connected && !device_t.ble_sleep_mode) return;`——默认配置下**连接期间根本不进该低功耗路径**；而 BLE 收包刷新 `sleep_delay` 的代码在 `USER/Drive/uart.c`（30-31、143-144、282-283、351-352 等）**全部处于注释状态**。即"连接后 5 s 无交互即睡眠断链"在 ParkingSensor 当前源码中不成立；5 s 窗口实际约束的是**未连接/唤醒后**的睡眠决策，或仅在 `ble_sleep_mode=1`（`communicate.c:729/733` 某配置路径置位）时生效。文档断言的"睡眠→断链"因果链未被源码支撑。1 s 轮询作为设计仍是安全且合理的（与 Android 对齐、保证状态新鲜度），但其定位应是"状态刷新策略"而非"经源码核实的保活硬约束"；UDS100/SVC100 是否同语义未核实。
- 依据：control_center.c:76-79（connected 早退）、uart.c 各注释行、systick.c:327（sleep_delay--）、control_center.h:13-14（常量定义）。
- 建议：将 §7.1 行 6 与 S-1 的措辞改为"MPS100 当前 checkout：连接态默认不进低功耗（control_center.c:76）；`ble_sleep_mode` 可配置路径待 M3 逐设备核实"。TC-ST-004（诱导入睡断链）在默认配置下可能无法复现，需改为"`ble_sleep_mode` 启用时"或标记条件性用例；TC-ST-001/003 预期保守成立可保留，但追溯依据需更正。

### P2（应修，不阻断架构评审通过）

**P2-1 `MAX_WAIT_SLEEP_TIMEOUT=60` 在源码中无任何引用**
- 文件/章节：architecture outline §7.1 行 6；firmware-traceability.md §7；需求 S-1。
- 问题：全仓 grep 仅命中 `control_center.h:14` 的 define，无实现引用。"60 s 未连接强制睡眠"目前是**无行为证据的常量**（待核实：可能在其他分支/固件版本使用）。
- 建议：M3 逐设备核实；在此之前该断言降级为"常量存在、行为未证实"。

**P2-2 评审通过标准 5（UI 状态 → ViewModel 状态映射）未完成对照**
- 文件/章节：architecture outline §11 行 5（"评审时核对"）。
- 问题：§3.4 五条评审标准中第 5 条仅登记了落点方向，未逐项核对 UI 设计计划 §3 页面状态集；架构评审闭环条件未满足。
- 建议：本评审通过前补一张"UI 状态 ↔ ViewModel/Session/OTA 状态"对照表，或明确列为 P-04 详设的准入条件。

**P2-3 MTU ≥160 前置检查未指明取 `maximumWriteValueLength(for: .withResponse)` 还是 `.withoutResponse`，OTA 写入方式未定**
- 文件/章节：architecture outline §7.1 行 3、§7 `BleTransport.write` 签名。
- 问题：iOS 两个类型的返回值可以不同；146 B OTA 帧若超出 withoutResponse 上限需走 withResponse（其节奏同样受 `peripheralIsReady` 与系统缓冲约束，语义不同）。§7 仅写"Write Without Response 优先"，未规定 OTA 大帧用哪种；前置检查阈值 160 因此不可实现为确定代码。
- 建议：明确"OTA 帧一律 withResponse（或 withoutResponse+分片）+ 对应的 MTU 判据"，写入 §7.1 并进 `OTATransferPlannerTests` 前置断言；TC-BC-001 的"注入低 MTU 上报"需注明注入的是哪个类型的值。

**P2-4 `waitingReboot/waitingAdvertisement/reconnecting` 的 T1/T2/T3 超时无数值**
- 文件/章节：architecture outline §8 状态图、§8.1 末行（"维持 §8 主流程取值（真机标定）"）。
- 问题：§8 主流程从未给出 T1/T2/T3 默认值；§8.5 场景 7/8/9 与 OTAEngineTests 的注入用例缺少可判定阈值，"真机标定"无登记载体。
- 建议：给出保守初始值（如 T1=15 s、T2=30 s、T3=15 s，标注"待 M6 标定"）并纳入 §13 风险表跟踪。

### P3（建议改进）

**P3-1 bootloader-ota.md 未写明 Bootloader 帧 CRC 覆盖范围**
- 固件对 `cmd+packnum+data` 计算 CRC（`uart.c` `crc16_ccitt(buffer, buflen)`，buffer 自 cmd 起）；app-frame.md 的 App 帧 CRC 覆盖 `cmd+data`。两者不同而 OTA 文档未声明，易被实现为 `cmd+data`。建议补充一行契约。

**P3-2 应用层 RX 缓冲有效容量为 127 B 而非 128 B**
- `uart.c` LPUART1_IRQHandler 判 `uartBle.recvLen < uart_recv_max_len - 1`。结论（响应 <128 B 即安全）不受影响，建议 §7.1 注明防御上限按 127 B。

**P3-3 测试用例措辞与引用问题**
- TC-ST-005 预期"延迟 <2 s 看门狗不误判失败"不可主动判定：设备上报时刻不受 App 控制，应改为双分支可判定表述（延迟 <2 s 不失败 且 注入 ≥2 s 延迟时看门狗按"响应慢/无响应"分别处置）。
- TC-BC-006 追溯写 "§9.5.1"，缺 `SD` 前缀，不符合追溯文档 §5 命名规则。
- TC-ST-006 引用 `UX-CAL-001`、TC-BC-005 引用 `UX-DEV-001 状态⑤`：存在性待核实（本评审未读到 UI 设计计划全文）。

**P3-4 与需求 §12.1 的映射未闭环**
- S-8 声称与 A-04/A-06/A-08/A-10 建立映射，实际追溯列仅出现 A-02/A-04/A-08；TC-ST-002/012（断链/重连/稳定性）应补 A-06/A-10 标注。测试基线 §4.3 自己也承认"M3 后建立映射"，与 S-8 表述不一致。

**P3-5 并发细节未规定**
- `AsyncStream<BleEvent>` 的缓冲策略（bufferingPolicy）未定——背压/丢事件语义影响 `writeReady` 可靠性；actor 内超时 Task 的取消与重入（reentrancy）规则未写。建议在完整设计说明（M2 产物）补齐。

**P3-6 DeviceSession 与 OTAEngine 在 `reconnecting` 阶段的驱动权归属未显式**
- §8 写"复用 §7 连接状态机"（Session 持有），但未写 OTAEngine 在该阶段是等待 Session 事件还是发起重连请求；建议明确"OTAEngine 发起意图、Session 执行并回事件"。

## 检查单结论表

| 检查单 | 结论 | 说明 |
| --- | --- | --- |
| A 需求覆盖 | **通过（1 项留尾）** | §3.4 设计说明 10 项全部有对应章节（1→§2、2→§3、3→§6、4→§5.2/5.3、5→§8、6→§9.1、7→§9.2、8→§9.4/§10、9→§9.3、10→§12 ADR-001~010）；硬性要求 10 条全部映射（§1 表 + §4 规则 1/2/3、§6 要求 7/8、§9.3 要求 9、§4 要求 10）；评审通过标准 1~4 落点成立，标准 5 未完成对照（P2-2） |
| B 技术正确性 | **基本通过（4 项 P1/P2 修正项）** | 分层依赖方向、CoreBluetooth 隔离、actor 单 owner、事件流快照链路正确；identifier 持久性/`retrievePeripherals` 重连/后台限制/前台 OTA 策略符合 iOS 实际；停等 ARQ 与固件语义吻合。错误项：包号起点（P1-1）、超时预算（P1-3）、睡眠语义（P1-4）、MTU 判据（P2-3） |
| C 固件证据链 | **基本扎实，1 项断言失实** | 50 ms、128/255、停等、页写、10 s 复位、更新标志均源码实证吻合（见抽查记录）；失实项：5 s 睡眠因果（P1-4）、60 s 常量无引用（P2-1）；`reget_flash_data`、`FLASH_ONE_PAGE_SIZE=0x800`、`WAIT_SLEEP_TIME_DELAY=5` 标注充分 |
| D 测试可执行性 | **有条件通过** | TC-ST-010~013、TC-BC-002/005~011 预期可判定、追溯成立；S-1~S-8 全部落点；不可判定/不可达项：TC-BC-003（P1-3）、TC-ST-004（P1-4 条件性）、TC-ST-005（P3-3）；T1/T2/T3 无阈值（P2-4） |
| E 可演进性 | **通过** | 新增设备=新增 `DeviceProfile`+能力位（评审标准 1 成立）；新增配置/任务=命令描述符+FeatureSurface，不动 Adapter（标准 2 成立）；协议/OTA 纯函数+Mock 回放保障标准 3。结构性返工风险点：OTAEngine 与单一 Bootloader 契约强绑（三固件"契约一致"目前仅核了 ParkingSensor，UDS100/SVC100 Bootloader 未抽查，建议 M3 补核）；`BleTransport.write` 签名如 P2-3 定型不当会波及 OTA 层 |
| F 文档一致性 | **不通过（本次扣分主项）** | 数值/状态名矛盾：包号起点（P1-1，SD 与 bootloader-ota.md 同错）、超时预算互斥（P1-3）、睡眠断言跨 4 份文档重复失实（P1-4）、CRC 向量三处同错（P1-2）、`<8 s` vs 6 s（并入 P1-3）、A-06/A-10 映射口径（P3-4）；无发现状态命名冲突 |

## 源码抽查记录

抽查对象：`HKT-Firmwares/LoRaWAN_ParkingSensor`（当前 checkout，追溯基线 `nix/device-config-ux@4e9f462`）；Android 对照 `Communicate.kt`；CRC 用独立 Python 实现复算。

| # | 文档断言 | 抽查位置 | 结果 |
| --- | --- | --- | --- |
| 1 | BLE 口 50 ms 判帧 | `USER/Drive/uart.c:349` `LPUART1_IRQHandler` 内 `uartBle.recvTimeout = 50` | ✅ 吻合（其他串口为 20 ms，文档只引 BLE 口，准确） |
| 2 | 应用层 128 B / Bootloader 255 B 缓冲 | `USER/Drive/include/uart.h:25-26`、`Compents/BootLoader/uart.h:25` | ✅ 吻合；有效容量实为 127 B（`< max-1`，P3-2） |
| 3 | 停等 ARQ、回 `InfoUartAck(2, 下一包号)`、坏包重请求 | `Compents/BootLoader/uart.c:381-406` | ✅ 吻合 |
| 4 | 包号"自 0x0002 起" | `uart.c:355-357`（首 ACK 为 (2, 0)）+ `Communicate.kt:386-390` | ❌ **不符**，首包号 0（P1-1） |
| 5 | 每秒 nudge、连续 ~10 次复位传输并回 `ACK(1,0)`（`reget_flash_data`） | `uart.c:312-327`（`recv_fail_cnt > 10`、`start_flag=0`、清计数） | ✅ 吻合 |
| 6 | `FLASH_ONE_PAGE_SIZE=0x800`、每 16 包页写 | `Compents/BootLoader/flash.h:12`、`uart.c:385-388`（256×u64=2 KB，128 B/包→16 包） | ✅ 吻合 |
| 7 | 146 B OTA 帧（hkt3+len2+cmd1+packnum2+data128+crc2+bootload8） | `uart.c` 解析 `len-15==buflen` + `Communicate.kt:788` 帧拼装 | ✅ 算术与实现吻合 |
| 8 | `WAIT_SLEEP_TIME_DELAY=5` / `MAX_WAIT_SLEEP_TIMEOUT=60` | `USER/Drive/include/control_center.h:13-14` | ✅ 常量存在；❌ "连接态 5 s 睡眠断链"不成立（`control_center.c:76` connected 早退、`uart.c` 刷新代码全注释，P1-4）；❌ 60 s 常量无引用（P2-1） |
| 9 | CRC 参数（poly 0x1021/init 0/reflected）与 golden vectors | `crclib.c:382-404` + 复算：`""→0x0000` ✅、`"hkt"→0x77D1` ✅、`"abc"→0x58E9`（文档 0x29B1 ❌，0x29B1 为 CCITT-FALSE("123456789")） | ⚠️ 2/3 向量通过，1 条错误（P1-2） |
| 10 | Bootloader 启动读更新标志、中断后驻留升级模式；`AppProgramRun` 仅校验 SP | 未逐行抽查（信任追溯文档 §7，与 §9.5.1 一致性核对通过） | 待核实（M3 逐设备核实项，文档已如实登记风险） |

## 修正后需同步修订的文档清单

1. `docs/ios/architecture/2026-09-03-ios-v1-architecture-outline.md`：§5.1 CRC 向量、§8 要点 2 包号、§8.1 超时预算与睡眠行、§7.1 行 3/行 6。
2. `shared/protocol/bootloader-ota.md`：包号起点、CRC 覆盖范围。
3. `shared/fixtures/crc16.json`：修正/替换 `abc` 条目，增加 `"123456789"→2189`。
4. `docs/ios/architecture/2026-09-04-ios-v1-stability-continuity-test-cases.md`：TC-BC-003 预期重写、TC-ST-004 条件化、TC-ST-005 双分支化、追溯前缀。
5. `docs/ios/2026-09-03-ios-v1-rd-requirements.md` §18 S-1：措辞降级为"待逐设备核实的候选约束"。
6. `shared/devices/firmware-traceability.md` §7：Sleep 条目同步更正，登记 MAX_WAIT_SLEEP_TIMEOUT 无引用事实。
