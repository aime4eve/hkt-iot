package com.hkt.ble.bletools.model

import androidx.compose.runtime.mutableStateListOf
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * P-07 诊断日志存储（原型 S.logs）：App 层事件（扫描启停/发现/连接/断开）追加，界面只读；
 * 环形上限 500 防长会话膨胀。iOS LogStore 的 Kotlin 移植（mutableStateListOf 供 Compose 观察）。
 */
object LogStore {
    data class Entry(val id: Int, val timestamp: String, val level: String, val message: String)

    private val formatter = SimpleDateFormat("HH:mm:ss", Locale.US)
    private val _entries = mutableStateListOf<Entry>()
    val entries: List<Entry> get() = _entries
    private var nextId = 0
    private val capacity = 500

    fun log(level: String, message: String) {
        _entries.add(Entry(nextId++, formatter.format(Date()), level, message))
        if (_entries.size > capacity) {
            _entries.removeRange(0, _entries.size - capacity)
        }
    }

    fun info(message: String) = log("INFO", message)
    fun warn(message: String) = log("WARN", message)
    fun error(message: String) = log("ERR", message)

    /** 导出文本（时间 级别 消息，倒序=最新在上），供日志页「导出」分享。 */
    val exportText: String
        get() {
            val header = "HKT BLETools 诊断日志 · ${_entries.size} 条 · 导出于 ${Date()}\n"
            return header + _entries.toList().asReversed()
                .joinToString("\n") { "${it.timestamp} [${it.level}] ${it.message}" }
        }
}
