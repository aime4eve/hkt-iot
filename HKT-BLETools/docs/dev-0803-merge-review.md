# dev_0803 合并方案评审报告

> 评审对象：`docs/dev-0803-merge-plan.md`
> 评审日期：2026-08-05
> 评审方法：逐条核实文档的事实声明，对照 `git diff main origin/dev_0803` 原始数据

---

## 一、总体结论

方案整体扎实、事实核查到位，但**不能直接落地**：有一处会导致编译失败的实质性遗漏（`Communicate.kt:366`），文档断言"Communicate 无需任何修改"不成立。

修正成本极低：在原文档基础上增加一行 `!!` 即可。

---

## 二、Git 现状核实（全部通过）

| 文档声明 | 核实结果 |
|---------|---------|
| 共同祖先 `1489788` | ✓ `148978846f01ea4c3f759409bccf18ac8decb6ef` |
| main = `1489788`（无后续提交） | ✓ `git log main` 仅 1 条提交 |
| dev_0803 = `0d57bf3` | ✓ `0d57bf39fd39560eed2de044478552f537fe5037` |
| fast-forward 场景，无内容冲突 | ✓ merge-base = main |
| 42 个文件差异 | ✓ `git diff --name-status` 确认 42 个 |

---

## 三、🔴 P0：遗漏的编译冲突（Communicate.kt:366）

### 问题

文档第 5 节声称冲突 1"唯一冲突点"是 DeviceActivity.kt:951，并在第 6 节断言 **"Communicate 无需任何修改"**。

但 dev_0803 的 Communicate.kt 第 366 行存在完全相同性质的冲突：

```kotlin
// Communicate.kt:366（dev_0803）— 顶层函数 streamRev 的第一行
fun streamRev(content: String) {
    val gatt: BluetoothGatt = MainActivity.getGatt()   // ← Type mismatch
```

### 根因

main 的 `MainActivity.kt:186` 返回可空类型：

```kotlin
// main: MainActivity.kt:185-186
private var gatt: BluetoothGatt? = null
fun getGatt(): BluetoothGatt? {
```

dev_0803 的 `MainActivity.kt:157` 返回非空类型：

```kotlin
// dev_0803: MainActivity.kt:156-157
private lateinit var gatt: BluetoothGatt
fun getGatt(): BluetoothGatt {
```

把 main 可空返回值赋给非空变量 → Kotlin 编译器必然报 `Type mismatch: inferred type is BluetoothGatt? but BluetoothGatt was expected`。

### diff 证据（Communicate.kt）

```diff
@@ -363,10 +363,10 @@
 fun streamRev(content: String) {
-    val gatt: BluetoothGatt? = MainActivity.getGatt()
+    val gatt: BluetoothGatt = MainActivity.getGatt()
```

### 后果

文档第 6 节的命令清单只改了 DeviceActivity.kt:951 和 strings.xml，执行后编译会在 Communicate.kt:366 失败。"2 处行级修补即可"的结论不成立，实际需要 **3 处**。

`streamRev` 即便当前无人调用，作为顶层函数也会被编译器类型检查，错误是确定的。

### 修正

Communicate.kt:366 末尾同样加 `!!`：

```kotlin
val gatt: BluetoothGatt = MainActivity.getGatt()!!
```

---

## 四、已核实通过的文档声明

### 4.1 冲突 1：DeviceActivity.kt:951（文档已捕获）

```kotlin
// dev_0803 DeviceActivity.kt:951
private var gatt: BluetoothGatt = MainActivity.getGatt()
private val stream = StreamThread(gatt)
```

main 的 `getGatt()` 返回 `BluetoothGatt?` → 赋给非空 `BluetoothGatt` 必然 `Type mismatch`。✓ 文档描述准确。

### 4.2 冲突 2：ble_disconnect_message 字符串缺失（文档已捕获）

```
# main strings.xml 中 grep ble_disconnect_message → exit 1（未定义）
# dev_0803 DeviceActivity.kt:966 引用：
Toast.makeText(this@DeviceActivity, getString(R.string.ble_disconnect_message), Toast.LENGTH_LONG).show()
```

✓ 文档描述准确。

### 4.3 BLEUtils 不影响本次合并（文档已捕获）

```
# grep BLEUtils in dev_0803 Communicate.kt → exit 1（无引用）
# grep BLEUtils in dev_0803 DeviceActivity.kt → exit 1（无引用）
```

✓ BLEUtils 虽然做了去可空改动，但两个合并文件都不直接调用它，签名变化不影响。

### 4.4 文件分类（42 个文件）

`git diff --name-status main origin/dev_0803` 确认 42 个文件，分类大体正确。

