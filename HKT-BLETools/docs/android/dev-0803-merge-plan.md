# dev_0803 分支合入 main 方案（评审稿）

> **状态**：待评审　|　**日期**：2026-08-05　|　**仓库**：`aime4eve/smart-bletools`
> **相关分支**：`main`、`origin/dev_0803`

---

## 1. 背景与目标

将协作者 zlbb112 提交的 `dev_0803` 分支（v2.2 更新）合入 `main` 分支。经核实，本次合并**业务范围仅涉及两个 Kotlin 文件**：`Communicate.kt` 与 `DeviceActivity.kt`。其余改动（MainActivity 大重写、BLEUtils、IDE 配置、APK、签名密钥等）**不纳入**本次合并。

**核心约束**："以 main 为主"——即以 main 现有的安全策略（`.gitignore`）、文件结构为基准，仅吸收上述两个文件的有效改动。

---

## 2. 仓库与分支现状

| 项 | 值 |
|------|------|
| 共同祖先 | `1489788` Initial commit |
| `main` | `1489788`（初始提交，无后续独立改动） |
| `dev_0803` | `0d57bf3` update 2.2 → `1489788` |
| `dev_0803` 提交者 | zlbb112 `<zlb_manage@163.com>`，2026-08-04 11:06 (+0800) |
| 历史关系 | **标准 fast-forward 场景**，无内容冲突 |

> 因 `main` 在共同祖先之后无独立提交，"以 main 为主"在「冲突解决」层面无意义（`-X ours` 无冲突可偏向）；其真实含义应落实为「以 main 的安全/文件策略为准」，见第 6 节。

---

## 3. dev_0803 相对 main 的完整差异（42 个文件）

| 类别 | 文件数 | 说明 |
|------|:---:|------|
| Kotlin 源码 | 6 | MainActivity(重写)、DeviceActivity、BLEUtils、Communicate、DebugActivity、QrCodeActivity.kt.kt(新增) |
| 布局/资源/清单 | 11 | dialog_scan_filter.xml 清空，其余小幅调整 |
| 构建配置 | 5 | build.gradle.kts / proguard 大幅精简 |
| 文档 | 1 | 更新记录.txt(新增) |
| ⚠️ 不应入库 | 20 | `key.jks`、3 个 APK、`output-metadata.json`、`.idea/`(13)、`.gitignore`(被改) |

**v2.2 更新记录（5 条）与代码对照结论**：5 条声明全部在代码中找到对应实现且属实；其中第 4 条（DiffUtil 生效）有明确根因修复（拷贝 list 避免同引用），第 5 条（power 误触发）新增 `isExternalPowerUpdate` 标志位。详见对话记录。

---

## 4. 合并范围决策

**纳入合并（2 个文件）**：
- `app/src/main/java/com/hkt/ble/bletools/Communicate.kt`
- `app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt`

**这两个文件承载的 v2.2 改动**：
- 第 2 条：BLE 断连/返回闪退修复（新增 `disconnectHandler` 轮询、断连 Toast、`resetEventFromDevice()`、`cleanUp()`）
- 第 5 条：power 开关误触发修复（新增 `isExternalPowerUpdate` 标志位）
- 配套：`BluetoothGatt?` → `BluetoothGatt` 去可空重构（两文件内部一致）

**排除项**：MainActivity、BLEUtils、DebugActivity、QrCodeActivity.kt.kt、全部布局/资源/配置改动、`.idea/`、`key.jks`、APK、`output-metadata.json`、`.gitignore` 改动、更新记录.txt。

---

## 5. ⚠️ 依赖冲突分析（关键）

仅合并这两个文件会触发 **2 处确定的编译错误**，源于它们与「不合并文件」的硬绑定：

### 冲突 1：`getGatt()` 返回类型不一致

| | `MainActivity.getGatt()` | `DeviceActivity` 第 951 行 |
|---|---|---|
| main | `BluetoothGatt?`（companion 内 `var gatt: BluetoothGatt? = null`） | — |
| dev_0803 | `BluetoothGatt`（非空） | `private var gatt: BluetoothGatt = MainActivity.getGatt()` |

→ 用 main 的可空返回值给非空变量赋值：**`Type mismatch` 编译错误**。

### 冲突 2：字符串资源缺失

- `DeviceActivity`（dev_0803）引用 `R.string.ble_disconnect_message`（断连 Toast 文案）
- main 的 `strings.xml` **未定义**该字符串 → **`Unresolved reference` 编译错误**

