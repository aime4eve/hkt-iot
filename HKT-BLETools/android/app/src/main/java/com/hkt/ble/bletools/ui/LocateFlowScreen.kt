package com.hkt.ble.bletools.ui

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.BackHandler
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.border
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import com.hkt.ble.bletools.core.ble.DiscoveredDevice
import com.hkt.ble.bletools.designsystem.BadgeKind
import com.hkt.ble.bletools.designsystem.CenterStateView
import com.hkt.ble.bletools.designsystem.DialogButtonKind
import com.hkt.ble.bletools.designsystem.HktDialogCard
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.NavbarHeader
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.ConnectModel
import com.hkt.ble.bletools.model.LanguageStore
import com.hkt.ble.bletools.model.ScanModel
import com.journeyapps.barcodescanner.BarcodeCallback
import com.journeyapps.barcodescanner.BarcodeResult
import com.journeyapps.barcodescanner.DecoratedBarcodeView

/**
 * R-2 扫码定位（P-01b；2026-09-20 重构同构 iOS LocateFlowView）：⌖ 直启相机（选择弹层与手输
 * 对话框废弃，DevEUI 检索改为列表模糊查询 R-33）。状态机 = ScanModel.locatePhase（finding/notFound，
 * 30s 超时未找到）；相机 = zxing-embedded（旧版 QrCodeActivity 同库，真会话）：
 * 识别文本须 16 位 hex DevEUI，否则红字提示继续扫。
 * 定位命中：由本定位层自己渲染连接覆盖层（内嵌，勿移回扫描页——iOS 2026-09-15 真机缺陷：
 * 两个全屏弹层竞争宿主导致覆盖层吞掉定位层）。驻留会话切换走 R-32 确认框。
 */
@Composable
fun LocateFlowScreen(model: ScanModel, onDismiss: () -> Unit, demoDevEUI: String? = null) {
    val zh = LanguageStore.isZh
    var connector by remember { mutableStateOf<ConnectModel?>(null) }   // 命中后的连接覆盖层（null=未命中）
    var pendingTarget by remember { mutableStateOf<DiscoveredDevice?>(null) }   // R-32 切换确认目标
    val resident by model.residentDevice.collectAsState()
    val activeSession by model.activeSession.collectAsState()

    val locatePhase by model.locatePhase.collectAsState()
    val locateSuffix by model.locateSuffix.collectAsState()
    val hit by model.locateHitDevice.collectAsState()

    // 定位命中消费（iOS onChange(of: locateHitDevice) 同构）：同设备直接回详情，异设备走切换确认
    LaunchedEffect(hit) {
        val target = hit ?: return@LaunchedEffect
        model.consumeLocateHit()
        val session = activeSession
        val current = resident
        if (session != null && current != null) {
            if (current.identifier == target.identifier) {
                model.requestDetailOpen()   // 定位到的就是当前设备：直接回详情
                onDismiss()
            } else {
                pendingTarget = target      // R-32 切换确认（见对话框）
            }
        } else {
            connector = model.connectModel(target)
        }
    }

    // 连接成功：扫描页消费 requestDetail 推详情页，本定位层随覆盖层退场
    if (connector != null) {
        val cm = connector ?: return
        ConnectOverlayScreen(model = cm, onFinished = {
            if (cm.outcome.value == ConnectModel.Outcome.CONNECTED) {
                model.makeAndStartSession(cm.target)
                model.requestDetailOpen()
            }
            connector = null
            onDismiss()
        })
        return
    }

    // R-32 切换确认框（R-31 释放语义：取消=维持现状退出定位，确认=断开旧会话连接目标）
    val switchTarget = pendingTarget
    val current = resident
    if (switchTarget != null && current != null) {
        val segment = { text: String, bold: Boolean ->
            AnnotatedString(text, SpanStyle(fontWeight = if (bold) FontWeight.Bold else null))
        }
        val message = buildAnnotatedString {
            append(if (zh) "当前已连接 " else "Currently connected to ")
            append(segment(current.name, true))
            append(" …${idSuffix(current.identifier)}" + (if (zh) "。切换将断开当前会话并连接 " else ". Switching will disconnect it and connect to "))
            append(segment(switchTarget.name, true))
            append(" …${idSuffix(switchTarget.identifier)}" + (if (zh) "。" else "."))
        }
        Box(
            Modifier
                .fillMaxSize()
                .background(LocalHktColors.current.bg),
            contentAlignment = Alignment.Center,
        ) {
            HktDialogCard(
                title = if (zh) "切换设备？" else "Switch device?",
                message = message,
                buttons = listOf(
                    Triple(if (zh) "取消" else "Cancel", DialogButtonKind.SECONDARY) {
                        pendingTarget = null
                        onDismiss()   // 维持现状：退出定位流程，原连接保持不动
                    },
                    Triple(if (zh) "切换并连接" else "Switch & Connect", DialogButtonKind.PRIMARY) {
                        pendingTarget = null
                        model.disconnectActive()
                        connector = model.connectModel(switchTarget)
                    },
                ),
            )
        }
        return
    }

    when (locatePhase) {
        ScanModel.LocatePhase.FINDING, ScanModel.LocatePhase.NOT_FOUND ->
            LocatePhasePage(model, locatePhase, locateSuffix, zh, onDismiss)
        else -> CameraPage(model, zh, onDismiss, demoDevEUI)
    }
}

