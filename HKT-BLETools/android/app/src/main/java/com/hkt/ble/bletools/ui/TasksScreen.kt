package com.hkt.ble.bletools.ui

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.background
import androidx.compose.foundation.border
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
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.hkt.ble.bletools.core.ble.DeviceSession
import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.HKTFrameEncoder
import com.hkt.ble.bletools.designsystem.BadgeKind
import com.hkt.ble.bletools.designsystem.DialogButton
import com.hkt.ble.bletools.designsystem.DialogButtonKind
import com.hkt.ble.bletools.designsystem.HktBanner
import com.hkt.ble.bletools.designsystem.HktCard
import com.hkt.ble.bletools.designsystem.HktProgress
import com.hkt.ble.bletools.designsystem.HktRadius
import com.hkt.ble.bletools.designsystem.LinkButton
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.NavbarHeader
import com.hkt.ble.bletools.designsystem.SectionHeader
import com.hkt.ble.bletools.designsystem.StateBadge
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.LanguageStore
import com.hkt.ble.bletools.model.LogStore
import com.hkt.ble.bletools.model.ValveTaskStore
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import java.util.Locale

/**
 * 阀门任务页（P_tasks）—— 1:1 克隆冻结原型（规格卡 docs/ios/design/ui-spec/P-tasks.md）。
 * 列表：实时任务卡(0x03) + 定时任务镜像表(0x04/0x05)；编辑页：槽位/阀门/动作/脉冲/时间/重复。
 * 真实收发：设备 ACK 驱动（0x03 设备忙时静默忽略=超时，显示 busy 横幅；镜像只在确认后记录）。
 * 实时开关语义对照固件 control_center.c:262：执行=保持命令状态至时长到/脉冲满，
 * 结束设备自动反向复位且无上报；仅定时任务执行中才忙（新 0x03 立即覆盖当前动作）。
 */
@Composable
fun TasksScreen(session: DeviceSession, onBack: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    var showEdit by remember { mutableStateOf(false) }

    if (showEdit) {
        TaskEditScreen(session = session, onSaved = { showEdit = false }, onCancel = { showEdit = false })
        return
    }

    // 系统返回=回详情（会话不受影响）
    BackHandler { onBack() }

    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(
            title = if (zh) "阀门任务" else "Valve Tasks",
            backText = if (zh) "‹ 返回" else "‹ Back",
            onBack = onBack,
        ) {
            LinkButton(title = "＋ " + (if (zh) "新建任务" else "New Task")) { newTaskDefaults(); showEdit = true }
        }
        Column(
            Modifier
                .weight(1f)
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 16.dp)
                .padding(bottom = 24.dp),
        ) {
            RealtimeCard(session, zh)
            SectionHeader((if (zh) "定时任务表（1–16）" else "Schedule Table (1–16)") + " · 0x04 / 0x05")
            HktBanner(
                BadgeKind.INFO,
                "ℹ︎ " + (if (zh) "设备无 BLE 回读命令，以下为本机写入记录的镜像（固件仅支持 LoRa 平台侧回读 0x3D）"
                else "No BLE read-back; this list mirrors what was written from this phone (firmware exposes 0x3D via LoRa only)"),
            )
            TaskList(session, zh)
        }
    }
}

/** 新建任务草稿初值：取未占用最小槽位（iOS newTask 同构：周一~周五 08:00–18:30 脉冲 100）。 */
private fun newTaskDefaults() {
    var slot = 1
    while (ValveTaskStore.isUsed(slot) && slot < 16) slot += 1
    TaskDraft.slot = slot
    TaskDraft.valve = 1
    TaskDraft.state = 1
    TaskDraft.pulseText = "100"
    TaskDraft.sh = 8; TaskDraft.sm = 0; TaskDraft.eh = 18; TaskDraft.em = 30
    TaskDraft.resetDays()
}

/** 编辑页草稿（对象级状态：编辑页与新建入口解耦，iOS @State editDraft 同构）。 */
private object TaskDraft {
    var slot by mutableStateOf(1)
    var valve by mutableStateOf(1)
    var state by mutableStateOf(1)
    var pulseText by mutableStateOf("100")
    var sh by mutableIntStateOf(8)
    var sm by mutableIntStateOf(0)
    var eh by mutableIntStateOf(18)
    var em by mutableIntStateOf(30)
    val days = mutableStateListOf(true, true, true, true, true, false, false)

