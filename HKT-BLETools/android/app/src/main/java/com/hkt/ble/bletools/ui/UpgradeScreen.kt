package com.hkt.ble.bletools.ui

import androidx.activity.compose.BackHandler
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
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
import com.hkt.ble.bletools.core.ota.DiffUpgradeEngine
import com.hkt.ble.bletools.core.ota.UpgradePackage
import com.hkt.ble.bletools.core.protocol.UpgradePkgType
import com.hkt.ble.bletools.designsystem.BadgeKind
import com.hkt.ble.bletools.designsystem.DialogButton
import com.hkt.ble.bletools.designsystem.DialogButtonKind
import com.hkt.ble.bletools.designsystem.HktBanner
import com.hkt.ble.bletools.designsystem.HktCard
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
 * 差分/全量升级页（地磁 DC200 专用，2026-11《蓝牙升级协议》新通道）—— 与旧 OTA 页
 * （OtaScreen，引导层回环）并存且互不相干：新协议为**设备拉包**模式（0xF0 通知→设备
 * 0xFE 应答→设备 0xF2 逐包拉取→APP 0xF1 回包→设备 0xF3 上报结果），传输完成后设备
 * **自行**合成新固件并复位安装（BLE 可能断开，APP 侧不算失败；成功判定 = 重连后读回
 * 版本 == 目标版本，协议可靠性要求④）。
 *
 * 选文件时用户必须显式选择包类型（全量 0x00 / 差分 0x01）：差分包须与设备当前固件
 * 严格匹配（设备端做底包 CRC 校验，不符即拒）；全量包兜底救砖（底包损坏/版本不明）。
 * 升级运行期不允许取消（与旧 OTA 页 2026-09-24 用户裁决同纪律），失败走失败页返回。
 *
 * 升级文件两种形态（[UpgradePackage]）：差分=带 24B HKTU 包头的发布包（PC 工具产生，
 * 目标版本/底包版本来自包头，无头即拒绝）；全量=裸 .bin（与烧录器同一份文件，目标版本
 * 读镜像 0x200 处内嵌 g_fw_info 记录，旧固件无记录显示"未知"——不编造，以升级后设备
 * 读回为准）。文件名 v<hw>.<sw> / full / diff 后缀仅作预选提示，最终由用户确认/包头校验。
 */
