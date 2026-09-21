package com.hkt.ble.bletools.core.ble

/** 蓝牙系统能力/权限映射（R-24 四态引导 + 就绪/初始化中）。 */
enum class BLEAvailability {
    /** 系统管理器尚未回报状态（含首次触发系统权限弹窗） */
    INITIALIZING,
    /** 已授权且蓝牙开启 */
    READY,
    /** 权限被拒（引导去系统设置） */
    DENIED,
    /** 蓝牙已关闭（引导开启） */
    POWERED_OFF,
    /** 设备不支持 BLE */
    UNSUPPORTED;

    val isUsable: Boolean get() = this == READY

    companion object {
        /** android.bluetooth.BluetoothAdapter.STATE_* → 能力态（SystemCentral 映射用）。 */
        fun fromAdapterState(state: Int): BLEAvailability = when (state) {
            10 /* STATE_OFF */ -> POWERED_OFF
            12 /* STATE_ON */ -> READY
            13 /* STATE_TURNING_OFF */, 11 /* STATE_TURNING_ON */ -> INITIALIZING
            else -> INITIALIZING
        }
    }
}
