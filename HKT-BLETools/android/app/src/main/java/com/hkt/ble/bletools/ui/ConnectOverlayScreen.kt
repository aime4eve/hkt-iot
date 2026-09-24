package com.hkt.ble.bletools.ui

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.hkt.ble.bletools.core.ble.ConnectFailure
import com.hkt.ble.bletools.core.ble.ConnectPhase
import com.hkt.ble.bletools.designsystem.DialogButton
import com.hkt.ble.bletools.designsystem.DialogButtonKind
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.Steps
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.ConnectModel
import com.hkt.ble.bletools.model.LanguageStore

/**
 * P-02 连接覆盖层 —— 1:1 克隆冻结原型（规格卡 docs/ios/design/ui-spec/P-02.md）。
 * 结构：.overlay 全屏 bg → 弹性居中（🔗44 + 设备名…后4位 17/600 + steps(3) + 阶段文案 13）
 * → 底部取消（.btn secondary 全宽，容器 padding 24）。
 * 失败态（§4-1 裁决 + iOS 布局对齐，评审 P2-4）：居中区整体换 ⚠️44 + 失败文案 + 重新连接/返回，
 * 🔗/Steps 不渲染。
 * 弹出即 start()（§4-2）；系统返回手势 = 取消连接（iOS fullScreenCover dismiss 语义，评审 P1-1）。
 */
@Composable
fun ConnectOverlayScreen(model: ConnectModel, onFinished: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh
    val outcome by model.outcome.collectAsState()
    val failure by model.failure.collectAsState()
    val phaseIndex by model.phaseIndex.collectAsState()

    // 系统返回=取消连接回扫描页（评审 P1-1）
    BackHandler { model.cancel() }
    // 弹出即启动（§4-2；iOS ConnectOverlayView onAppear start 同构）
    LaunchedEffect(Unit) { model.start() }
    // 成功/取消即关闭；失败态留在原地显示重试/返回（§4-1）
    LaunchedEffect(outcome) {
        if (outcome == ConnectModel.Outcome.CONNECTED || outcome == ConnectModel.Outcome.CANCELLED) onFinished()
    }

    val failed = outcome == ConnectModel.Outcome.FAILED
    val failureText = connectFailureText(failure, zh)

    Column(Modifier.fillMaxSize().background(c.bg)) {
        // ⚠️ 必须填满宽度再居中：Box 只包内容宽时会被外层 Column 贴左放置（真机截图 2026-09-23）
        Box(Modifier.weight(1f).fillMaxWidth(), contentAlignment = Alignment.Center) {
            if (failed) {
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.spacedBy(10.dp),
                    modifier = Modifier.padding(horizontal = 32.dp),
                ) {
                    Text("⚠️", fontSize = 44.sp)
                    Text(
                        failureText ?: "",
                        style = hkt(13f),
                        color = c.err,
                        lineHeight = (13f * 1.5f).sp,
                    )
                    Spacer(Modifier.height(6.dp))
                    DialogButton(title = if (zh) "重新连接" else "Retry", kind = DialogButtonKind.PRIMARY) {
                        model.retry()
                    }
                    DialogButton(title = if (zh) "返回" else "Back", kind = DialogButtonKind.SECONDARY) {
                        onFinished()
                    }
                }
            } else {
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.spacedBy(10.dp),
                    modifier = Modifier.padding(horizontal = 32.dp),
                ) {
                    Text("🔗", fontSize = 44.sp)
                    Text(
                        "${model.target.name} …${idSuffix(model.target.identifier)}",
                        style = hkt(17f, FontWeight.SemiBold),
                        color = c.text,
                    )
                    Steps(total = 3, onCount = phaseIndex + 1)
                    Text(
                        connectPhaseText(model.phaseFlow.value, zh),
                        style = hkt(13f),
                        color = c.text2,
                        lineHeight = (13f * 1.5f).sp,
                    )
                }
            }
        }
        if (!failed) {
            // 取消语义（cancelConn）：终止连接并回扫描页（不做驻留）
            Box(Modifier.fillMaxWidth().padding(24.dp)) {
                DialogButton(
                    title = if (zh) "取消" else "Cancel",
                    kind = DialogButtonKind.SECONDARY,
                ) { model.cancel() }
            }
        }
    }
}

/** 阶段文案（规格卡 §2.2）。 */
private fun connectPhaseText(phase: ConnectPhase?, zh: Boolean): String = when (phase) {
    ConnectPhase.LINK -> if (zh) "正在连接" else "Connecting"
    ConnectPhase.SERVICES -> if (zh) "正在发现服务…" else "Discovering services…"
    ConnectPhase.SUBSCRIBING -> if (zh) "正在订阅通知…" else "Subscribing to notifications…"
    null -> if (zh) "正在连接" else "Connecting"
}

/** 失败文案（R-G4：带阶段/原因定位；双语随 UI 语言——评审 P1-3，不进模型层）。 */
internal fun connectFailureText(failure: ConnectFailure?, zh: Boolean): String? = when (failure) {
    ConnectFailure.TIMEOUT_LINK -> if (zh) "连接超时" else "Connection timed out"
    ConnectFailure.TIMEOUT_SERVICES -> if (zh) "发现服务超时" else "Service discovery timed out"
    ConnectFailure.TIMEOUT_SUBSCRIBING -> if (zh) "订阅通知超时" else "Notification subscription timed out"
    ConnectFailure.SERVICE_MISSING -> if (zh) "设备缺少 HKT 服务" else "HKT service missing"
    ConnectFailure.CONNECTION_LOST -> if (zh) "连接已中断" else "Connection lost"
    ConnectFailure.CANCELLED -> if (zh) "已取消" else "Cancelled"
    null -> null
}
