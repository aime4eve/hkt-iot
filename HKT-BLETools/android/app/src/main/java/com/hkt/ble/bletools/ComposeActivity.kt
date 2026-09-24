package com.hkt.ble.bletools

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.activity.compose.setContent
import androidx.compose.runtime.CompositionLocalProvider
import androidx.lifecycle.lifecycleScope
import com.hkt.ble.bletools.ble.SystemCentral
import com.hkt.ble.bletools.core.ble.DiscoveredDevice
import com.hkt.ble.bletools.core.ble.MockCentral
import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.DeviceFamily
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.hktColors
import com.hkt.ble.bletools.demo.DemoBootloader
import com.hkt.ble.bletools.demo.DemoResponder
import com.hkt.ble.bletools.model.LanguageMode
import com.hkt.ble.bletools.model.LanguageStore
import com.hkt.ble.bletools.model.ScanModel
import com.hkt.ble.bletools.ui.ScanListScreen
import androidx.core.os.LocaleListCompat
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

/**
 * v2 Compose 宿主（M1 架构 §6）：单 Activity Compose 化的入口样板。
 * M5 阶段与旧 XML UI 并存（manifest 不设 launcher，adb 启动开发/截图用）；
 * M6 逐页替换完成后接替 MainActivity 成为唯一入口。
 *
 * demo=true：MockCentral 脚本设备 + 就绪自动扫描（-mockble 同构；EPS100 -91 被阈值过滤的演示断言同源）。
 * demo=true 时 DemoResponder 按**当前连接设备的 family** 应答（iOS 单实例 + family 传参同构）：
 * 轮询回夹具帧、0x02 回写夹具、0xFD 后 3s 注入 "Calibration Done" 文本（App 层钩子同构）。
 * demoConnect=true：扫描命中 SVC100 前缀即停扫直连（-demo-flow 同构），可演示驻留卡/轮询数据。
 */
class ComposeActivity : ComponentActivity() {

    // 运行时权限（真机必需）：旧 MainActivity 的请求逻辑未随 UI 迁移，覆盖安装沿用旧授权
    // 的设备无感，新装设备若不请求则扫描/连接静默失败
    private val permissionLauncher = registerForActivityResult(
        androidx.activity.result.contract.ActivityResultContracts.RequestMultiplePermissions(),
    ) { }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestBlePermissionsIfNeeded()
        val demo = intent.getBooleanExtra("demo", false)
        val demoConnect = intent.getBooleanExtra("demoConnect", false)
        // 演示深链（iOS -demo-page locate-finding 同构）：直入定位流并自动 startLocate
        // （16 位 hex，后 6 位 0D137C = 演示设备 SVC100_0D137C）
        val demoLocateEUI = if (intent.getBooleanExtra("demoLocate", false)) "0095690A000D137C" else null
        val scope = lifecycleScope
        // P-07 语言三态持久化：自管 SharedPreferences（ComponentActivity 不走 appcompat 自动存储）
        val prefs = getSharedPreferences("settings", MODE_PRIVATE)
        LanguageStore.restore(
            when (prefs.getString("language", "system")) {
                "zh" -> LanguageMode.ZH
                "en" -> LanguageMode.EN
                else -> LanguageMode.SYSTEM
            },
        )
        val onLocaleChange: (LanguageMode) -> Unit = { mode ->
            prefs.edit().putString("language", mode.name.lowercase()).apply()
            val locales = when (mode) {
                LanguageMode.ZH -> LocaleListCompat.forLanguageTags("zh-CN")
                LanguageMode.EN -> LocaleListCompat.forLanguageTags("en")
                LanguageMode.SYSTEM -> LocaleListCompat.getEmptyLocaleList()
            }
            AppCompatDelegate.setApplicationLocales(locales)
            recreate()   // ComponentActivity 不随 locale 自动重建，手动触发
        }
        val model: ScanModel = if (demo) {
            val mock = MockCentral(scope)
            // 脚本设备（四家族；EPS100 -91 低于 -80 阈值——入列规则过滤演示同源 iOS -mockble）
            mock.discover(DiscoveredDevice("SVC100_0D137C", "F8:1D:78:0D:13:7C", -55))
            mock.discover(DiscoveredDevice("UDS100_3F2A", "F8:1D:78:3F:2A:B7", -62))
            mock.discover(DiscoveredDevice("MPS100_9C01", "F8:1D:78:9C:01:D4", -75))
            mock.discover(DiscoveredDevice("EPS100_0288CF", "F8:1D:78:02:88:CF", -91))

            val responder = DemoResponder()
            val bootloader = DemoBootloader()
            val connectedFamily = arrayOfNulls<DeviceFamily>(1)
            val m = ScanModel(
                port = mock,
                scope = scope,
                autoStartOnReady = true,
                demoAutoConnectPrefix = if (demoConnect) "SVC100" else null,
                onSessionStarted = { session -> connectedFamily[0] = session.family },
            )
            mock.responder = { frame ->
                // 引导层帧（OTA 传输）：异步注入 ACK——同步回包会让 1284 包深递归爆栈，
                // 且小延迟让传输进度可见（模拟器走真实 OtaEngine 代码路径）
                val bootAck = bootloader.ackFor(frame)
                if (bootAck != null) {
                    scope.launch {
                        delay(8)
                        mock.inject(bootAck)
                    }
                    null
                } else {
                    val response = responder.respond(frame, connectedFamily[0])
                    // 校准完成文本延迟注入（iOS App 层钩子同构：0xFD ACK 后 3s 纯 ASCII 上报）
                    if (response != null && frame.size > 6 &&
                        frame[6].toInt() and 0xFF == CommandCode.CALIBRATE
                    ) {
                        scope.launch {
                            delay(3_000)
                            mock.inject("Calibration Done".toByteArray(Charsets.US_ASCII))
                        }
                    }
                    response
                }
            }
            m
        } else {
            ScanModel(port = SystemCentral(applicationContext), scope = scope)
        }

        setContent {
            CompositionLocalProvider(LocalHktColors provides hktColors()) {
                ScanListScreen(model = model, onLocaleChange = onLocaleChange, demoLocateEUI = demoLocateEUI)
            }
        }
    }

    /** Android 12+ 需 BLUETOOTH_CONNECT/SCAN 运行时授权；11 及以下需定位。 */
    private fun requestBlePermissionsIfNeeded() {
        val needed = buildList {
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.S) {
                add(android.Manifest.permission.BLUETOOTH_CONNECT)
                add(android.Manifest.permission.BLUETOOTH_SCAN)
            } else {
                add(android.Manifest.permission.ACCESS_FINE_LOCATION)
            }
        }.filter {
            androidx.core.content.ContextCompat.checkSelfPermission(this, it) !=
                android.content.pm.PackageManager.PERMISSION_GRANTED
        }
        if (needed.isNotEmpty()) {
            permissionLauncher.launch(needed.toTypedArray())
        }
    }
}
