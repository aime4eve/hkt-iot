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
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.text.style.TextAlign
import com.hkt.ble.bletools.core.ble.DeviceSession
import com.hkt.ble.bletools.core.protocol.DeviceFamily
import com.hkt.ble.bletools.designsystem.BadgeKind
import com.hkt.ble.bletools.designsystem.CalStepRow
import com.hkt.ble.bletools.designsystem.DialogButton
import com.hkt.ble.bletools.designsystem.DialogButtonKind
import com.hkt.ble.bletools.designsystem.HktCard
import com.hkt.ble.bletools.designsystem.HktDialogCard
import com.hkt.ble.bletools.designsystem.HktProgress
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.NavbarHeader
import com.hkt.ble.bletools.designsystem.StateBadge
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.model.LanguageStore
import com.hkt.ble.bletools.designsystem.hkt
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

/**
 * 校准页（P_cal）—— 克隆冻结原型 + 用户 2026-09-10 三条流程裁决（规格卡 P-cal.md §5）：
 * 1. 无二次确认，点「开始校准」直接开始；
 * 2. 成功 → 显示成功信息，停顿 3 秒自动返回详情页；
 * 3. 失败 → 弹窗询问是否再次尝试，取消则返回。
 * 真实流程：0xFD 触发（设备立即 ACK）→ 设备端执行 → "Calibration Done" 文本上报驱动成功；
 * 超时（UDS 120s / DC 180s，Android 同源）走失败分支。等待页内容居中、取消贴底（与 P-02 同风格）。
 */
@Composable
fun CalibrationScreen(session: DeviceSession, onBack: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    val scope = rememberCoroutineScope()
    var state by remember { mutableStateOf(CalState.IDLE) }
    var elapsed by remember { mutableIntStateOf(0) }
    var showRetry by remember { mutableStateOf(false) }
    var ticker by remember { mutableStateOf<Job?>(null) }
    var calJob by remember { mutableStateOf<Job?>(null) }

    val isUDS = session.family == DeviceFamily.UDS100

    fun stopTimer() {
        ticker?.cancel()
        ticker = null
    }

    fun begin() {
        state = CalState.RUNNING
        elapsed = 0
        stopTimer()
        ticker = scope.launch {
            while (true) {
                delay(1_000)
                elapsed += 1
            }
        }
        calJob = scope.launch {
            val outcome = session.startCalibration()
            stopTimer()
            state = if (outcome == DeviceSession.CalibrationOutcome.DONE) CalState.SUCCESS else CalState.FAILURE
        }
    }

    // 中途退出=放弃等待上报，设备端校准继续（iOS onDisappear 同构）
    DisposableEffect(Unit) {
        onDispose {
            stopTimer()
            calJob?.cancel()
            session.cancelCalibrationWait()
        }
    }
    // 系统返回与页面返回同语义
    BackHandler { stopTimer(); onBack() }

    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(
            title = if (zh) "校准" else "Calibration",
            backText = if (zh) "‹ 返回" else "‹ Back",
            onBack = { stopTimer(); onBack() },
        )
        when (state) {
            CalState.RUNNING -> {
                // 裁决：等待页内容居中、取消贴底（与连接覆盖层同风格）
                Column(Modifier.fillMaxSize()) {
                    Box(Modifier.weight(1f), contentAlignment = Alignment.Center) {
                        HktCard(modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp)) {
                            Column(
                                horizontalAlignment = Alignment.CenterHorizontally,
                                verticalArrangement = Arrangement.spacedBy(6.dp),
                                modifier = Modifier.fillMaxWidth().padding(13.dp),
                            ) {
                                StateBadge(BadgeKind.INFO, if (zh) "校准进行中…（以设备上报为准）" else "Calibrating… (as reported by the device)")
                                Text(
                                    if (zh) "已用时 $elapsed 秒 · " + (if (isUDS) "预计约 90 秒，请耐心等待" else "磁力计校准耗时较长（最长约 3 分钟），请耐心等待")
                                    else "Elapsed ${elapsed}s · " + (if (isUDS) "Expected ~90 seconds, please wait" else "Magnetometer calibration can take up to 3 minutes"),
                                    style = hkt(13f),
                                    color = c.text2,
                                    textAlign = TextAlign.Center,
                                )
                                HktProgress(fraction = (elapsed / 8f))
                            }
                        }
                    }
                    // 取消按钮：贴底全宽（与 P-02 取消同规格）
                    Box(Modifier.fillMaxWidth().padding(24.dp)) {
                        DialogButton(title = if (zh) "取消" else "Cancel", kind = DialogButtonKind.SECONDARY) {
                            stopTimer()
                            onBack()
                        }
                    }
                }
            }
            CalState.SUCCESS -> {
                // 裁决 2：显示成功信息，停顿 3 秒自动返回
                LaunchedEffect(Unit) {
                    delay(3_000)
                    onBack()
                }
                Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(10.dp),
                        modifier = Modifier.padding(horizontal = 32.dp),
                    ) {
                        Text("✓", fontSize = 46.sp, color = c.ok)
                        Text(if (zh) "校准完成" else "Calibration complete", style = hkt(17f, FontWeight.SemiBold), color = c.text)
                    }
                }
            }
            CalState.FAILURE -> {
                // 裁决 3：弹窗询问是否再次尝试
                LaunchedEffect(Unit) { showRetry = true }
                Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(10.dp),
                        modifier = Modifier.padding(horizontal = 32.dp),
                    ) {
                        Text("✕", fontSize = 46.sp, color = c.err)
                        Text(if (zh) "校准未成功" else "Calibration failed", style = hkt(17f, FontWeight.SemiBold), color = c.text)
                    }
                    if (showRetry) {
                        Box(
                            Modifier.fillMaxSize().background(androidx.compose.ui.graphics.Color.Black.copy(alpha = 0.42f))
                                .clickableBox { onBack() },
                            contentAlignment = Alignment.Center,
                        ) {
                            HktDialogCard(
                                title = if (zh) "校准未成功" else "Calibration failed",
                                message = androidx.compose.ui.text.AnnotatedString(
                                    if (zh) "本次校准未成功。是否再次尝试校准？"
                                    else "This calibration attempt did not succeed. Try again?"),
                                buttons = listOf(
                                    Triple(if (zh) "取消" else "Cancel", DialogButtonKind.SECONDARY) {
                                        stopTimer(); onBack()
                                    },
                                    Triple(if (zh) "再次尝试" else "Try Again", DialogButtonKind.PRIMARY) {
                                        showRetry = false
                                        begin()
                                    },
                                ),
                            )
                        }
                    }
                }
            }
            CalState.IDLE -> {
                // idle 指引卡片 + 裁决 1：无二次确认，直接开始
                Column(
                    Modifier
                        .fillMaxSize()
                        .verticalScroll(rememberScrollState())
                        .padding(horizontal = 16.dp)
                        .padding(bottom = 24.dp),
                ) {
                    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                        HktCard(modifier = Modifier.fillMaxWidth()) {
                            Column(Modifier.padding(4.dp)) {
                                Row(
                                    verticalAlignment = Alignment.CenterVertically,
                                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                                    modifier = Modifier.padding(bottom = 10.dp),
                                ) {
                                    Text("🧭", style = hkt(16f))
                                    Text(
                                        if (isUDS) (if (zh) "倾角校准" else "Tilt Calibration") else (if (zh) "磁力计校准" else "Magnetometer Calibration"),
                                        style = hkt(16f, FontWeight.Bold),
                                        color = c.text,
                                    )
                                }
                                guideSteps(isUDS, zh).forEachIndexed { index, step ->
                                    CalStepRow(index = index + 1, title = step.first, text = step.second)
                                }
                                Box(Modifier.padding(top = 10.dp)) {
                                    Column {
                                        Box(Modifier.fillMaxWidth().height(1.dp).background(c.line))
                                        Text(
                                            "⚠︎ " + (if (zh) "校准为长时操作，期间其他命令已禁用"
                                            else "Long-running operation; other commands are disabled meanwhile"),
                                            style = hkt(13f).copy(lineHeight = (13f * 1.6f).sp),
                                            color = c.text2,
                                        )
                                    }
                                }
                            }
                        }
                        DialogButton(title = if (zh) "开始校准" else "Start Calibration", kind = DialogButtonKind.PRIMARY) {
                            begin()
                        }
                    }
                }
            }
        }
    }
}

