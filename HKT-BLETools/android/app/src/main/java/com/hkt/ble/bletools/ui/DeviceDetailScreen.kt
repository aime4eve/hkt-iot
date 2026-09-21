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
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.hkt.ble.bletools.core.ble.DeviceSession
import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.DeviceFamily
import com.hkt.ble.bletools.core.protocol.DeviceSnapshot
import com.hkt.ble.bletools.core.protocol.HKTFrameEncoder
import com.hkt.ble.bletools.designsystem.BadgeKind
import com.hkt.ble.bletools.designsystem.CenterStateView
import com.hkt.ble.bletools.designsystem.FieldGrid
import com.hkt.ble.bletools.designsystem.FieldSpec
import com.hkt.ble.bletools.designsystem.HktBanner
import com.hkt.ble.bletools.designsystem.HktDialogCard
import com.hkt.ble.bletools.designsystem.LinkButton
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.NavbarLarge
import com.hkt.ble.bletools.designsystem.OpsPanel
import com.hkt.ble.bletools.designsystem.OpsPowerRow
import com.hkt.ble.bletools.designsystem.ScanDeviceCard
import com.hkt.ble.bletools.designsystem.SectionHeader
import com.hkt.ble.bletools.designsystem.SessionButton
import com.hkt.ble.bletools.designsystem.SessionControlCard
import com.hkt.ble.bletools.designsystem.SvcChannelModule
import com.hkt.ble.bletools.designsystem.SvcChannelSpec
import com.hkt.ble.bletools.designsystem.OpCard
import com.hkt.ble.bletools.designsystem.StateBadge
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.ScanModel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * P-03 设备详情页 —— 1:1 克隆冻结原型（规格卡 docs/ios/design/ui-spec/P-03.md）。
 * 结构：会话控制卡（页头，无导航栏）→ 滚动区（横幅/状态/设备操作）→ 确认对话框浮层。
 * 早退优先级：关机 > 断线 > 正常（升级模式随 OTA 里程碑接入）。
 * 数据 = DeviceSession 1s 轮询快照；校准/配置/任务/OTA/技术参数页为 M6.3 队列（按钮占位 notice）。
 */
