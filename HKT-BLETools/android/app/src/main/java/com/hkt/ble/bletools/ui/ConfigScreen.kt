package com.hkt.ble.bletools.ui

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.hkt.ble.bletools.core.ble.DeviceSession
import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.DeviceFamily
import com.hkt.ble.bletools.core.protocol.DeviceSnapshot
import com.hkt.ble.bletools.core.protocol.HKTFrameEncoder
import com.hkt.ble.bletools.designsystem.BadgeKind
import com.hkt.ble.bletools.designsystem.ChoiceCell
import com.hkt.ble.bletools.designsystem.ConfigCaption
import com.hkt.ble.bletools.designsystem.ConfigControlRow
import com.hkt.ble.bletools.designsystem.ConfigInputRow
import com.hkt.ble.bletools.designsystem.ConfigLabel
import com.hkt.ble.bletools.designsystem.ConfigSection
import com.hkt.ble.bletools.designsystem.ConfigStatePill
import com.hkt.ble.bletools.designsystem.DialogButton
import com.hkt.ble.bletools.designsystem.DialogButtonKind
import com.hkt.ble.bletools.designsystem.HktBanner
import com.hkt.ble.bletools.designsystem.HktDialogCard
import com.hkt.ble.bletools.designsystem.HktRadius
import com.hkt.ble.bletools.designsystem.HktSwitch
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.NavbarHeader
import com.hkt.ble.bletools.designsystem.NumFieldCard
import com.hkt.ble.bletools.designsystem.ChoiceChipRow
import com.hkt.ble.bletools.designsystem.PortSet
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.LanguageStore
import com.hkt.ble.bletools.model.LogStore
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

/**
 * 参数配置页（P_config）—— 1:1 克隆冻结原型（规格卡 docs/ios/design/ui-spec/P-config.md）。
 * 三家族三套表单 + 前置校验（与固件同规则）+ 确认摘要 + 写入中 + 结果横幅。
 * 保存走真实 0x02：设备 ACK（含 0xFF 应答段的专用帧）驱动结果横幅。
 * 配置页期间停轮询：固件 RX 单缓冲 + 50ms 空闲判帧，写帧撞上轮询会被整段丢弃（FD-004）。
 */
private class ConfigDraft {
    var report = ""
    var gps = ""
    var low = ""
    var high = ""
    var modeIndex = 0        // dc 工作模式
    var vol = 2              // svc 电压档 0/1/2
    var port = 1             // svc 端口原始值 0x00-0x03 | 0x80
    var stable = "5"
    var smart = false
    var tz = 0
    var period = ""
}