/** 定位中 / 未找到（全屏页；finding=📡 脉冲扩散环，notFound=🔍 + 核对提示）。 */
@Composable
private fun LocatePhasePage(
    model: ScanModel,
    phase: ScanModel.LocatePhase?,
    suffix: String?,
    zh: Boolean,
    onDismiss: () -> Unit,
) {
    val c = LocalHktColors.current
    BackHandler { model.cancelLocate(); onDismiss() }
    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(
            title = if (zh) "定位设备" else "Locate Device",
            backText = if (zh) "‹ 取消" else "‹ Cancel",
            onBack = { model.cancelLocate(); onDismiss() },
        )
        Spacer(Modifier.weight(1f))
        if (phase == ScanModel.LocatePhase.FINDING) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = androidx.compose.foundation.layout.Arrangement.spacedBy(10.dp),
                modifier = Modifier.fillMaxWidth().padding(horizontal = 32.dp),
            ) {
                PulseCircle()
                Text(
                    (if (zh) "正在寻找 …" else "Searching for …") + (suffix ?: ""),
                    style = hkt(17f, FontWeight.SemiBold),
                    color = c.text,
                )
                Text(
                    if (zh) "靠近目标设备可加快定位，找到后自动连接"
                    else "Move closer to speed up locating; connects automatically once found",
                    style = hkt(13f).copy(lineHeight = (13f * 1.5f).sp),
                    color = c.text2,
                    textAlign = TextAlign.Center,
                )
            }
        } else {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = androidx.compose.foundation.layout.Arrangement.spacedBy(10.dp),
                modifier = Modifier.fillMaxWidth().padding(horizontal = 32.dp),
            ) {
                Text("🔍", fontSize = 44.sp)
                Text(
                    (if (zh) "未找到 …" else "…") + (suffix ?: "") + (if (zh) "" else " not found"),
                    style = hkt(17f, FontWeight.SemiBold),
                    color = c.text,
                )
                Text(
                    if (zh) "请确认设备已上电、在信号范围内，并核对标签 DevEUI 后 6 位"
                    else "Make sure the device is powered and nearby; check the last 6 chars of the label DevEUI",
                    style = hkt(13f).copy(lineHeight = (13f * 1.5f).sp),
                    color = c.text2,
                    textAlign = TextAlign.Center,
                )
            }
        }
        Spacer(Modifier.weight(1f))
    }
}

