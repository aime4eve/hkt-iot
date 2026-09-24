package com.hkt.ble.bletools.designsystem

import androidx.compose.animation.core.animateDpAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.defaultMinSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.em
import androidx.compose.ui.unit.sp

/**
 * P-03 组件库（Compose 克隆）：SessionControlCard/SessionButton/FieldSpec/FieldGrid/FieldTile/
 * StatePill/SvcChannelModule/HktBanner/OpsPanel/OpsPowerRow/HktSwitch/OpCard。
 * 数值逐条来自规格卡 docs/ios/design/ui-spec/P-03.md §3（原型 CSS），与 iOS DesignSystem 同名同参数。
 */

private fun Modifier.hktShadow03(shape: RoundedCornerShape): Modifier =
    shadow(elevation = 2.dp, shape = shape, clip = false, ambientColor = Color(0x0D0F172A), spotColor = Color(0x0D0F172A))

/** 等宽数字（tabular-nums；原型 font-variant-numeric）。 */
fun tnum(style: androidx.compose.ui.text.TextStyle): androidx.compose.ui.text.TextStyle =
    style.copy(fontFeatureSettings = "tnum")

/** 会话动作按钮（.session-actions：高 34、line 描边、card2 底、12px/650；danger=err 变体）。 */
@Composable
fun SessionButton(title: String, danger: Boolean = false, action: () -> Unit) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.control.dp)
    Box(
        Modifier
            .height(34.dp)
            .background(if (danger) c.err.copy(alpha = 0.08f) else c.card2, shape)
            .border(1.dp, if (danger) c.err.copy(alpha = 0.24f) else c.line, shape)
            .clickableBox(action)
            .padding(horizontal = 4.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            title,
            style = hkt(12f, FontWeight.SemiBold),
            color = if (danger) c.err else c.text,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            textAlign = TextAlign.Center,
        )
    }
}

/**
 * 会话控制卡（.sessionbar，规格卡 §3.1）：r12、card 底、外边距 0 16 12、内边距 13 14；
 * 设备名 23px/750；badge 右侧；meta 右对齐 12px text2 等宽数字；
 * 动作单行 2+2（2026-09-23 用户裁决）：前半居左、后半居右。
 */
@Composable
fun SessionControlCard(
    deviceName: String,
    meta: String,
    badge: (@Composable () -> Unit)? = null,
    actions: List<@Composable () -> Unit>,
) {
    val c = hktColors()
    val shape = RoundedCornerShape(12.dp)
    Column(
        Modifier
            .padding(horizontal = 16.dp, vertical = 12.dp)
            .hktShadow03(shape)
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .padding(horizontal = 14.dp, vertical = 13.dp),
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(
                deviceName,
                style = hkt(23f, FontWeight.Bold).copy(lineHeight = (23f * 1.1f).sp),
                color = c.text,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier.weight(1f, fill = false),
            )
            Spacer(Modifier.width(10.dp))
            badge?.invoke()
        }
        Text(
            meta,
            style = tnum(hkt(12f)),
            color = c.text2,
            modifier = Modifier.fillMaxWidth().padding(top = 5.dp, bottom = 12.dp),
            textAlign = TextAlign.Right,
        )
        // session-actions 单行：左组（返回/首页）+ 弹性空隙 + 右组（固件升级/断开连接）；
        // ≤2 个动作全部居左（防其他调用点右组悬空）
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(6.dp),
            modifier = Modifier.fillMaxWidth(),
        ) {
            val split = if (actions.size <= 2) actions.size else actions.size / 2
            actions.take(split).forEach { it() }
            Spacer(Modifier.weight(1f))
            actions.drop(split).forEach { it() }
        }
    }
}

/** 字段定义（P-03 §4：label + 格式化值 + 可选 unit span）。 */
data class FieldSpec(val label: String, val value: String, val unit: String? = null)

/** 字段卡（.field：r10 padding 10/12 **min-height 70 border-box**；.k 11/600 text2；.v 16/650 等宽数字 + unit span）。
 *  ⚠️ heightIn(min) 必须在 padding 之前（链首）——链尾固定 height 会把 padding 加到外面变 90dp
 *  （iOS 已踩过的同款坑在 Compose 重演，M6.2 评审 P1-2）；min 而非固定高度允许长值换行。 */
@Composable
fun FieldTile(spec: FieldSpec, modifier: Modifier = Modifier) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.card.dp)
    Column(
        modifier
            .heightIn(min = 70.dp)
            .hktShadow03(shape)
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .padding(horizontal = 12.dp, vertical = 10.dp),
        verticalArrangement = Arrangement.SpaceBetween,
    ) {
        Text(spec.label, style = hkt(11f, FontWeight.SemiBold), color = c.text2, maxLines = 1, overflow = TextOverflow.Ellipsis)
        Row(verticalAlignment = Alignment.Bottom) {
            Text(
                spec.value,
                style = tnum(hkt(16f, FontWeight.SemiBold)).copy(lineHeight = 20.sp),
                color = c.text,
            )
            spec.unit?.let {
                Spacer(Modifier.width(2.dp))
                Text(it, style = hkt(11f), color = c.text2)
            }
        }
    }
}