@Composable
fun ConfigScreen(session: DeviceSession, onBack: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    val scope = rememberCoroutineScope()
    val snapshot by session.snapshot.collectAsState()

    var draft = remember { ConfigDraft() }
    var errors by remember { mutableStateOf(mapOf<String, String>()) }
    var banner by remember { mutableStateOf(null as BannerKind?) }
    var showConfirm by remember { mutableStateOf(false) }
    var showSaving by remember { mutableStateOf(false) }
    var tzHint by remember { mutableStateOf<String?>(null) }
    var initialized by remember { mutableStateOf(false) }
    var redraw by remember { mutableStateOf(0) }   // 草稿为可变类，字段变更时 bump 触发重组

    fun bump() { redraw += 1 }

    val isUDS = session.family == DeviceFamily.UDS100
    val isSVC = session.family == DeviceFamily.SVC100

    // 进入配置页即停轮询（FD-004：写帧撞轮询被固件单 RX 缓冲整段丢弃）；离开恢复
    androidx.compose.runtime.DisposableEffect(Unit) {
        session.setPollingSuspended(true)
        onDispose { session.setPollingSuspended(false) }
    }
    @Suppress("UNUSED_EXPRESSION")
    redraw   // 草稿可变类字段变更经 bump() 触发重组
    // 草稿初值 = 轮询快照（cfgInit），初始化一次
    LaunchedEffect(Unit) {
        errors = emptyMap(); banner = null
        when (session.family) {
            DeviceFamily.UDS100 -> {
                draft.report = (snapshot.reportPeriodMin ?: 20).toString()
                draft.gps = (snapshot.gpsPeriodMin ?: 60).toString()
                draft.low = (snapshot.lowThresholdMM ?: 400).toString()
                draft.high = (snapshot.highThresholdMM ?: 3000).toString()
            }
            DeviceFamily.DC200_FAMILY -> {
                draft.report = (snapshot.reportPeriodMin ?: 20).toString()
                draft.modeIndex = snapshot.parkMode ?: 0
            }
            DeviceFamily.SVC100 -> {
                draft.vol = snapshot.voltageLevel ?: 2
                draft.port = snapshot.portFunction ?: 1
                draft.stable = (snapshot.stableTimeS ?: 5).toString()
                draft.smart = snapshot.smartPower == 1
                draft.tz = snapshot.timezone ?: 0
                draft.period = (snapshot.reportPeriodMin ?: 60).toString()
            }
        }
        bump()
    }
    BackHandler { onBack() }

    fun tzLabel(value: Int): String = when {
        value == 25 -> "UTC+03:30"
        value == 26 -> "UTC+05:30"
        value < 13 -> String.format(java.util.Locale.US, "UTC+%02d:00", value)
        else -> String.format(java.util.Locale.US, "UTC−%02d:00", value - 12)
    }

    fun portText(port: Int): String {
        val values = listOf(0x00, 0x01, 0x02, 0x03, 0x81, 0x82, 0x83)
        val labels = if (zh)
            listOf("端口1+2 脉冲", "端口1 开关 · 端口2 脉冲", "端口1 脉冲 · 端口2 开关", "端口1+2 开关",
                "端口1 开关 · 端口2 脉冲（稳定时间）", "端口1 脉冲 · 端口2 开关（稳定时间）", "端口1+2 开关（稳定时间）")
        else
            listOf("Port 1 + Port 2 pulse", "Port 1 switch, Port 2 pulse", "Port 1 pulse, Port 2 switch", "Port 1 + Port 2 switch",
                "Port 1 switch, Port 2 pulse (stable time)", "Port 1 pulse, Port 2 switch (stable time)", "Port 1 + Port 2 switch (stable time)")
        val index = values.indexOf(port)
        return if (index >= 0) labels[index] else if (zh) "未知" else "Unknown"
    }

    fun derivedPort(port: Int, bit: Int, on: Boolean): Int {
        val base = (port and 3 and bit.inv()) or (if (on) bit else 0)
        return base or (port and 0x80)
    }

    fun matchPhoneTimezone(): Int? {
        val seconds = java.util.Calendar.getInstance().get(java.util.Calendar.ZONE_OFFSET).toDouble()
        val hours = seconds / 3600.0
        val half = (hours * 2).toInt().toDouble()
        return when {
            half % 2.0 == 0.0 -> {
                val hour = (half / 2).toInt()
                if (hour in 0..12) hour else if (hour in -12..-1) 12 - hour else null
            }
            half == 7.0 -> 25
            half == 11.0 -> 26
            else -> null
        }
    }

    // 校验（cfgValidate：与固件同规则，任一非法拦截保存）
    fun validationErrors(): Map<String, String> {
        val result = mutableMapOf<String, String>()
        fun outOfRange(key: String, min: Int, max: Int) {
            result[key] = if (zh) "超出范围 $min–$max" else "Out of range $min–$max"
        }
        fun number(text: String): Int? = text.trim().toIntOrNull()
        if (isUDS) {
            val report = number(draft.report)
            if (report == null || report !in 1..1440) outOfRange("report", 1, 1440)
            val gps = number(draft.gps)
            if (gps == null || (gps != 0 && gps !in 10..1440)) result["gps"] = if (zh) "合法值：0 或 10–1440" else "valid: 0 or 10–1440"
            val low = number(draft.low)
            if (low == null || low !in 30..4500) outOfRange("low", 30, 4500)
            val high = number(draft.high)
            if (high == null || (high != 0 && high !in 30..4500)) result["high"] = if (zh) "合法值：0 或 30–4500" else "valid: 0 or 30–4500"
        } else if (session.family == DeviceFamily.DC200_FAMILY) {
            val report = number(draft.report)
            if (report == null || report !in 0..1440) outOfRange("report", 0, 1440)
        } else {
            if (draft.port and 0x80 != 0) {
                val stable = number(draft.stable)
                if (stable == null || stable !in 1..255) outOfRange("stable", 1, 255)
            }
            val period = number(draft.period)
            if (period == null || (period != 0 && period !in 10..1440)) result["period"] = if (zh) "合法值：0 或 10–1440" else "valid: 0 or 10–1440"
        }
        return result
    }

    fun saveTapped(onErrors: (Map<String, String>) -> Unit, onValid: () -> Unit) {
        val found = validationErrors()
        onErrors(found)
        if (found.isEmpty()) onValid()   // 通过 → 确认摘要；否则行内错误已显示，拦截保存
    }

    fun write() {
        showSaving = true
        LogStore.info(if (zh) "0x02 写入配置 → ${session.deviceName}" else "0x02 write config → ${session.deviceName}")
        val payload = when (session.family) {
            DeviceFamily.UDS100 -> HKTFrameEncoder.udsConfigPayload(
                reportMin = draft.report.toIntOrNull() ?: 0, gpsMin = draft.gps.toIntOrNull() ?: 0,
                lowMM = draft.low.toIntOrNull() ?: 0, highMM = draft.high.toIntOrNull() ?: 0,
            )
            DeviceFamily.DC200_FAMILY -> HKTFrameEncoder.dcConfigPayload(
                reportMin = draft.report.toIntOrNull() ?: 0, mode = draft.modeIndex,
            )
            DeviceFamily.SVC100 -> HKTFrameEncoder.svcConfigPayload(
                volLevel = draft.vol, port = draft.port, stableS = draft.stable.toIntOrNull() ?: 0,
                autoPower = if (draft.smart) 1 else 0, timezone = draft.tz, reportMin = draft.period.toIntOrNull() ?: 0,
            )
        }
        scope.launch {
            val acked = session.sendWrite(cmd = CommandCode.CONFIG, data = payload)
            showSaving = false
            banner = if (acked) BannerKind.OK else BannerKind.FAIL
            LogStore.info(if (acked) (if (zh) "0x02 ACK（设备已确认）" else "0x02 ACK (device acknowledged)")
            else (if (zh) "0x02 等待确认超时（无 ACK）" else "0x02 ACK timeout"))
        }
    }

    // ===== 骨架 =====
    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(title = if (zh) "参数配置" else "Configuration", backText = if (zh) "‹ 返回" else "‹ Back", onBack = onBack)
        if (isSVC) {
            // SVC 专属：表单滚动 + 吸底 savebar（规格卡 §1）
            Column(Modifier.fillMaxSize()) {
                Column(
                    Modifier.weight(1f).verticalScroll(rememberScrollState())
                        .padding(horizontal = 16.dp).padding(top = 12.dp),
                ) {
                    banner?.let { ResultBanner(it, zh, onDone = { initDraftFrom(session, draft); bump() }, onRetry = { banner = null }) }
                    SvcForm(session, draft, errors, zh, snapshot, tzHint,
                        onBump = { bump() }, onTzHint = { tzHint = it })
                }
                // 吸底保存栏（.savebar：bg、上边框 line 68%）
                Column(
                    Modifier.fillMaxWidth().background(c.bg)
                        .padding(top = 11.dp, bottom = 24.dp).padding(horizontal = 16.dp),
                    horizontalAlignment = Alignment.CenterHorizontally,
                ) {
                    SaveButton { saveTapped({ errors = it }, { showConfirm = true }) }
                    Spacer(Modifier.height(7.dp))
                    Text(
                        "0x02 · " + (if (zh) "载荷" else "payload") + " 8 B",
                        style = androidx.compose.ui.text.TextStyle(fontSize = 11.sp, fontFamily = androidx.compose.ui.text.font.FontFamily.Monospace),
                        color = c.text2,
                    )
                }
            }
        } else {
            Column(
                Modifier
                    .weight(1f)
                    .verticalScroll(rememberScrollState())
                    .padding(horizontal = 16.dp)
                    .padding(bottom = 24.dp),
            ) {
                banner?.let { ResultBanner(it, zh, onDone = { initDraftFrom(session, draft); bump() }, onRetry = { banner = null }) }
                FormBody(session, draft, errors, zh, onBump = { bump() })
                Spacer(Modifier.height(12.dp))
                SaveButton { saveTapped({ errors = it }, { showConfirm = true }) }
                Spacer(Modifier.height(8.dp))
                Text(
                    "0x02 · " + (if (zh) "载荷" else "payload") + " " +
                        (if (isUDS) 9 else 4) + " B",
                    style = hkt(11f), color = c.text2,
                )
            }
        }
    }

    // ===== 对话框 =====
    if (showConfirm) {
        Box(
            Modifier.fillMaxSize().background(Color.Black.copy(alpha = 0.42f))
                .clickableBox { showConfirm = false },
            contentAlignment = Alignment.Center,
        ) {
            HktDialogCard(
                title = if (zh) "写入以下配置？" else "Write these values?",
                message = androidx.compose.ui.text.AnnotatedString(summaryText(session, draft, zh)),
                buttons = listOf(
                    Triple(if (zh) "取消" else "Cancel", DialogButtonKind.SECONDARY) { showConfirm = false },
                    Triple(if (zh) "确认写入" else "Write Values", DialogButtonKind.PRIMARY) {
                        showConfirm = false
                        write()
                    },
                ),
            )
        }
    }
    if (showSaving) {
        Box(
            Modifier.fillMaxSize().background(Color.Black.copy(alpha = 0.42f)),
            contentAlignment = Alignment.Center,
        ) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(10.dp),
                modifier = Modifier.padding(18.dp),
            ) {
                Text(if (zh) "正在写入配置…" else "Writing configuration…", style = hkt(16f, FontWeight.Bold), color = c.text)
                Text("0x02 → ${session.deviceName}", style = hkt(13f), color = c.text2, textAlign = TextAlign.Center)
            }
        }
    }
}

