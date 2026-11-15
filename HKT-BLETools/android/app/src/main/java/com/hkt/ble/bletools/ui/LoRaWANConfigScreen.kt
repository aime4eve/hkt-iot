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
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.hkt.ble.bletools.core.ble.DeviceSession
import com.hkt.ble.bletools.core.protocol.ChmaskCodec
import com.hkt.ble.bletools.core.protocol.LoRaWANConfig
import com.hkt.ble.bletools.core.protocol.LoRaWANFrame
import com.hkt.ble.bletools.core.protocol.LoRaWANInbound
import com.hkt.ble.bletools.core.protocol.LoRaWANPowerTable
import com.hkt.ble.bletools.core.protocol.LoRaWANRegion
import com.hkt.ble.bletools.core.protocol.LoRaWANRegions
import com.hkt.ble.bletools.core.protocol.LoRaWANSr0Parser
import com.hkt.ble.bletools.core.protocol.LoRaWANValidation
import com.hkt.ble.bletools.designsystem.BadgeKind
import com.hkt.ble.bletools.designsystem.CenterStateView
import com.hkt.ble.bletools.designsystem.ChoiceChipRow
import com.hkt.ble.bletools.designsystem.ConfigControlRow
import com.hkt.ble.bletools.designsystem.ConfigInputRow
import com.hkt.ble.bletools.designsystem.ConfigLabel
import com.hkt.ble.bletools.designsystem.ConfigSection
import com.hkt.ble.bletools.designsystem.DialogButton
import com.hkt.ble.bletools.designsystem.DialogButtonKind
import com.hkt.ble.bletools.designsystem.HktBanner
import com.hkt.ble.bletools.designsystem.HktDialogCard
import com.hkt.ble.bletools.designsystem.HktSwitch
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.NavbarHeader
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.LanguageStore
import com.hkt.ble.bletools.model.LogStore
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.launch
import kotlinx.coroutines.withTimeoutOrNull

/**
 * LoRaWAN 参数配置页（0xA0/0xA1，LoRaWAN_ParkingSensor 固件 2026-11）。
 *
 * 生命周期（与 ConfigScreen/UpgradeScreen 同纪律）：进页停轮询（固件 RX 单缓冲，写帧撞
 * 轮询会被整段丢弃 FD-004）+ 挂 rawFrameHandler 消费 40B 0xA0 应答帧；离开释放并恢复。
 * - 查询：0xA0·01（8B）→ 40B 应答帧回填表单；超时 4s → 错误态重试（旧固件无此协议即走此态）
 * - 保存：同固件规则前置校验 → 确认摘要 → 0xA0·33B 写入，等旧 TLV 0xFF ACK；
 *   设备 ACK 后 500ms **复位重启**（断链属预期，横幅明示）
 * - SR0：0xA1·01（8B）→ 模组 AT+SR0? 裸文本经 rawTextHandler 透传显示（超时 8s），原文下方按
 *   手册表 5-24 追加 40B STATUS0 结构化解析（[LoRaWANSr0Parser]，多字节字段小端）
 *
 * UI（2026-11-14 用户裁决·精简版）：面向懂行的使用者——除 DevEUI 与模组状态外全部参数
 * 收进一个「链路参数」框；小字只保留取值范围，错误提示保留；功率以 dBm 直显
 * （[LoRaWANPowerTable] 内部映射区域相对索引）；区域用下拉选择，CN470B 暂不开放。
 */
private enum class LoRaWANLoadPhase { LOADING, LOADED, TIMEOUT }

