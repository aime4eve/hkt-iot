package com.hkt.ble.bletools.ui

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.hkt.ble.bletools.core.ble.DeviceSession
import com.hkt.ble.bletools.core.protocol.LoRaWANFrame
import com.hkt.ble.bletools.designsystem.ConfigControlRow
import com.hkt.ble.bletools.designsystem.ConfigSection
import com.hkt.ble.bletools.designsystem.DialogButton
import com.hkt.ble.bletools.designsystem.DialogButtonKind
import com.hkt.ble.bletools.designsystem.HktSwitch
import com.hkt.ble.bletools.designsystem.LinkButton
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.NavbarHeader
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.LanguageStore
import com.hkt.ble.bletools.model.LogStore
import kotlinx.coroutines.launch
import java.util.Locale

/**
 * 蓝牙调试模式页（0xA2，LoRaWAN_ParkingSensor 固件 ≥ 60.X，2026-11）。
 *
 * 隐藏入口：详情页头部「固件 v…·电量…」meta 连点 10 次（计数与版本闸门在 DeviceDetailScreen）。
 * - 两个开关只做本地选择，选定后按「下发调试配置」一次下发 0xA2（17B 定长帧，仅蓝牙），
 *   等 0xFF ACK；超时/被拒回滚开关显示
 * - 数据接收窗口：固件 system_debug 周期透传文本经 rawTextHandler 追加（保留最近 [MAX_LINES] 行，
 *   行首加 APP 接收时刻）
 * - 生命周期与 LoRaWANConfigScreen 同纪律：进页停轮询（FD-004）+ 独占 rawTextHandler，离开释放；
 *   固件侧断开蓝牙自动全关，本页开关是本次连接的本地视图
 */