### 4.5 排除项决策合理

排除 `key.jks`、APK、`.idea/`、`.gitignore` 改动 — 安全策略正确。✓

---

## 五、完整 diff 中其余外部依赖排查（全部安全）

拿着完整 4 段 diff，对 dev_0803 新代码引用的所有外部符号逐一排查：

| 符号 | 引用位置 | 定义位置 | main 是否已有 | 结论 |
|------|---------|---------|:----------:|------|
| `FileUtil.getFileAbsolutePath` 等 | DeviceActivity:1316-1319 | `File.kt:22` (`object FileUtil`) | ✓ 已存在 | 安全 |
| `isExternalPowerUpdate` | DeviceActivity:1383 | DeviceActivity.kt:59（本文件内） | 自洽 | 安全 |
| `isPowerUpdate` | DeviceActivity:1384 | DeviceActivity.kt:62（本文件内） | 自洽 | 安全 |
| `isRefreshPaused` | DeviceActivity 多处 | DeviceActivity.kt:61（本文件内） | 自洽 | 安全 |
| `connectState` | Communicate.kt:203, DeviceActivity 断连检测 | `BLEUtils.kt:13` (`public var connectState`) | ✓ 已存在 | 安全 |
| `HighlightSpinnerAdapter` | main 版本使用 | dev_0803 已彻底移除 | — | 无残留引用，安全 |
| `DeviceFeatureConfig` | main 版本使用 | dev_0803 已彻底移除 | — | 无残留引用，安全 |

**结论：除 Communicate.kt:366 外，没有其他隐藏的编译炸弹。**

---

## 六、🟡 次要问题

### 6.1 分类计数笔误

文档第 3 节标称"不应入库 20 个"，实际是 19 个：

- `.gitignore` 1 + `key.jks` 1 + 3 个 APK + `output-metadata.json` 1 + `.idea/` 13 = 19

总数 42 正确，只是分类标签写错。

### 6.2 streamRev 的 `!!` 空安全论证缺失

文档给 DeviceActivity 的 `!!` 补了语义论证（进入 Activity 前已 setGatt），但对 `streamRev` 这个顶层函数没有论证——若在断连后被回调，`getGatt()` 返回 null 会 NPE。

因第 2 条 v2.2 修复（断连清理）会间接兜底，属提醒级，不阻塞。

---

## 七、修正后的落地方案

在原文档第 6 节基础上，执行命令清单第 2 步增加一处：

```
原方案 3 处修改：
  1. DeviceActivity.kt:951 末尾加 !!
  2. values/strings.xml 新增 ble_disconnect_message
  3. git commit + push

修正后 4 处修改：
  1. DeviceActivity.kt:951 末尾加 !!       ← 不变
  2. Communicate.kt:366 末尾加 !!          ← 新增
  3. values/strings.xml 新增 ble_disconnect_message  ← 不变
  4. git commit + push                     ← 不变
```

推送后按 AGENTS.md 要求本地跑一次 Android 编译验证（文档风险表已标注"本机无 Android SDK"，意识正确）。

---

## 八、验证命令清单（可复现）

```bash
# 1. 确认 getGatt() 返回类型（main vs dev_0803）
git show main:app/src/main/java/com/hkt/ble/bletools/MainActivity.kt | rg -n "fun getGatt|var gatt"
git show origin/dev_0803:app/src/main/java/com/hkt/ble/bletools/MainActivity.kt | rg -n "fun getGatt|var gatt"

# 2. 确认两处 BluetoothGatt 非空赋值
git show origin/dev_0803:app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt | sed -n '948,955p'
git show origin/dev_0803:app/src/main/java/com/hkt/ble/bletools/Communicate.kt | sed -n '363,368p'

# 3. 确认 ble_disconnect_message 缺失
rg "ble_disconnect_message" app/src/main/res/values/strings.xml   # 应无输出

# 4. 确认 FileUtil 定义位置
git ls-tree main --name-only -r | rg -i "File.kt"

# 5. 确认 connectState 定义位置
git grep -n "var connectState" main

# 6. 确认 BLEUtils 不被合并文件调用
git show origin/dev_0803:app/src/main/java/com/hkt/ble/bletools/Communicate.kt | rg "BLEUtils"   # 应无输出
git show origin/dev_0803:app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt | rg "BLEUtils"  # 应无输出

# 7. 查看完整 diff
git diff main origin/dev_0803 -- app/src/main/java/com/hkt/ble/bletools/Communicate.kt
git diff main origin/dev_0803 -- app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt
```

---

*本报告所有结论基于 2026-08-05 的 `git diff main origin/dev_0803` 静态分析。*
