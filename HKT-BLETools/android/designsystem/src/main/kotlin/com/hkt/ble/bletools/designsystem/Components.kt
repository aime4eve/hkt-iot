package com.hkt.ble.bletools.designsystem

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * P-01 组件库（Compose 克隆）：HktCard/SectionHeader/StateBadge/LinkButton/DialogButton/
 * HktDialogCard/NavbarLarge/ScanStatusRow/RssiBars/ScanDeviceCard/CenterStateView。
 * 全部数值逐条来自冻结原型 CSS（规格卡 docs/ios/design/ui-spec/P-01.md §2），
 * 与 iOS DesignSystem/Components.swift 同名同参数。禁止页内私调样式。
 */

private val shadowShape: RoundedCornerShape get() = RoundedCornerShape(HktRadius.card.dp)

/** 卡片阴影：light `0 1 2 rgba(15,23,42,.05)`。 */
private fun Modifier.hktCardShadow(): Modifier =
    shadow(elevation = 2.dp, shape = shadowShape, clip = false, ambientColor = Color(0x0D0F172A), spotColor = Color(0x0D0F172A))

/** 无涟漪点击（原型按钮无 Material 波纹）。 */
fun Modifier.clickableBox(action: () -> Unit): Modifier = this.clickable(onClick = action)

/** 卡片容器（.card）：card 底、`line 82%` 描边、r10、阴影；padding 由调用方给（.card=13/14）。 */
@Composable
fun HktCard(
    modifier: Modifier = Modifier,
    radius: Int = HktRadius.card,
    contentPadding: PaddingValues = PaddingValues(horizontal = 14.dp, vertical = 13.dp),
    content: @Composable () -> Unit,
) {
    val c = hktColors()
    val shape = RoundedCornerShape(radius.dp)
    Column(
        modifier
            .hktCardShadow()
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .padding(contentPadding),
    ) { content() }
}

/** .section：11px/700 字距小标题 + 横线。 */
@Composable
fun SectionHeader(title: String) {
    val c = hktColors()
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier.fillMaxWidth().padding(top = 16.dp).padding(horizontal = 4.dp).padding(bottom = 8.dp),
    ) {
        Text(title, style = hkt(11f, FontWeight.Bold), letterSpacing = 0.55.sp, color = c.text2)
        Spacer(Modifier.width(8.dp))
        Box(Modifier.weight(1f).height(1.dp).background(c.line.copy(alpha = 0.76f)))
    }
}

enum class BadgeKind { OK, WARN, ERR, INFO }

/** 状态徽章（.badge + .dot 7×7）；compact = liveCard 变体 padding 2/8。 */
@Composable
fun StateBadge(kind: BadgeKind, text: String, compact: Boolean = false) {
    val c = hktColors()
    val color = when (kind) {
        BadgeKind.OK -> c.ok
        BadgeKind.WARN -> c.warn
        BadgeKind.ERR -> c.err
        BadgeKind.INFO -> c.info
    }
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(5.dp),
        modifier = Modifier
            .background(color.copy(alpha = 0.15f), CircleShape)
            .padding(horizontal = if (compact) 8.dp else 9.dp, vertical = if (compact) 2.dp else 3.dp),
    ) {
        Box(Modifier.size(7.dp).background(color, CircleShape))
        Text(text, style = hkt(12f, FontWeight.SemiBold), color = color)
    }
}

/** .linkbtn：info 字色 14px/600 + info 9% 底 + r8（padding 6/9）。 */
@Composable
fun LinkButton(title: String, action: () -> Unit) {
    val c = hktColors()
    Box(
        Modifier
            .background(c.info.copy(alpha = 0.09f), RoundedCornerShape(HktRadius.control.dp))
            .clickableBox(action)
            .padding(horizontal = 9.dp, vertical = 6.dp),
    ) {
        Text(title, style = hkt(14f, FontWeight.SemiBold), color = c.info)
    }
}

enum class DialogButtonKind { PRIMARY, SECONDARY, DANGER }

/** .btn 对话框按钮：全宽 padding 13、r8、16px/650。primary=info底白字；secondary=card底 line描边；danger=err。 */
@Composable
fun DialogButton(title: String, kind: DialogButtonKind = DialogButtonKind.SECONDARY, action: () -> Unit) {
    val c = hktColors()
    val fg = when (kind) {
        DialogButtonKind.PRIMARY -> Color.White
        DialogButtonKind.SECONDARY -> c.text
        DialogButtonKind.DANGER -> c.err
    }
    val bg = when (kind) {
        DialogButtonKind.PRIMARY -> c.info
        DialogButtonKind.SECONDARY -> c.card
        DialogButtonKind.DANGER -> c.err.copy(alpha = 0.10f)
    }
    val shape = RoundedCornerShape(HktRadius.control.dp)
    Box(
        Modifier
            .fillMaxWidth()
            .background(bg, shape)
            .let { m ->
                when (kind) {
                    DialogButtonKind.SECONDARY -> m.border(1.dp, c.line, shape)
                    DialogButtonKind.DANGER -> m.border(1.dp, c.err.copy(alpha = 0.22f), shape)
                    else -> m
                }
            }
            .clickableBox(action)
            .padding(13.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(title, style = hkt(16f, FontWeight.SemiBold), color = fg, textAlign = TextAlign.Center)
    }
}

/** 大标题页头（.navbar 大标题版：P-01；padding 10/16/10，无返回键）。 */
@Composable
fun NavbarLarge(title: String, trailing: @Composable () -> Unit = {}) {
    val c = hktColors()
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(8.dp),
        modifier = Modifier.fillMaxWidth().padding(top = 10.dp, bottom = 10.dp).padding(horizontal = 16.dp),
    ) {
        Text(title, style = hkt(20f, FontWeight.Bold), color = c.text)
        Spacer(Modifier.weight(1f))
        trailing()
    }
}

