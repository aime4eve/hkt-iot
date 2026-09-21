package com.hkt.ble.bletools.designsystem

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp

/**
 * R-G6 工业遥测视觉语言 Token（与冻结原型 CSS 变量一一对应，浅/深双外观）。
 * iOS 对照：DesignSystem/Theme.swift —— 数值逐条同源，禁用 Material 语义色近似。
 */
data class HktColors(
    val bg: Color,
    val card: Color,
    val card2: Color,
    val text: Color,
    val text2: Color,
    val line: Color,
    val ok: Color,
    val warn: Color,
    val err: Color,
    val info: Color,
    val fill: Color,
)

private val LightColors = HktColors(
    bg = Color(0xFFEEF1F4),
    card = Color(0xFFFFFFFF),
    card2 = Color(0xFFF5F7F9),
    text = Color(0xFF151A20),
    text2 = Color(0xFF5F6B77),
    line = Color(0xFFD9E0E7),
    ok = Color(0xFF178A44),
    warn = Color(0xFFB65F00),
    err = Color(0xFFC81E1E),
    info = Color(0xFF0A68D6),
    fill = Color(0xFFE8ECF0),
)

private val DarkColors = HktColors(
    bg = Color(0xFF000000),
    card = Color(0xFF16181C),
    card2 = Color(0xFF222529),
    text = Color(0xFFF3F5F7),
    text2 = Color(0xFFA2AAB3),
    line = Color(0xFF35383F),
    ok = Color(0xFF33D269),
    warn = Color(0xFFFFB340),
    err = Color(0xFFFF6B60),
    info = Color(0xFF63AAFF),
    fill = Color(0xFF292C31),
)

val LocalHktColors = staticCompositionLocalOf { LightColors }

/** 随系统深浅色（原型浅/深双外观；App 内语言/主题切换在 P-07 接入后改为受控）。 */
@Composable
fun hktColors(): HktColors = if (isSystemInDarkTheme()) DarkColors else LightColors

/** 卡片圆角 10 / 控件圆角 8 / 对话框圆角 18（--r-card/--r-control/--r-dialog）。 */
object HktRadius {
    const val card = 10
    const val control = 8
    const val dialog = 18
}

/**
 * 原型字号全部为固定 px，禁用 Material 排版近似。
 * CSS 数值字重映射：400→Normal，550/600/650→SemiBold，700/750→Bold。
 */
fun hkt(size: Float, weight: FontWeight = FontWeight.Normal): TextStyle =
    TextStyle(fontSize = size.sp, fontWeight = weight)
