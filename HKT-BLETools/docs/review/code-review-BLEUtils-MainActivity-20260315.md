# BLETools 代码评审报告

**评审目标**: `BLEUtils.kt`, `MainActivity.kt`, `build.gradle.kts`
**评审日期**: 2026-03-15
**提交**: `ceccc73` - feat(BLETools): fix bluetooth scanning logic and migrate to Nordic library

---

## 一、评审摘要

本次评审针对最近一次提交的三个核心代码文件进行分析，发现以下问题：

| 严重程度 | 数量 |
|----------|------|
| 🔴 高 | 3 |
| 🟡 中 | 2 |
| 🟢 低 | 2 |

---

## 二、高优先级问题

### 2.1 重复初始化视图组件 (MainActivity.kt)

**位置**: 第469-471行

```kotlin
// 第129-132行 - 类级别已声明
private lateinit var btImage: ImageView
private lateinit var btTip: TextView
private lateinit var rvDeviceList: RecyclerView

// 第469-471行 - scan() 方法内又重复 findViewById
private fun scan() {
    val btImage = findViewById<ImageView>(R.id.bt_image)  // ❌ 重复声明
    val btTip = findViewById<TextView>(R.id.bt_tip)      // ❌ 重复声明
    val rvDeviceList = findViewById<RecyclerView>(R.id.rv_device_list)  // ❌ 重复声明
```

**影响**: 
- 创建了新的局部变量，遮蔽了类成员变量
- 资源浪费
- 代码冗余

**建议修复**:
```kotlin
private fun scan() {
    // 直接使用类成员变量
    btTip.visibility = View.GONE
    btImage.visibility = View.VISIBLE
    rvDeviceList.visibility = View.GONE
    // ...
}
```

---

### 2.2 WakeLock 可能永久持有 (MainActivity.kt)

**位置**: 第54-72行

```kotlin
class WakeLockManager(context: Context) {
    private val powerManager: PowerManager = context.getSystemService(Context.POWER_SERVICE) as PowerManager
    private var wakeLock: PowerManager.WakeLock? = null

    @RequiresPermission(android.Manifest.permission.WAKE_LOCK)
    fun acquireWakeLock(tag: String) {
        if (wakeLock == null || !wakeLock!!.isHeld) {
            wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, tag)
            wakeLock?.acquire(10*60*1000L)  // ⚠️ 可能返回false但未检查
        }
    }
    
    // ❌ 缺少 onDestroy 释放逻辑
}
```

**影响**:
- WakeLock 未在 `onDestroy` 中释放，可能导致电池消耗
- `acquire()` 可能失败但未处理返回值

**建议修复**:
```kotlin
override fun onDestroy() {
    super.onDestroy()
    wakeLockManager.releaseWakeLock()
}
```

---

### 2.3 BaseApp 静态 Context 泄漏风险 (MainActivity.kt)

**位置**: 第94-109行

```kotlin
class BaseApp : Application() {
    companion object {
        @SuppressLint("StaticFieldLeak")
        lateinit var context: Context  // ⚠️ 内存泄漏风险
    }
    
    override fun onCreate() {
        super.onCreate()
        context = applicationContext
    }
}
```

**影响**:
- 静态持有 Application Context 理论上可行，但容易被滥用
- 如果后续代码错误地传入 Activity Context，会导致内存泄漏

**建议**: 保持现状但添加注释说明仅用于 Application 级别

---

## 三、中等优先级问题

### 3.1 过滤逻辑变量名不一致

**位置**: MainActivity.kt 第190行 vs 第531行

```kotlin
// 第190行 - scanCallback 中
val name = result.device.name ?: "N/A"

// 第531行 - addDeviceList 中检查原始 device.name
if (MainActivity.shouldFilterDevice(isScanNullNameDevice, bleDevice.device.name))
```

**问题**: 逻辑正确但容易混淆，变量 `name` 被赋值为 "N/A" 但实际检查的是 `bleDevice.device.name`

**建议**: 统一使用 `bleDevice.name` 或增加注释

---

### 3.2 未使用的变量

**位置**: MainActivity.kt 第148行

```kotlin
private var connectTime = 0  // 在 startConnectionCheck 中递增但未重置
```

**问题**: `connectTime` 在每次连接时递增，但从未重置为0，可能导致计数不准确

**建议**: 在开始连接前重置
```kotlin
connectTime = 0
startConnectionCheck(device)
```

---

## 四、低优先级问题

### 4.1 MyActivity 类未使用

**位置**: MainActivity.kt 第74-91行

```kotlin
class MyActivity : AppCompatActivity() {
    // ... 代码完整但未被使用
}
```

**建议**: 删除或改为正确的基类

---

### 4.2 代码注释与实际不符

**位置**: MainActivity.kt 第90行

```kotlin
// ... 其他代码 ...  // ❌ 注释未更新
```

---

## 五、代码变更对比

### 修改前 (原问题)
```kotlin
// 错误的过滤逻辑
fun shouldFilterDevice(isScanNullNameDevice: Boolean, deviceName: String?): Boolean {
    return isScanNullNameDevice || deviceName == null  // ❌ 逻辑错误
}
```

### 修改后 (当前版本)
```kotlin
// 修复后的过滤逻辑
fun shouldFilterDevice(isScanNullNameDevice: Boolean, deviceName: String?): Boolean {
    return isScanNullNameDevice && deviceName == null  // ✅ 正确
}
```

---

## 六、依赖变更

| 依赖 | 版本 | 说明 |
|------|------|------|
| Nordic Scanner | 1.5.0 | 新增 - 替代原生 BluetoothLeScanner |
| ZXing | 4.3.0/3.4.1 | 保持 |
| Apache POI | 5.2.3 | 保持 |

---

## 七、建议修复优先级

| 优先级 | 问题 | 修复工作量 |
|--------|------|------------|
| 🔴 高 | 删除重复 findViewById | 2min |
| 🔴 高 | WakeLock onDestroy 释放 | 3min |
| 🟡 中 | 统一过滤变量名 | 5min |
| 🟡 中 | 重置 connectTime | 2min |
| 🟢 低 | 删除未使用的 MyActivity | 1min |

---

## 八、总结

本次提交的代码整体质量较好，主要完成了：
1. ✅ Nordic 库迁移
2. ✅ 过滤逻辑修复
3. ✅ 权限适配 Android 12+

建议优先修复高优先级问题，特别是视图组件重复初始化和 WakeLock 泄漏风险。

---

**评审人**: AI Code Review
**下一步**: 确认是否需要修复上述问题