    fun resetDays() {
        days.clear()
        days.addAll(listOf(true, true, true, true, true, false, false))
    }
}

// MARK: - 定时任务镜像表

/** 任务表：空态卡 / 任务卡 ×N + 全部删除（0x05 0xFF；ACK 才更新镜像）。 */
@Composable
private fun TaskList(session: DeviceSession, zh: Boolean) {
    val c = LocalHktColors.current
    val scope = rememberCoroutineScope()
    if (ValveTaskStore.tasks.isEmpty()) {
        HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 9.dp)) {
            Text(
                if (zh) "暂无定时任务" else "No scheduled tasks",
                style = hkt(13f),
                color = c.text2,
                modifier = Modifier.fillMaxWidth().padding(vertical = 24.dp),
                textAlign = TextAlign.Center,
            )
        }
        return
    }
    ValveTaskStore.tasks.forEach { task ->
        TaskCard(task = task, zh = zh, onDelete = { deleteTask(task.id, zh, session, scope) })
    }
    val shape = RoundedCornerShape(HktRadius.control.dp)
    Box(
        Modifier
            .fillMaxWidth()
            .background(c.err.copy(alpha = 0.10f), shape)
            .border(1.dp, c.err.copy(alpha = 0.22f), shape)
            .clickableBox { deleteAllTasks(zh, session, scope) }
            .padding(vertical = 13.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            if (zh) "全部删除 (0xFF)" else "Delete All (0xFF)",
            style = hkt(16f, FontWeight.SemiBold),
            color = c.err,
        )
    }
    Text(
        if (zh) "删除执行中的任务将强制停止阀门动作" else "Deleting a running task force-stops the valve",
        style = hkt(13f),
        color = c.text2,
        modifier = Modifier.fillMaxWidth().padding(top = 6.dp),
        textAlign = TextAlign.Center,
    )
}

@Composable
private fun TaskCard(task: ValveTaskStore.Task, zh: Boolean, onDelete: () -> Unit) {
    val c = LocalHktColors.current
    val dayNames = if (zh) listOf("一", "二", "三", "四", "五", "六", "日") else listOf("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
    val valveNames = listOf(if (zh) "双阀" else "Both", if (zh) "阀 1" else "Valve 1", if (zh) "阀 2" else "Valve 2")
    HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 11.dp)) {
        Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("#${task.id}", style = hkt(15f, FontWeight.Bold), color = c.text)
                Box(Modifier.padding(start = 8.dp)) {
                    StateBadge(
                        BadgeKind.INFO,
                        valveNames[task.valve] + " · " + (if (task.state == 1) (if (zh) "开阀" else "Open") else (if (zh) "关阀" else "Close")),
                    )
                }
                Spacer(Modifier.weight(1f))
                Text(
                    if (zh) "删除" else "Delete",
                    style = hkt(14f, FontWeight.SemiBold),
                    color = c.err,
                    modifier = Modifier.clickableBox(onDelete),
                )
            }
            Text(
                String.format(Locale.US, "⏰ %02d:%02d – %02d:%02d · ", task.sh, task.sm, task.eh, task.em) +
                    (if (zh) "脉冲数 " else "Pulse count ") + task.pulse,
                style = hkt(13f),
                color = c.text2,
            )
            Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                task.days.forEachIndexed { index, on ->
                    if (on) {
                        Text(
                            dayNames[index],
                            style = hkt(12f, FontWeight.SemiBold),
                            color = c.info,
                            modifier = Modifier
                                .background(c.info.copy(alpha = 0.15f), RoundedCornerShape(50))
                                .padding(horizontal = 7.dp, vertical = 2.dp),
                        )
                    }
                }
            }
        }
    }
}

private fun deleteTask(id: Int, zh: Boolean, session: DeviceSession, scope: CoroutineScope) {
    scope.launch {
        val acked = session.sendWrite(CommandCode.SVC_DELETE_TASK, HKTFrameEncoder.svcDeleteTaskPayload(id))
        if (!acked) {
            LogStore.info("0x05 " + (if (zh) "无 ACK，未删除 #$id" else "no ACK, #$id not deleted"))
            return@launch
        }
        ValveTaskStore.delete(id)
        LogStore.info("0x05 " + (if (zh) "删除任务" else "delete task") + " #$id ACK")
    }
}

