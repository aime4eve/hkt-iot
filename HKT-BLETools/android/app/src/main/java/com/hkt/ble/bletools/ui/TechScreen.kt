package com.hkt.ble.bletools.ui

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.Canvas
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
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.PathEffect
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import kotlin.math.max
import androidx.compose.ui.unit.sp
import com.hkt.ble.bletools.core.ble.DeviceSession
import com.hkt.ble.bletools.designsystem.BadgeKind
import com.hkt.ble.bletools.designsystem.HktCard
import com.hkt.ble.bletools.designsystem.HktRadius
import com.hkt.ble.bletools.designsystem.LocalHktColors
import com.hkt.ble.bletools.designsystem.NavbarHeader
import com.hkt.ble.bletools.designsystem.SectionHeader
import com.hkt.ble.bletools.designsystem.StateBadge
import com.hkt.ble.bletools.designsystem.hkt
import com.hkt.ble.bletools.model.LanguageStore

/**
 * P-03b 技术参数页（DC200Family 工程诊断，R-27/SP-27）——1:1 克隆冻结原型（iOS TechView 同构）。
 * 判定对照（0x3A 车位 / 0x3B 工作模式）+ 地磁三轴最近 30 组折线（X 蓝/Y 绿/Z 橙，随轮询积累）
 * + 雷达 10 段频谱柱状（0x60）。返回-only 导航（R-28：技术参数页不显示首页）。
 */
@Composable
fun TechScreen(session: DeviceSession, onBack: () -> Unit) {
    val c = LocalHktColors.current
    val zh = LanguageStore.isZh

    val snapshot by session.snapshot.collectAsState()
    val lastResponseAt by session.lastResponseAtMs.collectAsState()
    val histX = remember { mutableStateListOf<Int>() }
    val histY = remember { mutableStateListOf<Int>() }
    val histZ = remember { mutableStateListOf<Int>() }
    val cap = 30

    BackHandler { onBack() }

    // 样本积累（每轮询应答追加一组，环形 30；iOS appendSample 同构）
    LaunchedEffect(lastResponseAt) {
        if (lastResponseAt == null) return@LaunchedEffect
        val x = snapshot.magX ?: return@LaunchedEffect
        val y = snapshot.magY ?: return@LaunchedEffect
        val z = snapshot.magZ ?: return@LaunchedEffect
        histX.add(x); histY.add(y); histZ.add(z)
        if (histX.size > cap) {
            histX.removeRange(0, histX.size - cap)
            histY.removeRange(0, histY.size - cap)
            histZ.removeRange(0, histZ.size - cap)
        }
    }

    Column(Modifier.fillMaxSize().background(c.bg)) {
        NavbarHeader(
            title = (if (zh) "技术参数" else "Tech Parameters") + " · ${session.deviceName}",
            backText = if (zh) "‹ 返回" else "‹ Back",
            onBack = onBack,
        )
        Column(
            Modifier
                .weight(1f)
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 16.dp)
                .padding(top = 4.dp, bottom = 24.dp),
        ) {
            JudgeCard(snapshot.parkState, snapshot.parkMode, zh)
            Spacer(Modifier.height(14.dp))
            SectionHeader(if (zh) "地磁三轴 · 实时曲线（最近 30 组）" else "Magnetometer · last 30 samples")
            MagCard(histX, histY, histZ, cap, zh)
            Spacer(Modifier.height(14.dp))
            SectionHeader(if (zh) "雷达频谱" else "Radar Spectrum")
            RadarCard(snapshot.radarSpectrum, zh)
            Text(
                if (zh) "工程诊断页：数据随轮询实时刷新。判读参考——地磁三轴变化量过小说明灵敏度/安装异常；雷达各段能量整体偏低说明有遮挡或干扰。"
                else "Engineering view: values refresh with each poll. Low magnetometer delta suggests sensitivity/mounting issues; uniformly low radar bins suggest occlusion or interference.",
                style = hkt(11f).copy(lineHeight = (11f * 1.5f).sp),
                color = c.text2,
                modifier = Modifier.padding(top = 10.dp),
            )
        }
    }
}