/** .pulse：72 圆 info 16% 底 + ping 扩散环（原型 ping 动画）。 */
@Composable
private fun PulseCircle() {
    val c = LocalHktColors.current
    val transition = rememberInfiniteTransition(label = "pulse")
    val progress by transition.animateFloat(
        initialValue = 0f,
        targetValue = 1f,
        animationSpec = infiniteRepeatable(tween(1600), RepeatMode.Restart),
        label = "pulse-progress",
    )
    Box(contentAlignment = Alignment.Center) {
        // 扩散环：0.55x→1.3x，opacity 0.7→0
        Canvas(Modifier.size(130.dp)) {
            val ringScale = 0.55f + progress * 0.75f
            val radius = 36.dp.toPx() * ringScale
            drawCircle(
                c.info,
                radius = radius,
                alpha = (1f - progress) * 0.7f,
                style = Stroke(width = 2.dp.toPx()),
            )
        }
        Box(
            Modifier
                .size(72.dp)
                .background(c.info.copy(alpha = 0.16f), androidx.compose.foundation.shape.CircleShape),
            contentAlignment = Alignment.Center,
        ) {
            Text("📡", fontSize = 30.sp)
        }
    }
}

private enum class CamState { RUNNING, DENIED, NO_CAMERA }

/** ⌖ 直启相机（默认态）：深底 + 取景四角框 + 提示；识别结果 16 位 hex 才进定位。 */
@Composable
private fun CameraPage(model: ScanModel, zh: Boolean, onDismiss: () -> Unit, demoDevEUI: String? = null) {
    val context = LocalContext.current
    var qrHint by remember { mutableStateOf<String?>(null) }

    // 演示深链（iOS -demo-page locate-finding 同构）：定位中态直达
    LaunchedEffect(Unit) {
        demoDevEUI?.let { model.startLocate(it) }
    }

    val granted = remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED,
        )
    }
    val permissionLauncher = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) {
        granted.value = it
    }
    LaunchedEffect(Unit) {
        if (!granted.value) permissionLauncher.launch(Manifest.permission.CAMERA)
    }
    val hasCamera = remember {
        context.packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA_ANY)
    }
    val camState = when {
        !hasCamera -> CamState.NO_CAMERA
        granted.value -> CamState.RUNNING
        else -> CamState.DENIED
    }

    // 识别结果处理：16 位 hex 才进定位（iOS handleQR 同规），否则提示并继续扫
    fun handleQR(text: String) {
        if (model.locatePhase.value != null) return   // 已在定位流程
        val cleaned = text.trim().uppercase()
        if (cleaned.length == 16 && cleaned.all { it.isDigit() || it in 'A'..'F' }) {
            qrHint = null
            model.startLocate(cleaned)
        } else {
            val short = if (text.length > 40) text.take(40) + "…" else text
            qrHint = if (zh) "二维码内容不是 16 位 DevEUI：$short" else "Not a 16-hex DevEUI: $short"
        }
    }

    Box(Modifier.fillMaxSize().background(Color(0xFF0B0B0F))) {
        if (camState == CamState.RUNNING) {
            AndroidView(
                factory = { ctx ->
                    DecoratedBarcodeView(ctx).apply {
                        // 隐藏 zxing 自带底部状态文字（原型无此元素；nonTransitiveRClass 下用 getIdentifier）
                        val statusId = ctx.resources.getIdentifier("zxing_status_view", "id", ctx.packageName)
                        if (statusId != 0) findViewById<android.view.View>(statusId)?.visibility = android.view.View.GONE
                        decodeContinuous(BarcodeCallback { result -> handleQR(result.text) })
                        resume()
                    }
                },
                onRelease = { it.pause() },
                modifier = Modifier.fillMaxSize(),
            )
        }
        Column(Modifier.fillMaxSize()) {
            NavbarHeader(
                title = if (zh) "扫描设备二维码" else "Scan Device QR Code",
                backText = if (zh) "取消" else "Cancel",
                onBack = onDismiss,
            )
            Spacer(Modifier.weight(1f))
            if (camState == CamState.RUNNING) {
                Box(contentAlignment = Alignment.Center, modifier = Modifier.fillMaxWidth()) {
                    CamCornerFrame()
                }
                Text(
                    if (zh) "对准设备标签上的二维码，自动识别" else "Point at the QR code on the device label to scan",
                    style = hkt(13f),
                    color = Color(0xFFBBBBBB),
                    textAlign = TextAlign.Center,
                    modifier = Modifier.fillMaxWidth().padding(top = 26.dp),
                )
                qrHint?.let { hint ->
                    Text(
                        "✕ $hint",
                        style = hkt(12f),
                        color = Color(0xFFFF6B6B),
                        textAlign = TextAlign.Center,
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 24.dp, vertical = 10.dp),
                    )
                }
            } else {
                CameraFallback(camState, zh)
            }
            Spacer(Modifier.weight(1f))
        }
    }
}