private fun deleteAllTasks(zh: Boolean, session: DeviceSession, scope: CoroutineScope) {
    scope.launch {
        val acked = session.sendWrite(CommandCode.SVC_DELETE_TASK, HKTFrameEncoder.svcDeleteTaskPayload(0xFF))
        if (!acked) {
            LogStore.info("0x05 " + (if (zh) "全部删除无 ACK" else "delete ALL no ACK"))
            return@launch
        }
        ValveTaskStore.deleteAll()
        LogStore.info("0x05 " + (if (zh) "全部删除 (0xFF) ACK" else "delete ALL (0xFF) ACK"))
    }
}

// MARK: - 实时任务卡（0x03）

private enum class RtPhase { SENDING, EXECUTING, DONE }

/** 实时开关反馈状态机（iOS RTExec 值类型移植：remain 自减走 copy()，守卫按 phase 判定而非实例相等）。 */
private data class RtExec(
    val phase: RtPhase,
    val valve: Int,
    val state: Int,
    val dur: Int,
    val pulse: Int,
    val remain: Int,
    val flash: String? = null,
)

@Composable
private fun RealtimeCard(session: DeviceSession, zh: Boolean) {
    val c = LocalHktColors.current
    val snapshot by session.snapshot.collectAsState()
    val scope = rememberCoroutineScope()

    var rt by remember { mutableStateOf<RtExec?>(null) }
    var busyVisible by remember { mutableStateOf(false) }
    var advOpen by remember { mutableStateOf(false) }
    var rtDur by remember { mutableStateOf("60") }     // 高级：定时回位秒数（60s 后设备自动复位）
    var rtPulse by remember { mutableStateOf("100") }  // 高级：脉冲数（计满自动停止）
    var countdownJob by remember { mutableStateOf<Job?>(null) }

    DisposableEffect(Unit) {
        onDispose {
            countdownJob?.cancel()   // 页面退出：本地倒计时镜像随之失效（设备侧继续，无需取消命令）
            rt = null
        }
    }

    val v1 = (snapshot.valve1State ?: 0) == 1
    val v2 = (snapshot.valve2State ?: 0) == 1

    fun valveName(valve: Int): String =
        if (valve == 0) (if (zh) "双阀" else "Both") else if (valve == 1) (if (zh) "阀 1" else "Valve 1") else (if (zh) "阀 2" else "Valve 2")

    fun sendExec(valve: Int, state: Int) {
        if (rt?.phase == RtPhase.SENDING) return
        val dur = rtDur.toIntOrNull() ?: 0
        val pulse = rtPulse.toIntOrNull() ?: 0
        if (dur !in 0..65535 || pulse !in 0..65535) return
        countdownJob?.cancel()   // 新命令立即覆盖旧动作：旧本地倒计时不再生效
        busyVisible = false
        val cmd = RtExec(RtPhase.SENDING, valve, state, dur, pulse, dur)
        rt = cmd
        LogStore.info(
            "0x03 " + if (zh) "执行 valve=$valve state=$state dur=$dur pulse=$pulse"
            else "exec valve=$valve state=$state dur=$dur pulse=$pulse",
        )
        scope.launch {
            val acked = session.sendWrite(
                CommandCode.SVC_REALTIME_TASK,
                HKTFrameEncoder.svcRealtimeTaskPayload(valve = valve, state = state, durationS = dur, pulse = pulse),
            )
            if (rt !== cmd) return@launch   // 已被新命令取代或页面退出清理
            if (acked) {
                LogStore.info("0x03 " + (if (zh) "ACK（设备已开始执行）" else "ACK (device executing)"))
                if (dur > 0 || pulse > 0) {
                    rt = RtExec(RtPhase.EXECUTING, valve, state, dur, pulse, dur)
                    if (dur > 0) {
                        // 定时回位倒计时（本地镜像：固件无完成上报，结束时设备自动反向复位阀门）
                        countdownJob = scope.launch {
                            while (true) {
                                delay(1_000)
                                val cur = rt
                                if (cur?.phase != RtPhase.EXECUTING) return@launch
                                val remain = cur.remain - 1
                                if (remain <= 0) {
                                    rt = cur.copy(phase = RtPhase.DONE, remain = 0, flash = null)
                                    LogStore.info(
                                        "0x03 " + (if (zh) "定时回位结束（本地镜像）：设备已自动复位阀门"
                                        else "timed hold done (local mirror): valve restored by device"),
                                    )
                                    delay(3_000)
                                    if (rt?.phase == RtPhase.DONE) rt = null
                                    return@launch
                                }
                                rt = cur.copy(remain = remain)
                            }
                        }
                    }
                    // dur=0 pulse>0：执行中持续（设备脉冲打满自停），直至用户下发新命令
                } else {
                    // 纯开关：保持该状态直至下一命令（固件语义），直达完成闪条
                    val flash = "✓ ${valveName(valve)} " +
                        (if (state == 1) (if (zh) "开阀" else "open") else (if (zh) "关阀" else "close")) +
                        " · " + (if (zh) "已执行 · 阀门已动作" else "executed · valve actuated")
                    rt = RtExec(RtPhase.DONE, valve, state, dur, pulse, 0, flash)
                    LogStore.info("0x03 " + (if (zh) "阀门已动作（保持直至下一命令）" else "valve actuated (hold until next command)"))
                    delay(2_000)
                    if (rt?.phase == RtPhase.DONE && rt?.dur == 0) rt = null
                }
            } else {
                rt = null
                busyVisible = true
                LogStore.warn("0x03 " + (if (zh) "无 ACK（设备忙或已拒绝）" else "no ACK (busy or rejected)"))
                delay(2_600)
                busyVisible = false
            }
        }
    }

    HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 9.dp)) {
        Column {
            Text(
                (if (zh) "实时任务" else "Realtime Task") + " · 0x03",
                style = hkt(15f, FontWeight.Bold),
                color = c.text,
            )
            Text(
                if (zh) "立即开关阀门，点按即执行" else "Instant valve open/close - tap to actuate",
                style = hkt(11f),
                color = c.text2,
                modifier = Modifier.padding(top = 2.dp, bottom = 4.dp),
            )
            ValveRow(1, name = if (zh) "阀 1" else "Valve 1", cur = (if (zh) "当前 " else "Now ") + (if (v1) (if (zh) "开阀" else "open") else (if (zh) "关阀" else "closed")), v1 = v1, v2 = v2, sending = rt?.phase == RtPhase.SENDING, onSend = ::sendExec)
            ValveRow(2, name = if (zh) "阀 2" else "Valve 2", cur = (if (zh) "当前 " else "Now ") + (if (v2) (if (zh) "开阀" else "open") else (if (zh) "关阀" else "closed")), v1 = v1, v2 = v2, sending = rt?.phase == RtPhase.SENDING, onSend = ::sendExec)
            ValveRow(
                0, name = if (zh) "双阀" else "Both",
                cur = (if (zh) "阀1 " else "V1 ") + (if (v1) (if (zh) "开阀" else "open") else (if (zh) "关阀" else "closed")) + " · " +
                    (if (zh) "阀2 " else "V2 ") + (if (v2) (if (zh) "开阀" else "open") else (if (zh) "关阀" else "closed")),
                v1 = v1, v2 = v2, isLast = true, sending = rt?.phase == RtPhase.SENDING, onSend = ::sendExec,
            )
            // 高级折叠：定时回位时长 + 脉冲数
            Text(
                (if (advOpen) "▾ " else "▸ ") + (if (zh) "高级：定时回位与脉冲" else "Advanced: timed revert & pulses"),
                style = hkt(13f, FontWeight.SemiBold),
                color = c.info,
                modifier = Modifier
                    .padding(top = 9.dp)
                    .clickableBox { advOpen = !advOpen },
            )
            if (advOpen) {
                Row(horizontalArrangement = Arrangement.spacedBy(9.dp), modifier = Modifier.padding(top = 9.dp)) {
                    FieldCard((if (zh) "持续时间" else "Duration") + "(s)", Modifier.weight(1f)) {
                        NumInput(rtDur, { rtDur = it }, Modifier.fillMaxWidth())
                        Text(
                            if (zh) "动作保持 N 秒后自动复位（0 = 保持直至下一命令）"
                            else "Hold for N seconds then auto-revert (0 = hold until the next command)",
                            style = hkt(11f),
                            color = c.text2,
                        )
                    }
                    FieldCard(if (zh) "脉冲数" else "Pulse count", Modifier.weight(1f)) {
                        NumInput(rtPulse, { rtPulse = it }, Modifier.fillMaxWidth())
                        Text(
                            if (zh) "脉冲端口：发出 N 个脉冲后自动停止" else "PWM ports: stops automatically after N pulses",
                            style = hkt(11f),
                            color = c.text2,
                        )
                    }
                }
            }
            // 反馈条（四态：发送中/执行中/完成闪条/忙）
            val cmd = rt
            when {
                cmd?.phase == RtPhase.SENDING -> Box(Modifier.padding(top = 10.dp)) {
                    HktBanner(BadgeKind.INFO, "⏳ " + (if (zh) "发送中…" else "Sending…"))
                }
                cmd?.phase == RtPhase.EXECUTING -> {
                    val action = if (cmd.state == 1) (if (zh) "开阀" else "open") else (if (zh) "关阀" else "close")
                    Box(Modifier.padding(top = 10.dp)) {
                        HktBanner(
                            BadgeKind.INFO,
                            "● " + (if (zh) "设备执行中" else "Device executing") + " — ${valveName(cmd.valve)} · $action" +
                                (if (cmd.pulse > 0) " · " + (if (zh) "脉冲数 " else "pulses ") + "${cmd.pulse}" else ""),
                        )
                    }
                    if (cmd.dur > 0) {
                        // 进度条=剩余量：从满格随倒计时递减（自 0% 递增的条在长时长头几秒不可见）
                        Box(Modifier.padding(top = 8.dp)) { HktProgress(fraction = if (cmd.dur == 0) 0f else cmd.remain.toFloat() / cmd.dur) }
                        Text(
                            (if (zh) "剩余 ${cmd.remain} s / ${cmd.dur} s · " else "${cmd.remain}s / ${cmd.dur}s left · ") +
                                (if (zh) "结束后设备自动复位阀门" else "the device restores the valve when it ends"),
                            style = hkt(12f),
                            color = c.text2,
                            modifier = Modifier.padding(top = 4.dp),
                        )
                    } else {
                        Text(
                            if (cmd.pulse > 0) (if (zh) "执行中：设备达到脉冲数后自动结束并复位阀门"
                            else "Executing: the device stops and restores once the pulse count is reached")
                            else (if (zh) "无自动结束：持续执行，直至下发新命令（新命令立即覆盖当前动作）"
                            else "No auto-stop: runs until a new command replaces it (replacement actuates immediately)"),
                            style = hkt(11f),
                            color = c.text2,
                            modifier = Modifier.padding(top = 6.dp),
                        )
                    }
                }
                cmd?.phase == RtPhase.DONE -> Box(Modifier.padding(top = 10.dp)) {
                    HktBanner(BadgeKind.OK, cmd.flash ?: (if (zh) "✓ 任务完成 · 阀门已自动复位" else "✓ Task finished · valve restored"))
                }
                busyVisible -> Box(Modifier.padding(top = 10.dp)) {
                    HktBanner(
                        BadgeKind.ERR,
                        "⏱ " + (if (zh) "设备忙（本地定时任务执行中），实时任务被静默忽略（无 ACK）"
                        else "Device busy (schedule running); realtime task silently ignored (no ACK)"),
                    )
                }
            }
        }
    }
}