@Composable
fun DeviceDetailScreen(
    session: DeviceSession,
    scanModel: ScanModel,
    onDisconnect: () -> Unit,
    onBack: () -> Unit,
) {
    val c = LocalHktColors.current
    val zh = HktLang.isZh
    val scope = rememberCoroutineScope()
    val snapshot by session.snapshot.collectAsState()
    val linkLost by session.linkLost.collectAsState()
    val unknownTail by session.unknownTail.collectAsState()
    val secondsSince by session.secondsSinceLastResponse.collectAsState()
    val isSyncing by session.isTimeSyncing.collectAsState()
    val timeSyncOutcome by session.timeSyncOutcome.collectAsState()

    // 1s tick：TIME 实时时钟 + stale 徽章秒数刷新（iOS TimelineView periodic 同构）
    var tick by remember { mutableLongStateOf(System.currentTimeMillis()) }
    LaunchedEffect(Unit) {
        while (true) {
            delay(1_000)
            tick = System.currentTimeMillis()
        }
    }

    var confirmDisconnect by remember { mutableStateOf(false) }
    var confirmPowerOff by remember { mutableStateOf(false) }
    var powerSending by remember { mutableStateOf(false) }
    var powerError by remember { mutableStateOf<String?>(null) }
    var notice by remember { mutableStateOf<String?>(null) }
    LaunchedEffect(notice) {
        if (notice != null) {
            delay(3_000)
            notice = null
        }
    }

    fun sendPower(on: Boolean) {
        if (powerSending) return
        powerSending = true
        powerError = null
        scope.launch {
            // 开关机帧与 1Hz 轮询在设备单 RX 缓冲上互相挤撞（FD-004）：写窗口内停轮询
            session.setPollingSuspended(true)
            try {
                var acked = session.sendWrite(
                    CommandCode.POWER, HKTFrameEncoder.powerPayload(on), timeoutMs = 3_000,
                )
                if (!acked) {
                    // 无 ACK 自动重发一次（开关机幂等）；仍无确认才报失败
                    delay(700)
                    acked = session.sendWrite(
                        CommandCode.POWER, HKTFrameEncoder.powerPayload(on), timeoutMs = 3_000,
                    )
                }
                if (acked) {
                    if (!on) {
                        // 关机成功=预期断开（用户裁决 2026-09-20）：释放会话回首页
                        onDisconnect()
                    }
                } else {
                    powerError = if (zh) "设备未确认开关机命令（重试后仍无 ACK）"
                    else "Device did not acknowledge the power command"
                }
            } finally {
                session.setPollingSuspended(false)
                powerSending = false
            }
        }
    }

    val isPowerOff = snapshot.power == 0

    // 详情页路由层：校准/配置/任务/OTA/技术参数占位 notice（M6.3 队列）
    fun later(name: String) {
        notice = if (zh) "$name 页面 M6.3 接入" else "$name lands in M6.3"
    }

    // ===== 早退：关机（原型 !powerOn）=====
    if (isPowerOff) {
        Column(Modifier.fillMaxSize().background(c.bg)) {
            NavbarLarge(title = session.deviceName) {
                LinkButton(if (zh) "‹ 返回" else "‹ Back") { onBack() }
            }
            powerError?.let { err ->
                HktBanner(BadgeKind.ERR, "✕ $err", if (zh) "重试" else "Retry") { sendPower(true) }
            }
            CenterStateView(
                glyph = "⏻",
                glyphSize = 46,
                title = if (zh) "已关机" else "Powered off",
                subtitle = "",
                buttonTitle = if (powerSending) (if (zh) "发送中…" else "Sending…") else (if (zh) "开机" else "On"),
                onButton = { sendPower(true) },
            )
        }
        return
    }

    // ===== 正常骨架 =====
    Box(Modifier.fillMaxSize().background(c.bg)) {
        Column(Modifier.fillMaxSize()) {
            // 会话控制卡（规格卡 §3.1）
            SessionControlCard(
                deviceName = session.deviceName,
                meta = metaText(session, snapshot, zh),
                badge = {
                    // 首页/返回当前单层导航等效（多层导航接入时需分化，评审 P3）
                    headBadge(session, unknownTail, secondsSince, zh)()
                },
                actions = listOf(
                    { SessionButton(if (zh) "返回" else "Back") { onBack() } },
                    { SessionButton(if (zh) "首页" else "Home") { onBack() } },
                    { SessionButton(if (zh) "固件升级" else "Firmware Update") { later(if (zh) "OTA" else "OTA") } },
                    { SessionButton(if (zh) "断开连接" else "Disconnect", danger = true) { confirmDisconnect = true } },
                ),
            )
            if (linkLost) {
                // 原型 disconnected 分支：居中断线视图替换全部 body
                CenterStateView(
                    glyph = "📵",
                    title = if (zh) "连接已断开" else "Disconnected",
                    subtitle = "",
                    buttonTitle = if (zh) "重新连接" else "Retry",
                    onButton = { scanModel.reconnectResident() },
                )
            } else {
                Column(
                    Modifier
                        .weight(1f)
                        .verticalScroll(rememberScrollState())
                        .padding(horizontal = 16.dp)
                        .padding(bottom = 24.dp),
                ) {
                    if (unknownTail) {
                        HktBanner(
                            BadgeKind.ERR,
                            "⚠︎ " + (if (zh) "响应数据异常（未知类型），已显示可解析字段"
                            else "Response abnormal (unknown type); parsed fields shown"),
                            if (zh) "导出日志" else "Export Log",
                        ) { later(if (zh) "日志" else "Log") }
                    }
                    SectionHeader(if (zh) "状态（实时轮询）" else "Status (live polling)")
                    StatusContent(session, snapshot, unknownTail, zh)
                    SectionHeader(if (zh) "设备操作" else "Device actions")
                    OpsPanel(powerRow = {
                        OpsPowerRow(
                            label = if (zh) "电源" else "Power",
                            stateText = if (snapshot.power == 1) (if (zh) "开启" else "On") else (if (zh) "已关机" else "Off"),
                            isOn = snapshot.power == 1,
                            onToggle = {
                                if (snapshot.power == 1) confirmPowerOff = true else sendPower(true)
                            },
                        )
                    }) {
                        if (session.family != DeviceFamily.SVC100) {
                            OpCard(
                                badge = "CAL", badgeKind = BadgeKind.WARN,
                                title = if (session.family == DeviceFamily.UDS100) (if (zh) "角度校准" else "Tilt Calibration")
                                else (if (zh) "磁力计校准" else "Mag Calibration"),
                                desc = if (zh) "环境检查 · 长时操作" else "Environment checks · long-running",
                            ) { later(if (zh) "校准" else "Calibration") }
                        }
                        if (session.family == DeviceFamily.DC200_FAMILY) {
                            OpCard(
                                badge = "MAG",
                                title = if (zh) "技术参数" else "Tech Parameters",
                                desc = if (zh) "三轴曲线 · 雷达频谱" else "3-axis curves · radar spectrum",
                            ) { later(if (zh) "技术参数" else "Tech") }
                        }
                        if (session.family == DeviceFamily.SVC100) {
                            OpCard(
                                badge = "TSK",
                                title = if (zh) "阀门任务" else "Valve Tasks",
                                desc = if (zh) "实时任务 · 定时任务表" else "Realtime task · schedule table",
                            ) { later(if (zh) "任务" else "Tasks") }
                        }
                        OpCard(
                            badge = "CFG",
                            title = if (zh) "参数配置" else "Configuration",
                            desc = if (zh) "上报 / 端口 / 时区" else "Reporting / port / timezone",
                        ) { later(if (zh) "配置" else "Config") }
                        if (session.family != DeviceFamily.DC200_FAMILY) {
                            // DC200 家族隐藏对时入口（用户裁决 2026-09-20：FD-002/FD-003 固件缺陷期）
                            val outcome = timeSyncOutcome
                            OpCard(
                                badge = "TIME", badgeKind = BadgeKind.OK,
                                title = if (zh) "时间同步" else "Time Sync",
                                desc = clockText(tick),
                                disabled = isSyncing || outcome == DeviceSession.TimeSyncOutcome.UNSUPPORTED,
                                trailing = {
                                    Text(
                                        pillText(isSyncing, outcome, zh),
                                        style = hkt(11f, FontWeight.Bold),
                                        color = c.info,
                                        modifier = Modifier
                                            .background(c.info.copy(alpha = 0.10f), androidx.compose.foundation.shape.CircleShape)
                                            .padding(horizontal = 8.dp, vertical = 6.dp),
                                    )
                                },
                            ) {
                                scope.launch { session.sendTimeSync() }
                            }
                        }
                    }
                }
            }
        }
        notice?.let { n ->
            Box(Modifier.align(Alignment.BottomCenter).padding(24.dp)) {
                Text(
                    n,
                    style = hkt(12f),
                    color = c.text2,
                    modifier = Modifier
                        .background(c.card.copy(alpha = 0.95f), androidx.compose.foundation.shape.RoundedCornerShape(8.dp))
                        .padding(horizontal = 12.dp, vertical = 8.dp),
                )
            }
        }
        // ===== 确认对话框（规格卡 §3.7）=====
        if (confirmPowerOff) {
            DialogScrim(onDismiss = { confirmPowerOff = false }) {
                HktDialogCard(
                    title = if (zh) "确认关机？" else "Power off?",
                    message = androidx.compose.ui.text.AnnotatedString(
                        if (zh) "关机后设备停止上报并断开连接、返回首页，可从「最近设备」再次开机。"
                        else "The device stops reporting and disconnects, and the app returns home. Power it on again from Recents.",
                    ),
                    buttons = listOf(
                        Triple(if (zh) "取消" else "Cancel", com.hkt.ble.bletools.designsystem.DialogButtonKind.SECONDARY) {
                            confirmPowerOff = false
                        },
                        Triple(if (zh) "确认关机" else "Power Off", com.hkt.ble.bletools.designsystem.DialogButtonKind.DANGER) {
                            confirmPowerOff = false
                            sendPower(false)
                        },
                    ),
                )
            }
        }
        if (confirmDisconnect) {
            DialogScrim(onDismiss = { confirmDisconnect = false }) {
                HktDialogCard(
                    title = if (zh) "确认断开连接？" else "Disconnect?",
                    message = androidx.compose.ui.text.AnnotatedString(
                        if (zh) "断开后将停止实时轮询并断开 BLE；这是预期断开，不会自动重连。设备会进入最近设备。"
                        else "Disconnecting stops live polling and disconnects BLE. This is an expected disconnect and will not auto-reconnect. The device will be kept in Recents.",
                    ),
                    buttons = listOf(
                        Triple(if (zh) "取消" else "Cancel", com.hkt.ble.bletools.designsystem.DialogButtonKind.SECONDARY) {
                            confirmDisconnect = false
                        },
                        Triple(if (zh) "确认断开" else "Disconnect", com.hkt.ble.bletools.designsystem.DialogButtonKind.DANGER) {
                            confirmDisconnect = false
                            scanModel.disconnectActive()
                            onDisconnect()
                        },
                    ),
                )
            }
        }
    }
}

