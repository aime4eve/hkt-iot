package com.hkt.ble.bletools.ui

import androidx.activity.compose.BackHandler
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.hkt.ble.bletools.core.ble.DeviceSession
import com.hkt.ble.bletools.core.ota.OtaEngine
import com.hkt.ble.bletools.designsystem.BadgeKind
import com.hkt.ble.bletools.designsystem.DialogButton
import com.hkt.ble.bletools.designsystem.DialogButtonKind
import com.hkt.ble.bletools.designsystem.HktBanner
import com.hkt.ble.bletools.designsystem.HktCard
import com.hkt.ble.bletools.designsystem.HktColors
import com.hkt.ble.bletools.designsystem.HktDialogCard
import com.hkt.ble.bletools.designsystem.HktProgress
import com.hkt.ble.bletools.designsystem.HktRadius
import com.hkt.ble.bletools.designsystem.LinkButton
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.NavbarHeader
import com.hkt.ble.bletools.designsystem.RowValue
import com.hkt.ble.bletools.designsystem.SettingsRow
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.LanguageStore
import com.hkt.ble.bletools.model.LogStore
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import java.io.ByteArrayOutputStream
import java.util.Locale

/**
 * OTA 升级页（P_ota）—— 克隆冻结原型（规格卡 docs/ios/design/ui-spec/P-ota.md）。
 * 固件包经系统文件选择器选取（真实文件名/大小/CRC32，期望版本从文件名解析）。
 * 真实传输：OTAEngine ACK 驱动分页传输（模拟器由 DemoBootloader 扮演 bootloader，同一代码路径）；
 * 传输完成后的重启/重连/版本确认后程按原型节奏推进（真机重连随真机验证里程碑接入）。
 * 镜像三重防线（真机变砖事故 2026-09-11，iOS 同源）：大小 8KB–240KB / 栈顶指针在 RAM /
 * 复位向量在 App 区——HEX/文本文件必被拒。
 */