/** 阀行：名称+当前状态 | 开阀/关阀 一对大按钮（当前状态侧 info 高亮；发送中锁定）。 */
@Composable
private fun ValveRow(
    valve: Int,
    name: String,
    cur: String,
    v1: Boolean,
    v2: Boolean,
    sending: Boolean,
    isLast: Boolean = false,
    onSend: (Int, Int) -> Unit,
) {
    val c = LocalHktColors.current
    Column {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier.padding(vertical = 11.dp).fillMaxWidth(),
        ) {
            Column(Modifier.weight(1f)) {
                Text(name, style = hkt(14f, FontWeight.SemiBold), color = c.text)
                Text(cur, style = hkt(11f), color = c.text2, modifier = Modifier.padding(top = 2.dp))
            }
            Row(horizontalArrangement = Arrangement.spacedBy(6.dp), modifier = Modifier.width(186.dp)) {
                RtButton(valve, 1, active = if (valve == 0) (v1 && v2) else (if (valve == 1) v1 else v2), sending, onSend, Modifier.weight(1f))
                RtButton(valve, 0, active = if (valve == 0) (!v1 && !v2) else (if (valve == 1) !v1 else !v2), sending, onSend, Modifier.weight(1f))
            }
        }
        if (!isLast) {
            Box(Modifier.fillMaxWidth().height(1.dp).background(c.line.copy(alpha = 0.6f)))
        }
    }
}