/** fieldgrid：2 列 gap 8；奇数个字段时最后一个独占整行（规格卡 §3.3）。 */
@Composable
fun FieldGrid(fields: List<FieldSpec>) {
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        fields.chunked(2).forEach { row ->
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                row.forEach { spec ->
                    if (row.size == 1) {
                        FieldTile(spec, modifier = Modifier.weight(2f))
                    } else {
                        FieldTile(spec, modifier = Modifier.weight(1f))
                    }
                }
            }
        }
    }
}

/** 通道状态胶囊（.state-pill on/off：dot 7×7 + 11/700；on=ok 14% 底 ok 字、off=text2 10%）。 */
@Composable
fun StatePill(on: Boolean, text: String) {
    val c = hktColors()
    val color = if (on) c.ok else c.text2
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(4.dp),
        modifier = Modifier
            .background(
                (if (on) c.ok.copy(alpha = 0.14f) else c.text2.copy(alpha = 0.10f)),
                CircleShape,
            )
            .padding(horizontal = 7.dp, vertical = 3.dp),
    ) {
        Box(Modifier.size(7.dp).background(color, CircleShape))
        Text(text, style = hkt(11f, FontWeight.Bold), color = color)
    }
}

/** SVC 通道定义（规格卡 §3.4）。 */
data class SvcChannelSpec(
    val name: String,
    val on: Boolean,
    val onText: String,
    val insertLabel: String,
    val insertText: String,
    val pulseLabel: String,
    val pulseText: String,
    val portLabel: String,
    val portText: String,
)

/** 阀通道模块（.svc-module：仅 SVC100；通道卡 card2 底 line 72% r8）。 */
@Composable
fun SvcChannelModule(title: String, tag: String, channels: List<SvcChannelSpec>) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.card.dp)
    Column(
        Modifier
            .fillMaxWidth()
            .hktShadow03(shape)
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .padding(12.dp),
    ) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(8.dp),
            modifier = Modifier.fillMaxWidth().padding(bottom = 10.dp),
        ) {
            Text(title, style = hkt(13f, FontWeight.Bold), color = c.text, modifier = Modifier.weight(1f))
            Text(
                tag,
                style = hkt(10f, FontWeight.SemiBold),
                color = c.text2,
                modifier = Modifier
                    .background(c.card2, CircleShape)
                    .border(1.dp, c.line, CircleShape)
                    .padding(horizontal = 7.dp, vertical = 3.dp),
            )
        }
        // channel-grid 2 列 gap 8（规格卡 §3.4；M6.2 评审 P1-1：纵堆偏离 iOS 两列）
        Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
            channels.chunked(2).forEach { rowChannels ->
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    rowChannels.forEach { ch ->
                        val inner = RoundedCornerShape(HktRadius.control.dp)
                        Column(
                            Modifier
                                .weight(1f)
                                .background(c.card2, inner)
                                .border(1.dp, c.line.copy(alpha = 0.72f), inner)
                                .padding(10.dp),
                        ) {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Text(ch.name, style = hkt(12f, FontWeight.Bold), color = c.text2, modifier = Modifier.weight(1f))
                                StatePill(on = ch.on, text = ch.onText)
                            }
                            Column(verticalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.padding(top = 9.dp)) {
                                Kpi(label = ch.insertLabel, value = ch.insertText)
                                Kpi(label = ch.pulseLabel, value = ch.pulseText)
                                Kpi(label = ch.portLabel, value = ch.portText)
                            }
                        }
                    }
                    repeat(2 - rowChannels.size) { Spacer(Modifier.weight(1f)) }
                }
            }
        }
    }
}

@Composable
private fun Kpi(label: String, value: String) {
    val c = hktColors()
    Column {
        Text(label, style = hkt(10f, FontWeight.SemiBold), color = c.text2)
        Text(value, style = tnum(hkt(14f, FontWeight.SemiBold)).copy(lineHeight = (14f * 1.2f).sp), color = c.text)
    }
}

/** 横幅（.banner err/warn：13px 行高 1.4 margin-bottom 11；右侧可选动作）。 */
@Composable
fun HktBanner(kind: BadgeKind, text: String, actionTitle: String? = null, action: () -> Unit = {}) {
    val c = hktColors()
    val color = when (kind) {
        BadgeKind.OK -> c.ok
        BadgeKind.WARN -> c.warn
        BadgeKind.ERR -> c.err
        BadgeKind.INFO -> c.info
    }
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier
            .fillMaxWidth()
            .padding(bottom = 11.dp)   // 规格卡 §3.5：margin-bottom 11 是【外】间距（评审 P2-5）
            .background(color.copy(alpha = 0.12f), RoundedCornerShape(HktRadius.card.dp))
            .padding(horizontal = 12.dp, vertical = 10.dp),
    ) {
        Text(text, style = hkt(13f).copy(lineHeight = (13f * 1.4f).sp), color = color, modifier = Modifier.weight(1f))
        if (actionTitle != null) {
            Text(
                actionTitle,
                style = hkt(13f, FontWeight.SemiBold),
                color = color,
                modifier = Modifier.clickableBox(action),
            )
        }
    }
}