/** 判定对照（0x3A 车位 / 0x3B 工作模式）。 */
@Composable
private fun JudgeCard(parkState: Int?, parkMode: Int?, zh: Boolean) {
    val c = LocalHktColors.current
    val (parkText, parkKind) = when (parkState) {
        1 -> (if (zh) "有车" else "Occupied") to BadgeKind.INFO
        0 -> (if (zh) "空位" else "Vacant") to BadgeKind.WARN
        255 -> (if (zh) "被遮挡" else "Covered") to BadgeKind.WARN
        else -> (if (zh) "未知" else "Unknown") to BadgeKind.WARN
    }
    val modes = if (zh) listOf("融合模式", "仅地磁", "雷达优先") else listOf("Fusion", "Mag only", "Radar first")
    val mode = parkMode?.let { modes.getOrNull(it) } ?: "-"
    val shape = RoundedCornerShape(HktRadius.card.dp)
    Column(
        Modifier
            .fillMaxWidth()
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .padding(14.dp),
    ) {
        Text(if (zh) "当前判定" else "Current reading", style = hkt(12f), color = c.text2)
        Row(
            horizontalArrangement = Arrangement.spacedBy(14.dp),
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier.padding(top = 6.dp),
        ) {
            StateBadge(parkKind, parkText)
            Text((if (zh) "工作模式: " else "Work mode: ") + mode, style = hkt(13f), color = c.text2)
        }
    }
}

/** 地磁三轴折线卡（标题 + 三色当前值 + Canvas 图 + min/count/max）。 */
@Composable
private fun MagCard(histX: List<Int>, histY: List<Int>, histZ: List<Int>, cap: Int, zh: Boolean) {
    val c = LocalHktColors.current
    val xColor = Color(0xFF007AFF)
    val yColor = Color(0xFF34C759)
    val zColor = Color(0xFFFF9500)
    val shape = RoundedCornerShape(HktRadius.card.dp)
    Column(
        Modifier
            .fillMaxWidth()
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .padding(14.dp),
    ) {
        Row(verticalAlignment = Alignment.Bottom, modifier = Modifier.fillMaxWidth().padding(bottom = 8.dp)) {
            Text(if (zh) "地磁 X/Y/Z" else "Mag X/Y/Z", style = hkt(14f, FontWeight.Bold), color = c.text)
            Spacer(Modifier.weight(1f))
            val x = histX.lastOrNull()
            val y = histY.lastOrNull()
            val z = histZ.lastOrNull()
            if (x != null && y != null && z != null) {
                Text("X ${signed(x)}", style = hkt(14f, FontWeight.Bold), color = xColor)
                Text(" ${signed(y)}", style = hkt(14f, FontWeight.Bold), color = yColor)
                Text(" ${signed(z)}", style = hkt(14f, FontWeight.Bold), color = zColor)
            }
        }
        MagChart(histX, histY, histZ, xColor, yColor, zColor, c, zh)
        Row(
            horizontalArrangement = Arrangement.SpaceBetween,
            modifier = Modifier.fillMaxWidth().padding(top = 6.dp),
        ) {
            Text("min ${allOf(histX, histY, histZ) { it.minOrNull() } ?: 0}", style = hkt(11f), color = c.text2)
            Text("${histX.size} / $cap", style = hkt(11f), color = c.text2)
            Text("max ${allOf(histX, histY, histZ) { it.maxOrNull() } ?: 0}", style = hkt(11f), color = c.text2)
        }
    }
}

private inline fun allOf(a: List<Int>, b: List<Int>, cc: List<Int>, pick: (List<Int>) -> Int?): Int? =
    pick(a + b + cc)

private fun signed(v: Int): String = if (v > 0) "+$v" else "$v"