### 已排除的伪冲突

`BLEUtils.kt` 虽然也做了去可空改动，但 grep 确认 **`Communicate.kt` 与 `DeviceActivity.kt` 均不直接调用 `BLEUtils.*`**，故 BLEUtils 签名变化**不影响**本次合并。

### 适配成本评估

`DeviceActivity` 中 `gatt` **仅 2 处**用法（均为声明/传参，无 `gatt.xxx()` 业务调用）：

```kotlin
951:    private var gatt: BluetoothGatt = MainActivity.getGatt()   // 唯一冲突点
952:    private val stream = StreamThread(gatt)                    // 传给 Communicate.StreamThread
```

→ 适配成本极低，2 处行级修补即可解决，无需扩大合并范围。

---

## 6. 合并方案（推荐）

`Communicate.kt` 与 `DeviceActivity.kt` 内部均使用非空 `BluetoothGatt`，**两者互相自洽**；唯一边界冲突是从 main 可空 `getGatt()` 取值。方案如下：

| # | 操作 | 文件 | 内容 |
|---|------|------|------|
| 1 | 取文件 | `Communicate.kt` / `DeviceActivity.kt` | 从 `origin/dev_0803` checkout，**原样合并**（Communicate 无需任何修改） |
| 2 | 修补类型 | `DeviceActivity.kt` 第 951 行 | 末尾加 `!!`：`MainActivity.getGatt()!!`（进入 DeviceActivity 前设备已连接，语义上必非空） |
| 3 | 补字符串 | `values/strings.xml` | 新增 `<string name="ble_disconnect_message">设备已断开连接</string>` |
| 4 | 核对 | `git status` | 确认暂存区**仅**这 3 个文件，无 `key.jks` / APK / `.idea` |
| 5 | 提交推送 | — | commit + `git push origin main` |

**最终效果**：main 上仅多出 3 个文件的改动；MainActivity / BLEUtils / `.idea` / `key.jks` / APK / `.gitignore` 安全规则一概不动。

### 执行命令清单

```bash
# 1) 从 dev_0803 取出两个文件（覆盖 main 版本并自动暂存）
git checkout origin/dev_0803 -- \
  app/src/main/java/com/hkt/ble/bletools/Communicate.kt \
  app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt

# 2) 用编辑器精确修改：
#    - DeviceActivity.kt 第 951 行末尾追加 "!!"
#    - values/strings.xml 追加 <string name="ble_disconnect_message">设备已断开连接</string>

# 3) 核对暂存内容
git status

# 4) 提交并推送
git commit -m "Merge dev_0803: apply Communicate.kt & DeviceActivity.kt (v2.2 fixes)"
git push origin main
```

---

## 7. 风险与验证

| 风险 | 说明 | 缓解 |
|------|------|------|
| 🔴 未本地编译验证 | 本机无 Android SDK，无法编译确认 | **推送后务必在 Android Studio 跑一次编译**；重点看 `DeviceActivity` 第 951 行与 `ble_disconnect_message` 引用 |
| 🟡 `!!` 空安全 | 若 `getGatt()` 返回 null 会 NPE | 进入 DeviceActivity 前必然已 `setGatt`，语义上非空；若评审认为风险偏高，可改为 `?: finish(); return` 兜底 |
| 🟢 未合并的实质性改动 | 扫描库切换（Nordic→系统原生）、QrCodeActivity 双后缀、MainActivity 重写 | 本次不处理，记录在案，后续按需另行评估 |

---

## 8. 待评审决策点

请重点确认以下事项：

1. **合并范围**：是否确认仅合并 `Communicate.kt` + `DeviceActivity.kt` 两个文件？
2. **类型适配方式**：第 951 行用 `!!`（推荐），还是用 `?: finish(); return` 等更防御性写法？
3. **字符串文案**：`ble_disconnect_message` 用中文「设备已断开连接」，还是英文？（v2.2 文档提及"默认提示词全为英文"，可统一为英文）
4. **提交信息**：是否沿用 `Merge dev_0803: apply Communicate.kt & DeviceActivity.kt (v2.2 fixes)`？
5. **未合并项**：MainActivity 重写、BLEUtils 去可空、QrCodeActivity、扫描库切换等，是否需要排期另行处理？

---

*本方案基于 2026-08-05 的代码静态分析，所有差异数据来自 `git diff main origin/dev_0803`。*
