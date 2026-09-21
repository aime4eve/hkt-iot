package com.hkt.ble.bletools.ui

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
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.hkt.ble.bletools.designsystem.BadgeKind
import com.hkt.ble.bletools.designsystem.CenterStateView
import com.hkt.ble.bletools.designsystem.HktDialogCard
import com.hkt.ble.bletools.designsystem.HktRadius
import com.hkt.ble.bletools.designsystem.LinkButton
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.NavbarLarge
import com.hkt.ble.bletools.designsystem.RssiBars
import com.hkt.ble.bletools.designsystem.ScanDeviceCard
import com.hkt.ble.bletools.designsystem.ScanStatusRow
import com.hkt.ble.bletools.designsystem.SectionHeader
import com.hkt.ble.bletools.designsystem.StateBadge
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.ScanModel

/**
 * P-01 扫描页 —— 1:1 克隆冻结原型（规格卡 docs/ios/design/ui-spec/P-01.md）。
 * 结构：大标题页头（⌖ 扫描 + ⚙︎）→ 滚动区（扫描状态行/搜索框/驻留卡/设备卡/空态/最近设备）→ 切换确认框。
 * 交互=R-31/R-32：点驻留设备回详情、点其他设备弹切换确认（原子释放）、重扫带健康检测。
 * 连接覆盖层（P-02）与定位流（P-01b）在 M6 接入。
 */
object HktLang {
    val isZh: Boolean
        get() = androidx.core.os.LocaleListCompat.getDefault()[0]?.language?.startsWith("zh") == true
}

@Composable
fun ScanListScreen(model: ScanModel, onOpenSettings: () -> Unit = {}) {
    val c = LocalHktColors.current
    val zh = HktLang.isZh
    val devices by model.devices.collectAsState()
    val isScanning by model.isScanning.collectAsState()
    val round by model.scanRound.collectAsState()
    val elapsed by model.scanElapsed.collectAsState()
    var query by remember { mutableStateOf("") }   // R-33 列表模糊查询

    Box(Modifier.fillMaxSize().background(c.bg)) {
        Column(Modifier.fillMaxSize()) {
            NavbarLarge(title = "HKT BLETools") {
                Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                    // ⌖ 扫描：直启相机扫码定位（相机流 P-01b 于 M6 接入）
                    LinkButton(title = if (zh) "⌖ 扫描" else "⌖ Locate") { }
                    LinkButton(title = if (zh) "⚙︎ 设置" else "⚙︎ Settings") { onOpenSettings() }
                }
            }
            Column(
                Modifier
                    .weight(1f)
                    .verticalScroll(rememberScrollState())
                    .padding(horizontal = 16.dp)
                    .padding(bottom = 24.dp),
            ) {
                // 扫描状态行（规格卡 §2.2 + 3 轮生命周期）
                Column {
                    ScanStatusRow(
                        title = if (isScanning) {
                            if (zh) "扫描中… 已发现 ${devices.size} 台 · 第 $round/3 轮 · 剩余 ${ScanModel.SCAN_ROUND_SECONDS - elapsed} 秒"
                            else "Scanning… ${devices.size} found · round $round/3 · ${ScanModel.SCAN_ROUND_SECONDS - elapsed}s left"
                        } else {
                            if (zh) "扫描完成 · ${devices.size} 台" else "Scan finished · ${devices.size} found"
                        },
                        actionTitle = if (isScanning) (if (zh) "停止" else "Stop") else (if (zh) "附近设备" else "Nearby Devices"),
                        action = { if (isScanning) model.stopScan() else model.startScan() },
                    )
                    if (isScanning) {
                        Box(
                            Modifier
                                .fillMaxWidth()
                                .height(4.dp)
                                .background(c.fill, RoundedCornerShape(2.dp)),
                        ) {
                            Box(
                                Modifier
                                    .fillMaxWidth(elapsed.toFloat() / ScanModel.SCAN_ROUND_SECONDS)
                                    .height(4.dp)
                                    .background(c.info, RoundedCornerShape(2.dp)),
                            )
                        }
                    }
                }
                SearchField(query = query, onQuery = { query = it }, zh = zh, modifier = Modifier.padding(top = 10.dp))
                Content(model = model, query = query, zh = zh)
            }
        }
        SwitchDialogOverlay(model = model, zh = zh)
    }
}

/** R-33 模糊查询（广播名忽略大小写包含；与设置页类型过滤叠加）。 */
@Composable
private fun SearchField(query: String, onQuery: (String) -> Unit, zh: Boolean, modifier: Modifier = Modifier) {
    val c = LocalHktColors.current
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(8.dp),
        modifier = modifier
            .fillMaxWidth()
            .background(c.card2, RoundedCornerShape(HktRadius.control.dp))
            .padding(horizontal = 12.dp, vertical = 10.dp),
    ) {
        Text("⌕", style = hkt(15f, FontWeight.SemiBold), color = c.text2)
        BasicTextField(
            value = query,
            onValueChange = onQuery,
            singleLine = true,
            textStyle = hkt(14f).copy(color = c.text),
            modifier = Modifier.weight(1f),
            decorationBox = { inner ->
                Box {
                    if (query.isEmpty()) {
                        Text(
                            if (zh) "搜索广播名（模糊匹配）" else "Filter by broadcast name",
                            style = hkt(14f),
                            color = c.text2,
                        )
                    }
                    inner()
                }
            },
        )
        if (query.isNotEmpty()) {
            Text(
                "✕",
                style = hkt(13f),
                color = c.text2,
                modifier = Modifier.clickableBox { onQuery("") },
            )
        }
    }
}