@Composable
fun UpgradeScreen(session: DeviceSession, onBack: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    // 0 选择 / 1 确认 / 2 传输（通知+拉包+结果） / 3 设备合成安装等待 / 8 成功 / 9 失败
    var stage by remember { mutableIntStateOf(0) }
    var pickedFileName by remember { mutableStateOf("") }
    var pickedFileSize by remember { mutableIntStateOf(0) }
    var pickedCRC by remember { mutableStateOf("") }
    var targetVersionText by remember { mutableStateOf("") }
    var pkgTypeSel by remember { mutableStateOf<Int?>(null) }   // null=未选 / UpgradePkgType
    var failureReason by remember { mutableStateOf("") }
    var startedAtMs by remember { mutableStateOf(0L) }
    var totalText by remember { mutableStateOf("") }
    var waitTick by remember { mutableIntStateOf(0) }
    var showReport by remember { mutableStateOf(false) }
    var file by remember { mutableStateOf<ByteArray?>(null) }      // 剥头后的载荷（引擎实际发送的字节）
    var rawFile by remember { mutableStateOf<ByteArray?>(null) }   // 文件原始字节（含 24B 发布包头，若有）
    var adopted by remember { mutableStateOf<UpgradePackage.Adopt?>(null) }  // 收包防线结果（Ok/Err）
    var engine by remember { mutableStateOf<DiffUpgradeEngine?>(null) }
    var fileCrc32 by remember { mutableStateOf(0L) }
    var fileHw by remember { mutableStateOf(0) }
    var fileSw by remember { mutableStateOf(0) }

    var engineState by remember { mutableStateOf<DiffUpgradeEngine.State?>(null) }
    var packetsDone by remember { mutableIntStateOf(0) }
    var totalPackets by remember { mutableIntStateOf(0) }
    val snapshot by session.snapshot.collectAsState()

    val currentVersion = "v${snapshot.hardwareVersion}.${snapshot.softwareVersion}"
    val picked = rawFile != null
    val typeKnown = pkgTypeSel != null

    fun clearPicked() {
        rawFile = null
        file = null
        adopted = null
        pkgTypeSel = null
    }

    // 引擎观察
    LaunchedEffect(engine) {
        val e = engine ?: return@LaunchedEffect
        launch { e.state.collect { engineState = it } }
        launch { e.packetsDone.collect { packetsDone = it } }
    }

    // 退出清理：取消引擎、释放 raw 旁路、恢复轮询（与旧 OTA 页同构）
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
        totalPackets = 0
    }

    // 运行中系统返回直接拦截（升级过程不允许取消/离开），非运行态直接返回
    BackHandler(enabled = stage in 2..3) { /* 升级中：吞掉返回 */ }

    // 引擎终态驱动：Done（stage==2）→ 设备合成安装等待；Failed → 失败页
    LaunchedEffect(engineState) {
        val s = engineState ?: return@LaunchedEffect
        when (s) {
            is DiffUpgradeEngine.State.Done -> if (stage == 2) {
                stage = 3
                waitTick = 0
                LogStore.info("Upgrade " + (if (zh) "0xF3 成功：设备已接收全部升级包，自行合成/复位安装" else "0xF3 success: device installing"))
            }
            is DiffUpgradeEngine.State.Failed -> if (stage in 2..3) {
                val linkLost = session.linkLost.value
                failureReason = upgradeFailureText(s.error, engine?.reportedCode, linkLost, zh)
                LogStore.info("Upgrade " + (if (zh) "失败：$failureReason" else "failed: $failureReason"))
                stage = 9
            }
            else -> {}
        }
    }

    // 设备合成/安装等待（stage 3→8）：设备端合成+复位在 BLE 断链侧发生，此处按固定节奏
    // 收尾到成功页（重连核对版本由用户在成功页引导中完成，协议可靠性要求④）
    LaunchedEffect(stage) {
        if (stage == 3) {
            repeat(6) { delay(1_000); waitTick += 1 }
            val total = System.currentTimeMillis() - startedAtMs
            totalText = String.format(Locale.US, "%dm %ds", total / 60_000, total / 1_000 % 60)
            LogStore.info(if (zh) "升级包传输成功" else "upgrade package delivered")
            stage = 8
        }
    }

    fun runUpgrade(image: ByteArray, pkgType: Int) {
        val e = DiffUpgradeEngine(
            file = image,
            pkgType = pkgType,
            hwVersion = fileHw,
            swVersion = fileSw,
            fileCrc32 = fileCrc32,
            scope = scope,
        )
        engine = e
        e.bind { session.sendRaw(it) }
        session.rawFrameHandler = { frame -> e.handle(frame) }
        session.setPollingSuspended(true)   // 升级态设备不应答询帧（与旧 OTA 同纪律）
        totalPackets = e.packetCount
        packetsDone = 0
        startedAtMs = System.currentTimeMillis()
        stage = 2
        LogStore.info(
            "Upgrade " + (if (zh) "通知已发（${e.packetCount} 包，${pkgTypeText(pkgType, zh)}），等待设备应答"
            else "notify sent (${e.packetCount} packets, ${pkgTypeText(pkgType, zh)}), waiting for ack"),
        )
        e.begin()
    }

    /**
     * 收包防线（[UpgradePackage.adopt]）：文件选定或用户改选类型后调用——防线与所选
     * 类型绑定，改选必须重跑。目标版本优先级：包头/镜像内嵌记录 > 文件名提示 > 未知
     * （真源缺失时显示"未知、以升级后读回为准"，不编造 0——0xF0 版本字段仍兜底 0）。
     */
    fun reAdopt() {
        val raw = rawFile ?: return
        val result = UpgradePackage.adopt(raw, pkgTypeSel, snapshot.hardwareVersion)
        adopted = result
        if (result is UpgradePackage.Adopt.Err) {
            file = null
            pickedFileSize = raw.size
            pickedCRC = if (zh) "未通过校验" else "not verified"
            targetVersionText = ""
            // TYPE_PENDING 非错误（等用户选类型后自动重跑），不刷失败横幅
            if (result.reason != UpgradePackage.Adopt.Reason.TYPE_PENDING) {
                LogStore.warn("升级包校验拒绝：" + adoptErrorText(result, zh) + "（文件 $pickedFileName）")
            }
            return
        }
        val ok = result as UpgradePackage.Adopt.Ok   // Err 已提前 return，此处必为 Ok（is+return 不触发智能收敛）
        file = ok.payload
        pickedFileSize = ok.payload.size
        pickedCRC = String.format(Locale.US, "CRC32 %08X", ok.payloadCrc32)
        fileCrc32 = ok.payloadCrc32
        fileHw = ok.hwVersion.coerceIn(0, 255)   // 0xF0 版本字段：hw/sw 各 1 字节
        fileSw = (ok.swVersion ?: filenameVersion(pickedFileName)?.second ?: 0).coerceIn(0, 255)
        targetVersionText = ok.swVersion?.let { "v${ok.hwVersion}.$it" }
            ?: filenameVersion(pickedFileName)?.let { (h, s) -> "v$h.$s" }
            ?: (if (zh) "未知（以升级后读回为准）" else "Unknown (verify after update)")
        // 用户未显式选择时按文件证据预选类型（文件名后缀已在选文件时预选；此处兜底包头类型）
        if (pkgTypeSel == null) pkgTypeSel = ok.pkgType
    }

    // 系统文件选择器（升级包为二进制故 */* 全收，防线在 reAdopt → UpgradePackage.adopt）
    val filePicker = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        if (uri == null) return@rememberLauncherForActivityResult
        val read = readUpgradeFile(context, uri)
        if (read == null) {
            clearPicked()
        } else {
            val (name, raw) = read
            rawFile = raw
            pickedFileName = name
            // 文件名后缀（full/diff/patch）仅作未选择时的预选建议，最终由用户显式确认/包头校验
            if (pkgTypeSel == null) pkgTypeSel = suggestedTypeOf(name)
            reAdopt()
        }
    }

    // ===== 运行视图（stage 2-3：垂直居中，升级中无退出入口）=====
    if (stage in 2..3) {
        Column(Modifier.fillMaxSize().background(c.bg)) {
            NavbarHeader(
                title = if (zh) "固件升级（差分/全量）" else "Firmware Update (Diff/Full)",
                backText = "",   // 运行态无返回入口；升级中不允许离开
                onBack = {},
            )
            Column(
                Modifier
                    .weight(1f)
                    .padding(horizontal = 16.dp)
                    .padding(bottom = 24.dp),
            ) {
                StageListUpgrade(activeIndex = if (stage == 3) 2 else if (engineState is DiffUpgradeEngine.State.Notifying) 0 else 1, zh = zh)
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
                            Text(
                                if (stage == 3) (if (zh) "设备正在合成固件并重启…" else "Device synthesizing & rebooting…")
                                else if (engineState is DiffUpgradeEngine.State.Notifying) (if (zh) "发送升级通知…" else "Sending update notify…")
                                else (if (zh) "数据传输中（设备拉包）" else "Transferring (device-pull)"),
                                style = hkt(15f, FontWeight.SemiBold),
                                color = c.text,
                            )
                            Box(Modifier.padding(top = 9.dp)) {
                                HktProgress(
                                    fraction = if (stage == 3) 1f
                                    else packetsDone.toFloat() / totalPackets.coerceAtLeast(1),
                                )
                            }
                            Text(
                                (if (zh) "包 " else "Packet ") + String.format(Locale.US, "%,d / %,d", packetsDone, totalPackets) +
                                    (if (stage == 2 && engineState !is DiffUpgradeEngine.State.Notifying)
                                        " · ${packetsDone * 100 / totalPackets.coerceAtLeast(1)}%" else ""),
                                style = hkt(13f),
                                color = c.text2,
                                modifier = Modifier.padding(top = 4.dp),
                            )
                        }
                    }
                    if (stage == 2) {
                        HktBanner(
                            BadgeKind.WARN,
                            "⚠︎ " + (if (zh) "升级期间请保持 App 前台、勿锁屏" else "Keep the app in the foreground; don't lock the screen"),
                        )
                    }
                    if (stage == 3) {
                        Text("⏱ $waitTick s", style = hkt(13f), color = c.text2)
                    }
                }
            }
        }
        return
    }

    // ===== 选择 / 成功 / 失败（滚动区）=====
    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(
            title = if (zh) "固件升级（差分/全量）" else "Firmware Update (Diff/Full)",
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
                        label = "📄 " + (if (zh) "选择升级文件…" else "Select upgrade file…"),
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
                                        (if (zh) "目标版本" else "Target version") + ": " +
                                        targetVersionText.ifEmpty { "—" },
                                    style = hkt(13f),
                                    color = c.text2,
                                )
                            }
                        }
                        // 收包防线结果横幅：拒绝原因 / 差分底包匹配（底包不符设备必拒，事前告知）
                        val adoptErr = adopted as? UpgradePackage.Adopt.Err
                        if (adoptErr != null && adoptErr.reason != UpgradePackage.Adopt.Reason.TYPE_PENDING) {
                            HktBanner(BadgeKind.ERR, "✕ " + adoptErrorText(adoptErr, zh))
                        }
                        val adoptOk = adopted as? UpgradePackage.Adopt.Ok
                        if (adoptOk?.baseSwVersion != null) {
                            val match = adoptOk.baseSwVersion == snapshot.softwareVersion
                            HktBanner(
                                if (match) BadgeKind.OK else BadgeKind.ERR,
                                if (match) "✓ " + (if (zh) "底包匹配：该差分包适用当前固件 $currentVersion"
                                else "Base firmware matches ($currentVersion)")
                                else "⚠︎ " + (if (zh) "底包不匹配：该差分包适用 v${adoptOk.hwVersion}.${adoptOk.baseSwVersion}" +
                                    "，当前设备 $currentVersion，设备将拒绝升级"
                                else "Base mismatch: patch targets v${adoptOk.hwVersion}.${adoptOk.baseSwVersion} " +
                                    "but the device runs $currentVersion; it will reject the update"),
                            )
                        }
                    }
                    // 包类型必须由用户显式选择（协议 0xF0 包类型字段：0x00 全量 / 0x01 差分）
                    Text(
                        (if (zh) "包类型" else "Package type"),
                        style = hkt(13f, FontWeight.SemiBold),
                        color = c.text,
                        modifier = Modifier.padding(bottom = 7.dp),
                    )
                    // 改选类型必须重跑收包防线（防线与类型绑定，如全量结构校验/差分无头拒绝）
                    PkgTypeSelector(selected = pkgTypeSel, zh = zh) { pkgTypeSel = it; reAdopt() }
                    Text(
                        when (pkgTypeSel) {
                            UpgradePkgType.FULL ->
                                if (zh) "全量包：设备直接写入新固件（常规升级，也可用于底包损坏/版本不明的救援）"
                                else "Full image: written directly; also rescues devices with a damaged/unknown base"
                            UpgradePkgType.DIFF ->
                                if (zh) "差分包：体积小、升级快；必须与设备当前固件版本严格匹配（设备校验底包），不符将被拒绝"
                                else "Differential patch: small & fast; must strictly match the device's current firmware or the device rejects it"
                            else -> if (zh) "请选择升级包类型" else "Choose the package type"
                        },
                        style = hkt(12f),
                        color = c.text2,
                        modifier = Modifier.padding(top = 7.dp, bottom = 11.dp),
                    )
                    val ready = file != null && typeKnown   // file 仅在收包防线通过时非空
                    val shape = RoundedCornerShape(HktRadius.control.dp)
                    Box(
                        Modifier
                            .fillMaxWidth()
                            .background(if (ready) c.info else c.info.copy(alpha = 0.4f), shape)
                            .clickableBox(enabled = ready) { stage = 1 }
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
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(10.dp),
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 32.dp, vertical = 48.dp),
                    ) {
                        Text("✓", fontSize = 48.sp, color = c.ok)
                        Text(
                            (if (zh) "升级包传输成功 " else "Package delivered ") + "$currentVersion → $targetVersionText",
                            style = hkt(17f, FontWeight.SemiBold),
                            color = c.text,
                        )
                        Text(
                            if (zh) "设备已接收全部数据并通过校验（0xF3 成功），正在自行合成新固件并重启安装。"
                            else "The device received and verified the full package (0xF3 OK); it is synthesizing and installing the new firmware on its own.",
                            style = hkt(13f),
                            color = c.text2,
                            textAlign = TextAlign.Center,
                        )
                        Text(
                            if (zh) "安装完成后设备将重新广播，请重新连接并核对固件版本为 $targetVersionText（读回版本一致即升级成功）。"
                            else "Reconnect after the device re-advertises and confirm the firmware version is $targetVersionText.",
                            style = hkt(13f),
                            color = c.text2,
                            textAlign = TextAlign.Center,
                        )
                        Text("⏱ $totalText", style = hkt(13f), color = c.text2)
                        DialogButton(title = if (zh) "查看升级报告" else "View update report") { showReport = true }
                        Box(Modifier.padding(top = 2.dp)) {
                            DialogButton(title = if (zh) "完成" else "Done", kind = DialogButtonKind.PRIMARY, action = onBack)
                        }
                    }
                }
                stage == 9 -> {
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

    // ===== 确认升级对话框（stage 1）=====
    if (stage == 1) {
        Box(Modifier.fillMaxSize().background(c.bg))
        DialogScrimUpgrade(onDismiss = { stage = 0; clearPicked() }) {
            HktDialogCard(
                title = if (zh) "确认升级？" else "Start update?",
                message = AnnotatedString(
                    if (zh) "设备 ${session.deviceName} 将从 $currentVersion 升级到 $targetVersionText（${pkgTypeText(pkgTypeSel ?: 0, zh)}包，" +
                        "${totalPacketsOf(file, pickedFileSize)} 包）。升级期间请保持 App 前台、蓝牙开启，勿离开此页面。"
                    else "Device ${session.deviceName} will update from $currentVersion to $targetVersionText " +
                        "(${pkgTypeText(pkgTypeSel ?: 0, zh)}, ${totalPacketsOf(file, pickedFileSize)} packets). " +
                        "Keep the app in the foreground, keep Bluetooth on, and stay on this screen.",
                ),
                buttons = listOf(
                    Triple(if (zh) "取消" else "Cancel", DialogButtonKind.SECONDARY) { stage = 0; clearPicked() },
                    Triple(if (zh) "确认升级" else "Update", DialogButtonKind.PRIMARY) {
                        val image = file
                        val type = pkgTypeSel
                        if (image != null && type != null) {
                            runUpgrade(image, type)
                        } else {
                            failureReason = if (zh) "升级包无法读取" else "Upgrade file unreadable"
                            stage = 9
                        }
                    },
                ),
            )
        }
    }
    // ===== 升级报告对话框 =====
    if (showReport) {
        DialogScrimUpgrade(onDismiss = { showReport = false }) {
            HktDialogCard(
                title = if (zh) "升级报告" else "Update Report",
                message = AnnotatedString(
                    buildString {
                        listOf(
                            (if (zh) "设备" else "Device") to session.deviceName,
                            (if (zh) "包类型" else "Package type") to pkgTypeText(pkgTypeSel ?: 0, zh),
                            (if (zh) "升级前版本" else "Previous version") to currentVersion,
                            (if (zh) "目标版本" else "Target") to targetVersionText,
                            (if (zh) "传输包数" else "Packets sent") to "$totalPackets",
                            (if (zh) "结果" else "Result") to (if (zh) "传输完成，设备自行安装" else "Delivered; device installing"),
                            (if (zh) "总耗时" else "Total time") to totalText,
                        ).forEach { (k, v) -> appendLine("$k  $v") }
                    },
                ),
                buttons = listOf(Triple(if (zh) "关闭" else "Close", DialogButtonKind.SECONDARY) { showReport = false }),
            )
        }
    }
}