@Composable
private fun RtButton(
    valve: Int,
    state: Int,
    active: Boolean,
    sending: Boolean,
    onSend: (Int, Int) -> Unit,
    modifier: Modifier = Modifier,
) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    Box(
        modifier
            .alpha(if (sending) 0.5f else 1f)
            .background(if (active) c.info else c.card2, RoundedCornerShape(8.dp))
            .clickableBox(enabled = !sending) { onSend(valve, state) }
            .padding(vertical = 10.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            if (state == 1) (if (zh) "开阀" else "Open") else (if (zh) "关阀" else "Close"),
            style = hkt(13f, FontWeight.SemiBold),
            color = if (active) Color.White else c.text2,
        )
    }
}

/** 输入小卡（.field 内嵌：label 11/600 + input + hint）。 */
@Composable
private fun FieldCard(label: String, modifier: Modifier = Modifier, content: @Composable () -> Unit) {
    val c = LocalHktColors.current
    Column(
        modifier
            .background(c.card, RoundedCornerShape(HktRadius.card.dp))
            .border(1.dp, c.line.copy(alpha = 0.82f), RoundedCornerShape(HktRadius.card.dp))
            .padding(10.dp),
    ) {
        Text(label, style = hkt(11f, FontWeight.SemiBold), color = c.text2)
        Box(Modifier.padding(top = 4.dp)) { content() }
    }
}