private class LoRaWANDraft {
    var bandIdx by mutableIntStateOf(0)          // LoRaWANRegions.all 全表 index（含 CN470B）
    var chmaskText by mutableStateOf("FF00")     // 初始与 bandIdx=0（CN470）默认一致，进页查询即覆盖
    var rx2Dr by mutableIntStateOf(0)            // RX2 下行 DR（全域 0–13，与上行 drMax 无关）
    var rx2FreqHzText by mutableStateOf("869525000")
    var adr by mutableStateOf(true)
    var confirmIdx by mutableIntStateOf(0)       // 0=Confirm 1=Unconfirm
    var confirmCnt by mutableStateOf("3")
    var powerIdx by mutableIntStateOf(0)         // 区域相对索引，UI 以 dBm 直显
    var dr by mutableIntStateOf(5)
    var activationIdx by mutableIntStateOf(0)    // 0=OTAA 1=ABP
    var classIdx by mutableIntStateOf(0)         // 0=A(0) 1=C(2)
    var portText by mutableStateOf("10")
    var deveui by mutableStateOf("")
}

@Composable
fun LoRaWANConfigScreen(session: DeviceSession, onBack: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    val scope = rememberCoroutineScope()

    var phase by remember { mutableStateOf(LoRaWANLoadPhase.LOADING) }
    val draft = remember { LoRaWANDraft() }
    var errors by remember { mutableStateOf(mapOf<String, String>()) }
    var banner by remember { mutableStateOf(null as BannerKind?) }
    var showConfirm by remember { mutableStateOf(false) }
    var showSaving by remember { mutableStateOf(false) }
    var sr0Text by remember { mutableStateOf<String?>(null) }
    var sr0Loading by remember { mutableStateOf(false) }
    var sr0Error by remember { mutableStateOf<String?>(null) }
    val seq = remember { mutableIntStateOf(0) }
    val queryDeferred = remember { mutableStateOf<CompletableDeferred<LoRaWANConfig>?>(null) }

    // 进页停轮询 + 挂 0xA0 应答帧旁路；离开释放（rawFrameHandler 单槽，页面独占）
    DisposableEffect(Unit) {
        session.setPollingSuspended(true)
        session.rawFrameHandler = { frame ->
            when (val inbound = LoRaWANInbound.parse(frame)) {
                is LoRaWANInbound.ConfigResponse -> {
                    queryDeferred.value?.complete(inbound.cfg)
                    true
                }
                else -> true   // 0xA0 帧族已识别，无论有无等待方都不落入 TLV 解析
            }
        }
        onDispose {
            session.rawFrameHandler = null
            session.rawTextHandler = null
            session.setPollingSuspended(false)
        }
    }
    BackHandler { onBack() }

    val regions = LoRaWANRegions.all
    // 局部属性不允许 custom getter：普通 val 随重组重求值；事件回调内的即时读取
    // 一律显式 regions[draft.bandIdx]（见 band 切换的 applyRegionDefaults 参数化）
    val region = regions[draft.bandIdx]
    val visibleRegions = remember(regions) { regions.filter { it.band != 7 } }   // CN470B 暂不开放

    fun fillDraft(cfg: LoRaWANConfig) {
        draft.bandIdx = regions.indexOfFirst { it.band == cfg.band }.takeIf { it >= 0 } ?: 0
        draft.chmaskText = ChmaskCodec.toDisplay(cfg.chmask)
        draft.rx2Dr = cfg.rx2Dr
        draft.rx2FreqHzText = cfg.rx2FreqHz.toString()
        draft.adr = cfg.adr == 1
        draft.confirmIdx = if (cfg.confirm == 1) 0 else 1
        draft.confirmCnt = cfg.confirmCnt.toString()
        draft.powerIdx = cfg.power
        draft.dr = cfg.dr
        draft.activationIdx = if (cfg.activation == 1) 1 else 0
        draft.classIdx = if (cfg.deviceClass == 2) 1 else 0
        draft.portText = cfg.port.toString()
        draft.deveui = cfg.deveui.joinToString("") { "%02X".format(it) }
    }

    fun applyRegionDefaults(r: LoRaWANRegion) {
        draft.chmaskText = ChmaskCodec.toDisplay(ChmaskCodec.fromU16(r.chmaskDefault))
        draft.rx2Dr = r.rx2DrDefault
        draft.rx2FreqHzText = r.rx2FreqDefaultHz.toString()
        if (draft.powerIdx > r.powerMax) draft.powerIdx = 0
        if (draft.dr > r.drMax) draft.dr = r.drDefault
    }

    fun buildConfig(deveui: ByteArray): LoRaWANConfig? {
        val chmask = ChmaskCodec.parseHex(draft.chmaskText)
        if (chmask == null) {
            errors = errors + ("chmask" to (if (zh) "十六进制非法" else "invalid hex"))
            return null
        }
        return LoRaWANConfig(
            band = regions[draft.bandIdx].band,   // 即时读：局部 fun 捕获的 region 是定义帧快照（协程内调用会滞后）
            chmask = chmask,
            rx2Dr = draft.rx2Dr,
            rx2FreqHz = draft.rx2FreqHzText.trim().toLongOrNull() ?: 0L,
            adr = if (draft.adr) 1 else 0,
            confirm = if (draft.confirmIdx == 0) 1 else 0,
            confirmCnt = draft.confirmCnt.trim().toIntOrNull() ?: 0,
            power = draft.powerIdx,
            dr = draft.dr,
            activation = draft.activationIdx,
            deviceClass = if (draft.classIdx == 1) 2 else 0,
            port = draft.portText.trim().toIntOrNull() ?: 0,   // 非法数字已由 validate 先行拦截，此处兜底
            deveui = deveui,
        )
    }

    fun rangeText(field: String): String {
        val r = regions[draft.bandIdx]
        return when (field) {
            "dr" -> "0–${r.drMax}"
            "rx2Freq" -> "${r.freqMinHz / 1_000_000}–${r.freqMaxHz / 1_000_000} MHz"
            "confirmCnt" -> "1–9"
            "power" -> "0–${r.powerMax}"
            "port" -> "1–223"
            else -> ""
        }
    }

    fun validate(deveui: ByteArray): LoRaWANConfig? {
        errors = emptyMap()
        if (draft.rx2FreqHzText.trim().toLongOrNull() == null) {
            errors = errors + ("rx2Freq" to (if (zh) "非法数字" else "invalid number"))
        }
        draft.confirmCnt.trim().toIntOrNull() ?: run {
            errors = errors + ("confirmCnt" to (if (zh) "非法数字" else "invalid number"))
        }
        draft.portText.trim().toIntOrNull() ?: run {
            errors = errors + ("port" to (if (zh) "非法数字" else "invalid number"))
        }
        val cfg = buildConfig(deveui) ?: return null
        val key = LoRaWANValidation.errorKey(cfg)   // 与固件 LoRaWAN_CfgValidate 同规则
        if (key != null) {
            // chmask 专属自诊断文案（2026-11-14 真机反馈）：点名区域有效信道与违规类型，
            // 设备回显异常时用户可直接从错误信息定位（如 EU868 只认 8 个信道位）
            val msg = if (key == "chmask") {
                val r = regions[draft.bandIdx]   // 即时读，与 buildConfig 同源
                val anyOn = (0 until r.chValidBits).any {
                    ((cfg.chmask[it / 8].toInt() and 0xFF) shr (it % 8)) and 1 == 1
                }
                if (anyOn) {
                    if (zh) "信道位越界（${r.name} 有效信道 0–${r.chValidBits - 1}）"
                    else "bits beyond ch${r.chValidBits - 1} (${r.name} valid channels 0–${r.chValidBits - 1})"
                } else {
                    if (zh) "至少启用 1 个信道" else "enable at least 1 channel"
                }
            } else {
                if (zh) "超出范围 ${rangeText(key)}" else "out of range ${rangeText(key)}"
            }
            errors = errors + (key to msg)
            return null
        }
        return cfg
    }

    suspend fun query() {
        phase = LoRaWANLoadPhase.LOADING
        val deferred = CompletableDeferred<LoRaWANConfig>()
        queryDeferred.value = deferred
        LogStore.info("LoRaWAN " + (if (zh) "0xA0 查询配置 → ${session.deviceName}" else "0xA0 query → ${session.deviceName}"))
        session.sendRaw(LoRaWANFrame.queryFrame(++seq.intValue))
        val cfg = withTimeoutOrNull(4_000) { deferred.await() }
        queryDeferred.value = null
        if (cfg != null) {
            fillDraft(cfg)
            phase = LoRaWANLoadPhase.LOADED
            // 回填后立即跑一遍校验：设备回显值本身非法（flash 脏数据/异常）时进页即可见
            // 红色行内错误，而不是等到点写入才暴露
            validate(hexToBytesOrEmpty(draft.deveui))
            LogStore.info("LoRaWAN " + (if (zh) "配置已读取：${regions[draft.bandIdx].name}" else "config loaded: ${regions[draft.bandIdx].name}"))
        } else {
            phase = LoRaWANLoadPhase.TIMEOUT
            LogStore.info("LoRaWAN " + (if (zh) "查询超时（无 0xA0 应答）" else "query timeout (no 0xA0 response)"))
        }
    }
    LaunchedEffect(Unit) { query() }

    fun summary(cfg: LoRaWANConfig, zh: Boolean): String {
        val r = LoRaWANRegions.lookup(cfg.band)!!
        val powerText = LoRaWANPowerTable.dbmOf(cfg.band, cfg.power)?.let { "$it dBm" } ?: "idx ${cfg.power}"
        return buildString {
            append(if (zh) "区域 ${r.name}" else "Region ${r.name}")
            append("；chmask ${ChmaskCodec.toDisplay(cfg.chmask)}")
            append("；RX2 DR${cfg.rx2Dr} ${cfg.rx2FreqHz / 1_000_000.0} MHz")
            append("；DR${cfg.dr}")
            append("；$powerText")
            append("；${if (cfg.adr == 1) "ADR 开" else "ADR 关"}")
            append("；${if (cfg.confirm == 1) "Confirm ×${cfg.confirmCnt}" else "Unconfirm"}")
            append("；${if (cfg.activation == 0) "OTAA" else "ABP"}")
            append("；${if (cfg.deviceClass == 2) "Class C" else "Class A"}")
            append("；port ${cfg.port}")
        }
    }

    fun write(cfg: LoRaWANConfig) {
        showSaving = true
        LogStore.info("LoRaWAN " + (if (zh) "0xA0 写入配置 → ${session.deviceName}" else "0xA0 write → ${session.deviceName}"))
        scope.launch {
            val acked = session.sendRawAwaitingAck(LoRaWANFrame.configFrame(++seq.intValue, cfg), timeoutMs = 6_000)
            showSaving = false
            banner = if (acked) BannerKind.OK else BannerKind.FAIL
            LogStore.info(
                "LoRaWAN " + (
                    if (acked) (if (zh) "0xA0 ACK：配置已写入 flash，设备即将重启（断链属预期）"
                    else "0xA0 ACK: config stored, device rebooting (link drop expected)")
                    else (if (zh) "0xA0 等待确认超时（可能被固件校验拒绝）" else "0xA0 ACK timeout (possibly rejected)")
                    ),
            )
        }
    }

    fun fetchSr0() {
        if (sr0Loading) return
        sr0Loading = true
        sr0Error = null
        sr0Text = null
        scope.launch {
            val deferred = CompletableDeferred<ByteArray>()
            session.rawTextHandler = { data -> deferred.complete(data); true }
            try {
                LogStore.info("LoRaWAN " + (if (zh) "0xA1 读取模组状态（AT+SR0?）" else "0xA1 module status (AT+SR0?)"))
                session.sendRaw(LoRaWANFrame.sr0Frame(++seq.intValue))
                val text = withTimeoutOrNull(8_000) { deferred.await() }
                    ?.decodeToString()?.trim()
                if (text.isNullOrEmpty()) {
                    sr0Error = if (zh) "模组无应答（AT+SR0? 超时）" else "No module response (AT+SR0? timeout)"
                    LogStore.info("LoRaWAN " + (if (zh) "SR0 超时" else "SR0 timeout"))
                } else {
                    sr0Text = text
                    LogStore.info("LoRaWAN SR0: $text")
                }
            } finally {
                session.rawTextHandler = null
                sr0Loading = false
            }
        }
    }

    // ===== 查询态 =====
    if (phase == LoRaWANLoadPhase.LOADING) {
        Column(Modifier.fillMaxSize().background(c.bg)) {
            NavbarHeader(title = if (zh) "LoRaWAN 配置" else "LoRaWAN Config", backText = if (zh) "‹ 返回" else "‹ Back", onBack = onBack)
            Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Text(if (zh) "正在读取设备配置…" else "Reading device config…",
                    style = hkt(14f), color = c.text2, textAlign = TextAlign.Center)
            }
        }
        return
    }
    if (phase == LoRaWANLoadPhase.TIMEOUT) {
        Column(Modifier.fillMaxSize().background(c.bg)) {
            NavbarHeader(title = if (zh) "LoRaWAN 配置" else "LoRaWAN Config", backText = if (zh) "‹ 返回" else "‹ Back", onBack = onBack)
            CenterStateView(
                glyph = "⌛",
                glyphSize = 46,
                title = if (zh) "未读到设备配置" else "No config response",
                subtitle = if (zh) "设备固件可能不支持 0xA0 协议" else "Device firmware may not support 0xA0",
                buttonTitle = if (zh) "重试" else "Retry",
                onButton = { scope.launch { query() } },
            )
        }
        return
    }

    // ===== 表单：链路参数一个框 + DevEUI + 模组状态 =====
    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(title = if (zh) "LoRaWAN 配置" else "LoRaWAN Config", backText = if (zh) "‹ 返回" else "‹ Back", onBack = onBack)
        Column(
            Modifier
                .weight(1f)
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 16.dp),
        ) {
            banner?.let { kind ->
                if (kind == BannerKind.OK) {
                    HktBanner(
                        BadgeKind.OK,
                        "✓ " + (if (zh) "配置已写入，设备正在重启并重新入网（连接将断开，稍后请重新连接）"
                        else "Config stored; device rebooting to rejoin (link will drop — reconnect later)"),
                        if (zh) "完成" else "Done",
                    ) { onBack() }
                } else {
                    HktBanner(
                        BadgeKind.ERR,
                        "✕ " + (if (zh) "设备未确认配置（可能被固件校验拒绝），请检查参数后重试"
                        else "Device did not acknowledge (possibly rejected); check ranges and retry"),
                        if (zh) "重试" else "Retry",
                    ) { banner = null }
                }
            }

            ConfigSection(
                title = if (zh) "链路参数" else "Link parameters",
                state = region.name,
            ) {
                ConfigLabel(if (zh) "区域" else "Region")
                DropdownFieldRow(
                    text = region.name,
                    options = visibleRegions.map { it.name },
                    selectedIndex = visibleRegions.indexOfFirst { it.band == region.band },
                    onSelect = { index ->
                        val target = visibleRegions.getOrNull(index) ?: return@DropdownFieldRow
                        val fullIdx = regions.indexOfFirst { it.band == target.band }
                        if (fullIdx >= 0) {
                            draft.bandIdx = fullIdx
                            // 直接取目标区域（回调捕获的 region 是切换前的旧值）；
                            // 显式选择（含重选当前区域）即取该区域默认，兼作回显脏数据的复位入口
                            applyRegionDefaults(regions[fullIdx])
                        }
                    },
                )
                Spacer(Modifier.height(10.dp))

                ConfigLabel(if (zh) "信道掩码" else "chmask")
                HexFieldRow(text = draft.chmaskText, onTextChange = {
                    // 输入即过滤（含全角/粘贴脏字符）+ 即时清除本字段陈旧错误：用户改对了
                    // 红字必须立刻消失，否则会被误读为"改了还是错"（2026-11-14 真机反馈）
                    draft.chmaskText = it.filter { ch -> ch in "0123456789abcdefABCDEF" }
                        .take(ChmaskCodec.MAX_HEX_DIGITS).uppercase()
                    errors = errors - "chmask"
                })
                errors["chmask"]?.let { FieldError(it) }
                Spacer(Modifier.height(10.dp))

                ConfigLabel("RX2 " + (if (zh) "速率" else "DR"))
                DropdownFieldRow(
                    text = "DR${draft.rx2Dr}",
                    options = (0..LoRaWANValidation.RX2_DR_MAX).map { "DR$it" },
                    selectedIndex = draft.rx2Dr.coerceIn(0, LoRaWANValidation.RX2_DR_MAX),
                    onSelect = { draft.rx2Dr = it },
                )
                Spacer(Modifier.height(10.dp))

                ConfigLabel(
                    "RX2 " + (if (zh) "频率" else "frequency") +
                        (if (zh) "（${region.freqMinHz / 1_000_000}–${region.freqMaxHz / 1_000_000} MHz）"
                        else " (${region.freqMinHz / 1_000_000}–${region.freqMaxHz / 1_000_000} MHz)"),
                )
                ConfigInputRow(
                    label = "",
                    text = draft.rx2FreqHzText,
                    onTextChange = { draft.rx2FreqHzText = it; errors = errors - "rx2Freq" },
                    unit = "Hz",
                )
                errors["rx2Freq"]?.let { FieldError(it) }
                Spacer(Modifier.height(10.dp))

                ConfigLabel(if (zh) "工作通讯速率" else "Data rate")
                DropdownFieldRow(
                    text = "DR${draft.dr.coerceIn(0, region.drMax)}",
                    options = (0..region.drMax).map { "DR$it" },
                    selectedIndex = draft.dr.coerceIn(0, region.drMax),
                    onSelect = { draft.dr = it },
                )
                Spacer(Modifier.height(10.dp))

                ConfigLabel(if (zh) "发射功率" else "TX power")
                val powerOptions = LoRaWANPowerTable.dbmList(region.band).map { "$it dBm" }
                DropdownFieldRow(
                    // 索引超出映射表（异常防御）时回退显示索引
                    text = LoRaWANPowerTable.dbmOf(region.band, draft.powerIdx)
                        ?.let { "$it dBm" } ?: "idx ${draft.powerIdx}",
                    options = powerOptions,
                    selectedIndex = if (draft.powerIdx in powerOptions.indices) draft.powerIdx else -1,
                    onSelect = { draft.powerIdx = it },
                )
                Spacer(Modifier.height(10.dp))

                ConfigLabel(if (zh) "应用端口" else "Port")
                ConfigInputRow(
                    label = "",
                    text = draft.portText,
                    onTextChange = { draft.portText = it; errors = errors - "port" },
                    unit = "1–223",
                )
                errors["port"]?.let { FieldError(it) }
                Spacer(Modifier.height(10.dp))

                Box {
                    ConfigControlRow("ADR") { HktSwitch(isOn = draft.adr) }
                    Box(Modifier.matchParentSize().clickableBox { draft.adr = !draft.adr })
                }
                errors["adr"]?.let { FieldError(it) }
                Spacer(Modifier.height(10.dp))

                ChoiceChipRow(
                    label = if (zh) "上行传输类型" else "Uplink type",
                    options = listOf("Confirm", "Unconfirm"),
                    selection = draft.confirmIdx,
                    onSelect = { draft.confirmIdx = it },
                )
                if (draft.confirmIdx == 0) {
                    ConfigLabel(if (zh) "重发次数" else "Retries")
                    ConfigInputRow(
                        label = "",
                        text = draft.confirmCnt,
                        onTextChange = { draft.confirmCnt = it; errors = errors - "confirmCnt" },
                        unit = "1–9",
                    )
                    errors["confirmCnt"]?.let { FieldError(it) }
                    Spacer(Modifier.height(10.dp))
                } else {
                    Spacer(Modifier.height(10.dp))
                }

                ChoiceChipRow(
                    label = if (zh) "激活模式" else "Activation",
                    options = listOf("OTAA", "ABP"),
                    selection = draft.activationIdx,
                    onSelect = { draft.activationIdx = it },
                )
                Spacer(Modifier.height(10.dp))
                ChoiceChipRow(
                    label = "Class " + (if (zh) "模式" else "mode"),
                    options = listOf("A", "C"),
                    selection = draft.classIdx,
                    onSelect = { draft.classIdx = it },
                )
            }

            ConfigSection(title = "DevEUI" + (if (zh) "（只读）" else " (read-only)")) {
                Text(
                    draft.deveui.chunked(2).joinToString(" ").ifEmpty { "--" },
                    style = TextStyle(fontSize = 15.sp, fontFamily = FontFamily.Monospace, fontWeight = FontWeight.SemiBold),
                    color = c.text,
                )
            }

            ConfigSection(title = if (zh) "模组状态调试" else "Module status") {
                DialogButton(
                    title = if (sr0Loading) (if (zh) "读取中…" else "Reading…") else (if (zh) "读取模组状态" else "Read module status"),
                    kind = DialogButtonKind.SECONDARY,
                ) { fetchSr0() }
                sr0Error?.let {
                    Text("✕ $it", style = hkt(12f), color = c.err, modifier = Modifier.padding(top = 8.dp))
                }
                sr0Text?.let { text ->
                    Text(
                        text,
                        style = TextStyle(fontSize = 12.sp, fontFamily = FontFamily.Monospace),
                        color = c.text,
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(top = 8.dp)
                            .background(c.card2, RoundedCornerShape(8.dp))
                            .border(1.dp, c.line, RoundedCornerShape(8.dp))
                            .padding(10.dp),
                    )
                    // 原文之外追加结构化解析（模组手册表 5-24 的 40B STATUS0）；无法识别时仅显原文
                    LoRaWANSr0Parser.parse(text)?.let { st ->
                        Text(
                            if (zh) "解析结果" else "Parsed",
                            style = hkt(12f, FontWeight.SemiBold),
                            color = c.text2,
                            modifier = Modifier.padding(top = 10.dp),
                        )
                        st.rows(zh).forEach { (label, value) ->
                            Row(Modifier.fillMaxWidth().padding(top = 4.dp)) {
                                Text(label, style = hkt(12f), color = c.text2, modifier = Modifier.weight(1f))
                                Text(
                                    value,
                                    style = hkt(12f, FontWeight.Medium),
                                    color = c.text,
                                    textAlign = TextAlign.Right,
                                )
                            }
                        }
                    }
                }
            }

            Spacer(Modifier.height(12.dp))
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(10.dp),
            ) {
                // 读取配置（次级）：重新 0xA0 查询并回填表单——进页读取失败可重试；
                // 改了参数未写入时一键恢复设备当前值（2026-11-14 用户需求）。
                // query() 置 LOADING 全屏态，成功即回 LOADED 表单刷新
                Box(
                    Modifier
                        .weight(1f)
                        .background(c.card2, RoundedCornerShape(10.dp))
                        .border(1.dp, c.line, RoundedCornerShape(10.dp))
                        .clickableBox { scope.launch { query() } }
                        .padding(vertical = 13.dp),
                    contentAlignment = Alignment.Center,
                ) {
                    Text(
                        if (zh) "读取配置" else "Read Config",
                        style = hkt(15f, FontWeight.SemiBold),
                        color = c.text,
                    )
                }
                Box(
                    Modifier
                        .weight(2f)
                        .background(c.info, RoundedCornerShape(10.dp))
                        .clickableBox {
                            // 回填查询到的 DevEUI（固件写入时忽略该字段）
                            val deveui = if (draft.deveui.length == 16) hexToBytesOrEmpty(draft.deveui) else ByteArray(8)
                            val cfg = validate(deveui)
                            if (cfg != null) showConfirm = true
                        }
                        .padding(vertical = 13.dp),
                    contentAlignment = Alignment.Center,
                ) {
                    Text(if (zh) "写入配置" else "Write Config", style = hkt(16f, FontWeight.SemiBold), color = Color.White)
                }
            }
            Spacer(Modifier.height(24.dp))
        }
    }

    // ===== 确认对话框 =====
    if (showConfirm) {
        val deveui = if (draft.deveui.length == 16) hexToBytesOrEmpty(draft.deveui) else ByteArray(8)
        val cfg = buildConfig(deveui)
        if (cfg != null) {
            Box(
                Modifier.fillMaxSize().background(Color.Black.copy(alpha = 0.42f))
                    .clickableBox { showConfirm = false },
                contentAlignment = Alignment.Center,
            ) {
                HktDialogCard(
                    title = if (zh) "写入以下 LoRaWAN 配置？" else "Write these LoRaWAN values?",
                    message = androidx.compose.ui.text.AnnotatedString(summary(cfg, zh)),
                    buttons = listOf(
                        Triple(if (zh) "取消" else "Cancel", DialogButtonKind.SECONDARY) { showConfirm = false },
                        Triple(if (zh) "确认写入" else "Write Values", DialogButtonKind.PRIMARY) {
                            showConfirm = false
                            write(cfg)
                        },
                    ),
                )
            }
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
                Text("0xA0 → ${session.deviceName}", style = hkt(13f), color = c.text2, textAlign = TextAlign.Center)
            }
        }
    }
}

