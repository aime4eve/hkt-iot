package com.hkt.ble.bletools.model

import androidx.compose.runtime.mutableStateListOf

/**
 * 阀门任务本地镜像存储（P-tasks 规格卡 §3）：0x04/0x05 的 App 侧记录，内存态。
 * iOS ValveTaskStore 的 Kotlin 移植（mutableStateListOf 供 Compose 观察，同 LogStore 模式）。
 * 设备无 BLE 回读命令（固件仅支持 LoRa 平台侧回读 0x3D），故镜像只在 ACK 确认后记录。
 */
object ValveTaskStore {
    data class Task(
        val id: Int,       // 槽位 1–16
        val valve: Int,    // 0 双阀 / 1 阀1 / 2 阀2
        val state: Int,    // 1 开 / 0 关
        val pulse: Int,
        val sh: Int, val sm: Int, val eh: Int, val em: Int,
        val days: List<Boolean>,   // 周一…周日
    )

    val tasks = mutableStateListOf<Task>()

    fun upsert(task: Task) {
        val index = tasks.indexOfFirst { it.id == task.id }
        if (index >= 0) tasks[index] = task else tasks.add(task)
        tasks.sortBy { it.id }
    }

    fun delete(id: Int) {
        tasks.removeAll { it.id == id }
    }

    fun deleteAll() {
        tasks.clear()
    }

    fun isUsed(id: Int): Boolean = tasks.any { it.id == id }
}