@Composable
fun BleDebugScreen(session: DeviceSession, onBack: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    val scope = rememberCoroutineScope()

    var debugOn by remember { mutableStateOf(false) }      // 乐观显示值（ACK 前即可见）
    var sleepOn by remember { mutableStateOf(false) }
    var ackedDebug by remember { mutableStateOf(false) }   // 最近一次确认值（失败回滚基准）
    var ackedSleep by remember { mutableStateOf(false) }
    var busy by remember { mutableStateOf(false) }
    var statusErr by remember { mutableStateOf<String?>(null) }
    val lines = remember { mutableStateListOf<String>() }  // 接收窗口环形缓冲
    val seq = remember { mutableIntStateOf(0) }
    val listState = rememberLazyListState()

    DisposableEffect(Unit) {
        session.setPollingSuspended(true)
        session.rawTextHandler = { data ->
            val text = data.decodeToString().trim()
            if (text.isNotEmpty()) {
                if (lines.size >= MAX_LINES) lines.removeAt(0)
                lines.add(stamp() + text)
            }
            true
        }
        onDispose {
            session.rawTextHandler = null
            session.setPollingSuspended(false)
        }
    }
    BackHandler { onBack() }

    // 新行自动滚到底（跟随透传流）
    LaunchedEffect(lines.size) {
        if (lines.isNotEmpty()) listState.animateScrollToItem(lines.size - 1)
    }

    fun apply(newDebug: Boolean, newSleep: Boolean) {
        if (busy) return
        busy = true
        statusErr = null
        scope.launch {
            LogStore.info(
                if (zh) "0xA2 调试模式 debug=${if (newDebug) 1 else 0} sleep=${if (newSleep) 1 else 0} → ${session.deviceName}"
                else "0xA2 ble debug debug=${if (newDebug) 1 else 0} sleep=${if (newSleep) 1 else 0} → ${session.deviceName}"
            )
            val acked = session.sendRawAwaitingAck(LoRaWANFrame.bleDebugFrame(++seq.intValue, newDebug, newSleep))
            busy = false
            if (acked) {
                ackedDebug = newDebug
                ackedSleep = newSleep
            } else {
                debugOn = ackedDebug   // 无 ACK（超时/帧被拒）：回滚到最近确认值
                sleepOn = ackedSleep
                statusErr = if (zh) "无应答（超时或被固件拒绝），已回滚" else "No ACK (timeout or rejected), reverted"
            }
            LogStore.info(if (acked) "0xA2 ACK" else "0xA2 ACK timeout")
        }
    }

    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(
            title = if (zh) "蓝牙调试模式" else "BLE Debug",
            backText = if (zh) "‹ 返回" else "‹ Back",
            onBack = onBack,
        )
        Column(
            Modifier
                .fillMaxWidth()
                .weight(1f)
                .padding(horizontal = 16.dp)
                .padding(top = 12.dp, bottom = 16.dp),
        ) {
            ConfigSection(title = if (zh) "模式控制" else "Mode Control") {
                Box {
                    ConfigControlRow(if (zh) "蓝牙调试模式" else "Debug Stream") { HktSwitch(isOn = debugOn) }
                    Box(Modifier.matchParentSize().clickableBox(enabled = !busy) { debugOn = !debugOn })
                }
                Box {
                    ConfigControlRow(if (zh) "连接时允许睡眠" else "Sleep When Connected") { HktSwitch(isOn = sleepOn) }
                    Box(Modifier.matchParentSize().clickableBox(enabled = !busy) { sleepOn = !sleepOn })
                }
                // 两开关选定后统一一次下发（2026-11-15 用户裁决：不再逐开关即时发）
                Box(Modifier.padding(top = 9.dp)) {
                    DialogButton(
                        title = when {
                            busy -> if (zh) "下发中…" else "Sending…"
                            else -> if (zh) "下发调试配置" else "Apply"
                        },
                        kind = DialogButtonKind.PRIMARY,
                    ) {
                        if (!busy) apply(debugOn, sleepOn)
                    }
                }
                Text(
                    when {
                        busy -> if (zh) "正在下发 0xA2…" else "Sending 0xA2…"
                        statusErr != null -> "✕ $statusErr"
                        else -> {
                            val d = if (ackedDebug) (if (zh) "开" else "on") else (if (zh) "关" else "off")
                            val s = if (ackedSleep) (if (zh) "允许睡眠" else "sleep ok") else (if (zh) "不睡眠" else "no sleep")
                            (if (zh) "当前生效：调试$d · 连接$s · 断开蓝牙自动全关" else "Active: debug $d · $s · auto-off on disconnect")
                        }
                    },
                    style = hkt(11f),
                    color = if (statusErr != null) c.err else c.text2,
                    modifier = Modifier.padding(top = 8.dp),
                )
            }
            Spacer(Modifier.height(12.dp))
            ConfigSection(title = if (zh) "数据接收" else "Data Stream") {
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Text(
                        if (zh) "${lines.size} 条" else "${lines.size} lines",
                        style = hkt(11f),
                        color = c.text2,
                        modifier = Modifier.weight(1f),
                    )
                    LinkButton(title = if (zh) "清空" else "Clear") { lines.clear() }
                }
            }
            Spacer(Modifier.height(8.dp))
            // 接收窗口（等宽控制台样式；ConfigSection content 无 ColumnScope，故置于卡片外取 weight）
            val shape = RoundedCornerShape(8.dp)
            Box(
                Modifier
                    .fillMaxWidth()
                    .weight(1f)
                    .background(c.card2, shape)
                    .border(1.dp, c.line, shape)
                    .padding(8.dp),
            ) {
                if (lines.isEmpty()) {
                    Text(
                        if (zh) "开启蓝牙调试模式后，设备透传数据将显示在此" else "Device passthrough appears here after debug mode is on",
                        style = hkt(12f),
                        color = c.text2,
                        textAlign = TextAlign.Center,
                        modifier = Modifier.align(Alignment.Center).padding(8.dp),
                    )
                } else {
                    LazyColumn(state = listState, modifier = Modifier.fillMaxSize()) {
                        items(lines) { line ->
                            Text(
                                line,
                                style = androidx.compose.ui.text.TextStyle(
                                    fontSize = 11.sp,
                                    fontFamily = FontFamily.Monospace,
                                    lineHeight = 15.sp,
                                ),
                                color = c.text,
                            )
                        }
                    }
                }
            }
        }
    }
}

/** 接收窗口行首时刻戳（APP 本地接收时刻，HH:mm:ss.SSS）。 */
private fun stamp(): String {
    val now = java.util.Calendar.getInstance()
    return "[%02d:%02d:%02d.%03d] ".format(
        Locale.US,
        now.get(java.util.Calendar.HOUR_OF_DAY), now.get(java.util.Calendar.MINUTE),
        now.get(java.util.Calendar.SECOND), now.get(java.util.Calendar.MILLISECOND),
    )
}

private const val MAX_LINES = 300
