package com.hkt.ble.bletools.ui

/**
 * 广播标识后 4 位大写（原型 `…9C01` 展示约定；扫描卡/驻留卡/连接覆盖层共用——M6.1 评审 P2-6 抽共享）。
 */
internal fun idSuffix(identifier: String): String =
    identifier.replace(":", "").takeLast(4).uppercase()
