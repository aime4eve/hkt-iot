package com.hkt.ble.bletools.ui

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.hkt.ble.bletools.BuildConfig
import com.hkt.ble.bletools.designsystem.HktCard
import com.hkt.ble.bletools.designsystem.HktRadius
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.LogLine
import com.hkt.ble.bletools.designsystem.NavbarHeader
import com.hkt.ble.bletools.designsystem.PrefixChip
import com.hkt.ble.bletools.designsystem.RowValue
import com.hkt.ble.bletools.designsystem.SectionHeader
import com.hkt.ble.bletools.designsystem.SettingsRow
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.LanguageMode
import com.hkt.ble.bletools.model.LanguageStore
import com.hkt.ble.bletools.model.LogStore
import com.hkt.ble.bletools.model.ScanModel

/**
 * P-07 设置页 —— 1:1 克隆冻结原型（规格卡 docs/ios/design/ui-spec/P-07_settings_log.md）。
 * 语言（三态循环）/ 诊断日志入口 / 扫描过滤（阈值滑杆 + 前缀芯片）/ 关于（版本 7 击调试 + 隐私入口）。
 * 用户 2026-09-10 裁决：二级页不显示「⌂ 首页」。子页（日志/隐私）内嵌本文件。
 */
@Composable
fun SettingsScreen(
    model: ScanModel,
    onBack: () -> Unit,
    onLocaleChange: (LanguageMode) -> Unit,
) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    var showLog by remember { mutableStateOf(false) }
    var showPrivacy by remember { mutableStateOf(false) }
    var debugTaps by remember { mutableIntStateOf(0) }
    val allowedPrefixes by model.allowedPrefixesFlow.collectAsState()
    val rssiThreshold by model.rssiThresholdFlow.collectAsState()

    if (showLog) {
        BackHandler { showLog = false }
        LogScreen(onBack = { showLog = false })
        return
    }
    if (showPrivacy) {
        BackHandler { showPrivacy = false }
        PrivacyScreen(onBack = { showPrivacy = false })
        return
    }
    BackHandler { onBack() }

    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(title = if (zh) "设置" else "Settings", backText = if (zh) "‹ 返回" else "‹ Back", onBack = onBack)
        Column(
            Modifier
                .weight(1f)
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 16.dp)
                .padding(bottom = 24.dp),
        ) {
            // 语言
            SectionHeader(if (zh) "语言" else "Language")
            SettingsRow(
                label = if (zh) "语言" else "Language",
                value = {
                    RowValue {
                        Text(LanguageStore.label, style = hkt(14f), color = c.text2)
                        Text("▸", style = hkt(14f), color = c.text2)
                    }
                },
                action = { onLocaleChange(LanguageStore.cycle()) },
            )

            // 诊断日志
            SectionHeader(if (zh) "诊断日志" else "Diagnostic Log")
            SettingsRow(
                label = if (zh) "诊断日志" else "Diagnostic Log",
                value = {
                    RowValue { Text(if (zh) "查看 ▸" else "View ▸", style = hkt(14f), color = c.text2) }
                },
                action = {
                    android.util.Log.d("BLEProbe", "log row tapped")
                    showLog = true
                },
            )

            // 扫描过滤
            SectionHeader(if (zh) "扫描过滤" else "Scan filter")
            SettingsRow(
                label = if (zh) "信号强度阈值" else "Signal threshold",
                value = {
                    RowValue {
                        Slider(
                            value = rssiThreshold.toFloat(),
                            onValueChange = { model.rssiThreshold = it.toInt() },
                            valueRange = -95f..-40f,
                            steps = 10,   // 5dBm 步进
                            modifier = Modifier.width(110.dp),
                        )
                        Text("$rssiThreshold dBm", style = hkt(14f), color = c.text2)
                    }
                },
            )
            HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 9.dp)) {
                Column {
                    Text(
                        if (zh) "设备名称前缀过滤" else "Name prefix filter",
                        style = hkt(12f),
                        color = c.text2,
                    )
                    Row(horizontalArrangement = Arrangement.spacedBy(6.dp), modifier = Modifier.padding(top = 8.dp)) {
                        listOf("MPS", "SVC", "UDS", "EPS").forEach { prefix ->
                            PrefixChip(
                                text = prefix,
                                selected = prefix in allowedPrefixes,
                                modifier = Modifier.weight(1f),
                                action = { model.togglePrefix(prefix) },
                            )
                        }
                    }
                    Text(
                        if (zh) "仅显示名称以所选前缀开头的设备；未命名设备自动排除"
                        else "Only devices whose name starts with a selected prefix; unnamed devices are excluded",
                        style = hkt(11f),
                        color = c.text2,
                        modifier = Modifier.padding(top = 6.dp),
                    )
                }
            }

            // 关于
            SectionHeader(if (zh) "关于" else "About")
            SettingsRow(
                label = if (zh) "版本" else "Version",
                value = { RowValue { Text(BuildConfig.VERSION_NAME, style = hkt(14f), color = c.text2) } },
                action = { debugTaps += 1 },
            )
            SettingsRow(
                label = if (zh) "隐私说明" else "Privacy",
                value = { RowValue { Text("▸", style = hkt(14f), color = c.text2) } },
                action = { showPrivacy = true },
            )
            if (debugTaps > 0 && debugTaps < 7) {
                Text(
                    if (zh) "再点 ${7 - debugTaps} 次开启调试模式" else "$debugTaps more taps to enable Debug",
                    style = hkt(13f),
                    color = c.text2,
                    modifier = Modifier.fillMaxWidth().padding(top = 6.dp),
                    textAlign = TextAlign.Center,
                )
            }
            if (debugTaps >= 7) {
                Text(
                    if (zh) "已开启调试模式" else "Debug mode enabled",
                    style = hkt(13f),
                    color = c.ok,
                    modifier = Modifier.fillMaxWidth().padding(top = 6.dp),
                    textAlign = TextAlign.Center,
                )
            }
        }
    }
}