/** 权限拒绝引导 / 无相机（模拟器）兜底。 */
@Composable
private fun CameraFallback(camState: CamState, zh: Boolean) {
    val context = LocalContext.current
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = androidx.compose.foundation.layout.Arrangement.spacedBy(14.dp),
        modifier = Modifier.fillMaxWidth().padding(horizontal = 32.dp),
    ) {
        Text(if (camState == CamState.DENIED) "📷" else "⚠️", fontSize = 44.sp)
        Text(
            if (camState == CamState.DENIED) (if (zh) "相机权限未开启" else "Camera access is off")
            else (if (zh) "没有可用相机" else "No camera available"),
            style = hkt(17f, FontWeight.SemiBold),
            color = Color.White,
        )
        Text(
            if (camState == CamState.DENIED) (if (zh) "请在 系统设置 › HKT BLETools › 允许「相机」后返回重试"
            else "Allow the camera in Settings › HKT BLETools, then come back")
            else (if (zh) "此设备无法扫码，可返回使用列表搜索定位设备" else "This device has no camera; use the list search to locate devices"),
            style = hkt(13f).copy(lineHeight = (13f * 1.5f).sp),
            color = Color(0xFFBBBBBB),
            textAlign = TextAlign.Center,
        )
        if (camState == CamState.DENIED) {
            val shape = androidx.compose.foundation.shape.RoundedCornerShape(com.hkt.ble.bletools.designsystem.HktRadius.control.dp)
            Box(
                Modifier
                    .border(1.dp, Color(0xFF23ADE5), shape)
                    .clickableBox {
                        runCatching {
                            context.startActivity(
                                android.content.Intent(
                                    android.provider.Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                                    android.net.Uri.parse("package:${context.packageName}"),
                                ),
                            )
                        }
                    }
                    .padding(horizontal = 28.dp, vertical = 11.dp),
            ) {
                Text(
                    if (zh) "打开设置" else "Open Settings",
                    style = hkt(15f, FontWeight.SemiBold),
                    color = Color(0xFF23ADE5),
                )
            }
        }
    }
}

/// 相机取景四角框（.cam .frame：240×240、四角 34px 3px 线、圆角端点）。
@Composable
private fun CamCornerFrame() {
    val accent = Color(0xFF23ADE5)
    Canvas(Modifier.size(240.dp)) {
        val corner = 34.dp.toPx()
        val stroke = 3.dp.toPx()
        // 左上
        drawLine(accent, Offset(0f, corner), Offset(0f, 0f), stroke, StrokeCap.Round)
        drawLine(accent, Offset(0f, 0f), Offset(corner, 0f), stroke, StrokeCap.Round)
        // 右上
        drawLine(accent, Offset(size.width - corner, 0f), Offset(size.width, 0f), stroke, StrokeCap.Round)
        drawLine(accent, Offset(size.width, 0f), Offset(size.width, corner), stroke, StrokeCap.Round)
        // 左下
        drawLine(accent, Offset(0f, size.height - corner), Offset(0f, size.height), stroke, StrokeCap.Round)
        drawLine(accent, Offset(0f, size.height), Offset(corner, size.height), stroke, StrokeCap.Round)
        // 右下
        drawLine(accent, Offset(size.width - corner, size.height), Offset(size.width, size.height), stroke, StrokeCap.Round)
        drawLine(accent, Offset(size.width, size.height), Offset(size.width, size.height - corner), stroke, StrokeCap.Round)
    }
}