/** 对话框遮罩（.mask rgba(0,0,0,.42) 全屏居中）。 */
@Composable
private fun DialogScrim(onDismiss: () -> Unit, content: @Composable () -> Unit) {
    Box(
        Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.42f))
            .clickableBox(onDismiss),
        contentAlignment = Alignment.Center,
    ) {
        Box(Modifier.clickableBox { /* 阻断穿透 */ }) { content() }
    }
}

/** 状态徽章：ready=ok / stale=warn / abnormal=err（规格卡 §3.1）。 */
private fun headBadge(
    session: DeviceSession,
    unknownTail: Boolean,
    secondsSince: Int?,
    zh: Boolean,
): @Composable () -> Unit {
    val abnormal = if (zh) "响应数据异常（未知类型），已显示可解析字段"
    else "Response abnormal (unknown type); parsed fields shown"
    if (unknownTail) {
        return { StateBadge(BadgeKind.ERR, abnormal) }
    }
    if (session.isStale) {
        val s = secondsSince ?: 0
        return {
            StateBadge(
                BadgeKind.WARN,
                if (zh) "最后更新 $s 秒前 · 正在重试" else "Updated ${s}s ago · retrying",
            )
        }
    }
    return { StateBadge(BadgeKind.OK, if (zh) "已连接 · 轮询正常" else "Connected · polling") }
}

