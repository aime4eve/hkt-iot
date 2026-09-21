package com.hkt.ble.bletools.designsystem

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * P-07 设置/日志 + P_cal 校准组件库（Compose 克隆）：
 * NavbarHeader/SettingsRow/RowValue/PrefixChip/LogLine/CalStepRow/HktProgress。
 * 数值逐条来自规格卡（docs/ios/design/ui-spec/P-07_settings_log.md、P-cal.md），与 iOS 同名同参数。
 */

/** 页头导航条（.navbar.small：back 胶囊 + 17px/700 标题 + 右侧 linkbtn；padding 6/16）。 */
@Composable
fun NavbarHeader(
    title: String,
    backText: String = "‹ 返回",
    onBack: () -> Unit = {},
    trailing: @Composable () -> Unit = {},
) {
    val c = hktColors()
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(8.dp),
        modifier = Modifier.fillMaxWidth().padding(top = 6.dp, bottom = 6.dp).padding(horizontal = 16.dp),
    ) {
        if (backText.isNotEmpty()) {
            // 原型运行态（OTA 传输中）无返回入口，离开走页面内按钮
            Box(
                Modifier
                    .background(c.info.copy(alpha = 0.09f), RoundedCornerShape(HktRadius.control.dp))
                    .clickableBox(onBack)
                    .padding(horizontal = 9.dp, vertical = 5.dp),
            ) {
                Text(backText, style = hkt(14f, FontWeight.SemiBold), color = c.info)
            }
        }
        Text(title, style = hkt(17f, FontWeight.Bold), color = c.text, maxLines = 1, overflow = TextOverflow.Ellipsis)
        Spacer(Modifier.weight(1f))
        trailing()
    }
}

/** 设置行（.row：label 左 + value 右；可选点击）。 */
@Composable
fun SettingsRow(label: String, value: @Composable () -> Unit = {}, action: (() -> Unit)? = null) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.card.dp)
    val row = @Composable {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier.fillMaxWidth().padding(horizontal = 14.dp, vertical = 12.dp),
        ) {
            Text(label, style = hkt(15f), color = c.text, modifier = Modifier.weight(1f))
            Spacer(Modifier.width(8.dp))
            value()
        }
    }
    Box(Modifier.fillMaxWidth().padding(bottom = 9.dp)) {
        HktCard(modifier = Modifier.fillMaxWidth(), contentPadding = PaddingValues(0.dp)) { row() }
        if (action != null) {
            Box(Modifier.matchParentSize().clickableBox(action))
        }
    }
}

/** 行右值样式（.row .value：14px text2，横排 gap 6；内容 Text 自带样式时以内容为准）。 */
@Composable
fun RowValue(content: @Composable () -> Unit) {
    Row(horizontalArrangement = Arrangement.spacedBy(6.dp), verticalAlignment = Alignment.CenterVertically) {
        content()
    }
}

/** 前缀过滤芯片（P-07：选中 info 底白字 / 未选中 card2 底 text2 字，等宽撑满）。 */
@Composable
fun PrefixChip(text: String, selected: Boolean, action: () -> Unit, modifier: Modifier = Modifier) {
    val c = hktColors()
    Box(
        modifier
            .background(if (selected) c.info else c.card2, RoundedCornerShape(9.dp))
            .clickableBox(action)
            .padding(vertical = 8.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text,
            style = hkt(13f, FontWeight.SemiBold),
            color = if (selected) Color.White else c.text2,
        )
    }
}

/** 日志条目（.logline：等宽 11px、底边 line、级别色 INFO=info/WARN=warn/ERR=err）。 */
@Composable
fun LogLine(timestamp: String, level: String, message: String) {
    val c = hktColors()
    val levelColor = when (level) {
        "ERR" -> c.err
        "WARN" -> c.warn
        else -> c.info
    }
    val mono = TextStyleMonospaced
    Row(Modifier.fillMaxWidth().padding(vertical = 2.dp)) {
        Text(
            timestamp,
            style = mono,
            color = c.text2,
            modifier = Modifier.width(64.dp).padding(end = 6.dp),
        )
        Text(
            level,
            style = mono.copy(fontWeight = FontWeight.Bold),
            color = levelColor,
            modifier = Modifier.width(44.dp),
        )
        Text(message, style = mono, color = c.text, modifier = Modifier.weight(1f))
    }
    Box(Modifier.fillMaxWidth().height(1.dp).background(c.line))
}

private val TextStyleMonospaced: androidx.compose.ui.text.TextStyle
    get() = androidx.compose.ui.text.TextStyle(
        fontSize = 11.sp,
        fontFamily = FontFamily.Monospace,
    )

/** 校准步骤行（P_cal：圆形序号 26×26 + 可选粗体标题 + 文本；index 0=无序号）。 */
@Composable
fun CalStepRow(index: Int, title: String?, text: String) {
    val c = hktColors()
    Row(Modifier.fillMaxWidth().padding(vertical = 12.dp)) {
        if (index > 0) {
            Box(
                Modifier
                    .size(26.dp)
                    .background(c.info, CircleShape),
                contentAlignment = Alignment.Center,
            ) {
                Text("$index", style = hkt(13f, FontWeight.Bold), color = Color.White)
            }
            Spacer(Modifier.width(12.dp))
        }
        val body = androidx.compose.ui.text.buildAnnotatedString {
            if (!title.isNullOrEmpty()) {
                withStyle(androidx.compose.ui.text.SpanStyle(fontWeight = FontWeight.Bold)) { append("$title: ") }
            }
            append(text)
        }
        Text(
            body,
            style = hkt(13f),
            color = c.text2,
            lineHeight = (13f * 1.6f).sp,
            modifier = Modifier.weight(1f).padding(top = if (index > 0) 3.dp else 0.dp),
        )
    }
}

/** 进度条（.progress：高 8 圆角 4，fill 底 info 填充，0.25s 缓动）。 */
@Composable
fun HktProgress(fraction: Float) {
    val c = hktColors()
    Box(
        Modifier
            .fillMaxWidth()
            .height(8.dp)
            .background(c.fill, RoundedCornerShape(4.dp)),
    ) {
        Box(
            Modifier
                .fillMaxWidth(fraction = fraction.coerceIn(0f, 1f))
                .height(8.dp)
                .background(c.info, RoundedCornerShape(4.dp))
        )
    }
}