/** P-07b 诊断日志页 —— navbar.small（导出）+ 日志行列表 / 空态。 */
@Composable
fun LogScreen(onBack: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    val entries = LogStore.entries
    val context = androidx.compose.ui.platform.LocalContext.current

    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(
            title = if (zh) "诊断日志" else "Diagnostic Log",
            backText = if (zh) "‹ 返回" else "‹ Back",
            onBack = onBack,
        ) {
            // 导出（iOS ShareLink 同位：系统分享面板，数据去向由用户选择）
            Box(
                Modifier
                    .background(c.info.copy(alpha = 0.09f), RoundedCornerShape(HktRadius.control.dp))
                    .clickableBox {
                        shareText(context, if (zh) "HKT BLETools 诊断日志" else "HKT BLETools diagnostics", LogStore.exportText)
                    }
                    .padding(horizontal = 9.dp, vertical = 5.dp),
            ) {
                Text(if (zh) "导出" else "Export", style = hkt(14f, FontWeight.SemiBold), color = c.info)
            }
        }
        if (entries.isEmpty()) {
            Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Text(if (zh) "暂无日志" else "No entries yet", style = hkt(13f), color = c.text2)
            }
        } else {
            Column(
                Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
                    .padding(horizontal = 16.dp)
                    .padding(bottom = 24.dp),
            ) {
                entries.forEach { entry ->
                    LogLine(timestamp = entry.timestamp, level = entry.level, message = entry.message)
                }
            }
        }
    }
}

/** 隐私说明页（P_privacy）—— 原型 PRIVACY 对象逐字移植（ZH/EN 各 11 节）；随语言切换。 */
@Composable
fun PrivacyScreen(onBack: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh

    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(title = if (zh) "隐私说明" else "Privacy", backText = if (zh) "‹ 返回" else "‹ Back", onBack = onBack)
        Column(
            Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 16.dp)
                .padding(bottom = 24.dp),
        ) {
            Text(
                (if (zh) "生效日期" else "Effective date") + ": 2026-09-08 · v1.0",
                style = hkt(13f),
                color = c.text2,
                modifier = Modifier.fillMaxWidth().padding(vertical = 2.dp).padding(bottom = 10.dp),
                textAlign = TextAlign.Center,
            )
            PrivacyContent.sections(zh).forEach { section ->
                HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 11.dp)) {
                    Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
                        Text(section.title, style = hkt(14f, FontWeight.Bold), color = c.text)
                        Text(
                            section.body,
                            style = hkt(13f).copy(lineHeight = (13f * 1.6f).sp),
                            color = c.text2,
                        )
                    }
                }
            }
        }
    }
}

/** 原型 PRIVACY 对象的逐字移植（ZH/EN 各 11 节）。 */
object PrivacyContent {
    data class Section(val title: String, val body: String)

    fun sections(zh: Boolean): List<Section> = if (zh) zhSections else enSections