private fun metaText(session: DeviceSession, snapshot: DeviceSnapshot, zh: Boolean): String {
    val fw = "v${snapshot.hardwareVersion}.${snapshot.softwareVersion}"
    val batt = if (session.family == DeviceFamily.UDS100) {
        snapshot.batteryVoltageMV?.let { (if (zh) "电压 " else "Voltage ") + "$it mV" }
            ?: "—"
    } else {
        snapshot.batteryPercent?.let { (if (zh) "电量 " else "Battery ") + "$it%" } ?: "—"
    }
    return (if (zh) "固件 " else "Firmware ") + fw + " · " + batt
}

/** 状态区（规格卡 §4：字段顺序=原型 defs，不可重排；unknownTail 时末字段值 "…"）。 */
@Composable
private fun StatusContent(session: DeviceSession, snapshot: DeviceSnapshot, unknownTail: Boolean, zh: Boolean) {
    when (session.family) {
        DeviceFamily.SVC100 -> {
            val mode = (snapshot.portFunction ?: 0) and 0x03
            val channels = listOf(
                SvcChannelSpec(
                    name = if (zh) "阀 1" else "V1",
                    on = snapshot.valve1State == 1,
                    onText = if (snapshot.valve1State == 1) (if (zh) "开启" else "On") else (if (zh) "关闭" else "Off"),
                    insertLabel = if (zh) "插入检测" else "Insert detect",
                    insertText = if (snapshot.valve1Inserted == 1) (if (zh) "是" else "Yes") else (if (zh) "否" else "No"),
                    pulseLabel = if (zh) "脉冲计数" else "Pulse count",
                    pulseText = "%,d".format(Locale.US, snapshot.valve1Pulse ?: 0),
                    portLabel = if (zh) "端口功能" else "Port function",
                    portText = if (mode == 1 || mode == 3) (if (zh) "开关控制" else "Switch control") else (if (zh) "PWM 控制" else "PWM control"),
                ),
                SvcChannelSpec(
                    name = if (zh) "阀 2" else "V2",
                    on = snapshot.valve2State == 1,
                    onText = if (snapshot.valve2State == 1) (if (zh) "开启" else "On") else (if (zh) "关闭" else "Off"),
                    insertLabel = if (zh) "插入检测" else "Insert detect",
                    insertText = if (snapshot.valve2Inserted == 1) (if (zh) "是" else "Yes") else (if (zh) "否" else "No"),
                    pulseLabel = if (zh) "脉冲计数" else "Pulse count",
                    pulseText = "%,d".format(Locale.US, snapshot.valve2Pulse ?: 0),
                    portLabel = if (zh) "端口功能" else "Port function",
                    portText = if (mode == 2 || mode == 3) (if (zh) "开关控制" else "Switch control") else (if (zh) "PWM 控制" else "PWM control"),
                ),
            )
            val tag = if ((snapshot.portFunction ?: 0) and 0x80 != 0) (if (zh) "含稳定时长" else "Stable time on")
            else (if (zh) "标准模式" else "Standard mode")
            Column {
                SvcChannelModule(title = if (zh) "阀状态" else "Valve state", tag = tag, channels = channels)
                Spacer(Modifier.height(8.dp))
            }
            FieldGrid(tailUnknown(svcFields(snapshot, zh), unknownTail))
        }
        DeviceFamily.UDS100 -> FieldGrid(tailUnknown(udsFields(snapshot, zh), unknownTail))
        DeviceFamily.DC200_FAMILY -> FieldGrid(tailUnknown(dcFields(snapshot, zh), unknownTail))
    }
}