/** 原型 magChart：4 分格网格 + 过零虚线 + 三色折线 + 末端点，上下留 8% 边距。 */
@Composable
private fun MagChart(
    histX: List<Int>,
    histY: List<Int>,
    histZ: List<Int>,
    xColor: Color,
    yColor: Color,
    zColor: Color,
    c: com.hkt.ble.bletools.designsystem.HktColors,
    zh: Boolean,
) {
    if (histX.size < 2) {
        Text(
            if (zh) "正在积累数据（至少 2 组）…" else "Collecting samples (need ≥2)…",
            style = hkt(12f),
            color = c.text2,
            modifier = Modifier.fillMaxWidth().height(130.dp).padding(top = 48.dp),
            textAlign = androidx.compose.ui.text.style.TextAlign.Center,
        )
        return
    }
    Canvas(Modifier.fillMaxWidth().height(150.dp)) {
        val pad = 8.dp.toPx()
        val all = histX + histY + histZ
        var lo = (all.minOrNull() ?: 0).toFloat()
        var hi = (all.maxOrNull() ?: 1).toFloat()
        if (hi == lo) hi = lo + 1f
        val span = hi - lo
        lo -= span * 0.08f
        hi += span * 0.08f
        fun yOf(v: Int): Float = size.height - pad - (v.toFloat() - lo) / (hi - lo) * (size.height - 2 * pad)
        fun xOf(i: Int, count: Int): Float = pad + i * (size.width - 2 * pad) / maxOf(count - 1, 1).coerceAtLeast(1)
        // 网格（1/4、2/4、3/4 三条横线）
        for (i in 1..3) {
            val gy = pad + i * (size.height - 2 * pad) / 4f
            drawLine(c.line, Offset(pad, gy), Offset(size.width - pad, gy), strokeWidth = 1.dp.toPx())
        }
        // 过零虚线
        if (lo < 0f && hi > 0f) {
            val zy = yOf(0)
            drawLine(
                c.text2.copy(alpha = 0.45f),
                Offset(pad, zy),
                Offset(size.width - pad, zy),
                strokeWidth = 1.dp.toPx(),
                pathEffect = PathEffect.dashPathEffect(floatArrayOf(10f, 8f)),
            )
        }
        val series = listOf(xColor to histX, yColor to histY, zColor to histZ)
        for ((color, values) in series) {
            if (values.size < 2) continue
            val path = Path()
            values.forEachIndexed { i, v ->
                val p = Offset(xOf(i, values.size), yOf(v))
                if (i == 0) path.moveTo(p.x, p.y) else path.lineTo(p.x, p.y)
            }
            drawPath(
                path,
                color,
                style = Stroke(width = 2.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round),
            )
            values.lastOrNull()?.let { last ->
                drawCircle(color, radius = 3.2.dp.toPx(), center = Offset(xOf(values.size - 1, values.size), yOf(last)))
            }
        }
    }
}

/** 雷达 10 段频谱（0x60；柱高=值/峰值×130，下标 1..10，底部数值串）。 */
@Composable
private fun RadarCard(radar: List<Int>?, zh: Boolean) {
    val c = LocalHktColors.current
    val shape = RoundedCornerShape(HktRadius.card.dp)
    Column(
        Modifier
            .fillMaxWidth()
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .padding(14.dp),
    ) {
        if (radar.isNullOrEmpty()) {
            Text(
                if (zh) "等待雷达数据…" else "Waiting for radar data…",
                style = hkt(12f),
                color = c.text2,
                modifier = Modifier.fillMaxWidth().height(150.dp).padding(top = 60.dp),
                textAlign = androidx.compose.ui.text.style.TextAlign.Center,
            )
            return
        }
        val peak = maxOf(radar.maxOrNull() ?: 0, 1)
        Row(
            horizontalArrangement = Arrangement.spacedBy(5.dp),
            verticalAlignment = Alignment.Bottom,
            modifier = Modifier.fillMaxWidth().height(150.dp),
        ) {
            radar.forEachIndexed { i, value ->
                Column(horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.weight(1f)) {
                    Box(
                        Modifier
                            .fillMaxWidth()
                            .height(max(4f, value.toFloat() / peak * 130f).dp)
                            .background(c.info, RoundedCornerShape(3.dp)),
                    )
                    Text("${i + 1}", style = hkt(9f), color = c.text2)
                }
            }
        }
        Text(
            radar.joinToString(" / "),
            style = hkt(11f),
            color = c.text2,
            modifier = Modifier.fillMaxWidth().padding(top = 6.dp),
        )
    }
}