    private val zhSections = listOf(
        Section("概述", "HKT BLETools（下称“本应用”）是在您的手机本地运行的设备管理工具，用于查找、连接和管理 HKT 蓝牙设备（MPS / SVC / UDS / EPS 系列）。\n本应用不收集、不上传、不共享任何个人数据，也不包含分析统计、广告或追踪组件。"),
        Section("我们处理的信息（仅存于您的设备）", "• 蓝牙扫描数据：附近 HKT 设备的广播名称、系统标识符与信号强度，仅用于建立和维持连接；\n• 设备数据：所连接设备的固件版本、电量/电压、温湿度、经纬度（由设备上报）、阀门状态、地磁/雷达读数等状态与配置信息；\n• 本地记录：诊断日志、固件升级报告、最近连接的设备、语言与扫描过滤等设置。"),
        Section("存储与保留", "上述数据仅保存在您的设备本地（系统沙盒）中，不会传输到任何服务器；删除本应用即同时永久删除上述全部数据。"),
        Section("无网络传输与第三方", "本应用不具备任何服务器通信能力，不嵌入第三方分析、广告或追踪 SDK，不存在向外部第三方披露数据的行为。"),
        Section("相机使用", "仅在您使用“定位设备 → 扫描设备二维码”功能时访问相机，用于读取设备标签上的二维码；相机画面不会被拍摄、录制或保存。"),
        Section("蓝牙权限", "蓝牙权限仅用于扫描和连接 HKT 设备，不用于推断位置或建立行为画像。"),
        Section("数据导出", "仅当您主动导出诊断日志或固件升级报告时，数据经系统分享面板发送，目的地由您选择；在您操作之前，任何数据都不会离开设备。"),
        Section("您的权利", "数据仅存于本机：删除全部数据 = 卸载本应用；导出数据 = 使用应用内导出功能。本应用无账号体系，不涉及在线行使权利的流程。"),
        Section("儿童隐私", "本应用为工业/工程作业工具，不面向儿童，不有意收集任何个人信息。"),
        Section("本说明的变更", "若未来版本的功能影响本说明（例如增加联网能力），我们将更新本页并标注版本与生效日期；重大变更会在应用内提示您。"),
        Section("联系我们", "如对本说明有任何疑问，可通过邮件 sales@hktlora.com 或公司门户网站 www.hktlora.com 与我们联系。"),
    )

    private val enSections = listOf(
        Section("Overview", "HKT BLETools (the “app”) is a device-management tool that runs locally on your phone to find, connect to and manage HKT Bluetooth devices (MPS / SVC / UDS / EPS series).\nThe app does not collect, upload or share any personal data, and contains no analytics, advertising or tracking components."),
        Section("Information we process (stored only on your device)", "• Bluetooth scan data: advertised names, system identifiers and signal strength of nearby HKT devices, used only to establish and maintain connections;\n• Device data: firmware version, battery/voltage, temperature/humidity, latitude/longitude (reported by the device), valve states, magnetometer/radar readings and configuration of the connected device;\n• Local records: diagnostic logs, firmware-update reports, recently connected devices, language and scan-filter settings."),
        Section("Storage and retention", "All data above is stored only on your device (system sandbox) and is never transmitted to any server. Uninstalling the app permanently deletes all of it."),
        Section("No network transfer, no third parties", "The app has no server-communication capability, embeds no third-party analytics, advertising or tracking SDKs, and does not disclose data to external parties."),
        Section("Camera use", "The camera is accessed only when you use “Locate Device → Scan Device QR Code”, to read the QR code on the device label. The camera feed is never photographed, recorded or saved."),
        Section("Bluetooth permission", "Bluetooth is used only to scan for and connect to HKT devices — never to infer your location or build behavioural profiles."),
        Section("Data export", "Only when you actively export a diagnostic log or update report does data leave the device, via the system share sheet, to a destination you choose. Nothing leaves the device without your action."),
        Section("Your rights", "Data lives only on this device: delete all data = uninstall the app; export data = use the in-app export. The app has no account system."),
        Section("Children", "The app is an industrial/engineering tool, is not directed at children, and does not knowingly collect any personal information."),
        Section("Changes to this statement", "If a future version changes what this statement covers (e.g. adding networking), we will update this page with a new version and date; material changes will be announced in the app."),
        Section("Contact us", "If you have any questions about this statement, contact us at sales@hktlora.com or visit www.hktlora.com."),
    )
}