@Composable
private fun NumInput(text: String, onText: (String) -> Unit, modifier: Modifier = Modifier) {
    val c = LocalHktColors.current
    BasicTextField(
        value = text,
        onValueChange = { v -> onText(v.filter { it.isDigit() }.take(5)) },
        singleLine = true,
        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
        textStyle = hkt(14f).copy(color = c.text),
        modifier = modifier,
    )
}

// MARK: - 编辑页（规格卡 §2）

@Composable
private fun TaskEditScreen(session: DeviceSession, onSaved: () -> Unit, onCancel: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    val scope = rememberCoroutineScope()
    var validationMessage by remember { mutableStateOf<String?>(null) }
    var savePending by remember { mutableStateOf(false) }

    BackHandler { onCancel() }

    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(
            title = "＋ " + (if (zh) "新建任务" else "New Task"),
            backText = if (zh) "‹ 返回" else "‹ Back",
            onBack = onCancel,
        )
        Column(
            Modifier
                .weight(1f)
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 16.dp)
                .padding(bottom = 24.dp),
        ) {
            validationMessage?.let {
                Box(Modifier.padding(bottom = 9.dp)) { HktBanner(BadgeKind.ERR, "✕ $it") }
            }
            SlotCard(zh)
            ChoiceCard(if (zh) "阀门" else "Valve", listOf(if (zh) "双阀" else "Both", if (zh) "阀 1" else "Valve 1", if (zh) "阀 2" else "Valve 2"), TaskDraft.valve) { TaskDraft.valve = it }
            ChoiceCard(
                if (zh) "动作" else "Action",
                listOf(if (zh) "开阀" else "Open", if (zh) "关阀" else "Close"),
                if (TaskDraft.state == 1) 0 else 1,
            ) { TaskDraft.state = if (it == 0) 1 else 0 }
            InputCard(if (zh) "脉冲数" else "Pulse count", TaskDraft.pulseText) { TaskDraft.pulseText = it }
            TimeCard(zh)
            DaysCard(zh)
            DialogButton(title = if (zh) "保存配置" else "Save", kind = DialogButtonKind.PRIMARY) {
                if (savePending) return@DialogButton
                val pulse = TaskDraft.pulseText.toIntOrNull()
                if (TaskDraft.eh * 60 + TaskDraft.em <= TaskDraft.sh * 60 + TaskDraft.sm) {
                    validationMessage = if (zh) "时间无效（结束须晚于开始）" else "Invalid time (end must be after start)"
                    return@DialogButton
                }
                if (pulse == null || pulse !in 0..65535) {
                    validationMessage = if (zh) "超出范围 0–65535" else "Out of range 0–65535"
                    return@DialogButton
                }
                if (TaskDraft.days.none { it }) {
                    validationMessage = if (zh) "至少选择一天" else "Pick at least one day"
                    return@DialogButton
                }
                validationMessage = null
                savePending = true
                // 重复位：days[0]=周一 … days[6]=周日 → bit0…bit6（固件 repeat_duty，0x7F 上限）
                val repeatMask = TaskDraft.days.foldIndexed(0) { index, mask, on -> if (on) mask or (1 shl index) else mask }
                val slot = TaskDraft.slot
                LogStore.info("0x04 " + (if (zh) "任务" else "task") + " #$slot")
                scope.launch {
                    val acked = session.sendWrite(
                        CommandCode.SVC_TIMED_TASK,
                        HKTFrameEncoder.svcTimedTaskPayload(
                            id = slot, valve = TaskDraft.valve, state = TaskDraft.state,
                            pulse = pulse, startMinute = TaskDraft.sh * 60 + TaskDraft.sm,
                            endMinute = TaskDraft.eh * 60 + TaskDraft.em, repeatMask = repeatMask,
                        ),
                    )
                    savePending = false
                    if (acked) {
                        ValveTaskStore.upsert(
                            ValveTaskStore.Task(
                                id = slot, valve = TaskDraft.valve, state = TaskDraft.state, pulse = pulse,
                                sh = TaskDraft.sh, sm = TaskDraft.sm, eh = TaskDraft.eh, em = TaskDraft.em,
                                days = TaskDraft.days.toList(),
                            ),
                        )
                        LogStore.info("0x04 " + (if (zh) "任务" else "task") + " #$slot ACK")
                        onSaved()
                    } else {
                        validationMessage = if (zh) "设备未确认（可能被固件拒绝），请检查参数后重试"
                        else "Device did not acknowledge (possibly rejected); check values and retry"
                    }
                }
            }
            Box(Modifier.padding(top = 12.dp)) {
                DialogButton(title = if (zh) "取消" else "Cancel", kind = DialogButtonKind.SECONDARY, action = onCancel)
            }
        }
    }
}