private fun tailUnknown(fields: List<FieldSpec>, unknownTail: Boolean): List<FieldSpec> {
    if (!unknownTail || fields.isEmpty()) return fields
    return fields.dropLast(1) + FieldSpec(fields.last().label, "…")
}

private fun dcFields(s: DeviceSnapshot, zh: Boolean): List<FieldSpec> = listOf(
    FieldSpec(
        if (zh) "车位状态" else "Parking",
        when (s.parkState) {
            0 -> if (zh) "空位" else "Vacant"
            1 -> if (zh) "有车" else "Occupied"
            0xFF -> if (zh) "被遮挡" else "Covered"
            null -> "—"
            else -> if (zh) "未知" else "Unknown"
        },
    ),
    FieldSpec(
        if (zh) "工作模式" else "Work mode",
        when (s.parkMode) {
            0 -> if (zh) "融合模式" else "Fusion"
            1 -> if (zh) "仅地磁" else "Mag only"
            2 -> if (zh) "雷达优先" else "Radar first"
            null -> "—"
            else -> if (zh) "未知" else "Unknown"
        },
    ),
    FieldSpec(if (zh) "防拆状态" else "Tamper", triggeredText(s.tamper, zh)),
    FieldSpec(if (zh) "上报周期" else "Report period", minutesEmbedded(s.reportPeriodMin, zh)),
)

private fun udsFields(s: DeviceSnapshot, zh: Boolean): List<FieldSpec> = listOf(
    FieldSpec(if (zh) "温度" else "Temperature", milli3(s.temperatureMilli), "°C"),
    FieldSpec(if (zh) "湿度" else "Humidity", milli3(s.humidityMilli), "%"),
    FieldSpec(if (zh) "距离" else "Distance", ofInt(s.distanceMM), "mm"),
    FieldSpec(
        if (zh) "满溢状态" else "Overflow",
        when (s.overflowState) {
            0 -> if (zh) "正常" else "Normal"
            1 -> if (zh) "低阈值触发" else "Low threshold triggered"
            2 -> if (zh) "高阈值触发" else "High threshold triggered"
            0xFF -> if (zh) "无效" else "Invalid"
            null -> "—"
            else -> if (zh) "未知" else "Unknown"
        },
    ),
    FieldSpec(if (zh) "低阈值" else "Low threshold", ofInt(s.lowThresholdMM), "mm"),
    FieldSpec(if (zh) "高阈值" else "High threshold", ofInt(s.highThresholdMM), "mm"),
    FieldSpec(if (zh) "倾角" else "Tilt angle", centi2(s.angleCenti), "°"),
    FieldSpec(if (zh) "倾斜" else "Slant", flagText(s.slant, zh)),
    FieldSpec(if (zh) "温湿告警" else "HT alarm", triggeredText(s.htAlarm, zh)),
    FieldSpec(if (zh) "GPS 周期" else "GPS period", minutesEmbedded(s.gpsPeriodMin, zh)),
    FieldSpec(if (zh) "上报周期" else "Report period", minutesEmbedded(s.reportPeriodMin, zh)),
    FieldSpec(if (zh) "纬度" else "Latitude", fixed4(s.latitude)),
    FieldSpec(if (zh) "经度" else "Longitude", fixed4(s.longitude)),
)