private fun initDraftFrom(session: DeviceSession, draft: ConfigDraft) {
    val s = session.snapshot.value
    when (session.family) {
        DeviceFamily.UDS100 -> {
            draft.report = (s.reportPeriodMin ?: 20).toString()
            draft.gps = (s.gpsPeriodMin ?: 60).toString()
            draft.low = (s.lowThresholdMM ?: 400).toString()
            draft.high = (s.highThresholdMM ?: 3000).toString()
        }
        DeviceFamily.DC200_FAMILY -> {
            draft.report = (s.reportPeriodMin ?: 20).toString()
            draft.modeIndex = s.parkMode ?: 0
        }
        DeviceFamily.SVC100 -> {
            draft.vol = s.voltageLevel ?: 2
            draft.port = s.portFunction ?: 1
            draft.stable = (s.stableTimeS ?: 5).toString()
            draft.smart = s.smartPower == 1
            draft.tz = s.timezone ?: 0
            draft.period = (s.reportPeriodMin ?: 60).toString()
        }
    }
}

enum class BannerKind { OK, FAIL }

private fun summaryText(session: DeviceSession, draft: ConfigDraft, zh: Boolean): String {
    val minutes = if (zh) "分钟" else "min"
    return when (session.family) {
        DeviceFamily.UDS100 -> "上报周期 ${draft.report}$minutes；GPS ${draft.gps}$minutes；低阈值 ${draft.low}mm；高阈值 ${draft.high}mm"
        DeviceFamily.DC200_FAMILY -> "上报周期 ${draft.report}$minutes；${listOf("融合", "仅地磁", "雷达优先")[draft.modeIndex]}"
        DeviceFamily.SVC100 -> {
            val tzLabel = when (draft.tz) {
                25 -> "UTC+03:30"
                26 -> "UTC+05:30"
                else -> if (draft.tz < 13) "UTC+$draft.tz" else "UTC\u2212${draft.tz - 12}"
            }
            "电压 ${listOf("12V", "9V", "5V")[draft.vol]}; 稳定 ${draft.stable}s; ${tzLabel}; ${draft.period}$minutes"
        }
    }
}