/** 三段进度：通知 → 数据传输 → 设备安装。 */
@Composable
private fun StageListUpgrade(activeIndex: Int, zh: Boolean) {
    val c = LocalHktColors.current
    val names = if (zh) listOf("升级通知", "数据传输", "设备安装") else listOf("Notify", "Transfer", "Install")
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

/** 包类型二选一（全量/差分）——选中的高亮为品牌蓝，未选灰底。 */
@Composable
private fun PkgTypeSelector(selected: Int?, zh: Boolean, onSelect: (Int) -> Unit) {
    val c = LocalHktColors.current
    val options = listOf(
        UpgradePkgType.FULL to (if (zh) "全量升级" else "Full Image"),
        UpgradePkgType.DIFF to (if (zh) "差分升级" else "Diff Patch"),
    )
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        options.forEach { (type, label) ->
            val sel = selected == type
            Box(
                Modifier
                    .weight(1f)
                    .background(if (sel) c.info else c.fill, RoundedCornerShape(HktRadius.control.dp))
                    .clickableBox { onSelect(type) }
                    .padding(vertical = 11.dp),
                contentAlignment = Alignment.Center,
            ) {
                Text(
                    label,
                    style = hkt(13f, FontWeight.SemiBold),
                    color = if (sel) Color.White else c.text2,
                )
            }
        }
    }
}

