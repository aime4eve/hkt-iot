package com.hkt.ble.bletools

import android.os.Bundle
import androidx.activity.ComponentActivity
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
import com.hkt.ble.bletools.demo.DemoResponder
import com.hkt.ble.bletools.model.ScanModel
import com.hkt.ble.bletools.ui.ScanListScreen
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
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val demo = intent.getBooleanExtra("demo", false)
        val demoConnect = intent.getBooleanExtra("demoConnect", false)
        val scope = lifecycleScope
        val model: ScanModel = if (demo) {
            val mock = MockCentral(scope)
            // 脚本设备（四家族；EPS100 -91 低于 -80 阈值——入列规则过滤演示同源 iOS -mockble）
            mock.discover(DiscoveredDevice("SVC100_0D137C", "F8:1D:78:0D:13:7C", -55))
            mock.discover(DiscoveredDevice("UDS100_3F2A", "F8:1D:78:3F:2A:B7", -62))
            mock.discover(DiscoveredDevice("MPS100_9C01", "F8:1D:78:9C:01:D4", -75))
            mock.discover(DiscoveredDevice("EPS100_0288CF", "F8:1D:78:02:88:CF", -91))

            val responder = DemoResponder()
            val connectedFamily = arrayOfNulls<DeviceFamily>(1)
            val m = ScanModel(
                port = mock,
                scope = scope,
                autoStartOnReady = true,
                demoAutoConnectPrefix = if (demoConnect) "SVC100" else null,
                onSessionStarted = { session -> connectedFamily[0] = session.family },
            )
            mock.responder = { frame ->
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
            m
        } else {
            ScanModel(port = SystemCentral(applicationContext), scope = scope)
        }

        setContent {
            CompositionLocalProvider(LocalHktColors provides hktColors()) {
                ScanListScreen(model = model)
            }
        }
    }
}