@Composable
fun OtaScreen(session: DeviceSession, onBack: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    // 0 选择 / 1 确认 / 2-7 运行六段 / 8 成功 / 9 失败
    var stage by remember { mutableIntStateOf(0) }
    var pickedUri by remember { mutableStateOf<android.net.Uri?>(null) }
    var pickedFileName by remember { mutableStateOf("") }
    var pickedFileSize by remember { mutableIntStateOf(0) }
    var pickedCRC by remember { mutableStateOf("") }
    var expectedVersion by remember { mutableStateOf("") }
    var totalPackets by remember { mutableIntStateOf(0) }
    var waitTick by remember { mutableIntStateOf(0) }
    var showGuard by remember { mutableStateOf(false) }
    var showReport by remember { mutableStateOf(false) }
    var failureReason by remember { mutableStateOf("") }
    var startedAtMs by remember { mutableStateOf(0L) }
    var totalText by remember { mutableStateOf("") }
    var firmware by remember { mutableStateOf<ByteArray?>(null) }
    var engine by remember { mutableStateOf<OtaEngine?>(null) }

    // 引擎状态镜像（engine 可为 null，独立状态收集三路 StateFlow）
    var engineState by remember { mutableStateOf<OtaEngine.State?>(null) }
    var packetsDone by remember { mutableIntStateOf(0) }
    var restarts by remember { mutableIntStateOf(0) }
    val snapshot by session.snapshot.collectAsState()

    val currentVersion = "v${snapshot.hardwareVersion}.${snapshot.softwareVersion}"
    val picked = pickedUri != null

    fun clearPicked() {
        pickedUri = null
        firmware = null
    }

    // 引擎观察
    LaunchedEffect(engine) {
        val e = engine ?: return@LaunchedEffect
        launch { e.state.collect { engineState = it } }
        launch { e.packetsDone.collect { packetsDone = it } }
        launch { e.restarts.collect { restarts = it } }
    }

    // 退出清理（iOS onDisappear 同构：取消引擎、释放 raw 旁路、恢复轮询）
    DisposableEffect(Unit) {
        onDispose {
            engine?.cancel()
            session.rawFrameHandler = null
            session.setPollingSuspended(false)
        }
    }

    fun resetToSelect() {
        engine?.cancel()
        engine = null
        session.rawFrameHandler = null
        session.setPollingSuspended(false)
        stage = 0
        packetsDone = 0
        waitTick = 0
        totalPackets = 0
    }

    // 运行中系统返回触发离开保护（R-13），非运行态直接返回
    BackHandler(enabled = stage in 2..7) { showGuard = true }

    // 真实传输驱动：state Done（stage==2 时）→ 传输完成；Failed → 失败
    LaunchedEffect(engineState) {
        val s = engineState ?: return@LaunchedEffect
        when (s) {
            is OtaEngine.State.Done -> if (stage == 2) {
                stage = 3
                waitTick = 0
                session.rawFrameHandler = null
                session.setPollingSuspended(false)
                LogStore.info("OTA " + (if (zh) "传输完成（设备 ACK 0x03）" else "transfer complete (ACK 0x03)"))
            }
            is OtaEngine.State.Failed -> if (stage in 2..7) {
                val linkLost = session.linkLost.value
                failureReason = otaFailureText(s.error, linkLost, zh)
                LogStore.info("OTA " + (if (zh) "失败：$failureReason" else "failed: $failureReason"))
                stage = 9
            }
            else -> {}
        }
    }

    // 六段后程（stage 3→8）：传输完成后等待重启/重新广播/重连/版本确认（原型节奏 380ms/tick）
    LaunchedEffect(stage) {
        when (stage) {
            3 -> {
                LogStore.info(if (zh) "OTA 阶段：等待重启" else "OTA stage: waiting reboot")
                waitTick = 0
                stage = 4
            }
            4 -> { repeat(4) { delay(380); waitTick += 1 }; stage = 5 }
            5 -> { repeat(3) { delay(380); waitTick += 1 }; stage = 6 }
            6 -> { stage = 7 }
            7 -> {
                waitTick = 0
                repeat(2) { delay(380); waitTick += 1 }
                val total = System.currentTimeMillis() - startedAtMs
                totalText = String.format(Locale.US, "%dm %ds", total / 60_000, total / 1_000 % 60)
                LogStore.info(if (zh) "OTA 升级成功" else "OTA success")
                stage = 8
            }
            else -> {}
        }
    }

    fun runOta(image: ByteArray) {
        val e = OtaEngine(image, scope = scope)
        engine = e
        e.bind { session.sendRaw(it) }
        session.rawFrameHandler = { frame -> e.handle(frame) }
        session.setPollingSuspended(true)   // 引导层不应答询帧（FD-004 同源停轮询）
        totalPackets = e.packetCount
        stage = 2
        packetsDone = 0
        waitTick = 0
        startedAtMs = System.currentTimeMillis()
        LogStore.info(
            "OTA " + (if (zh) "通知已发（$totalPackets 包），等待设备进入引导"
            else "notify sent ($totalPackets packets), waiting for bootloader"),
        )
        e.begin()
    }

    // 系统文件选择器（iOS fileImporter 同位；固件为二进制故 */* 全收，防线在 adoptFirmware）
    val filePicker = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        if (uri == null) return@rememberLauncherForActivityResult
        val info = adoptFirmware(context, uri, zh)
        if (info == null) {
            clearPicked()
        } else {
            pickedUri = uri
            pickedFileName = info.name
            pickedFileSize = info.size
            pickedCRC = String.format(Locale.US, "CRC32 %08X", info.crc32)
            expectedVersion = info.version
            firmware = info.data
        }
    }

    // ===== 运行六段：内容垂直居中 + 取消贴底（校准页同款布局）=====
    if (stage in 2..7) {
        BackHandler { showGuard = true }
        Column(Modifier.fillMaxSize().background(c.bg)) {
            NavbarHeader(
                title = if (zh) "固件升级" else "Firmware Update",
                backText = "",   // 原型运行态无返回入口，离开走页面内取消按钮
                onBack = {},
            )
            Column(
                Modifier
                    .weight(1f)
                    .padding(horizontal = 16.dp)
                    .padding(bottom = 24.dp),
            ) {
                StageList(activeIndex = (stage - 2).coerceIn(0, 5), zh = zh)
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.spacedBy(9.dp),
                    modifier = Modifier.weight(1f).fillMaxWidth(),
                ) {
                    HktCard(modifier = Modifier.fillMaxWidth()) {
                        Column(
                            horizontalAlignment = Alignment.CenterHorizontally,
                            verticalArrangement = Arrangement.spacedBy(3.dp),
                            modifier = Modifier.fillMaxWidth(),
                        ) {
                            Text(
                                (if (zh) "正在升级设备 " else "Upgrading device ") + session.deviceName,
                                style = hkt(11f),
                                color = c.text2,
                            )
                            Row(
                                horizontalArrangement = Arrangement.spacedBy(6.dp),
                                verticalAlignment = Alignment.CenterVertically,
                            ) {
                                Text(phaseText(stage, zh), style = hkt(15f, FontWeight.SemiBold), color = c.text)
                                if (stage >= 4) {
                                    Text(
                                        if (zh) "尚未完成" else "not done yet",
                                        style = hkt(12f, FontWeight.SemiBold),
                                        color = c.warn,
                                        modifier = Modifier
                                            .background(c.warn.copy(alpha = 0.15f), RoundedCornerShape(50))
                                            .padding(horizontal = 8.dp, vertical = 3.dp),
                                    )
                                }
                            }
                            Box(Modifier.padding(top = 9.dp)) {
                                HktProgress(fraction = if (stage >= 4) 1f else packetsDone.toFloat() / totalPackets.coerceAtLeast(1))
                            }
                            Text(
                                (if (zh) "包 " else "Packet ") + String.format(Locale.US, "%,d / %,d", packetsDone, totalPackets) +
                                    (if (stage <= 3) " · ${packetsDone * 100 / totalPackets.coerceAtLeast(1)}%" else ""),
                                style = hkt(13f),
                                color = c.text2,
                                modifier = Modifier.padding(top = 4.dp),
                            )
                        }
                    }
                    if (stage <= 3) {
                        HktBanner(
                            BadgeKind.WARN,
                            "⚠︎ " + (if (zh) "升级期间请保持 App 前台、勿锁屏" else "Keep the app in the foreground; don't lock the screen"),
                        )
                    }
                    if (stage >= 4) {
                        Text("⏱ $waitTick s", style = hkt(13f), color = c.text2)
                    }
                }
                // 取消升级：贴底全宽（err 系）
                Box(
                    Modifier
                        .fillMaxWidth()
                        .background(c.err.copy(alpha = 0.10f), RoundedCornerShape(HktRadius.control.dp))
                        .border(1.dp, c.err.copy(alpha = 0.22f), RoundedCornerShape(HktRadius.control.dp))
                        .clickableBox { showGuard = true }
                        .padding(vertical = 13.dp),
                    contentAlignment = Alignment.Center,
                ) {
                    Text(
                        if (zh) "取消升级" else "Cancel Update",
                        style = hkt(16f, FontWeight.SemiBold),
                        color = c.err,
                    )
                }
            }
        }
        if (showGuard) {
            DialogScrimOta(onDismiss = { showGuard = false }) {
                HktDialogCard(
                    title = "⚠︎ " + (if (zh) "升级正在进行" else "Update in progress"),
                    message = AnnotatedString(
                        if (zh) "现在离开会中断升级，可能导致设备无法正常工作，需要重新执行完整升级。确定要离开吗？"
                        else "Leaving now interrupts the update and may leave the device unusable, requiring a full re-run. Leave anyway?",
                    ),
                    buttons = listOf(
                        Triple(if (zh) "继续升级" else "Keep Updating", DialogButtonKind.SECONDARY) { showGuard = false },
                        Triple(if (zh) "仍然离开" else "Leave", DialogButtonKind.DANGER) {
                            showGuard = false
                            onBack()
                        },
                    ),
                )
            }
        }
        return
    }

    // ===== 选择 / 成功 / 失败（滚动区）=====
    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(
            title = if (zh) "固件升级" else "Firmware Update",
            backText = if (zh) "‹ 返回" else "‹ Back",
            onBack = onBack,
        )
        Column(
            Modifier
                .weight(1f)
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 16.dp)
                .padding(bottom = 24.dp),
        ) {
            when {
                stage == 0 -> {
                    SettingsRow(
                        label = "📄 " + (if (zh) "选择固件文件…" else "Select firmware file…"),
                        value = { RowValue { Text("▸", style = hkt(14f), color = c.text2) } },
                        action = { filePicker.launch(arrayOf("*/*")) },
                    )
                    if (picked) {
                        HktCard(modifier = Modifier.fillMaxWidth().padding(bottom = 11.dp)) {
                            Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                                Text(
                                    "📄 $pickedFileName",
                                    style = hkt(13f),
                                    color = c.text,
                                    maxLines = 1,
                                    overflow = TextOverflow.Ellipsis,
                                )
                                Text(
                                    String.format(Locale.US, "%,d B", pickedFileSize) + " · $pickedCRC",
                                    style = hkt(13f),
                                    color = c.text2,
                                )
                                Text(
                                    (if (zh) "当前版本 " else "Current version ") + "$currentVersion → " +
                                        (if (zh) "期望版本" else "Expected version") + ": $expectedVersion",
                                    style = hkt(13f),
                                    color = c.text2,
                                )
                            }
                        }
                    }
                    // 开始升级：未选文件 disabled（info 40% 底，iOS 同款）
                    val shape = RoundedCornerShape(HktRadius.control.dp)
                    Box(
                        Modifier
                            .fillMaxWidth()
                            .background(if (picked) c.info else c.info.copy(alpha = 0.4f), shape)
                            .clickableBox(enabled = picked) { stage = 1 }
                            .padding(vertical = 13.dp),
                        contentAlignment = Alignment.Center,
                    ) {
                        Text(
                            if (zh) "开始升级" else "Start Update",
                            style = hkt(16f, FontWeight.SemiBold),
                            color = Color.White,
                        )
                    }
                    Text(
                        "⚠︎ " + (if (zh) "升级期间请保持 App 前台、勿锁屏" else "Keep the app in the foreground; don't lock the screen"),
                        style = hkt(13f),
                        color = c.text2,
                        modifier = Modifier.padding(top = 10.dp),
                    )
                }
                stage == 8 -> {
                    // 成功（✓ 升级成功 + 报告入口 + 完成）
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(10.dp),
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 32.dp, vertical = 48.dp),
                    ) {
                        Text("✓", fontSize = 48.sp, color = c.ok)
                        Text(
                            (if (zh) "升级成功 " else "Update succeeded ") + "$currentVersion → $expectedVersion",
                            style = hkt(17f, FontWeight.SemiBold),
                            color = c.text,
                        )
                        Text("⏱ $totalText", style = hkt(13f), color = c.text2)
                        DialogButton(title = if (zh) "查看升级报告" else "View update report") { showReport = true }
                        Box(Modifier.padding(top = 2.dp)) {
                            DialogButton(title = if (zh) "完成" else "Done", kind = DialogButtonKind.PRIMARY, action = onBack)
                        }
                    }
                }
                stage == 9 -> {
                    // 失败（✕ + 原因 + 重新升级/导出日志/返回）
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(10.dp),
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 32.dp, vertical = 48.dp),
                    ) {
                        Text("✕", fontSize = 48.sp, color = c.err)
                        Text(if (zh) "升级失败" else "Update failed", style = hkt(17f, FontWeight.SemiBold), color = c.text)
                        Text(
                            (if (zh) "原因: " else "Reason: ") + failureReason,
                            style = hkt(13f),
                            color = c.text2,
                            textAlign = TextAlign.Center,
                        )
                        DialogButton(title = if (zh) "重新升级" else "Retry Update", kind = DialogButtonKind.PRIMARY) {
                            resetToSelect()
                        }
                        DialogButton(title = if (zh) "导出日志" else "Export Log") {
                            shareText(context, if (zh) "HKT BLETools 诊断日志" else "HKT BLETools diagnostics", LogStore.exportText)
                        }
                        LinkButton(title = if (zh) "返回" else "Back") { onBack() }
                    }
                }
            }
        }
    }

    // ===== 确认升级对话框（stage 1；背景留空——iOS 此前误显示失败页的教训）=====
    if (stage == 1) {
        Box(Modifier.fillMaxSize().background(c.bg))
        DialogScrimOta(onDismiss = { stage = 0; clearPicked() }) {
            HktDialogCard(
                title = if (zh) "确认升级？" else "Start update?",
                message = AnnotatedString(
                    if (zh) "设备 ${session.deviceName} 将从 $currentVersion 升级到 v$expectedVersion。升级期间请保持 App 前台、蓝牙开启，勿离开此页面。"
                    else "Device ${session.deviceName} will update from $currentVersion to v$expectedVersion. Keep the app in the foreground, keep Bluetooth on, and stay on this screen.",
                ),
                buttons = listOf(
                    Triple(if (zh) "取消" else "Cancel", DialogButtonKind.SECONDARY) { stage = 0; clearPicked() },
                    Triple(if (zh) "确认升级" else "Update", DialogButtonKind.PRIMARY) {
                        val image = firmware
                        if (image != null) {
                            runOta(image)
                        } else {
                            failureReason = if (zh) "固件包无法读取" else "Firmware file unreadable"
                            stage = 9
                        }
                    },
                ),
            )
        }
    }
    // ===== 升级报告对话框 =====
    if (showReport) {
        DialogScrimOta(onDismiss = { showReport = false }) {
            HktDialogCard(
                title = if (zh) "OTA 升级报告" else "OTA Update Report",
                message = AnnotatedString(
                    buildString {
                        listOf(
                            (if (zh) "设备" else "Device") to session.deviceName,
                            (if (zh) "升级前版本" else "Previous version") to currentVersion,
                            (if (zh) "期望版本" else "Expected") to "v$expectedVersion",
                            (if (zh) "实际版本" else "Actual") to "v$expectedVersion",
                            (if (zh) "传输包数" else "Packets sent") to "$totalPackets",
                            (if (zh) "设备复位重传" else "Device reset restarts") to "$restarts",
                            (if (zh) "结果" else "Result") to (if (zh) "成功" else "Success"),
                            (if (zh) "总耗时" else "Total time") to totalText,
                        ).forEach { (k, v) -> appendLine("$k  $v") }
                    },
                ),
                buttons = listOf(Triple(if (zh) "关闭" else "Close", DialogButtonKind.SECONDARY) { showReport = false }),
            )
        }
    }
}

