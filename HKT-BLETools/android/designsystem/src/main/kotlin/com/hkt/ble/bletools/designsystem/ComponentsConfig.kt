package com.hkt.ble.bletools.designsystem

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * P_config 配置页组件库（Compose 克隆）：ConfigLabel/ConfigCaption/ConfigStatePill/ConfigSection/
 * NumFieldCard/ChoiceChipRow/ChoiceCell/ModeOption/PortSet/ConfigInputRow/ConfigControlRow。
 * 数值逐条来自规格卡 P-config.md §4（原型 CSS cfg-* 类），与 iOS ConfigComponents.swift 同名同参数。
 */

private fun Modifier.cfgShadow(): Modifier =
    shadow(elevation = 2.dp, shape = RoundedCornerShape(HktRadius.card.dp), clip = false, ambientColor = Color(0x0D0F172A), spotColor = Color(0x0D0F172A))

/** .cfg-label：11px/650 text2，margin 10 0 6。 */
@Composable
fun ConfigLabel(text: String) {
    val c = hktColors()
    Text(
        text,
        style = hkt(11f, FontWeight.SemiBold),
        color = c.text2,
        modifier = Modifier.padding(top = 10.dp, bottom = 6.dp),
    )
}

/** .cfg-caption：11px text2 行高 1.35。 */
@Composable
fun ConfigCaption(text: String, topSpacing: Int = 6) {
    val c = hktColors()
    Text(
        text,
        style = hkt(11f),
        color = c.text2,
        lineHeight = (11f * 1.35f).sp,
        modifier = Modifier.fillMaxWidth().padding(top = topSpacing.dp),
    )
}

/** 状态胶囊（.cfg-state：10px/700，card2 底 line 描边；on=ok 色系）。 */
@Composable
fun ConfigStatePill(text: String, on: Boolean = true) {
    val c = hktColors()
    Text(
        text,
        style = hkt(10f, FontWeight.Bold),
        color = if (on) c.ok else c.text2,
        modifier = Modifier
            .background(if (on) c.ok.copy(alpha = 0.11f) else c.card2, CircleShape)
            .border(1.dp, if (on) c.ok.copy(alpha = 0.20f) else c.line, CircleShape)
            .padding(horizontal = 7.dp, vertical = 3.dp),
    )
}

/** .cfg-section：标题(15/700)+caption+右上状态胶囊+内容；r10 card 底阴影 margin-bottom 10。 */
@Composable
fun ConfigSection(
    title: String,
    caption: String? = null,
    state: String? = null,
    content: @Composable () -> Unit,
) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.card.dp)
    Column(
        Modifier
            .fillMaxWidth()
            .cfgShadow()
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .padding(12.dp)
            .padding(bottom = 10.dp),
    ) {
        Row(Modifier.fillMaxWidth().padding(bottom = 10.dp)) {
            Column(Modifier.weight(1f)) {
                Text(
                    title,
                    style = hkt(15f, FontWeight.Bold).copy(lineHeight = (15f * 1.2f).sp),
                    color = c.text,
                )
                caption?.let {
                    Text(it, style = hkt(11f), color = c.text2, lineHeight = (11f * 1.35f).sp)
                }
            }
            Spacer(Modifier.width(8.dp))
            state?.let { ConfigStatePill(text = it, on = true) }
        }
        content()
    }
}

/** 数字输入行（numRow → .card：label + 输入+单位 + hint + error）。 */
@Composable
fun NumFieldCard(
    label: String,
    text: String,
    onTextChange: (String) -> Unit,
    unit: String? = null,
    hint: String? = null,
    error: String? = null,
    disabled: Boolean = false,
    modifier: Modifier = Modifier,
) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.card.dp)
    Column(
        modifier
            .fillMaxWidth()
            .cfgShadow()
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .padding(horizontal = 14.dp, vertical = 13.dp),
    ) {
        Text(label, style = hkt(12f), color = c.text2)
        Row(
            horizontalArrangement = Arrangement.spacedBy(8.dp),
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier.padding(top = 6.dp),
        ) {
            BasicTextField(
                value = text,
                onValueChange = onTextChange,
                singleLine = true,
                keyboardOptions = androidx.compose.foundation.text.KeyboardOptions(keyboardType = androidx.compose.ui.text.input.KeyboardType.Number),
                textStyle = hkt(14f).copy(color = c.text),
                modifier = Modifier.weight(1f).alpha(if (disabled) 0.5f else 1f),
            )
            if (!unit.isNullOrEmpty()) {
                Text(unit, style = hkt(12f), color = c.text2)
            }
        }
        hint?.let {
            Text(it, style = hkt(11f), color = c.text2, modifier = Modifier.fillMaxWidth().padding(top = 4.dp))
        }
        error?.let {
            Text("✕ $it", style = hkt(12f), color = c.err, modifier = Modifier.fillMaxWidth().padding(top = 4.dp))
        }
    }
}