/** 开关（44×26 r13；off=fill 底 on=ok 底；旋钮 22×22 白，on 左移 20）。 */
@Composable
fun HktSwitch(isOn: Boolean) {
    val c = hktColors()
    val knobX by animateDpAsState(if (isOn) 20.dp else 2.dp)
    Box(
        Modifier
            .width(44.dp)
            .height(26.dp)
            .background(if (isOn) c.ok else c.fill, RoundedCornerShape(13.dp)),
    ) {
        Box(
            Modifier
                .offset(x = knobX, y = 2.dp)
                .size(22.dp)
                .background(Color.White, CircleShape),
        )
    }
}

/** 电源整行（.ops-power：min-height 58、底边 line 72%）；[enabled]=false 整行不可点（校准置灰，评审 P2-7）。 */
@Composable
fun OpsPowerRow(label: String, stateText: String, isOn: Boolean, enabled: Boolean = true, onToggle: () -> Unit) {
    val c = hktColors()
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(12.dp),
        modifier = Modifier
            .fillMaxWidth()
            .heightIn(min = 58.dp)
            .clickableBox(enabled = enabled, action = onToggle)
            .padding(horizontal = 14.dp),
    ) {
        Text(label, style = hkt(15f, FontWeight.SemiBold), color = c.text)
        Text(stateText, style = hkt(12f), color = c.text2, modifier = Modifier.weight(1f))
        HktSwitch(isOn)
    }
    Box(Modifier.fillMaxWidth().height(1.dp).background(c.line.copy(alpha = 0.72f)))
}

private fun Modifier.heightIn(min: androidx.compose.ui.unit.Dp) = this.then(
    Modifier.defaultMinSize(minHeight = min)
)

/** 设备操作面板（.ops-panel：电源行 + ops-grid card2 底 2 列 gap 8 padding 10）。
 *  [disabled]（校准运行中）：透明度 0.45 + 全面板拦截点击（评审 P2-7 休眠陷阱提前拆除）。 */
@Composable
fun OpsPanel(
    powerRow: @Composable () -> Unit,
    disabled: Boolean = false,
    content: @Composable () -> Unit,
) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.card.dp)
    Box(
        Modifier
            .fillMaxWidth()
            .hktShadow03(shape),
    ) {
        Column(
            Modifier
                .fillMaxWidth()
                .background(c.card, shape)
                .border(1.dp, c.line.copy(alpha = 0.82f), shape)
                .alpha(if (disabled) 0.45f else 1f),
        ) {
            powerRow()
            Column(
                verticalArrangement = Arrangement.spacedBy(8.dp),
                modifier = Modifier
                    .fillMaxWidth()
                    .background(c.card2)
                    .padding(10.dp),
            ) {
                content()
            }
        }
        if (disabled) {
            // 置灰时拦截全部点击（powerRow/OpCard 均被盖住）
            Box(
                Modifier
                    .matchParentSize()
                    .clickableBox { }
            )
        }
    }
}

/** 操作卡（OpCard：全部 wide 独占整行、min-height 62；badge 38×38 r10；trailing go「›」或自定义）。 */
@Composable
fun OpCard(
    badge: String,
    badgeKind: BadgeKind = BadgeKind.INFO,
    title: String,
    desc: String,
    trailing: (@Composable () -> Unit)? = null,
    disabled: Boolean = false,
    action: () -> Unit = {},
) {
    val c = hktColors()
    val badgeColor = when (badgeKind) {
        BadgeKind.OK -> c.ok
        BadgeKind.WARN -> c.warn
        BadgeKind.ERR -> c.err
        BadgeKind.INFO -> c.info
    }
    val shape = RoundedCornerShape(HktRadius.control.dp)
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(10.dp),
        modifier = Modifier
            .fillMaxWidth()
            .heightIn(min = 62.dp)
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .clickable(enabled = !disabled, onClick = action)
            .padding(11.dp),
    ) {
        Box(
            Modifier
                .size(38.dp)
                .background(badgeColor.copy(alpha = 0.10f), RoundedCornerShape(10.dp)),
            contentAlignment = Alignment.Center,
        ) {
            Text(
                badge,
                style = hkt(10f, FontWeight.Bold),
                color = badgeColor,
                letterSpacing = 0.04.em,
            )
        }
        Column(Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(3.dp)) {
            Text(title, style = hkt(13f, FontWeight.SemiBold).copy(lineHeight = (13f * 1.2f).sp), color = c.text)
            Text(desc, style = hkt(11f), color = c.text2, lineHeight = (11f * 1.25f).sp)
        }
        trailing?.invoke() ?: Text("›", style = hkt(17f), color = c.text2)
    }
}