private fun hexToBytesOrEmpty(hex: String): ByteArray {
    val clean = hex.trim()
    if (clean.length != 16) return ByteArray(8)
    return ByteArray(8) { i -> clean.substring(i * 2, i * 2 + 2).toInt(16).toByte() }
}

@Composable
private fun FieldError(text: String) {
    Text("✕ $text", style = hkt(12f), color = LocalHktColors.current.err, modifier = Modifier.padding(top = 4.dp))
}

/** 下拉选择行（区域/发射功率 dBm）。 */
@Composable
private fun DropdownFieldRow(
    text: String,
    options: List<String>,
    selectedIndex: Int,
    onSelect: (Int) -> Unit,
) {
    val c = LocalHktColors.current
    val shape = RoundedCornerShape(10.dp)
    var expanded by remember { mutableStateOf(false) }
    Box {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier
                .fillMaxWidth()
                .background(c.card2, shape)
                .border(1.dp, c.line, shape)
                .clickableBox { expanded = true }
                .padding(horizontal = 10.dp, vertical = 10.dp),
        ) {
            Text(
                text,
                style = TextStyle(fontSize = 15.sp, fontFamily = FontFamily.Monospace, fontWeight = FontWeight.SemiBold),
                color = c.text,
                modifier = Modifier.weight(1f),
            )
            Text("▾", style = hkt(13f), color = c.text2)
        }
        DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
            options.forEachIndexed { index, option ->
                DropdownMenuItem(
                    text = { Text(option + (if (index == selectedIndex) "  ✓" else "")) },
                    onClick = {
                        expanded = false
                        onSelect(index)
                    },
                )
            }
        }
    }
}

/** 十六进制文本输入行（chmask 用：Ascii 键盘可输 A-F，数字键盘不适用）。 */
@Composable
private fun HexFieldRow(
    text: String,
    onTextChange: (String) -> Unit,
) {
    val c = LocalHktColors.current
    val shape = RoundedCornerShape(10.dp)
    Column(
        Modifier
            .fillMaxWidth()
            .background(c.card2, shape)
            .border(1.dp, c.line, shape)
            .padding(horizontal = 10.dp, vertical = 10.dp),
    ) {
        BasicTextField(
            value = text,
            onValueChange = onTextChange,
            singleLine = true,
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Ascii),
            textStyle = TextStyle(fontSize = 15.sp, fontFamily = FontFamily.Monospace, fontWeight = FontWeight.SemiBold)
                .copy(color = c.text),
            modifier = Modifier.fillMaxWidth(),
        )
    }
}