/** 槽位格（1–16；选中=info、已占用=描边）。 */
@Composable
private fun SlotCard(zh: Boolean) {
    val c = LocalHktColors.current
    HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 9.dp)) {
        Column {
            Text(
                if (zh) "任务槽位（1–16；写入覆盖该槽位现有任务）"
                else "Task slot (1–16; writing overwrites the task in that slot)",
                style = hkt(12f),
                color = c.text2,
            )
            Column(verticalArrangement = Arrangement.spacedBy(5.dp), modifier = Modifier.padding(top = 8.dp)) {
                (1..16).toList().chunked(7).forEach { rowSlots ->
                    Row(horizontalArrangement = Arrangement.spacedBy(5.dp)) {
                        rowSlots.forEach { slot ->
                            val selected = TaskDraft.slot == slot
                            val used = ValveTaskStore.isUsed(slot)
                            val shape = RoundedCornerShape(9.dp)
                            Box(
                                Modifier
                                    .weight(1f)
                                    .background(if (selected) c.info else c.card2, shape)
                                    .border(1.dp, if (used) c.line else Color.Transparent, shape)
                                    .clickableBox { TaskDraft.slot = slot }
                                    .padding(vertical = 8.dp),
                                contentAlignment = Alignment.Center,
                            ) {
                                Text("$slot", style = hkt(13f), color = if (selected) Color.White else c.text2)
                            }
                        }
                        if (rowSlots.size < 7) repeat(7 - rowSlots.size) { Spacer(Modifier.weight(1f)) }
                    }
                }
            }
            Text(
                if (zh) "实线圈 = 槽位已有任务，写入即覆盖" else "Outlined = slot in use; writing overwrites it",
                style = hkt(11f),
                color = c.text2,
                modifier = Modifier.padding(top = 6.dp),
            )
        }
    }
}