private fun svcFields(s: DeviceSnapshot, zh: Boolean): List<FieldSpec> = listOf(
    FieldSpec(
        if (zh) "输出电压档" else "Voltage level",
        when (s.voltageLevel) {
            0 -> "12V"
            1 -> "9V"
            2 -> "5V"
            null -> "—"
            else -> "?"
        },
    ),
    FieldSpec(if (zh) "稳定时长" else "Stable time", ofInt(s.stableTimeS), "s"),
    FieldSpec(
        if (zh) "自动开关机" else "Auto power",
        when (s.smartPower) {
            1 -> if (zh) "自动" else "Auto"
            0 -> if (zh) "手动" else "Manual"
            else -> "—"
        },
    ),
    FieldSpec(
        if (zh) "时区" else "Time zone",
        // 原型：25=UTC+03:30，26=UTC+05:30，<13=UTC+{n}，否则=UTC−{n-12}（无 :00 后缀）
        when (val tz = s.timezone) {
            25 -> "UTC+03:30"
            26 -> "UTC+05:30"
            null -> "—"
            else -> if (tz < 13) "UTC+$tz" else if (tz <= 24) "UTC−${tz - 12}" else "—"
        },
    ),
    FieldSpec(if (zh) "上报周期" else "Report period", ofInt(s.reportPeriodMin), if (zh) "分钟" else "min"),
)

private fun triggeredText(value: Int?, zh: Boolean) = when (value) {
    1 -> if (zh) "已触发" else "Triggered"
    0 -> if (zh) "正常" else "Normal"
    else -> if (zh) "未知" else "Unknown"
}

private fun flagText(value: Int?, zh: Boolean) = when (value) {
    1 -> if (zh) "是" else "Yes"
    0 -> if (zh) "否" else "No"
    else -> if (zh) "未知" else "Unknown"
}

/// `{n} 分钟`：DC/UDS 的周期字段单位嵌在值文本里（原型 fval f_period/f_gps）。
private fun minutesEmbedded(value: Int?, zh: Boolean): String =
    value?.let { "$it ${if (zh) "分钟" else "min"}" } ?: "—"

private fun milli3(value: Int?): String = value?.let { String.format(Locale.US, "%.3f", it / 1000.0) } ?: "—"
private fun centi2(value: Int?): String = value?.let { String.format(Locale.US, "%.2f", it / 100.0) } ?: "—"
private fun fixed4(value: Double?): String = value?.let { String.format(Locale.US, "%.4f", it) } ?: "—"
private fun ofInt(value: Int?): String = value?.toString() ?: "—"

private val clockFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US)

/// 原型 liveClockStr 固定格式。
private fun clockText(tickMs: Long): String = clockFormat.format(Date(tickMs))

/// TIME 卡胶囊文案状态机（规格卡 §3.6-4）：同步中/完成(2.6s 回落)/已发送/不支持/未确认/同步。
private fun pillText(
    isSyncing: Boolean,
    outcome: DeviceSession.TimeSyncOutcome?,
    zh: Boolean,
): String = when {
    isSyncing -> if (zh) "同步中" else "Syncing"
    outcome == DeviceSession.TimeSyncOutcome.ACKNOWLEDGED -> if (zh) "完成" else "Done"
    outcome == DeviceSession.TimeSyncOutcome.SENT -> if (zh) "已发送" else "Sent"
    outcome == DeviceSession.TimeSyncOutcome.UNSUPPORTED -> if (zh) "不支持" else "N/A"
    outcome == DeviceSession.TimeSyncOutcome.FAILED -> if (zh) "未确认" else "No ACK"
    else -> if (zh) "同步" else "Sync"
}