/** 扫描状态行（.row：P-01；card 底 r10 padding 12/14 阴影 margin-bottom 9）。 */
@Composable
fun ScanStatusRow(title: String, actionTitle: String, action: () -> Unit) {
    val c = hktColors()
    HktCard(
        modifier = Modifier.fillMaxWidth().padding(bottom = 9.dp),
        contentPadding = PaddingValues(horizontal = 14.dp, vertical = 12.dp),
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(title, style = hkt(15f), color = c.text, modifier = Modifier.weight(1f))
            LinkButton(title = actionTitle, action = action)
        }
    }
}

/** RSSI 信号条（.rssi：4 根 3dp 宽竖条 gap 2，高 4/7/10/14，圆角 1；亮=ok、灭=fill）。 */
@Composable
fun RssiBars(lit: Int) {
    val c = hktColors()
    val heights = listOf(4, 7, 10, 14)
    Row(
        verticalAlignment = Alignment.Bottom,
        horizontalArrangement = Arrangement.spacedBy(2.dp),
        modifier = Modifier.height(14.dp),
    ) {
        heights.forEachIndexed { i, h ->
            Box(
                Modifier
                    .width(3.dp)
                    .height(h.dp)
                    .background(if (i < lit) c.ok else c.fill, RoundedCornerShape(1.dp)),
            )
        }
    }
}

/** 扫描设备卡（.card hcard：P-01；也用于驻留卡/最近设备卡）。 */
@Composable
fun ScanDeviceCard(
    name: String,
    subtitle: String,
    badge: (@Composable () -> Unit)? = null,
    trailing: @Composable () -> Unit = {},
) {
    val c = hktColors()
    HktCard(modifier = Modifier.fillMaxWidth()) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Column(Modifier.weight(1f)) {
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                ) {
                    Text(
                        name,
                        style = hkt(15f, FontWeight.Bold),
                        color = c.text,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                    )
                    badge?.invoke()
                }
                Spacer(Modifier.height(2.dp))
                Text(
                    subtitle,
                    style = hkt(12f),
                    color = c.text2,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
            }
            Spacer(Modifier.width(8.dp))
            trailing()
        }
    }
}

/** 空态（.center：📡44 + 标题 17/600 + 副文案 13 行高 1.5 + secondary 按钮 padding 10/26）。 */
@Composable
fun CenterStateView(
    glyph: String,
    title: String,
    subtitle: String,
    buttonTitle: String,
    onButton: () -> Unit,
    glyphSize: Int = 44,
) {
    val c = hktColors()
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(8.dp),
        modifier = Modifier.fillMaxWidth().padding(vertical = 24.dp),
    ) {
        Text(glyph, fontSize = glyphSize.sp)
        Text(title, style = hkt(17f, FontWeight.SemiBold), color = c.text)
        Text(
            subtitle,
            style = hkt(13f),
            color = c.text2,
            textAlign = TextAlign.Center,
            lineHeight = (13f * 1.5f).sp,
        )
        val shape = RoundedCornerShape(HktRadius.control.dp)
        Box(
            Modifier
                .padding(top = 4.dp)
                .background(c.card, shape)
                .border(1.dp, c.line, shape)
                .clickableBox(onButton)
                .padding(horizontal = 26.dp, vertical = 10.dp),
            contentAlignment = Alignment.Center,
        ) {
            Text(buttonTitle, style = hkt(16f, FontWeight.SemiBold), color = c.text)
        }
    }
}

/**
 * 对话框卡片（.dialog 296 宽 / r18 / padding 18）：标题 + 正文 + 按钮纵列。
 * 遮罩与居中由页面 overlay 负责（switchTo 确认框等）。
 */
@Composable
fun HktDialogCard(
    title: String,
    message: String,
    buttons: List<Triple<String, DialogButtonKind, () -> Unit>>,
) {
    val c = hktColors()
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(10.dp),
        modifier = Modifier
            .widthIn(max = 296.dp)
            .background(c.card, RoundedCornerShape(HktRadius.dialog.dp))
            .padding(18.dp),
    ) {
        Text(title, style = hkt(17f, FontWeight.Bold), color = c.text)
        Text(message, style = hkt(13f), color = c.text2, textAlign = TextAlign.Center)
        Spacer(Modifier.height(4.dp))
        buttons.forEach { (label, kind, action) ->
            DialogButton(title = label, kind = kind, action = action)
        }
    }
}