@Composable
private fun Content(model: ScanModel, query: String, zh: Boolean) {
    val c = LocalHktColors.current
    val devices by model.devices.collectAsState()
    val isScanning by model.isScanning.collectAsState()
    val resident = model.residentDevice
    val residentActive = resident != null && model.activeSession != null
    val shown = devices.filter {
        query.trim().isEmpty() || it.name.contains(query.trim(), ignoreCase = true)
    }

    if (!isScanning && devices.isEmpty() && !residentActive) {
        CenterStateView(
            glyph = "📡",
            title = if (zh) "未发现支持设备" else "No supported devices found",
            subtitle = if (zh) "请确认设备已上电、在信号范围内" else "Make sure devices are powered and nearby",
            buttonTitle = if (zh) "附近设备" else "Nearby Devices",
            onButton = { model.startScan() },
        )
        model.lastSession?.let { last ->
            SectionHeader(if (zh) "最近设备" else "Recent")
            ScanDeviceCard(
                name = last.name,
                subtitle = "ID …${idSuffix(last.identifier)}" + (if (zh) " · 昨天" else " · Yesterday"),
                trailing = { Text("↻", style = hkt(14f, FontWeight.SemiBold), color = c.info) },
            )
        }
        return
    }

    Column(Modifier.padding(top = 0.dp)) {
        if (residentActive && resident != null) {
            ScanDeviceCard(
                name = resident.name,
                subtitle = "ID …${idSuffix(resident.identifier)}",
                badge = { StateBadge(BadgeKind.OK, if (zh) "已连接" else "Connected", compact = true) },
                trailing = { Text("›", style = hkt(14f, FontWeight.SemiBold), color = c.info) },
            )
            Spacer(Modifier.height(11.dp))
        }
        shown.forEach { device ->
            if (device.identifier != resident?.identifier) {
                ScanDeviceCard(
                    name = device.name,
                    subtitle = "ID …${idSuffix(device.identifier)} · ${device.rssi} dBm",
                    trailing = { RssiBars(lit = rssiLit(device.rssi)) },
                )
                Spacer(Modifier.height(11.dp))
            }
        }
        if (query.trim().isNotEmpty() && shown.isEmpty() && devices.isNotEmpty()) {
            Text(
                if (zh) "无匹配设备" else "No matching devices",
                style = hkt(12f),
                color = c.text2,
                modifier = Modifier.fillMaxWidth().padding(vertical = 14.dp),
                textAlign = androidx.compose.ui.text.style.TextAlign.Center,
            )
        }
        if (!residentActive && devices.isEmpty()) {
            model.lastSession?.let { last ->
                ScanDeviceCard(
                    name = last.name,
                    subtitle = "ID …${idSuffix(last.identifier)}" + (if (zh) " · 昨天" else " · Yesterday"),
                    trailing = { Text("↻", style = hkt(14f, FontWeight.SemiBold), color = c.info) },
                )
            }
        }
    }
}

/** R-32 切换确认框（.dialog 296/r18 遮罩；确认键主色 info）。 */
@Composable
private fun SwitchDialogOverlay(model: ScanModel, zh: Boolean) {
    val target = model.pendingSwitch ?: return
    val current = model.residentDevice ?: return
    Box(
        Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.40f))
            .clickableBox { model.cancelSwitch() },
        contentAlignment = Alignment.Center,
    ) {
        HktDialogCard(
            title = if (zh) "切换设备？" else "Switch device?",
            message = (if (zh) "当前已连接 " else "Currently connected to ") +
                current.name + " …${idSuffix(current.identifier)}" + (if (zh) "。" else ". ") +
                (if (zh) "切换将断开当前会话并连接 " else "Switching will disconnect it and connect to ") +
                target.name + " …${idSuffix(target.identifier)}" + (if (zh) "。" else "."),
            buttons = listOf(
                Triple(if (zh) "取消" else "Cancel", com.hkt.ble.bletools.designsystem.DialogButtonKind.SECONDARY) {
                    model.cancelSwitch()
                },
                Triple(if (zh) "切换并连接" else "Switch & Connect", com.hkt.ble.bletools.designsystem.DialogButtonKind.PRIMARY) {
                    val newTarget = model.confirmSwitch()
                    // 连接覆盖层（P-02）M6 接入；本样板确认后仅释放旧会话
                    newTarget
                },
            ),
        )
    }
}

private fun idSuffix(identifier: String): String =
    identifier.replace(":", "").takeLast(4).uppercase()

/// 原型映射：rssi > -70 → 4 亮；> -85 → 3 亮；否则 2 亮。
private fun rssiLit(rssi: Int): Int = if (rssi > -70) 4 else (if (rssi > -85) 3 else 2)