/** 对话框遮罩（.mask rgba(0,0,0,.42) 全屏居中，阻断穿透）。 */
@Composable
private fun DialogScrimUpgrade(onDismiss: () -> Unit, content: @Composable () -> Unit) {
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

private fun pkgTypeText(pkgType: Int, zh: Boolean): String = when (pkgType) {
    UpgradePkgType.DIFF -> if (zh) "差分" else "differential"
    else -> if (zh) "全量" else "full"
}

private fun totalPacketsOf(file: ByteArray?, size: Int): Int {
    val n = file?.size ?: size
    return if (n <= 0) 0 else (n + DiffUpgradeEngine.DEFAULT_PACKET_SIZE - 1) / DiffUpgradeEngine.DEFAULT_PACKET_SIZE
}

private fun upgradeFailureText(
    error: DiffUpgradeEngine.EngineError,
    code: Int?,
    linkLost: Boolean,
    zh: Boolean,
): String = when (error) {
    DiffUpgradeEngine.EngineError.NOTIFY_TIMEOUT ->
        if (zh) "设备未应答升级通知（重发后仍无 0xFE 应答；新升级协议需版本 ≥ 60.X；更低版本请用「固件升级（旧版）」）"
        else "Device did not acknowledge the update notify (no 0xFE after retries; new protocol requires version ≥ 60.X — use Firmware Update (Legacy) on older)"
    DiffUpgradeEngine.EngineError.PULL_TIMEOUT ->
        if (linkLost) (if (zh) "蓝牙连接丢失（传输中断）" else "Bluetooth connection lost during transfer")
        else (if (zh) "设备长时间未请求下一升级包（传输超时）" else "Device stopped requesting packets (transfer timeout)")
    DiffUpgradeEngine.EngineError.DEVICE_REJECTED -> when (code) {
        0x01 -> if (zh) "设备校验失败：差分包与设备当前固件版本不匹配，或数据损坏" else "Device verification failed: patch does not match the device firmware, or data corrupted"
        0x02 -> if (zh) "设备写入失败（Flash 异常）" else "Device flash write failed"
        0x03 -> if (zh) "设备等待升级数据超时" else "Device timed out waiting for upgrade data"
        else -> (if (zh) "设备拒绝升级（结果码 " else "Device rejected the update (code ") + "$code)"
    }
    DiffUpgradeEngine.EngineError.REPEATED_PACKET ->
        if (zh) "同一升级包连续多次校验失败，升级中止" else "The same packet failed verification repeatedly; upgrade aborted"
    DiffUpgradeEngine.EngineError.CANCELLED ->
        if (zh) "升级已取消" else "Upgrade cancelled"
}

// MARK: - 升级包读取与收包防线接线（防线本体在 core-ota UpgradePackage；新旧协议
// 分区/校验规则不同，独立于 OtaScreen.adoptFirmware）

/** 外部 Patch / NewFW 区各 256KB（协议 FLASH 分区表）→ 读取上界 512KB 防误选大文件 OOM。 */
private const val UPGRADE_READ_CAP = 512_000

/** 读所选文件原始字节（含可能的 24B 发布包头）；空/超限拒绝（返回 null，日志留痕）。 */
private fun readUpgradeFile(context: android.content.Context, uri: android.net.Uri): Pair<String, ByteArray>? {
    var displayName = uri.lastPathSegment ?: "upgrade.bin"
    runCatching {
        context.contentResolver.query(uri, arrayOf(android.provider.OpenableColumns.DISPLAY_NAME), null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) cursor.getString(0)?.let { displayName = it }
        }
    }

    val out = ByteArrayOutputStream()
    var overflow = false
    context.contentResolver.openInputStream(uri)?.use { stream ->
        val chunk = ByteArray(16_384)
        while (true) {
            val n = stream.read(chunk)
            if (n < 0) break
            if (out.size() + n > UPGRADE_READ_CAP) {
                overflow = true
                break
            }
            out.write(chunk, 0, n)
        }
    } ?: return null
    if (overflow) {
        LogStore.warn("升级包校验拒绝：文件超出外部 Patch/NewFW 区 256KB 上限（文件 $displayName）")
        return null
    }
    val data = out.toByteArray()
    if (data.isEmpty()) {
        LogStore.warn("升级包校验拒绝：文件为空（0 B）（文件 $displayName）")
        return null
    }
    return displayName to data
}