/** 单选芯片组（segRow：card 内 label + 等宽芯片；选中=info 底白字，未选=card2 底 text2）。 */
@Composable
fun ChoiceChipRow(label: String, options: List<String>, selection: Int, onSelect: (Int) -> Unit) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.card.dp)
    Column(
        Modifier
            .fillMaxWidth()
            .cfgShadow()
            .background(c.card, shape)
            .border(1.dp, c.line.copy(alpha = 0.82f), shape)
            .padding(horizontal = 14.dp, vertical = 13.dp),
    ) {
        Text(label, style = hkt(12f), color = c.text2)
        Row(
            horizontalArrangement = Arrangement.spacedBy(6.dp),
            modifier = Modifier.fillMaxWidth().padding(top = 8.dp),
        ) {
            options.forEachIndexed { index, option ->
                val selected = selection == index
                Box(
                    Modifier
                        .weight(1f)
                        .background(if (selected) c.info else c.card2, RoundedCornerShape(9.dp))
                        .clickableBox { onSelect(index) }
                        .padding(horizontal = 4.dp, vertical = 9.dp),
                    contentAlignment = Alignment.Center,
                ) {
                    Text(
                        option,
                        style = hkt(13f, if (selected) FontWeight.SemiBold else FontWeight.Normal),
                        color = if (selected) Color.White else c.text2,
                    )
                }
            }
        }
    }
}

/** .choice（cfg-grid 3 列：选中=info 10% 底 info 字 info 描边；15px/700）。 */
@Composable
fun ChoiceCell(text: String, selected: Boolean, action: () -> Unit, modifier: Modifier = Modifier) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.control.dp)
    Box(
        modifier
            .background(if (selected) c.info.copy(alpha = 0.10f) else c.card2, shape)
            .border(1.dp, if (selected) c.info.copy(alpha = 0.35f) else c.line, shape)
            .clickableBox(action)
            .padding(horizontal = 6.dp, vertical = 10.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text,
            style = tnum(hkt(15f, FontWeight.Bold)),
            color = if (selected) c.info else c.text,
        )
    }
}

/** .mode-option（端口组内 开关控制/PWM 控制：选中=info 10% 底 info 字）。 */
@Composable
fun ModeOption(text: String, selected: Boolean, modifier: Modifier = Modifier, action: () -> Unit) {
    val c = hktColors()
    val shape = RoundedCornerShape(7.dp)
    Box(
        modifier
            .background(if (selected) c.info.copy(alpha = 0.10f) else c.card, shape)
            .border(1.dp, if (selected) c.info.copy(alpha = 0.32f) else c.line, shape)
            .clickableBox(action)
            .padding(horizontal = 6.dp, vertical = 8.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text,
            style = hkt(12f, FontWeight.SemiBold).copy(lineHeight = (12f * 1.25f).sp),
            color = if (selected) c.info else c.text2,
            textAlign = TextAlign.Center,
        )
    }
}

/** .port-set（阀 1/阀 2 端口组：card2 底 r8，标题 11px/700 text2 + 两组 mode-option）。 */
@Composable
fun PortSet(
    title: String,
    onText: String,
    offText: String,
    isOn: Boolean,
    setOn: (Boolean) -> Unit,
    modifier: Modifier = Modifier,
) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.control.dp)
    Column(
        modifier
            .background(c.card2, shape)
            .border(1.dp, c.line, shape)
            .padding(10.dp),
        verticalArrangement = Arrangement.spacedBy(5.dp),
    ) {
        Text(title, style = hkt(11f, FontWeight.Bold), color = c.text2, modifier = Modifier.padding(bottom = 2.dp))
        ModeOption(text = onText, selected = isOn) { setOn(true) }
        ModeOption(text = offText, selected = !isOn) { setOn(false) }
    }
}

/** .cfg-input-row（输入 + 单位；disabled=opacity .5）。 */
@Composable
fun ConfigInputRow(
    label: String,
    text: String,
    onTextChange: (String) -> Unit,
    unit: String? = null,
    disabled: Boolean = false,
) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.control.dp)
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier
            .fillMaxWidth()
            .background(c.card2, shape)
            .border(1.dp, c.line, shape)
            .alpha(if (disabled) 0.5f else 1f)
            .padding(horizontal = 10.dp, vertical = 10.dp),
    ) {
        BasicTextField(
            value = text,
            onValueChange = onTextChange,
            singleLine = true,
            keyboardOptions = androidx.compose.foundation.text.KeyboardOptions(keyboardType = androidx.compose.ui.text.input.KeyboardType.Number),
            textStyle = tnum(hkt(15f, FontWeight.SemiBold)).copy(color = c.text),
            modifier = Modifier.weight(1f),
        )
        unit?.let {
            Spacer(Modifier.width(8.dp))
            Text(it, style = hkt(12f), color = c.text2)
        }
    }
}

/** .cfg-control（label 左 + 自定义右侧如 switch；card2 底 r8）。 */
@Composable
fun ConfigControlRow(label: String, trailing: @Composable () -> Unit) {
    val c = hktColors()
    val shape = RoundedCornerShape(HktRadius.control.dp)
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier
            .fillMaxWidth()
            .background(c.card2, shape)
            .border(1.dp, c.line, shape)
            .padding(horizontal = 10.dp, vertical = 9.dp),
    ) {
        Text(label, style = hkt(13f, FontWeight.SemiBold), color = c.text, modifier = Modifier.weight(1f))
        Spacer(Modifier.width(8.dp))
        trailing()
    }
}