private enum class CalState { IDLE, RUNNING, SUCCESS, FAILURE }

/// 校准指引步骤（UDS 3 条纯文本 / DC 7 条带标题——规格卡 P-cal §1）。
private fun guideSteps(isUDS: Boolean, zh: Boolean): List<Pair<String?, String>> = if (isUDS) {
    listOf(
        null to (if (zh) "避免强磁与金属台面干扰；清理设备周围杂物" else "Avoid strong magnets and metal surfaces; clear debris around the device"),
        null to (if (zh) "设备水平静止放置" else "Place the device level and still"),
        null to (if (zh) "开始后保持设备完全静止，等待完成提示" else "After starting, keep the device completely still until prompted"),
    )
} else {
    listOf(
        (if (zh) "邻位车辆" else "Neighboring vehicles") to (if (zh) "相邻车位 2 米内无车辆停放，理想时机为安装后车位尚空时" else "No vehicles parked within 2 m of adjacent spaces; ideally calibrate while the parking space is still empty after installation"),
        (if (zh) "固定铁磁结构" else "Fixed ferromagnetic structures") to (if (zh) "车位 1 米内无井盖、灯柱基座、裸露钢筋、消防栓" else "No manhole covers, lamp-post bases, exposed rebar, or fire hydrants within 1 m of the parking space"),
        (if (zh) "通电电缆" else "Energized cables") to (if (zh) "距地埋电缆、配电箱 1 米以上" else "Stay more than 1 m from buried cables and distribution boxes"),
        (if (zh) "随身磁物" else "Personal magnetic items") to (if (zh) "磁吸手机支架、钥匙串、机械手表等距设备 0.5 米以上" else "Keep magnetic phone mounts, keychains, mechanical watches, and similar items at least 0.5 m from the device"),
        (if (zh) "设备状态" else "Device status") to (if (zh) "传感器已按规范水平嵌入路面，校准期间不得触碰" else "The sensor is embedded level in the road surface per specification; do not touch it during calibration"),
        (if (zh) "触发并等待" else "Trigger and wait") to (if (zh) "指令触发后人员退开 2 米，等待自动完成，勿重复触发" else "After triggering the command, move at least 2 m away and wait for automatic completion; do not trigger repeatedly"),
        (if (zh) "验证与失败处理" else "Verification and failure handling") to (if (zh) "完成上报后做一次停车 / 驶离验证；失败时先排查邻位车辆、井盖等干扰源，排除后再重试，勿原地反复重试" else "After completion is reported, perform one parking/departure verification; if it fails, first check interference sources such as adjacent vehicles and manhole covers, retry only after clearing them, and do not retry repeatedly in place"),
    )
}