/** 文件名后缀（full/diff/patch）推断的建议包类型；仅作未选择时的预选，最终由用户确认/包头校验。 */
private fun suggestedTypeOf(name: String): Int? {
    val lower = name.lowercase()
    return when {
        lower.contains("full") -> UpgradePkgType.FULL
        lower.contains("diff") || lower.contains("patch") -> UpgradePkgType.DIFF
        else -> null
    }
}

/** 文件名 v<hw>.<sw>（如 dc200_v2.31_full.bin → 2 to 31）；无版本提示返回 null。 */
private fun filenameVersion(name: String): Pair<Int, Int>? {
    val m = Regex("v(\\d+)\\.(\\d+)").find(name) ?: return null
    val hw = m.groupValues[1].toIntOrNull() ?: return null
    val sw = m.groupValues[2].toIntOrNull() ?: return null
    if (hw !in 0..255 || sw !in 0..255) return null
    return hw to sw
}

/** 收包防线拒绝文案（横幅/日志用；原因枚举见 UpgradePackage.Adopt.Reason）。 */
private fun adoptErrorText(err: UpgradePackage.Adopt.Err, zh: Boolean): String {
    val type = err.actualType?.let { pkgTypeText(it, zh) }
    return when (err.reason) {
        UpgradePackage.Adopt.Reason.EMPTY ->
            if (zh) "文件为空" else "file is empty"
        UpgradePackage.Adopt.Reason.TYPE_PENDING ->
            if (zh) "请选择包类型以完成文件校验" else "choose the package type to verify the file"
        UpgradePackage.Adopt.Reason.TYPE_MISMATCH ->
            if (zh) "包类型不符：该文件是${type}包，请改选「${type}升级」"
            else "package type mismatch: this file is a ${type} package; switch the type"
        UpgradePackage.Adopt.Reason.HEADER_CORRUPT ->
            if (zh) "发布包头损坏（${err.detail}）" else "corrupt package header (${err.detail})"
        UpgradePackage.Adopt.Reason.PAYLOAD_CRC_MISMATCH ->
            if (zh) "文件校验失败：包头 CRC32 与内容不符，文件可能损坏或不完整"
            else "payload CRC mismatch; the file may be corrupted or truncated"
        UpgradePackage.Adopt.Reason.IMAGE_SIZE ->
            if (zh) "全量镜像大小超出新 APP 区合法范围 8KB–168KB（${err.detail}）"
            else "full image size out of range 8KB–168KB (${err.detail})"
        UpgradePackage.Adopt.Reason.IMAGE_STRUCTURE ->
            if (zh) "不是合法的全量镜像（栈顶/复位向量校验失败；若这是差分包请改选差分类型）"
            else "not a valid full image (stack top / reset vector); if this is a diff patch, switch the type"
        UpgradePackage.Adopt.Reason.FW_INFO_MISMATCH ->
            if (zh) "包头版本与镜像内嵌版本记录不一致（${err.detail}），打包工具与固件不同源"
            else "header version disagrees with the embedded record (${err.detail})"
        UpgradePackage.Adopt.Reason.DIFF_SIZE ->
            if (zh) "差分包大小超出外部 Patch 区 256KB 上限（${err.detail}）"
            else "diff package too large (${err.detail})"
        UpgradePackage.Adopt.Reason.DIFF_NEEDS_HEADER ->
            if (zh) "差分包必须由升级工具打包（缺 HKTU 包头${if (err.detail.contains("疑似全量镜像")) "；该文件疑似全量镜像，请核对包类型" else ""}）"
            else "diff packages must be packed by the upgrade tool (HKTU header missing)"
    }
}
