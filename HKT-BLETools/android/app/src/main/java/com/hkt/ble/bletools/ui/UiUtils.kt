package com.hkt.ble.bletools.ui

import android.content.Context
import android.content.Intent

/**
 * 广播标识后 4 位大写（原型 `…9C01` 展示约定；扫描卡/驻留卡/连接覆盖层共用——M6.1 评审 P2-6 抽共享）。
 */
internal fun idSuffix(identifier: String): String =
    identifier.replace(":", "").takeLast(4).uppercase()

/**
 * 系统分享文本（iOS ShareLink 同位）：日志导出 / OTA 失败导出日志共用。
 * 数据仅经系统分享面板离开设备，目的地由用户选择（隐私页 §数据导出承诺）。
 */
internal fun shareText(context: Context, title: String, text: String) {
    val intent = Intent(Intent.ACTION_SEND).apply {
        type = "text/plain"
        putExtra(Intent.EXTRA_SUBJECT, title)
        putExtra(Intent.EXTRA_TEXT, text)
    }
    context.startActivity(Intent.createChooser(intent, title))
}