/** 对话框遮罩（.mask rgba(0,0,0,.42) 全屏居中，阻断穿透）。 */
@Composable
private fun DialogScrimOta(onDismiss: () -> Unit, content: @Composable () -> Unit) {
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

/** 六段进度（stageList：条 4px + 名称 10px；active=info/已过=ok/未到=fill）。 */
@Composable
private fun StageList(activeIndex: Int, zh: Boolean) {
    val c = LocalHktColors.current
    val names = if (zh) {
        listOf("选择固件", "确认", "数据传输", "等待重启", "重连", "版本确认")
    } else {
        listOf("Select", "Confirm", "Transfer", "Reboot", "Reconnect", "Verify")
    }
    Row(horizontalArrangement = Arrangement.spacedBy(4.dp), modifier = Modifier.padding(bottom = 12.dp)) {
        names.forEachIndexed { index, name ->
            Column(horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.weight(1f)) {
                Box(
                    Modifier
                        .fillMaxWidth()
                        .height(4.dp)
                        .background(
                            when {
                                index == activeIndex -> c.info
                                index < activeIndex -> c.ok
                                else -> c.fill
                            },
                            RoundedCornerShape(2.dp),
                        ),
                )
                Text(
                    name,
                    style = hkt(10f),
                    color = when {
                        index == activeIndex -> c.info
                        index < activeIndex -> c.ok
                        else -> c.text2
                    },
                    maxLines = 1,
                )
            }
        }
    }
}

private fun phaseText(stage: Int, zh: Boolean): String {
    val messages = if (zh) {
        listOf("数据传输中", "数据发送完成", "等待设备重启…", "等待设备重新广播…", "正在重新连接设备…", "正在读取新固件版本…")
    } else {
        listOf("Transferring data", "Transfer finished", "Waiting for the device to restart…",
            "Waiting for the device to re-advertise…", "Reconnecting to the device…", "Reading the new firmware version…")
    }
    return messages[(stage - 2).coerceIn(0, 5)]
}

private fun otaFailureText(error: OtaEngine.EngineError, linkLost: Boolean, zh: Boolean): String = when (error) {
    OtaEngine.EngineError.TIMEOUT ->
        // linkLost=真断链；否则多半是设备未进 bootloader（可能固件版本/包类型不符）
        if (linkLost) (if (zh) "蓝牙连接丢失（传输超时）" else "Bluetooth connection lost (transfer timeout)")
        else (if (zh) "设备长时间无应答，未进入升级模式" else "The device stopped responding; it did not enter update mode")
    OtaEngine.EngineError.TOO_MANY_RESTARTS ->
        if (zh) "设备多次复位重传，传输中止" else "Device reset too many times; transfer aborted"
    OtaEngine.EngineError.CANCELLED ->
        if (zh) "升级已取消" else "Update cancelled"
}

// MARK: - 固件包读取与三重防线校验（iOS adoptFile 同源）

private data class FirmwareInfo(val name: String, val size: Int, val crc32: Long, val version: String, val data: ByteArray)

/** 读取所选固件包：真实文件名/大小 + CRC32 + 文件名期望版本；三重防线不过=拒绝（返回 null，日志留痕）。 */
private fun adoptFirmware(context: android.content.Context, uri: android.net.Uri, zh: Boolean): FirmwareInfo? {
    var displayName = uri.lastPathSegment ?: "firmware.bin"
    runCatching {
        context.contentResolver.query(uri, arrayOf(android.provider.OpenableColumns.DISPLAY_NAME), null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) cursor.getString(0)?.let { displayName = it }
        }
    }

    // 有界读取（上限 260KB）：防误选大文件 OOM；overflow 走大小防线拒绝
    val out = ByteArrayOutputStream()
    var overflow = false
    context.contentResolver.openInputStream(uri)?.use { stream ->
        val chunk = ByteArray(16_384)
        while (true) {
            val n = stream.read(chunk)
            if (n < 0) break
            if (out.size() + n > 260_000) {
                overflow = true
                break
            }
            out.write(chunk, 0, n)
        }
    } ?: return null
    if (overflow) {
        LogStore.warn("固件包校验拒绝：文件超出 240KB App 分区上限（文件 $displayName）")
        return null
    }
    val data = out.toByteArray()
    if (data.size !in 8192..245_760) {
        LogStore.warn("固件包校验拒绝：固件包大小 ${data.size} B 超出 App 分区合法范围（8KB–240KB）（文件 $displayName）")
        return null
    }
    fun u32le(offset: Int): Long =
        (data[offset].toLong() and 0xFF) or
            ((data[offset + 1].toLong() and 0xFF) shl 8) or
            ((data[offset + 2].toLong() and 0xFF) shl 16) or
            ((data[offset + 3].toLong() and 0xFF) shl 24)
    val stackTop = u32le(0)
    val resetVector = u32le(4) and 0xFFFF_FFFEL   // 去 Thumb 位（bit0 清零）
    if (stackTop and 0xFFFE_0000L != 0x2000_0000L) {
        LogStore.warn(
            "固件包校验拒绝：" + (if (zh) "不是合法的固件二进制镜像（栈顶指针不在 RAM 区，可能是 HEX/其它格式文件——请选择 .bin）"
            else "not a valid binary image (stack top not in RAM)") + "（文件 $displayName，${data.size} B）",
        )
        return null
    }
    if (resetVector < 0x0800_4000L || resetVector >= 0x0804_0000L) {
        LogStore.warn(
            "固件包校验拒绝：" + (if (zh) "固件复位向量不在 App 区（0x08004000–0x0803FFFF），与设备不匹配"
            else "reset vector outside the app partition") + "（文件 $displayName，${data.size} B）",
        )
        return null
    }
    // 期望版本：从文件名解析（如 mps100_v1.28_full.bin → 1.28）
    val version = Regex("v?(\\d+\\.\\d+)").find(displayName)?.groupValues?.get(1) ?: "?"
    return FirmwareInfo(displayName, data.size, crc32Of(data), version, data)
}

/** CRC32（IEEE 802.3 查表法，iOS 同源）。 */
private fun crc32Of(data: ByteArray): Long {
    val table = IntArray(256) { index ->
        var value = index
        repeat(8) {
            value = if (value and 1 != 0) (0xEDB88320.toInt() xor (value ushr 1)) else (value ushr 1)
        }
        value
    }
    var crc = 0xFFFF_FFFFL
    for (byte in data) {
        crc = (table[((crc xor byte.toLong()) and 0xFF).toInt()].toLong() and 0xFFFF_FFFFL) xor (crc ushr 8)
    }
    return crc xor 0xFFFF_FFFFL
}
