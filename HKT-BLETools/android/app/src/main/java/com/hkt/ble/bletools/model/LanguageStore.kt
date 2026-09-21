package com.hkt.ble.bletools.model

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.hkt.ble.bletools.ui.HktLang

/** 语言三态（P-07：跟随系统/简体中文/English；cycle 顺序=原型 cycleLang）。 */
enum class LanguageMode { SYSTEM, ZH, EN }

/**
 * 语言存储（iOS LanguageStore 同构）：设置页三态循环，切换全 App 即时生效。
 * Android 落地 = AppCompatDelegate.setApplicationLocales + Activity recreate（调用方负责）；
 * 本存储只管模式状态与持久化键。
 */
object LanguageStore {
    var mode by mutableStateOf(LanguageMode.SYSTEM)
        private set

    val label: String
        get() = when (mode) {
            LanguageMode.SYSTEM -> if (isZh) "跟随系统" else "Follow System"
            LanguageMode.ZH -> "简体中文"
            LanguageMode.EN -> "English"
        }

    /// 系统语言或显式选择为中文。
    val isZh: Boolean
        get() = when (mode) {
            LanguageMode.SYSTEM -> HktLang.isZh
            LanguageMode.ZH -> true
            LanguageMode.EN -> false
        }

    /** 原型 cycleLang：跟随系统 → 简体中文 → English → 跟随系统。返回切换后的模式供持久化。 */
    fun cycle(): LanguageMode {
        mode = when (mode) {
            LanguageMode.SYSTEM -> LanguageMode.ZH
            LanguageMode.ZH -> LanguageMode.EN
            LanguageMode.EN -> LanguageMode.SYSTEM
        }
        return mode
    }

    /** 从持久化恢复（调用方在 Activity.onCreate 传入存储值）。 */
    fun restore(saved: LanguageMode) {
        mode = saved
    }
}
