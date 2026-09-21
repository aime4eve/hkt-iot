package com.hkt.ble.bletools.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.hkt.ble.bletools.core.ble.ConnectPhase
import com.hkt.ble.bletools.designsystem.DialogButton
import com.hkt.ble.bletools.designsystem.DialogButtonKind
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.Steps
import com.hkt.ble.bletools.designsystem.clickableBox
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.ConnectModel
import com.hkt.ble.bletools.model.ScanModel

/**
 * P-02 连接覆盖层 —— 1:1 克隆冻结原型（规格卡 docs/ios/design/ui-spec/P-02.md）。
 * 结构：.overlay 全屏 bg → 弹性居中（🔗44 + 设备名 …后4位 17/600 + steps(3) + 阶段文案 13）
 * → 底部取消（.btn secondary 全宽，容器 padding 24）。
 * 失败态（§4-1 裁决）：居中改显 err 文案 + primary「重新连接」+ secondary「返回」。
 * 弹出即 start()（spec §4-2：此前连接不启动的根因）。
 */
@Composable
fun ConnectOverlayScreen(model: ConnectModel, onFinished: () -> Unit) {
    val c = LocalHktColors.current
    val zh = HktLang.isZh
    val outcome by model.outcome.collectAsState()
    val phase by model.phaseFlow.collectAsState()
    // 弹出即启动（spec §4-2；iOS ConnectOverlayView onAppear start 同构）
    androidx.compose.runtime.LaunchedEffect(Unit) { model.start() }
    // 成功/取消即关闭；失败态留在原地显示重试/返回（spec §4-1）
    androidx.compose.runtime.LaunchedEffect(outcome) {
        if (outcome == ConnectModel.Outcome.CONNECTED || outcome == ConnectModel.Outcome.CANCELLED) onFinished()
    }

    val failed = outcome == ConnectModel.Outcome.FAILED
    val failureText = model.failureText

    Column(Modifier.fillMaxSize().background(c.bg)) {
        Box(Modifier.weight(1f), contentAlignment = Alignment.Center) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = androidx.compose.foundation.layout.Arrangement.spacedBy(10.dp),
                modifier = Modifier.padding(horizontal = 32.dp),
            ) {
                Text("🔗", fontSize = 44.sp)
                Text(
                    "${model.target.name} …${idSuffixPublic(model.target.identifier)}",
                    style = hkt(17f, FontWeight.SemiBold),
                    color = c.text,
                )
                Steps(total = 3, onCount = if (failed) 0 else model.phaseIndex + 1)
                if (failed && failureText != null) {
                    // 失败态（§4-1 裁决）：err 色文案 + 重试/返回
                    Text(failureText, style = hkt(13f), color = c.err)
                    Spacer(Modifier.height(6.dp))
                    DialogButton(title = if (zh) "重新连接" else "Retry", kind = DialogButtonKind.PRIMARY) {
                        model.retry()
                    }
                    DialogButton(title = if (zh) "返回" else "Back", kind = DialogButtonKind.SECONDARY) {
                        onFinished()
                    }
                } else {
                    Text(
                        when (phase) {
                            com.hkt.ble.bletools.core.ble.ConnectPhase.LINK -> if (zh) "正在连接" else "Connecting"
                            com.hkt.ble.bletools.core.ble.ConnectPhase.SERVICES -> if (zh) "正在发现服务…" else "Discovering services…"
                            com.hkt.ble.bletools.core.ble.ConnectPhase.SUBSCRIBING -> if (zh) "正在订阅通知…" else "Subscribing to notifications…"
                            null -> if (zh) "正在连接" else "Connecting"
                        },
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

/** 占位详情（⚠️ 临时脚手架——P-03 详情页全量克隆在 M6 下一站替换，非 §9 交付物）。 */
@Composable
fun DeviceDetailPlaceholder(
    session: com.hkt.ble.bletools.core.ble.DeviceSession,
    onBack: () -> Unit,
) {
    val c = LocalHktColors.current
    val zh = HktLang.isZh
    val snapshot by session.snapshot.collectAsState()
    val polls by session.pollsSent.collectAsState()
    val linkLost by session.linkLost.collectAsState()
    androidx.compose.foundation.layout.Column(Modifier.fillMaxSize().background(c.bg)) {
        Box(Modifier.fillMaxWidth().padding(top = 10.dp, bottom = 10.dp).padding(horizontal = 16.dp)) {
            Text(
                if (zh) "‹ 返回（P-03 全量克隆待替换）" else "‹ Back (P-03 clone pending)",
                style = hkt(14f, FontWeight.SemiBold),
                color = c.info,
                modifier = Modifier.clickableBox(onBack),
            )
        }
        Column(Modifier.padding(horizontal = 16.dp)) {
            Text(session.deviceName, style = hkt(23f, FontWeight.Bold), color = c.text)
            Spacer(Modifier.height(8.dp))
            Text(
                (if (zh) "轮询 " else "polls ") + "$polls" + (if (zh) " 次" else "") +
                    (if (linkLost) (if (zh) " · 链路丢失" else " · link lost") else ""),
                style = hkt(13f),
                color = c.text2,
            )
            Spacer(Modifier.height(12.dp))
            Text(
                "hw ${snapshot.hardwareVersion} · sw ${snapshot.softwareVersion} · " +
                    "power ${snapshot.power} · battery ${snapshot.batteryPercent ?: "-"}",
                style = hkt(13f),
                color = c.text,
            )
        }
    }
}

internal fun idSuffixPublic(identifier: String): String =
    identifier.replace(":", "").takeLast(4).uppercase()