/** 单选芯片卡（label + 等宽芯片；选中=info 底白字 600，未选=card2 底 text2 400）。 */
@Composable
private fun ChoiceCard(label: String, options: List<String>, selection: Int, onSelect: (Int) -> Unit) {
    val c = LocalHktColors.current
    HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 9.dp)) {
        Column {
            Text(label, style = hkt(12f), color = c.text2)
            Row(horizontalArrangement = Arrangement.spacedBy(6.dp), modifier = Modifier.padding(top = 8.dp)) {
                options.forEachIndexed { index, option ->
                    val selected = selection == index
                    Box(
                        Modifier
                            .weight(1f)
                            .background(if (selected) c.info else c.card2, RoundedCornerShape(9.dp))
                            .clickableBox { onSelect(index) }
                            .padding(vertical = 9.dp),
                        contentAlignment = Alignment.Center,
                    ) {
                        Text(
                            option,
                            style = hkt(13f, if (selected) FontWeight.SemiBold else FontWeight.Normal),
                            color = if (selected) Color.White else c.text2,
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun InputCard(label: String, text: String, onText: (String) -> Unit) {
    val c = LocalHktColors.current
    HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 9.dp)) {
        Column {
            Text(label, style = hkt(12f), color = c.text2)
            Box(Modifier.padding(top = 6.dp)) {
                BasicTextField(
                    value = text,
                    onValueChange = { v -> onText(v.filter { it.isDigit() }.take(5)) },
                    singleLine = true,
                    keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                    textStyle = hkt(15f, FontWeight.SemiBold).copy(color = c.text),
                    modifier = Modifier.fillMaxWidth(),
                )
            }
        }
    }
}

/** 开始/结束时间（两组 时:分 菜单选择，中间 ↓）。 */
@Composable
private fun TimeCard(zh: Boolean) {
    val c = LocalHktColors.current
    HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 9.dp)) {
        Column {
            Text(if (zh) "开始 / 结束" else "Start / End", style = hkt(12f), color = c.text2)
            TimePickerRow(
                if (zh) "开始" else "Start",
                hourValue = TaskDraft.sh, onHour = { TaskDraft.sh = it },
                minuteValue = TaskDraft.sm, onMinute = { TaskDraft.sm = it },
            )
            Text(
                "↓",
                style = hkt(13f),
                color = c.text2,
                modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
                textAlign = TextAlign.Center,
            )
            TimePickerRow(
                if (zh) "结束" else "End",
                hourValue = TaskDraft.eh, onHour = { TaskDraft.eh = it },
                minuteValue = TaskDraft.em, onMinute = { TaskDraft.em = it },
            )
        }
    }
}

/** 时:分选择行（label 30 宽 + 时 + ":" + 分，card2 底 r8 下拉菜单）。 */
@Composable
private fun TimePickerRow(
    label: String,
    hourValue: Int,
    onHour: (Int) -> Unit,
    minuteValue: Int,
    onMinute: (Int) -> Unit,
) {
    val c = LocalHktColors.current
    Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(6.dp)) {
        Text(label, style = hkt(13f), color = c.text2, modifier = Modifier.width(30.dp))
        PickerCell(String.format(Locale.US, "%02d", hourValue), 0..23, Modifier.weight(1f), onHour)
        Text(":", style = hkt(13f), color = c.text2)
        PickerCell(String.format(Locale.US, "%02d", minuteValue), 0..59, Modifier.weight(1f), onMinute)
    }
}

@Composable
private fun PickerCell(text: String, range: IntRange, modifier: Modifier = Modifier, onSelect: (Int) -> Unit) {
    val c = LocalHktColors.current
    var expanded by remember { mutableStateOf(false) }
    Box(modifier) {
        Box(
            Modifier
                .fillMaxWidth()
                .background(c.card2, RoundedCornerShape(HktRadius.control.dp))
                .border(1.dp, c.line, RoundedCornerShape(HktRadius.control.dp))
                .clickableBox { expanded = true }
                .padding(vertical = 8.dp),
            contentAlignment = Alignment.Center,
        ) {
            Text(text, style = hkt(14f), color = c.text)
        }
        androidx.compose.material3.DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
            range.forEach { value ->
                androidx.compose.material3.DropdownMenuItem(
                    text = { Text(String.format(Locale.US, "%02d", value), style = hkt(14f)) },
                    onClick = { onSelect(value); expanded = false },
                )
            }
        }
    }
}

/** 重复日 chips（周一…周日 多选，选中=info 底白字）。 */
@Composable
private fun DaysCard(zh: Boolean) {
    val c = LocalHktColors.current
    val dayList = if (zh) listOf("一", "二", "三", "四", "五", "六", "日") else listOf("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
    HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 9.dp)) {
        Column {
            Text(if (zh) "重复" else "Repeat", style = hkt(12f), color = c.text2)
            Row(horizontalArrangement = Arrangement.spacedBy(5.dp), modifier = Modifier.padding(top = 8.dp)) {
                dayList.forEachIndexed { index, day ->
                    val on = TaskDraft.days[index]
                    Box(
                        Modifier
                            .weight(1f)
                            .background(if (on) c.info else c.card2, RoundedCornerShape(9.dp))
                            .clickableBox { TaskDraft.days[index] = !on }
                            .padding(vertical = 7.dp),
                        contentAlignment = Alignment.Center,
                    ) {
                        Text(day, style = hkt(13f), color = if (on) Color.White else c.text2)
                    }
                }
            }
        }
    }
}