@Composable
private fun SaveButton(onSave: () -> Unit) {
    val c = LocalHktColors.current
    Box(
        Modifier
            .fillMaxWidth()
            .background(c.info, RoundedCornerShape(HktRadius.control.dp))
            .clickableBox(onSave)
            .padding(13.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(if (HktLang.isZh) "保存配置" else "Save", style = hkt(16f, FontWeight.SemiBold), color = Color.White)
    }
}

@Composable
private fun ResultBanner(kind: BannerKind, zh: Boolean, onDone: () -> Unit, onRetry: () -> Unit) {
    if (kind == BannerKind.OK) {
        HktBanner(BadgeKind.OK, "✓ " + (if (zh) "配置已保存（设备已确认）" else "Configuration saved (device acknowledged)"),
            if (zh) "完成" else "Done", action = onDone)
    } else {
        HktBanner(BadgeKind.ERR, "✕ " + (if (zh) "设备未确认配置（可能被固件拒绝），请检查参数范围后重试"
        else "Device did not acknowledge (possibly rejected). Check ranges and retry."),
            if (zh) "重试" else "Retry", action = onRetry)
    }
}

@Composable
private fun FormBody(
    session: DeviceSession,
    draft: ConfigDraft,
    errors: Map<String, String>,
    zh: Boolean,
    onBump: () -> Unit,
) {
    if (session.family == DeviceFamily.UDS100) {
        NumFieldCard(label = if (zh) "上报周期" else "Report period", text = draft.report,
            onTextChange = { draft.report = it; onBump() }, unit = if (zh) "分钟" else "min", error = errors["report"])
        NumFieldCard(label = if (zh) "GPS 周期" else "GPS period", text = draft.gps,
            onTextChange = { draft.gps = it; onBump() }, unit = if (zh) "分钟" else "min",
            hint = if (zh) "0 = 关闭 GPS 定位" else "0 = GPS positioning off", error = errors["gps"])
        NumFieldCard(label = if (zh) "低阈值" else "Low threshold", text = draft.low,
            onTextChange = { draft.low = it; onBump() }, unit = "mm", error = errors["low"])
        NumFieldCard(label = if (zh) "高阈值" else "High threshold", text = draft.high,
            onTextChange = { draft.high = it; onBump() }, unit = "mm",
            hint = if (zh) "0 = 关闭高阈值告警" else "0 = high-threshold alarm off", error = errors["high"])
        Text(
            if (zh) "固件校验任一参数非法时整包静默拒绝（无 ACK），App 侧已前置同规则校验"
            else "Firmware silently rejects the whole payload if any value is invalid; the app pre-validates with the same rules",
            style = hkt(11f), color = LocalHktColors.current.text2, modifier = Modifier.padding(top = 4.dp),
        )
    } else if (session.family == DeviceFamily.DC200_FAMILY) {
        NumFieldCard(label = if (zh) "上报周期" else "Report period", text = draft.report,
            onTextChange = { draft.report = it; onBump() }, unit = if (zh) "分钟" else "min",
            hint = if (zh) "取值范围 0–1440，0=连续上报" else "Range 0–1440; 0 = report continuously", error = errors["report"])
        ChoiceChipRow(
            label = if (zh) "工作模式" else "Work mode",
            options = listOf(
                if (zh) "融合模式" else "Fusion",
                if (zh) "仅地磁" else "Mag only",
                if (zh) "雷达优先" else "Radar first",
            ),
            selection = draft.modeIndex, onSelect = { draft.modeIndex = it; onBump() },
        )
    }
}

@Composable
private fun SvcForm(
    session: DeviceSession,
    draft: ConfigDraft,
    errors: Map<String, String>,
    zh: Boolean,
    snapshot: DeviceSnapshot,
    tzHint: String?,
    onBump: () -> Unit,
    onTzHint: (String?) -> Unit,
) {
    val c = LocalHktColors.current
    val stableOn = draft.port and 0x80 != 0
    val rawPort = String.format(java.util.Locale.US, "0x%02X", draft.port and 0xff)

    fun derivedPort(port: Int, bit: Int, on: Boolean): Int {
        val base = (port and 3 and bit.inv()) or (if (on) bit else 0)
        return base or (port and 0x80)
    }

    Column {
        ConfigSection(
            title = if (zh) "输出与端口" else "Output & port",
            caption = if (zh) "选择供电档位与两路端口动作" else "Choose supply level and both port actions",
            state = rawPort,
        ) {
            ConfigLabel(if (zh) "输出电压档" else "Voltage level")
            Row(horizontalArrangement = Arrangement.spacedBy(7.dp)) {
                listOf("12V", "9V", "5V").forEachIndexed { index, label ->
                    ChoiceCell(
                        text = label, selected = draft.vol == index,
                        modifier = Modifier.weight(1f),
                        action = { draft.vol = index; onBump() },
                    )
                }
            }
            ConfigCaption(if (zh) "固件映射 0=12V / 1=9V / 2=5V" else "firmware: 0=12V / 1=9V / 2=5V")
            ConfigLabel(if (zh) "端口功能" else "Port function")
            Row(horizontalArrangement = Arrangement.spacedBy(7.dp)) {
                PortSet(
                    title = if (zh) "阀 1" else "Valve 1",
                    onText = if (zh) "开关控制" else "Switch control",
                    offText = if (zh) "PWM 控制" else "PWM control",
                    isOn = draft.port and 0x01 != 0,
                    modifier = Modifier.weight(1f),
                    setOn = { on -> draft.port = derivedPort(draft.port, 0x01, on); onBump() },
                )
                PortSet(
                    title = if (zh) "阀 2" else "Valve 2",
                    onText = if (zh) "开关控制" else "Switch control",
                    offText = if (zh) "PWM 控制" else "PWM control",
                    isOn = draft.port and 0x02 != 0,
                    modifier = Modifier.weight(1f),
                    setOn = { on -> draft.port = derivedPort(draft.port, 0x02, on); onBump() },
                )
            }
            ConfigCaption(
                (if (zh) "合成端口值: " else "Derived port value: ") + rawPort, topSpacing = 7,
            )
            Row(horizontalArrangement = Arrangement.spacedBy(7.dp), modifier = Modifier.padding(top = 10.dp)) {
                Box(Modifier.weight(1f)) {
                    ConfigControlRow(if (zh) "含稳定时长" else "Stable time on") {
                        HktSwitch(isOn = stableOn)
                        // 0x80 位切换由点击整行触发
                    }
                    Box(Modifier.matchParentSize().clickableBox { draft.port = draft.port xor 0x80; onBump() })
                }
                ConfigInputRow(
                    label = if (zh) "稳定时长" else "Stable time",
                    text = draft.stable, onTextChange = { draft.stable = it; onBump() },
                    unit = "s", disabled = !stableOn,
                )
            }
            errors["stable"]?.let {
                Text("✕ $it", style = hkt(12f), color = c.err, modifier = Modifier.padding(top = 5.dp))
            } ?: ConfigCaption(
                if (stableOn) (if (zh) "1–255 秒" else "1–255 s")
                else (if (zh) "仅在带稳定时间的端口模式下可编辑" else "Only editable in stable-time port modes"),
                topSpacing = 5,
            )
            Box(Modifier.padding(top = 10.dp)) {
                ConfigControlRow(if (zh) "自动开关机" else "Auto power") {
                    HktSwitch(isOn = draft.smart)
                }
                Box(Modifier.matchParentSize().clickableBox { draft.smart = !draft.smart; onBump() })
            }
            ConfigCaption(
                if (zh) "开启后：阀插入自动开机，阀拔出自动关机" else "When on: the valve powers on when inserted and off when removed",
                topSpacing = 7,
            )
        }
        Spacer(Modifier.height(10.dp))
        fun tzLabel(value: Int): String = when {
            value == 25 -> "UTC+03:30"
            value == 26 -> "UTC+05:30"
            value < 13 -> String.format(java.util.Locale.US, "UTC+%02d:00", value)
            else -> String.format(java.util.Locale.US, "UTC\u2212%02d:00", value - 12)
        }
        ConfigSection(
            title = if (zh) "时间与上报" else "Time & reporting",
            caption = if (zh) "用于 0x06 对时换算与定时任务触发" else "Used for 0x06 time sync and schedule timing",
            state = tzLabel(draft.tz),
        ) {
            ConfigLabel(if (zh) "时区" else "Time zone")
            TimeZonePicker(selected = draft.tz, onSelect = { draft.tz = it; onBump() })
            snapshot.timezone?.let { deviceTz ->
                ConfigCaption((if (zh) "设备当前: " else "Device current: ") + tzLabel(deviceTz), topSpacing = 6)
            }
            Box(Modifier.padding(top = 9.dp)) {
                DialogButton(title = if (zh) "与手机时区一致" else "Match phone timezone", kind = DialogButtonKind.SECONDARY) {
                    val seconds = java.util.Calendar.getInstance().get(java.util.Calendar.ZONE_OFFSET).toDouble()
                    val hours = seconds / 3600.0
                    val half = Math.round(hours * 2).toDouble()
                    val encoded: Int? = when {
                        Math.abs(hours * 2 - half) < 0.01 && half % 2.0 == 0.0 -> {
                            val hour = (half / 2).toInt()
                            if (hour in 0..12) hour else if (hour in -12..-1) 12 - hour else null
                        }
                        half == 7.0 -> 25
                        half == 11.0 -> 26
                        else -> null
                    }
                    if (encoded != null) {
                        draft.tz = encoded
                        onTzHint(if (zh) "已选为手机时区" else "Set to phone timezone")
                    } else {
                        draft.tz = 0
                        onTzHint(if (zh) "手机时区超出设备支持范围，已选 UTC+00:00" else "Phone timezone unsupported; set to UTC+00:00")
                    }
                    onBump()
                }
            }
            tzHint?.let {
                Text(it, style = hkt(11f), color = c.text2, modifier = Modifier.padding(top = 4.dp))
            }
            ConfigLabel(if (zh) "上报周期" else "Report period")
            ConfigInputRow(
                label = if (zh) "上报周期" else "Report period",
                text = draft.period, onTextChange = { draft.period = it; onBump() }, unit = if (zh) "分钟" else "min",
            )
            Text(
                if (zh) "0=关闭周期上报，范围 10–1440" else "0 = periodic reporting off; range 10–1440",
                style = hkt(11f), color = c.text2, modifier = Modifier.padding(top = 4.dp),
            )
            errors["period"]?.let {
                Text("✕ $it", style = hkt(12f), color = c.err, modifier = Modifier.padding(top = 5.dp))
            }
        }
    }
}

/// 时区分组选择（原型 tzGroups：西半球 / UTC+00:00 / 东半球 / 半小时）。
@Composable
private fun TimeZonePicker(selected: Int, onSelect: (Int) -> Unit) {
    val c = LocalHktColors.current
    val entries = buildList {
        (13..24).reversed().forEach { add(it) }
        add(0)
        (1..12).forEach { add(it) }
        add(25); add(26)
    }
    Column(
        Modifier
            .fillMaxWidth()
            .background(c.card2, RoundedCornerShape(HktRadius.control.dp))
            .border(1.dp, c.line, RoundedCornerShape(HktRadius.control.dp))
            .padding(horizontal = 10.dp, vertical = 6.dp),
    ) {
        fun tzLabel(value: Int): String = when {
            value == 25 -> "UTC+03:30"
            value == 26 -> "UTC+05:30"
            value < 13 -> String.format(java.util.Locale.US, "UTC+%02d:00", value)
            else -> String.format(java.util.Locale.US, "UTC\u2212%02d:00", value - 12)
        }
        entries.forEach { value ->
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier.fillMaxWidth().clickableBox { onSelect(value) }.padding(vertical = 6.dp),
            ) {
                Box(
                    Modifier.size(14.dp).background(if (selected == value) c.info else Color.Transparent, CircleShape)
                        .border(1.dp, if (selected == value) c.info else c.line, CircleShape)
                )
                Spacer(Modifier.width(8.dp))
                Text(tzLabel(value), style = hkt(14f), color = c.text)
                if (selected == value) {
                    Spacer(Modifier.weight(1f))
                    Text("✓", style = hkt(14f, FontWeight.Bold), color = c.info)
                }
            }
        }
    }
}
