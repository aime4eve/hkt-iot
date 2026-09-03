package com.hkt.ble.bletools

import android.Manifest
import android.annotation.SuppressLint
import android.app.AlertDialog
import android.app.TimePickerDialog
import android.bluetooth.BluetoothGatt
import android.content.Context
import android.content.DialogInterface
import android.content.DialogInterface.OnMultiChoiceClickListener
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.Looper
import android.text.InputType
import android.text.TextUtils
import android.util.Log
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.BaseExpandableListAdapter
import android.widget.Button
import android.widget.EditText
import android.widget.ExpandableListView
import android.widget.ImageView
import android.widget.ProgressBar
import android.widget.Spinner
import android.widget.Switch
import android.widget.TextView
import android.widget.TimePicker
import android.widget.Toast
import android.view.inputmethod.EditorInfo
import androidx.activity.ComponentActivity
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.fragment.app.DialogFragment
import java.io.File
import java.text.SimpleDateFormat
import java.util.Calendar
import java.util.concurrent.Executors
import java.util.concurrent.ScheduledExecutorService
import java.util.concurrent.TimeUnit
import kotlin.system.exitProcess

private var statusHandler: Handler? = null
private var statusRunnable: Runnable? = null
private var timestampHandler: Handler? = null
private var timestampRunnable: Runnable? = null
private var powerHandler: Handler? = null
private var powerRunnable: Runnable? = null

private var isExternalPowerUpdate = false
private var lastPower = 0
private var isRefreshPaused = false
private var isPowerUpdate = false
private var otaFileName: String? = null


private fun startUpdatingStatus() {
    // 停止之前的任务（如果有的话）
    stopUpdatingStatus()

    statusRunnable = Runnable {
        // 每秒执行一次
        statusHandler?.postDelayed(statusRunnable!!, 3000)
    }

    // 首次立即执行
    statusHandler?.post(statusRunnable!!)
}
private fun stopUpdatingStatus() {
    statusRunnable?.let {
        statusHandler?.removeCallbacks(it)
        statusRunnable = null
    }
}

private fun startUpdatingTimestamp(timestampText: TextView) {
    // 停止之前的任务（如果有的话）
    stopUpdatingTimestamp()

    timestampRunnable = Runnable {
        val currentTimeMillis = System.currentTimeMillis() / 1000 // 获取当前时间的秒级时间戳
        timestampText.text = currentTimeMillis.toString()
        // 每秒执行一次
        timestampHandler?.postDelayed(timestampRunnable!!, 1000)
    }

    // 首次立即执行
    timestampHandler?.post(timestampRunnable!!)
}
private fun stopUpdatingTimestamp() {
    timestampRunnable?.let {
        timestampHandler?.removeCallbacks(it)
        timestampRunnable = null
    }
}

private fun updatePowerSwitchUI(@SuppressLint("UseSwitchCompatOrMaterialCode") powerSwitch: Switch, power: Int) {
    when (power) {
        1 -> {
            powerSwitch.text = "ON"
            mDeviceDataString.power = "ON"
            powerSwitch.isChecked = true
            Log.i("updatePowerSwitchUI", "powerSwitch is true")
        }
        else -> {
            powerSwitch.text = "OFF"
            mDeviceDataString.power = "OFF"
            powerSwitch.isChecked = false
            Log.i("updatePowerSwitchUI", "powerSwitch is false")
        }
    }
}

private fun startUpdatingPower(powerSwitch: Switch) {
    // 停止之前的任务（如果有的话）
    stopUpdatingPower()
    powerHandler = Handler(Looper.getMainLooper()) // 确保在主线程上更新UI
    powerRunnable = Runnable {
        if(lastPower != mDeviceData.power){
            lastPower = mDeviceData.power
            isExternalPowerUpdate = true
        }
        updatePowerSwitchUI(powerSwitch, mDeviceData.power)
//        powerHandler?.postDelayed(powerRunnable!!, 3000)
    }
    // 首次立即执行
    powerHandler?.post(powerRunnable!!)
}

private fun stopUpdatingPower() {
    powerRunnable?.let {
        powerHandler?.removeCallbacks(it)
        powerHandler = null
    }
}


data class Group(val id: Int, val title: String, val groupType: Int)
data class Child(val id: Int, val groupId: Int, val title: String, val word: String)

class ExpandableListAdapter(private val context: Context, private var groups: List<Group>, private var children: Map<Int, List<Child>>) : BaseExpandableListAdapter() {

    private val configViews = mutableMapOf<Int, View>()
    private val shouldRefreshConfigValues = mutableMapOf<Int, Boolean>()

    fun requestConfigValueRefresh() {
        configViews.keys.forEach { shouldRefreshConfigValues[it] = true }
    }

    private fun getCachedConfigView(layoutRes: Int, groupType: Int, parent: ViewGroup?): View {
        shouldRefreshConfigValues.putIfAbsent(layoutRes, true)
        val view = configViews.getOrPut(layoutRes) {
            LayoutInflater.from(context).inflate(layoutRes, parent, false)
        }
        view.tag = groupType
        setupNumericInputs(view)
        return view
    }

    private data class NumericInputSpec(
        val titleRes: Int,
        val min: Int,
        val max: Int,
        val allowZero: Boolean = false
    )

    private fun setupNumericInputs(view: View) {
        if (view is EditText) {
            setupNumericInput(view)
        } else if (view is ViewGroup) {
            for (index in 0 until view.childCount) {
                setupNumericInputs(view.getChildAt(index))
            }
        }
    }

    private fun setupNumericInput(editText: EditText) {
        val spec = when (editText.id) {
            R.id.tv_low_threshold ->
                NumericInputSpec(R.string.numeric_low_threshold, 30, 4500)
            R.id.tv_high_threshold ->
                NumericInputSpec(R.string.numeric_high_threshold, 30, 4500, allowZero = true)
            R.id.tv_report_period ->
                NumericInputSpec(R.string.numeric_report_period, 1, 1440)
            R.id.tv_gps_period ->
                NumericInputSpec(R.string.numeric_gps_period, 10, 1440, allowZero = true)
            R.id.et_report_period_dc200 ->
                NumericInputSpec(R.string.numeric_report_period, 1, 1440)
            R.id.et_buffeting_duration ->
                NumericInputSpec(R.string.numeric_buffeting_duration, 1, 255)
            R.id.et_report_period_svc100 ->
                NumericInputSpec(R.string.numeric_report_period, 1, 1440)
            R.id.et_time_realtime_task ->
                NumericInputSpec(R.string.numeric_time_seconds, 0, 65535)
            R.id.et_pulse_realtime_task ->
                NumericInputSpec(R.string.numeric_pulse, 0, 65535)
            R.id.et_pulse_timed ->
                NumericInputSpec(R.string.numeric_pulse, 0, 65535)
            else -> return
        }

        editText.isFocusable = false
        editText.isFocusableInTouchMode = false
        editText.isCursorVisible = false
        editText.inputType = InputType.TYPE_NULL
        editText.setOnClickListener {
            showNumericInputDialog(editText, spec)
        }
    }

    private fun showNumericInputDialog(currentValueEditText: EditText, spec: NumericInputSpec) {
        val input = EditText(context).apply {
            inputType = InputType.TYPE_CLASS_NUMBER
            imeOptions = EditorInfo.IME_ACTION_DONE
            setSingleLine()
            hint = currentValueEditText.hint
            setText(currentValueEditText.text.toString())
            setSelection(length())
        }

        val dialog = AlertDialog.Builder(context)
            .setTitle(context.getString(spec.titleRes))
            .setView(input)
            .setPositiveButton(context.getString(R.string.ok)) { _, _ -> }
            .setNegativeButton(context.getString(R.string.cancel), null)
            .create()

        fun submitIfValid() {
            val value = input.text.toString().toIntOrNull()
            val isValid = value != null &&
                ((value == 0 && spec.allowZero) || value in spec.min..spec.max)
            if (isValid) {
                currentValueEditText.setText(input.text.toString())
                dialog.dismiss()
            } else {
                input.error = context.getString(R.string.invalid_numeric_range)
            }
        }

        input.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                submitIfValid()
                true
            } else {
                false
            }
        }

        dialog.setOnShowListener {
            dialog.getButton(AlertDialog.BUTTON_POSITIVE)?.setOnClickListener { submitIfValid() }
            input.requestFocus()
            dialog.window?.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE)
        }
        dialog.show()
    }

    // 获取组的视图
    private fun getStatusView(convertView: View?, parent: ViewGroup?, isExpanded: Boolean): View {
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.group_item, parent, false)

        val titleTextView = view.findViewById<TextView>(R.id.tv_group_title)
        val imageView = view.findViewById<ImageView>(R.id.tv_group_icon)

        titleTextView?.text = "Status"

        if (isExpanded) {
            imageView?.setImageResource(R.drawable.arrow_down)
        } else {
            imageView?.setImageResource(R.drawable.arrow_right)
        }
        return view
    }
    @SuppressLint("UseSwitchCompatOrMaterialCode")
    private fun getPowerView(convertView: View?, parent: ViewGroup?): View {
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.dialog_config_power, parent, false)
        val powerSwitch = view.findViewById<Switch>(R.id.tv_switch_power) ?: return view

        // 确保每次获取视图时更新 Power 状态
        if(isPowerUpdate) {
            isPowerUpdate = false
            startUpdatingPower(powerSwitch)
        }

        // 设置监听器，但使用标志位来控制是否执行内部逻辑
        powerSwitch.setOnCheckedChangeListener { _, isChecked ->
            isRefreshPaused = true
            Log.i("getPowerView", "isRefreshPaused is true")
            if (isChecked) {
                powerSwitch.text = "ON"
                if (!isExternalPowerUpdate) {
                    mDeviceEvent.power = 1
                    mDeviceDataString.power = "ON"
                    mDeviceEvent.event = DeviceEventEnum.POWER_ON_EVENT.ordinal
                }
            } else {
                powerSwitch.text = "OFF"
                if (!isExternalPowerUpdate) {
                    mDeviceEvent.power = 0
                    mDeviceDataString.power = "OFF"
                    mDeviceEvent.event = DeviceEventEnum.POWER_OFF_EVENT.ordinal
                }
            }
            // 每次监听器执行完后，将isExternalUpdate重置为false，避免影响后续正常操作
            isExternalPowerUpdate = false
        }

        return view
    }

    private fun getCalibrationView(convertView: View?, parent: ViewGroup?): View {
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.dialog_config_calibration, parent, false)
        val calibrationButton = view.findViewById<Button>(R.id.tv_button_calibration)

        calibrationButton.setOnClickListener(View.OnClickListener {
            // 处理点击事件
            if(mDeviceData.power == 1){
                Toast.makeText(context, "Performing Calibration", Toast.LENGTH_SHORT).show()
                val locale =
                    context.resources.configuration.locales.get(0)
                val tipText = when (locale.language) {
                    "zh" -> "请耐心等待校准完成，预计需要90秒"
                    "en" -> "Please wait patiently for the calibration to complete, it is expected to take 90 seconds"
                    // 可以添加更多语言判断分支，如日语等
                    else -> "Please wait patiently for the calibration to complete, it is expected to take 90 seconds" // 默认显示英文或其他合适的提示语
                }
                Toast.makeText(context, tipText, Toast.LENGTH_LONG).show()

                mDeviceEvent.event = DeviceEventEnum.CALIBRATION_EVENT.ordinal
            }else{
                Toast.makeText(context, "Calibration Error, Result Power OFF", Toast.LENGTH_SHORT).show()
            }
        })
        return view
    }
    private fun getSyncTimestampView(convertView: View?, parent: ViewGroup?): View {
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.dialog_config_timestamp, parent, false)
        val syncTimestamp = view.findViewById<Button>(R.id.bt_sync_timestamp)
        val timestampText = view.findViewById<TextView>(R.id.tv_timestamp)

        timestampHandler = Handler(Looper.getMainLooper()) // 确保在主线程上更新UI
        startUpdatingTimestamp(timestampText)

        syncTimestamp?.setOnClickListener(View.OnClickListener {
            Toast.makeText(context, "Synchronize time", Toast.LENGTH_SHORT).show()
            var inputText = timestampText.text.toString()
            if (inputText.isNotEmpty() && TextUtils.isDigitsOnly(inputText)) {
                try {
                    val number: Int = inputText.toInt()
                    // 在这里使用转换后的整型
                    println("转换后的数字是: $number")
                    mDeviceEvent.Timestamp = number
                } catch (e: NumberFormatException) {
                    // 这里通常不会触发，因为已经用TextUtils.isDigitsOnly检查过了
                    e.printStackTrace()
                }
            } else {
                // 输入为空或不是纯数字
                println("输入无效")
            }
            mDeviceEvent.event = DeviceEventEnum.SYNC_TIMESTAMP_EVENT.ordinal
        })
        return view
    }
    private fun getConfigUDS100View(convertView: View?, parent: ViewGroup?): View {
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.dialog_config_uds100, parent, false)
        val configButton = view.findViewById<Button>(R.id.tv_button_config)
        val lowThresholdEditText = view.findViewById<EditText>(R.id.tv_low_threshold)
        val highThresholdEditText = view.findViewById<EditText>(R.id.tv_high_threshold)
        val reportPeriodEditText = view.findViewById<EditText>(R.id.tv_report_period)
        val gpsPeriodEditText = view.findViewById<EditText>(R.id.tv_gps_period)

        // Refresh from device data only for initial sync or an explicit reset.
        if (shouldRefreshConfigValues.getOrDefault(R.layout.dialog_config_uds100, true) && mDeviceData.reportPeriod > 0) {
            lowThresholdEditText.setText(mDeviceData.overflowLowThreshold.toString())
            highThresholdEditText.setText(mDeviceData.overflowHighThreshold.toString())
            reportPeriodEditText.setText(mDeviceData.reportPeriod.toString())
            gpsPeriodEditText.setText(mDeviceData.gpsPeriod.toString())
            shouldRefreshConfigValues[R.layout.dialog_config_uds100] = false
        }
        // 设置点击监听器
        configButton.setOnClickListener(View.OnClickListener {
            var error = 0

            fun validate(field: EditText, isValid: (Int) -> Boolean): Int? {
                val inputText = field.text.toString()
                val number = if (inputText.isNotEmpty() && TextUtils.isDigitsOnly(inputText)) {
                    try {
                        inputText.toInt()
                    } catch (e: NumberFormatException) {
                        null
                    }
                } else {
                    null
                }

                return if (number != null && isValid(number)) {
                    field.error = null
                    number
                } else {
                    field.error = context.getString(R.string.invalid_numeric_range)
                    null
                }
            }

            validate(highThresholdEditText) { (it < 30 && it != 0) || it > 4500 }
                ?.let { mDeviceEvent.overflowHighThreshold = it } ?: run { error++ }
            validate(lowThresholdEditText) { it < 30 || it > 4500 }
                ?.let { mDeviceEvent.overflowLowThreshold = it } ?: run { error++ }
            validate(reportPeriodEditText) { it < 1 || it > 1440 }
                ?.let { mDeviceEvent.reportPeriod = it } ?: run { error++ }
            validate(gpsPeriodEditText) { (it < 10 && it != 0) || it > 1440 }
                ?.let { mDeviceEvent.gpsPeriod = it } ?: run { error++ }

            if(error > 0){
                Toast.makeText(context, R.string.invalid_numeric_range, Toast.LENGTH_SHORT).show()
            }else{
                mDeviceEvent.event = DeviceEventEnum.CONFIG_PARAMETER_EVENT.ordinal
            }
        })
        return view
    }
    private fun getConfigDC200View(convertView: View?, parent: ViewGroup?): View {
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.dialog_config_dc200, parent, false)
        val configButton = view.findViewById<Button>(R.id.tv_button_config_dc200)
        val reportPeriodEditText = view.findViewById<EditText>(R.id.et_report_period_dc200)
        val spinner: Spinner = view.findViewById(R.id.sp_work_mode_dc200)

        val items = listOf(
            context.getString(R.string.parking_mode_fusion),
            context.getString(R.string.parking_mode_magnetic_only),
            context.getString(R.string.parking_mode_radar_priority)
        )

        // 创建并设置Adapter
        val adapter = ArrayAdapter<String>(context, android.R.layout.simple_spinner_item, items)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        spinner.adapter = adapter

        // mDeviceEvent.parkMode 可能是用户选择或 resetEventFromDevice 后的设备值
        if (mDeviceEvent.parkMode in 0 until items.size) {
            spinner.setSelection(mDeviceEvent.parkMode)
        } else if (mDeviceData.parkMode in 0 until items.size) {
            spinner.setSelection(mDeviceData.parkMode)
        }

        // 设置选择监听器
        spinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                // Spinner order intentionally matches the firmware values 0/1/2.
                mDeviceEvent.parkMode = position
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {
                // 当没有项目被选中时调用（通常不需要处理）
            }
        }

        // Refresh from device data only for initial sync or an explicit reset.
        if (shouldRefreshConfigValues.getOrDefault(R.layout.dialog_config_dc200, true) && mDeviceData.reportPeriod > 0) {
            reportPeriodEditText.setText(mDeviceData.reportPeriod.toString())
            shouldRefreshConfigValues[R.layout.dialog_config_dc200] = false
        }
        // 设置点击监听器
        configButton.setOnClickListener(View.OnClickListener {
            var error = 0
            val inputText = reportPeriodEditText.text.toString()
            if (inputText.isNotEmpty() && TextUtils.isDigitsOnly(inputText)) {
                try {
                    val number: Int = inputText.toInt()
                    // 在这里使用转换后的整型
//                    println("转换后的数字是: $number")
                    mDeviceEvent.reportPeriod = number
                    if(number < 1 || number > 1440) {
                        error++
                    }
                } catch (e: NumberFormatException) {
                    // 这里通常不会触发，因为已经用TextUtils.isDigitsOnly检查过了
                    e.printStackTrace()
                }
            } else {
                // 输入为空或不是纯数字
                println("输入无效")
                error++
            }

            if(error > 0){
                Toast.makeText(context, "The input format is incorrect", Toast.LENGTH_SHORT).show()
            }else{
                mDeviceEvent.event = DeviceEventEnum.CONFIG_PARAMETER_EVENT.ordinal
            }
        })
        return view
    }
    private fun getConfigSVC100View(convertView: View?, parent: ViewGroup?): View {
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.dialog_config_svc100, parent, false)

        val volOutSpinner = view.findViewById<Spinner>(R.id.sp_vol_out)
        val valveModeSpinner = view.findViewById<Spinner>(R.id.sp_valve_mode)
        val buffetingDurationEditText = view.findViewById<EditText>(R.id.et_buffeting_duration)
        val autoPowerOnSpinner = view.findViewById<Spinner>(R.id.sp_auto_power_on)
        val timezoneOnSpinner = view.findViewById<Spinner>(R.id.sp_timezone)
        val reportPeriodEditText = view.findViewById<EditText>(R.id.et_report_period_svc100)
        val configButton = view.findViewById<Button>(R.id.tv_button_config_svc100)

        val outputVoltageItems = context.resources.getStringArray(R.array.spinner_items_vol)
        val portFunctionItems = context.resources.getStringArray(R.array.spinner_items_valve_mode)
        val portFunctionValues = intArrayOf(0x00, 0x01, 0x02, 0x03, 0x81, 0x82, 0x83)
        val powerModeItems = context.resources.getStringArray(R.array.spinner_items_auto_power_on)
        val timezoneItems = context.resources.getStringArray(R.array.spinner_items_timezone)

        volOutSpinner.adapter = HighlightSpinnerAdapter(context, outputVoltageItems.toList(), volOutSpinner)
        valveModeSpinner.adapter = HighlightSpinnerAdapter(context, portFunctionItems.toList(), valveModeSpinner)
        autoPowerOnSpinner.adapter = HighlightSpinnerAdapter(context, powerModeItems.toList(), autoPowerOnSpinner)
        timezoneOnSpinner.adapter = HighlightSpinnerAdapter(context, timezoneItems.toList(), timezoneOnSpinner)

        fun updateStableTimeEditor() {
            val usesStableTime = portFunctionValues.getOrNull(valveModeSpinner.selectedItemPosition)
                ?.let { it and 0x80 != 0 } == true
            buffetingDurationEditText.isEnabled = usesStableTime
            buffetingDurationEditText.alpha = if (usesStableTime) 1.0F else 0.5F
            buffetingDurationEditText.hint = context.getString(
                if (usesStableTime) R.string.range_stable_time_seconds else R.string.stable_time_hint
            )
        }

        // Refresh from device data only for initial sync or an explicit reset.
        if (shouldRefreshConfigValues.getOrDefault(R.layout.dialog_config_svc100, true) && mDeviceData.reportPeriod > 0) {
            if (mDeviceData.volOut in outputVoltageItems.indices) {
                volOutSpinner.setSelection(mDeviceData.volOut)
            }
            val portFunctionIndex = portFunctionValues.indexOf(mDeviceData.valveMode)
            if (portFunctionIndex >= 0) {
                valveModeSpinner.setSelection(portFunctionIndex)
            }
            buffetingDurationEditText.setText(mDeviceData.buffetingDuration.toString())
            if (mDeviceData.autoPower in powerModeItems.indices) {
                autoPowerOnSpinner.setSelection(mDeviceData.autoPower)
            }
            if (mDeviceData.timeZone in timezoneItems.indices) {
                timezoneOnSpinner.setSelection(mDeviceData.timeZone)
            }
            reportPeriodEditText.setText(mDeviceData.reportPeriod.toString())
            shouldRefreshConfigValues[R.layout.dialog_config_svc100] = false
        }

        updateStableTimeEditor()
        valveModeSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                updateStableTimeEditor()
            }

            override fun onNothingSelected(parent: AdapterView<*>?) = Unit
        }

        // 设置点击监听器
        configButton?.setOnClickListener(View.OnClickListener {
            var error = 0

            fun validate(field: EditText, isValid: (Int) -> Boolean): Int? {
                val inputText = field.text.toString()
                val number = if (inputText.isNotEmpty() && TextUtils.isDigitsOnly(inputText)) {
                    try {
                        inputText.toInt()
                    } catch (e: NumberFormatException) {
                        null
                    }
                } else {
                    null
                }

                return if (number != null && isValid(number)) {
                    field.error = null
                    number
                } else {
                    field.error = context.getString(R.string.invalid_numeric_range)
                    null
                }
            }

            validate(reportPeriodEditText) { it < 1 || it > 1440 }
                ?.let { mDeviceEvent.reportPeriod = it } ?: run { error++ }
            validate(buffetingDurationEditText) { it < 1 || it > 255 }
                ?.let { mDeviceEvent.buffetingDuration = it } ?: run { error++ }

            if(error > 0){
                Toast.makeText(context, R.string.invalid_numeric_range, Toast.LENGTH_SHORT).show()
            }else{
                mDeviceEvent.volOut = volOutSpinner.selectedItemPosition
                mDeviceEvent.valveMode = portFunctionValues[valveModeSpinner.selectedItemPosition]
                mDeviceEvent.autoPower = autoPowerOnSpinner.selectedItemPosition
                mDeviceEvent.timeZone = timezoneOnSpinner.selectedItemPosition
                mDeviceEvent.event = DeviceEventEnum.CONFIG_PARAMETER_EVENT.ordinal
            }
        })
        return view
    }
    private fun getRealtimeTask(convertView: View?, parent: ViewGroup?): View {
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.dialog_config_realtime_task, parent, false)

        val valveSpinner = view.findViewById<Spinner>(R.id.sp_valve_realtime_task)
        val openStateSpinner = view.findViewById<Spinner>(R.id.sp_open_state_realtime_task)
        val pulseEditText = view.findViewById<EditText>(R.id.et_pulse_realtime_task)
        val timeEditText = view.findViewById<EditText>(R.id.et_time_realtime_task)
        val configButton = view.findViewById<Button>(R.id.bt_config_realtime_task)

        valveSpinner.adapter = HighlightSpinnerAdapter(
            context,
            context.resources.getStringArray(R.array.spinner_items_valve).toList(),
            valveSpinner
        )
        openStateSpinner.adapter = HighlightSpinnerAdapter(
            context,
            context.resources.getStringArray(R.array.spinner_items_check).toList(),
            openStateSpinner
        )

        // 设置点击监听器
        configButton?.setOnClickListener(View.OnClickListener {
            var error = 0
            val pulse = pulseEditText.text.toString()
            val time=  timeEditText.text.toString()
            if (pulse.isNotEmpty() && TextUtils.isDigitsOnly(pulse)) {
                try {
                    val number: Int = pulse.toInt()
                    // 在这里使用转换后的整型
                    mDeviceEvent.pulseRealtime = number
                    if(number > 65535) {
                        error++
                        pulseEditText.error = context.getString(R.string.invalid_numeric_range)
                    } else {
                        pulseEditText.error = null
                    }
                } catch (e: NumberFormatException) {
                    // 这里通常不会触发，因为已经用TextUtils.isDigitsOnly检查过了
                    e.printStackTrace()
                }
            } else {
                error++
                pulseEditText.error = context.getString(R.string.invalid_numeric_range)
            }

            if (time.isNotEmpty() && TextUtils.isDigitsOnly(time)) {
                try {
                    val number: Int = time.toInt()
                    // 在这里使用转换后的整型
                    mDeviceEvent.timeRealtime = number
                    if(number > 65535) {
                        error++
                        timeEditText.error = context.getString(R.string.invalid_numeric_range)
                    } else {
                        timeEditText.error = null
                    }
                } catch (e: NumberFormatException) {
                    // 这里通常不会触发，因为已经用TextUtils.isDigitsOnly检查过了
                    e.printStackTrace()
                }
            } else {
                error++
                timeEditText.error = context.getString(R.string.invalid_numeric_range)
            }

            if(error > 0){
                Toast.makeText(context, "The input format is incorrect", Toast.LENGTH_SHORT).show()
            }else{
                mDeviceEvent.valveRealtime = valveSpinner.selectedItemPosition
                mDeviceEvent.stateRealtime = openStateSpinner.selectedItemPosition
                mDeviceEvent.event = DeviceEventEnum.CONFIG_REALTIME_TASK.ordinal
            }
        })
        return view
    }
    private fun getTimedTask(convertView: View?, parent: ViewGroup?): View {
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.dialog_config_timed_task, parent, false)

        val idSpinner = view.findViewById<Spinner>(R.id.sp_task_number)
        val valveSpinner = view.findViewById<Spinner>(R.id.sp_valve_timed)
        val openStateSpinner = view.findViewById<Spinner>(R.id.sp_open_state_timed)
        val pulseEditText = view.findViewById<EditText>(R.id.et_pulse_timed)
        val startTimeEditText = view.findViewById<EditText>(R.id.et_task_start_time_timed)
        val endTimeEditText = view.findViewById<EditText>(R.id.et_task_end_time_timed)
        val repeatTextView = view.findViewById<TextView>(R.id.tv_task_repeat_timed)
        val configButton = view.findViewById<Button>(R.id.bt_config_timed_task)
        val deleteButton = view.findViewById<Button>(R.id.bt_delete_timed_task)

        idSpinner.adapter = HighlightSpinnerAdapter(
            context,
            context.resources.getStringArray(R.array.spinner_items_task).toList(),
            idSpinner
        )
        valveSpinner.adapter = HighlightSpinnerAdapter(
            context,
            context.resources.getStringArray(R.array.spinner_items_valve).toList(),
            valveSpinner
        )
        openStateSpinner.adapter = HighlightSpinnerAdapter(
            context,
            context.resources.getStringArray(R.array.spinner_items_check).toList(),
            openStateSpinner
        )

        val repeatDays = listOf("1", "2", "3", "4", "5", "6", "7")
        val repeatSelected = BooleanArray(repeatDays.size)
        val repeatValues = arrayOfNulls<String>(repeatDays.size).also { values ->
            repeatDays.forEachIndexed { index, day -> values[index] = day }
        }
        repeatTextView?.setOnClickListener {
            checkboxEdit(repeatTextView, repeatSelected, repeatValues, IntArray(repeatDays.size))
        }

        startTimeEditText?.setOnClickListener {
            val calendar: Calendar = Calendar.getInstance()
            val hour: Int = calendar.get(Calendar.HOUR_OF_DAY)
            val minute: Int = calendar.get(Calendar.MINUTE)
            val timePickerDialog = TimePickerDialog(
                context,
                { _: TimePicker?, selectedHour: Int, selectedMinute: Int ->
                    startTimeEditText.setText(formatTime(selectedHour, selectedMinute))
                    mDeviceEvent.startTimeTimed = selectedHour * 60 + selectedMinute
                },
                hour,
                minute,
                false
            )
            timePickerDialog.show()
        }
        endTimeEditText?.setOnClickListener {
            val calendar: Calendar = Calendar.getInstance()
            val hour: Int = calendar.get(Calendar.HOUR_OF_DAY)
            val minute: Int = calendar.get(Calendar.MINUTE)
            val timePickerDialog = TimePickerDialog(
                context,
                { _: TimePicker?, selectedHour: Int, selectedMinute: Int ->
                    endTimeEditText.setText(formatTime(selectedHour, selectedMinute))
                    mDeviceEvent.endTimeTimed = selectedHour * 60 + selectedMinute
                },
                hour,
                minute,
                false
            )
            timePickerDialog.show()
        }
        // 设置点击监听器
        configButton?.setOnClickListener(View.OnClickListener {
            var error = 0
            val pulse = pulseEditText.text.toString()
            var id = idSpinner.selectedItemPosition

            if(id == 16){
                id = 0xFF
            }else{
                id += 1
            }

            if (pulse.isNotEmpty() && TextUtils.isDigitsOnly(pulse)) {
                try {
                    val number: Int = pulse.toInt()
                    // 在这里使用转换后的整型
                    mDeviceEvent.pulseTimed = number
                    if(number > 65535) {
                        error++
                        pulseEditText.error = context.getString(R.string.invalid_numeric_range)
                    } else {
                        pulseEditText.error = null
                    }
                } catch (e: NumberFormatException) {
                    // 这里通常不会触发，因为已经用TextUtils.isDigitsOnly检查过了
                    e.printStackTrace()
                }
            } else {
                error++
                pulseEditText.error = context.getString(R.string.invalid_numeric_range)
            }

            if(mDeviceEvent.endTimeTimed - mDeviceEvent.startTimeTimed <= 0){
                error++
                startTimeEditText.error = context.getString(R.string.invalid_time_range)
                endTimeEditText.error = context.getString(R.string.invalid_time_range)
            } else {
                startTimeEditText.error = null
                endTimeEditText.error = null
            }

            if(id == 0xFF){
                error++
            }

            if(error > 0){
                Toast.makeText(context, R.string.invalid_numeric_range, Toast.LENGTH_SHORT).show()
            }else{
                mDeviceEvent.idTimed = id
                mDeviceEvent.valveTimed = valveSpinner.selectedItemPosition
                mDeviceEvent.stateTimed = openStateSpinner.selectedItemPosition
//                mDeviceEvent.repeatTimed = repeatSpinner.selectedItemPosition
                mDeviceEvent.event = DeviceEventEnum.CONFIG_TIMED_TASK.ordinal
            }
        })
        deleteButton?.setOnClickListener(View.OnClickListener {
            var id = idSpinner.selectedItemPosition

            if(id == 16){
                id = 0xFF
            }else{
                id += 1
            }

            mDeviceEvent.idTimed = id
            mDeviceEvent.event = DeviceEventEnum.DELETE_TIMED_TASK.ordinal
        })
        return view
    }
    private fun getOTAView(convertView: View?, parent: ViewGroup?): View {
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.dialog_ota, parent, false)
        val fileInfoLayout = view.findViewById<View>(R.id.ll_ota_file_info)
        val fileNameTextView = view.findViewById<TextView>(R.id.tv_ota_file_name)
        val fileSizeTextView = view.findViewById<TextView>(R.id.tv_ota_file_size)
        val selectFileButton = view.findViewById<Button>(R.id.tv_select_file)
        val updateButton = view.findViewById<Button>(R.id.tv_update)

        val selectedFileName = otaFileName
        if (fileBin.isEmpty() || selectedFileName == null) {
            fileInfoLayout?.visibility = View.GONE
        } else {
            fileInfoLayout?.visibility = View.VISIBLE
            fileNameTextView?.text = context.getString(R.string.ota_file_name, selectedFileName)
            fileSizeTextView?.text = context.getString(R.string.ota_file_size, fileBin.length / 2)
        }

        // 设置点击监听器
        selectFileButton?.setOnClickListener(View.OnClickListener {
            mDeviceEvent.event = DeviceEventEnum.SELECT_FILE.ordinal
        })

        // 设置点击监听器
        updateButton?.setOnClickListener(View.OnClickListener {
            mDeviceEvent.event = DeviceEventEnum.START_OTA.ordinal
//            Toast.makeText(context, "Executed successfully", Toast.LENGTH_SHORT).show();
        })
        return view
    }

    // 定义一个函数来添加键值对到HashMap并添加到列表中
    private fun addItemToMapAndList(key: String, value: String, list: MutableList<MutableMap<String, String>>): MutableMap<String, String> {
        val map = HashMap<String, String>()
        map[key] = value
        list.add(map)
        return map  // 如果需要返回这个map
    }
    private fun checkboxEdit(edit: TextView, selected: BooleanArray, str: Array<String?>, id: IntArray) {
        val builder: AlertDialog.Builder = AlertDialog.Builder(context)
        builder.setTitle("Repeat")
        val multiChoiceClickListener = OnMultiChoiceClickListener { _, which, isChecked -> selected[which] = isChecked }
        builder.setMultiChoiceItems(str, selected, multiChoiceClickListener)
        val clickListener = DialogInterface.OnClickListener { _, _ ->
                var selectStr = ""
                var ids = ""
                mDeviceEvent.repeatTimed = 0
                for (i in selected.indices) {
                    if (selected[i]) {
                        if (TextUtils.isEmpty(selectStr)) {
                            selectStr += str[i]
                            ids += id[i]
                        } else {
                            selectStr = selectStr + "," + str[i]
                            ids = ids + "," + id[i]
                            mDeviceEvent.repeatTimed += (1 shl i)
                        }
                    }
                }
                edit.text = selectStr
            }
        builder.setCancelable(false)
        builder.setNegativeButton("cancel", null)
        builder.setPositiveButton("ok", clickListener)
        val dialog: AlertDialog = builder.create()
        dialog.show()
    }

    fun updateChildren(newChildren: Map<Int, List<Child>>) {
        this.children = newChildren
        updateUserNotify()
    }

    fun updateGroups(newGroups: List<Group>) {
        this.groups  = newGroups
        notifyDataSetChanged()
    }

    fun updateUserNotify() {
        if (!isRefreshPaused) {
            notifyDataSetChanged()
        }
    }

    @SuppressLint("SimpleDateFormat")
    private fun formatTime(hour: Int, minute: Int): String {
        val sdf = SimpleDateFormat("HH:mm")
        val cal = Calendar.getInstance()
        cal.set(Calendar.HOUR_OF_DAY, hour)
        cal.set(Calendar.MINUTE, minute)
        cal.set(Calendar.SECOND, 0)
        cal.set(Calendar.MILLISECOND, 0)
        return sdf.format(cal.time)
    }


    override fun getGroupView(groupPosition: Int, isExpanded: Boolean, convertView: View?, parent: ViewGroup?): View {
        val group = groups[groupPosition]

        val configLayoutRes = when {
            mDeviceData.name == DeviceNameEnum.NAME_SVC100.ordinal && group.groupType == 2 ->
                R.layout.dialog_config_svc100
            mDeviceData.name == DeviceNameEnum.NAME_SVC100.ordinal && group.groupType == 3 ->
                R.layout.dialog_config_realtime_task
            mDeviceData.name == DeviceNameEnum.NAME_SVC100.ordinal && group.groupType == 4 ->
                R.layout.dialog_config_timed_task
            mDeviceData.name == DeviceNameEnum.NAME_UDS100.ordinal && group.groupType == 3 ->
                R.layout.dialog_config_uds100
            mDeviceData.name == DeviceNameEnum.NAME_DC200.ordinal && group.groupType == 3 ->
                R.layout.dialog_config_dc200
            else -> null
        }
        if (configLayoutRes != null) {
            val view = getCachedConfigView(configLayoutRes, group.groupType, parent)
            return when (configLayoutRes) {
                R.layout.dialog_config_svc100 -> getConfigSVC100View(view, parent)
                R.layout.dialog_config_realtime_task -> getRealtimeTask(view, parent)
                R.layout.dialog_config_timed_task -> getTimedTask(view, parent)
                R.layout.dialog_config_dc200 -> getConfigDC200View(view, parent)
                else -> getConfigUDS100View(view, parent)
            }
        }

        // 尝试复用 convertView，如果为空则通过 LayoutInflater 创建新视图
//        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.group_item, parent, false)
        Log.d("getGroupView", "$group")

        var view: View
        // 尝试复用 convertView，如果为空或 tag 不匹配则创建新视图
        if (convertView == null || convertView.tag != group.groupType) {
            view = LayoutInflater.from(context).inflate(R.layout.group_item, parent, false)
            when (group.groupType) {
                0 -> {
                    // 配置状态视图
                    view = LayoutInflater.from(context).inflate(R.layout.group_item, parent, false)
                }
                1, 2, 3, 4, 5, 6 -> {
                    if (mDeviceData.name == DeviceNameEnum.NAME_SVC100.ordinal) {
                        when (group.groupType) {
                            1 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_config_power, parent, false)
                            2 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_config_svc100, parent, false)
                            3 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_config_realtime_task, parent, false)
                            4 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_config_timed_task, parent, false)
                            5 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_config_timestamp, parent, false)
                            6 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_ota, parent, false)
                        }
                    }
                    else if (mDeviceData.name == DeviceNameEnum.VALUE_NULL.ordinal) {
                        when (group.groupType) {
                            1 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_config_timestamp, parent, false)
                            2  -> view = LayoutInflater.from(context).inflate(R.layout.dialog_ota, parent, false)
                        }
                    }
                    else {
                        // 处理非 SVC100 设备的情况
                        when (group.groupType) {
                            1 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_config_power, parent, false)
                            2 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_config_calibration, parent, false)
                            3 -> {
                                // 根据设备类型配置 Config 视图
                                if (mDeviceData.name == DeviceNameEnum.NAME_UDS100.ordinal)
                                    view = LayoutInflater.from(context).inflate(R.layout.dialog_config_uds100, parent, false)
                                else if (mDeviceData.name == DeviceNameEnum.NAME_DC200.ordinal)
                                    view = LayoutInflater.from(context).inflate(R.layout.dialog_config_dc200, parent, false)
                                // 如果没有匹配的设备类型，可以选择不修改 view 或设置默认视图
                            }
                            4 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_config_timestamp, parent, false)
                            5 -> view = LayoutInflater.from(context).inflate(R.layout.dialog_ota, parent, false)
                        }
                    }
                }
            }
            view.tag = group.groupType // 设置 tag 以帮助复用

        } else {
            view = convertView
        }

        // 根据 groupType 和设备类型配置视图
        when (group.groupType) {
            0 -> {
                // 配置状态视图
                return getStatusView(view, parent, isExpanded)
            }
            1, 2, 3, 4, 5, 6 -> {
                if (mDeviceData.name == DeviceNameEnum.NAME_SVC100.ordinal) {
                    when (group.groupType) {
                        1 -> return getPowerView(view, parent)
                        2 -> return getConfigSVC100View(view, parent)
                        3 -> return getRealtimeTask(view, parent)
                        4 -> return getTimedTask(view, parent)
                        5 -> return getSyncTimestampView(view, parent)
                        6 -> return getOTAView(view, parent)
                    }
                }else if (mDeviceData.name == DeviceNameEnum.VALUE_NULL.ordinal) {
                    when (group.groupType) {
                        1 -> return getSyncTimestampView(view, parent)
                        2 -> return getOTAView(view, parent)
                    }
                }
                else {
                    // 处理非 SVC100 设备的情况
                    when (group.groupType) {
                        1 -> return getPowerView(view, parent) // 假设所有设备都需要 Power 视图
                        2 -> return getCalibrationView(view, parent) // 假设这是非 SVC100 设备的特定视图
                        3 -> {
                            // 根据设备类型配置 Config 视图
                            if (mDeviceData.name == DeviceNameEnum.NAME_UDS100.ordinal)
                                return getConfigUDS100View(view, parent)
                            else if (mDeviceData.name == DeviceNameEnum.NAME_DC200.ordinal)
                                return getConfigDC200View(view, parent)
                            // 如果没有匹配的设备类型，可以选择不修改 view 或设置默认视图
                        }
                        4 -> return getSyncTimestampView(view, parent)
                        5 -> return getOTAView(view, parent)
                    }
                }
            }
            // 可以添加其他 groupType 的处理
            else -> {
                // 处理未知的 groupType
                Log.d("getGroupView", "处理未知的 groupType: $group")
            }
        }

        // 假设这些方法总是修改并返回传入的 view（或至少不返回 null）
        return view
    }

    // 你可以考虑将获取子项的逻辑封装到一个辅助函数中，以减少重复代码
    private fun getChildAt(groupPosition: Int, childPosition: Int): Child {
        val groupId = groups[groupPosition].id
        return children[groupId]?.get(childPosition) ?: throw IndexOutOfBoundsException("No child at position $childPosition in group $groupId")
    }

    // 获取子项的视图（使用辅助函数）
    override fun getChildView(groupPosition: Int, childPosition: Int, isLastChild: Boolean, convertView: View?, parent: ViewGroup?): View {
        val child = getChildAt(groupPosition, childPosition)
        val group = groups[groupPosition]
        val view = convertView ?: LayoutInflater.from(context).inflate(R.layout.child_item, parent, false)
        val titleTextView = view.findViewById<TextView>(R.id.tv_child_title)
        val wordTextView = view.findViewById<TextView>(R.id.tv_child_word)
        if (titleTextView == null || wordTextView == null) {
            Log.e(
                "ExpandableListAdapter",
                "titleTextView/wordTextView is null for child $childPosition"
            )
        } else {
            titleTextView.text = child.title
            wordTextView.text = child.word
        }
        return view
    }

    // 子项是否可选
    override fun isChildSelectable(groupPosition: Int, childPosition: Int): Boolean = true

    // 组和子项的ID是否稳定
    override fun hasStableIds(): Boolean = true

    // 获取组的数量
    override fun getGroupCount(): Int = groups.size

    // 获取指定组的子项数量
    override fun getChildrenCount(groupPosition: Int): Int = children[groups[groupPosition].id]?.size ?: 0

    // 获取组的对象
    override fun getGroup(groupPosition: Int): Any = groups[groupPosition]

    // 获取指定组的子项对象
    override fun getChild(groupPosition: Int, childPosition: Int): Any {
        val groupId = groups[groupPosition].id
        return children[groupId]?.get(childPosition) ?: throw IndexOutOfBoundsException()
    }

    // 组的ID
    override fun getGroupId(groupPosition: Int): Long = groups[groupPosition].id.toLong()

    // 子项的ID
    override fun getChildId(groupPosition: Int, childPosition: Int): Long =
        children[groups[groupPosition].id]?.get(childPosition)?.id?.toLong() ?: -1
}

class DeviceActivity: AppCompatActivity(){

    private var gatt: BluetoothGatt? = MainActivity.getGatt()
    private val stream: StreamThread? = gatt?.let { StreamThread(it) }
    private var syncHandler: Handler? = Handler(Looper.getMainLooper())
    private var otaHandler: Handler? = Handler(Looper.getMainLooper())
    private lateinit var adapter: ExpandableListAdapter
    private var executorService: ScheduledExecutorService = Executors.newSingleThreadScheduledExecutor()
    private var syncTime: Int = 0
    private var isCleanedUp = false
    private var needRestart = false
    private val disconnectHandler = Handler(Looper.getMainLooper())
    private val disconnectCheckRunnable = object : Runnable {
        override fun run() {
            if (isCleanedUp) return
            if (!connectState && mDeviceData.name != DeviceNameEnum.VALUE_NULL.ordinal) {
                FileLogger.log("Device", "disconnectCheck 触发退出: connectState=$connectState name=${mDeviceData.name} → onBackPressed")
                if (!isFinishing) {
                    Toast.makeText(this@DeviceActivity, getString(R.string.ble_disconnect_message), Toast.LENGTH_LONG).show()
                    onBackPressed()
                }
                return
            }
            disconnectHandler.postDelayed(this, 1000)
        }
    }

    // 在你的Activity或Fragment中
    private val progressDialogFragment = ProgressDialogFragment()
    private val processDialogFragment = ProcessDialogFragment()

    private lateinit var groupsList: List<Group>
    private lateinit var children : Map<Int, List<Child>>
    private lateinit var newChildren : Map<Int, List<Child>>

    private val counterRunnable  =  Runnable {
        /* 在这里定义周期性执行的任务 */
        runOnUiThread {
            if (mDeviceData.name == DeviceNameEnum.NAME_UDS100.ordinal) {
                newChildren = mapOf(
                    0 to listOf(
                        Child(0, 0, "Name", mDeviceDataString.name),
                        Child(1, 0, "Version", mDeviceDataString.version),
                        Child(2, 0, "Power", mDeviceDataString.power),
                        Child(4, 0, "Battery", mDeviceDataString.battery),
                        Child(5, 0, "Temperature", mDeviceDataString.temperature),
                        Child(6, 0, "Humidity", mDeviceDataString.humidity),
                        Child(7, 0, "Alarm Status", mDeviceDataString.htAlarm),
                        Child(8, 0, "Angle", mDeviceDataString.angle),
                        Child(9, 0, "Slant", mDeviceDataString.slant),
                        Child(10, 0, "Distance", mDeviceDataString.distance),
                        Child(11, 0, "Overflow State", mDeviceDataString.overflowState),
                        Child(12, 0, "Low Threshold", mDeviceDataString.overflowLowThreshold),
                        Child(13, 0, "High Threshold", mDeviceDataString.overflowHighThreshold),
                        Child(14, 0, "Latitude", mDeviceDataString.latitude),
                        Child(15, 0, "Longitude", mDeviceDataString.longitude),
                        Child(16, 0, "Report Period", mDeviceDataString.reportPeriod),
                        Child(17, 0, "Gps Period", mDeviceDataString.gpsPeriod)
                    ),
                )
            } else if (mDeviceData.name == DeviceNameEnum.NAME_DC200.ordinal) {
                newChildren = mapOf(
                    0 to listOf(
                        Child(0, 0, "Name", mDeviceDataString.name),
                        Child(1, 0, "Version", mDeviceDataString.version),
                        Child(2, 0, "Power", mDeviceDataString.power),
                        Child(3, 0, "Battery", mDeviceDataString.battery),
                        Child(4, 0, "Parking Status", mDeviceDataString.parkStatus),
                        Child(5, 0, "Tamper Status", mDeviceDataString.tamperAlarm),
                        Child(6, 0, "Report Period", mDeviceDataString.reportPeriod),
                        Child(7, 0, "Mag X", mDeviceDataString.magX),
                        Child(8, 0, "Mag Y", mDeviceDataString.magY),
                        Child(9, 0, "Mag Z", mDeviceDataString.magZ),
                        Child(10, 0, "Radar Spectrum", mDeviceDataString.radarSpectrum)
                    )
                )
            } else if (mDeviceData.name == DeviceNameEnum.NAME_SVC100.ordinal) {
                newChildren = mapOf(
                    0 to listOf(
                        Child(0, 0, "Name", mDeviceDataString.name),
                        Child(1, 0, "Version", mDeviceDataString.version),
                        Child(2, 0, "Power", mDeviceDataString.power),
                        Child(3, 0, "Battery", mDeviceDataString.battery),
                        Child(4, 0, "Port1 value state", mDeviceDataString.value1State),
                        Child(5, 0, "Port1 insert state", mDeviceDataString.insert1Connected),
                        Child(6, 0, "Port1 pulse count", mDeviceDataString.pulse1Count),
                        Child(7, 0, "Port2 value state", mDeviceDataString.value2State),
                        Child(8, 0, "Port2 insert state", mDeviceDataString.insert2Connected),
                        Child(9, 0, "Port2 pulse count", mDeviceDataString.pulse2Count),
                        Child(10, 0, "Voltage out level", mDeviceDataString.volOut),
                        Child(11, 0, "Interface function", mDeviceDataString.valveMode),
                        Child(12, 0, "Buffeting duration", mDeviceDataString.buffetingDuration),
                        Child(13, 0, "Timezone", mDeviceDataString.timeZone),
                        Child(14, 0, "Report Period", mDeviceDataString.reportPeriod)
                    )
                )
            } else if (mDeviceData.name == DeviceNameEnum.VALUE_NULL.ordinal) {
                children = mapOf(
                    0 to listOf(
                        Child(0, 0, "Name", mDeviceDataString.name),
                    )
                )
            }
            else {
                // MPS100 等其他未明确处理类型：默认 children，避免 newChildren lateinit 崩溃
                newChildren = mapOf(
                    0 to listOf(
                        Child(0, 0, "Name", mDeviceDataString.name),
                        Child(1, 0, "Version", mDeviceDataString.version),
                        Child(2, 0, "Power", mDeviceDataString.power),
                        Child(3, 0, "Battery", mDeviceDataString.battery),
                        Child(4, 0, "Report Period", mDeviceDataString.reportPeriod)
                    )
                )
            }

            if (mDeviceData.name != DeviceNameEnum.VALUE_NULL.ordinal) {
                adapter.updateChildren(newChildren)
            }

            if (mDeviceEvent.event == DeviceEventEnum.SELECT_FILE.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                launchFileSelector()
            } else if (mDeviceEvent.event == DeviceEventEnum.START_OTA.ordinal) {
                if (fileBin.isEmpty()) {
                    Toast.makeText(this, "Please Choose OTA File", Toast.LENGTH_SHORT).show()
                    mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                } else {
                    mDeviceEvent.event = DeviceEventEnum.ENTER_OTA.ordinal
                    progressDialogFragment.show(supportFragmentManager, "OTA")
                    startOTA()
                }
            } else if (mDeviceEvent.event == DeviceEventEnum.POWER_ON_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.POWER_ON_START_EVENT.ordinal
                processDialogFragment.show(supportFragmentManager, "")
                startSyncCheck()
            } else if (mDeviceEvent.event == DeviceEventEnum.POWER_ON_FINISH_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                Toast.makeText(context, "Executed successfully", Toast.LENGTH_SHORT).show()
            } else if (mDeviceEvent.event == DeviceEventEnum.POWER_OFF_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.POWER_OFF_START_EVENT.ordinal
                processDialogFragment.show(supportFragmentManager, "")
                startSyncCheck()
            } else if (mDeviceEvent.event == DeviceEventEnum.POWER_OFF_FINISH_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                Toast.makeText(context, "Executed successfully", Toast.LENGTH_SHORT).show()
            } else if (mDeviceEvent.event == DeviceEventEnum.CALIBRATION_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.CALIBRATION_START_EVENT.ordinal
                processDialogFragment.show(supportFragmentManager, "")
                startSyncCheck()
            } else if (mDeviceEvent.event == DeviceEventEnum.CALIBRATION_FINISH_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                Toast.makeText(context, "Executed successfully", Toast.LENGTH_SHORT).show()
            } else if (mDeviceEvent.event == DeviceEventEnum.CONFIG_PARAMETER_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.CONFIG_PARAMETER_START_EVENT.ordinal
                processDialogFragment.show(supportFragmentManager, "")
                startSyncCheck()
            } else if (mDeviceEvent.event == DeviceEventEnum.CONFIG_PARAMETER_FINISH_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                adapter.requestConfigValueRefresh()
                Toast.makeText(context, "Executed successfully", Toast.LENGTH_SHORT).show()
            } else if (mDeviceEvent.event == DeviceEventEnum.SYNC_TIMESTAMP_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.SYNC_TIMESTAMP_START_EVENT.ordinal
                processDialogFragment.show(supportFragmentManager, "")
                startSyncCheck()
            } else if (mDeviceEvent.event == DeviceEventEnum.SYNC_TIMESTAMP_FINISH_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                Toast.makeText(context, "Executed successfully", Toast.LENGTH_SHORT).show()
            } else if (mDeviceEvent.event == DeviceEventEnum.CONFIG_TIMED_TASK.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.CONFIG_TIMED_TASK_START_EVENT.ordinal
                processDialogFragment.show(supportFragmentManager, "")
                startSyncCheck()
            } else if (mDeviceEvent.event == DeviceEventEnum.CONFIG_TIMED_TASK_FINISH_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                Toast.makeText(context, "Executed successfully", Toast.LENGTH_SHORT).show()
            } else if (mDeviceEvent.event == DeviceEventEnum.DELETE_TIMED_TASK.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.DELETE_TIMED_TASK_START_EVENT.ordinal
                processDialogFragment.show(supportFragmentManager, "")
                startSyncCheck()
            } else if (mDeviceEvent.event == DeviceEventEnum.DELETE_TIMED_TASK_FINISH_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                Toast.makeText(context, "Executed successfully", Toast.LENGTH_SHORT).show()
            } else if (mDeviceEvent.event == DeviceEventEnum.CONFIG_REALTIME_TASK.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.CONFIG_REALTIME_TASK_START_EVENT.ordinal
                processDialogFragment.show(supportFragmentManager, "")
                startSyncCheck()
            } else if (mDeviceEvent.event == DeviceEventEnum.CONFIG_REALTIME_TASK_FINISH_EVENT.ordinal) {
                mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                Toast.makeText(context, "Executed successfully", Toast.LENGTH_SHORT).show()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_device)
        FileLogger.log("Device", "DeviceActivity.onCreate name=${mDeviceData.name} connectState=$connectState")

        if(mDeviceData.name == DeviceNameEnum.NAME_UDS100.ordinal){
            groupsList = listOf(
                Group(0, "Status",0),
                Group(1, "Config",1),
                Group(2, "Config",2),
                Group(3, "Config",3),
                Group(4, "Config",4),
                Group(5, "Config",5),
                // ...更多组
            )
            children = mapOf(
                0 to listOf(
                    Child(0, 0, "Name", mDeviceDataString.name),
                    Child(1, 0, "Version", mDeviceDataString.version),
                    Child(2, 0, "Power", mDeviceDataString.power),
                    Child(4, 0, "Battery", mDeviceDataString.battery),
                    Child(5, 0, "Temperature", mDeviceDataString.temperature),
                    Child(6, 0, "Humidity", mDeviceDataString.humidity),
                    Child(7, 0, "Alarm Status", mDeviceDataString.htAlarm),
                    Child(8, 0, "Angle", mDeviceDataString.angle),
                    Child(9, 0, "Slant", mDeviceDataString.slant),
                    Child(10, 0, "Distance", mDeviceDataString.distance),
                    Child(11, 0, "Overflow State", mDeviceDataString.overflowState),
                    Child(12, 0, "Low Threshold", mDeviceDataString.overflowLowThreshold),
                    Child(13, 0, "High Threshold", mDeviceDataString.overflowHighThreshold),
                    Child(14, 0, "Latitude", mDeviceDataString.latitude),
                    Child(15, 0, "Longitude", mDeviceDataString.longitude),
                    Child(16, 0, "Report Period", mDeviceDataString.reportPeriod),
                    Child(17, 0, "Gps Period", mDeviceDataString.gpsPeriod)),
            )
        }

        else if(mDeviceData.name == DeviceNameEnum.NAME_DC200.ordinal){
            groupsList = listOf(
                Group(0, "Status",0),
                Group(1, "Config",1),
                Group(2, "Config",2),
                Group(3, "Config",3),
                Group(4, "Config",4),
                Group(5, "Config",5),
                // ...更多组
            )
            children = mapOf(
                0 to listOf(
                    Child(0, 0, "Name", mDeviceDataString.name),
                    Child(1, 0, "Version", mDeviceDataString.version),
                    Child(2, 0, "Power", mDeviceDataString.power),
                    Child(3, 0, "Battery", mDeviceDataString.battery),
                    Child(4, 0, "Parking Status", mDeviceDataString.parkStatus),
                    Child(5, 0, "Tamper Status", mDeviceDataString.tamperAlarm),
                    Child(6, 0, "Report Period", mDeviceDataString.reportPeriod),
                    Child(7, 0, "Mag X", mDeviceDataString.magX),
                    Child(8, 0, "Mag Y", mDeviceDataString.magY),
                    Child(9, 0, "Mag Z", mDeviceDataString.magZ),
                    Child(10, 0, "Radar Spectrum", mDeviceDataString.radarSpectrum))
            )
        }

        else if(mDeviceData.name == DeviceNameEnum.NAME_SVC100.ordinal){
            groupsList = listOf(
                Group(0, "Status",0),
                Group(1, "Config",1),
                Group(2, "Config",2),
                Group(3, "Config",3),
                Group(4, "Config",4),
                Group(5, "Config",5),
                Group(6, "Config",6),
            )
            children = mapOf(
                0 to listOf(
                    Child(0, 0, "Name", mDeviceDataString.name),
                    Child(1, 0, "Version", mDeviceDataString.version),
                    Child(2, 0, "Power", mDeviceDataString.power),
                    Child(3, 0, "Battery", mDeviceDataString.battery),
                    Child(4, 0, "Port1 value state", mDeviceDataString.value1State),
                    Child(5, 0, "Port1 insert state", mDeviceDataString.insert1Connected),
                    Child(6, 0, "Port1 pulse count", mDeviceDataString.pulse1Count),
                    Child(7, 0, "Port2 value state", mDeviceDataString.value2State),
                    Child(8, 0, "Port2 insert state", mDeviceDataString.insert2Connected),
                    Child(9, 0, "Port2 pulse count", mDeviceDataString.pulse2Count),
                    Child(10, 0, "Voltage out level", mDeviceDataString.volOut),
                    Child(11, 0, "Interface function", mDeviceDataString.valveMode),
                    Child(12, 0, "Buffeting duration", mDeviceDataString.buffetingDuration),
                    Child(13, 0, "Timezone", mDeviceDataString.timeZone),
                    Child(14, 0, "Report Period", mDeviceDataString.reportPeriod))
            )
        }

        else if(mDeviceData.name == DeviceNameEnum.VALUE_NULL.ordinal){
            groupsList = listOf(
                Group(0, "Config",0),
                Group(1, "Config",1),
            )
            children = mapOf(
                0 to listOf(
                    Child(0, 0, "Name", mDeviceDataString.name)
                )
            )
        }
        else {
            // MPS100 等其他未明确处理类型：使用通用结构，避免 groupsList lateinit 未初始化导致崩溃
            FileLogger.log("Device", "onCreate 默认分支(未识别类型) name=${mDeviceData.name}")
            groupsList = listOf(
                Group(0, "Status",0),
                Group(1, "Config",1),
                Group(2, "Config",2),
                Group(3, "Config",3),
                Group(4, "Config",4),
                Group(5, "Config",5),
            )
            children = mapOf(
                0 to listOf(
                    Child(0, 0, "Name", mDeviceDataString.name),
                    Child(1, 0, "Version", mDeviceDataString.version),
                    Child(2, 0, "Power", mDeviceDataString.power),
                    Child(3, 0, "Battery", mDeviceDataString.battery),
                    Child(4, 0, "Report Period", mDeviceDataString.reportPeriod)
                )
            )
        }

        val expandableListView = findViewById<ExpandableListView>(R.id.expandable_list_view)
        adapter = ExpandableListAdapter(this, groupsList, children)
        expandableListView.setAdapter(adapter)

        // 点击 Status 组时重置待配置参数为设备当前值
        expandableListView.setOnGroupClickListener { _, _, groupPosition, _ ->
            if (groupsList[groupPosition].groupType == 0) {
                resetEventFromDevice()
                adapter.requestConfigValueRefresh()
                adapter.notifyDataSetChanged()
            }
            false
        }

        initView()
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        menuInflater.inflate(R.menu.debug_menu, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.item_debug -> {
                val intent = Intent(this@DeviceActivity, DebugActivity::class.java)
                startActivity(intent)
                true
            }
            else -> false // 如果有其他菜单项需要处理，可以在这里添加逻辑；否则，默认返回 false
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        // 文件读取权限
        if (requestCode == 1 && Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                Toast.makeText(this, "Failed to obtain storage permission", Toast.LENGTH_SHORT).show()
            }
        }

        if (requestCode == 0xFF && resultCode == ComponentActivity.RESULT_OK) {
            data?.data?.let { selectedFileUri ->
                // Read through the transient SAF grant returned by the picker.
//                contentResolver.takePersistableUriPermission(selectedFileUri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
                try {
                    // 持久化授予读取 URI 的权限
                    contentResolver.takePersistableUriPermission(
                        selectedFileUri,
                        Intent.FLAG_GRANT_READ_URI_PERMISSION
                    )
                    // 现在您的应用可以在设备重启后仍然访问这个 Uri
                } catch (e: SecurityException) {
                    // 处理权限被拒绝的情况
                    // 例如，可以提示用户权限被拒绝，或者需要用户手动授予权限
                }
                val uri =
                    selectedFileUri.toString().replace("%3A", ":").replace("%2F", "/")  //过滤URL 包含中文
                        .replace("%3F", "?").replace("%3D", "=").replace("%26", "&")
//                val selectedFile: InputStream? = contentResolver.openInputStream(uri.toUri())
//                val selectedFile = data?.data //The uri with the location of the file

                val fileData = FileUtil.readBinFile(this, selectedFileUri)
                if (fileData.isNullOrEmpty()) {
                    fileBin.setLength(0)
                    otaFileName = null
                    adapter.notifyDataSetChanged()
                    FileLogger.log("OTA", "readFileFailed uri=$selectedFileUri")
                    Toast.makeText(this, getString(R.string.ota_file_read_fail), Toast.LENGTH_SHORT).show()
                }else {
                    fileBin.setLength(0)
                    fileBin.append(fileData)
                    otaFileName = FileUtil.getDisplayName(this, selectedFileUri)
                        ?: getString(R.string.ota_unknown_file_name)
                    adapter.notifyDataSetChanged()
                    FileLogger.log("OTA", "selectedFile uri=$selectedFileUri bytes=${fileBin.length / 2}")
                    Toast.makeText(
                        this,
                        getString(R.string.ota_file_size, fileBin.length / 2),
                        Toast.LENGTH_SHORT
                    ).show()
                }
            }
        }
    }

    // 选择文件
    private fun launchFileSelector() {
//        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
////        intent.type = "*/*"
//        intent.type = "application/octet-stream"
//        intent.addCategory(Intent.CATEGORY_OPENABLE)
//        if (intent.resolveActivity(packageManager) != null) {
//            // 启动 Intent，等待结果
//            startActivityForResult(intent, 0xFF)
//        } else {
//            // 处理没有应用可以处理 Intent 的情况
//            Toast.makeText(this, "应用权限不足", Toast.LENGTH_SHORT).show()
//        }

        val intent = Intent(Intent.ACTION_GET_CONTENT)
        intent.type = "application/octet-stream"
        intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, false);
        intent.addCategory(Intent.CATEGORY_OPENABLE)
        // 检查是否有应用可以处理这个Intent
        if (intent.resolveActivity(packageManager) != null) {
            startActivityForResult(intent, 0xFF)
        } else {
            Toast.makeText(this, "Insufficient app permissions", Toast.LENGTH_SHORT).show()
        }
    }

    /**
     * 初始化
     */
    @SuppressLint("MissingPermission")
    private fun initView() {
        stream?.start()
        executorService.scheduleAtFixedRate(counterRunnable, 0, 1, TimeUnit.SECONDS)
        disconnectHandler.post(disconnectCheckRunnable)  // 启动断连检测
        processDialogFragment.show(supportFragmentManager, "")
        if(mDeviceData.name != DeviceNameEnum.VALUE_NULL.ordinal) {
            mDeviceEvent.event = DeviceEventEnum.SYNC_EVENT.ordinal
        }
        startSyncCheck()
    }

    // 使用lambda表达式作为Runnable
    private fun startSyncCheck() {
        syncHandler?.postDelayed({
            if (mDeviceEvent.event == DeviceEventEnum.VALUE_NULL.ordinal || (mDeviceData.hardVersion > 0 && mDeviceEvent.event == DeviceEventEnum.SYNC_EVENT.ordinal)) {
                processDialogFragment.dismiss()
                syncTime = 0
                if(mDeviceEvent.event == DeviceEventEnum.SYNC_EVENT.ordinal) {
                    mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                    Toast.makeText(context, "Sync successfully", Toast.LENGTH_SHORT).show()
                    isExternalPowerUpdate = true  // 同步状态变化不触发指令
                    isPowerUpdate = true
                    resetEventFromDevice()
                    adapter.requestConfigValueRefresh()
                    adapter.notifyDataSetChanged()
                }
            } else {
                // 如果还没有连接，则再次检查（递归调用）
                var timeout = 40
                if(mDeviceEvent.event == DeviceEventEnum.CALIBRATION_EXECUTE_EVENT.ordinal) {
                    // DC200 covers EPS100/MPS100; their calibration can take up to 3 minutes.
                    timeout = if (mDeviceData.name == DeviceNameEnum.NAME_DC200.ordinal) 360 else 240
                }
                if (syncTime++ >= timeout) {
                    syncTime = 0
                    processDialogFragment.dismiss()
                    mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                    Toast.makeText(this, "timeout", Toast.LENGTH_SHORT).show()
                }
                else{
                    startSyncCheck() // 注意这里调用的是函数本身，而不是Handler的postDelayed
                }
            }
        }, 500) // 延迟1秒执行
    }

    // 使用lambda表达式作为Runnable
    private fun startOTA() {
        otaHandler?.postDelayed({
            if(otaLevel >= 100){
                Thread.sleep(3000)
                progressDialogFragment.dismiss()
                mDeviceEvent.event = DeviceEventEnum.VALUE_NULL.ordinal
                otaLevel = 0
                Toast.makeText(this, "OTA Successful", Toast.LENGTH_SHORT).show()
            }else{
                startOTA() // 注意这里调用的是函数本身，而不是Handler的postDelayed
                progressDialogFragment.updateProgress(otaLevel)
            }
        }, 1000) // 延迟1秒执行
    }

    override fun onBackPressed() {
        FileLogger.log("Device", "onBackPressed 调用 connectState=$connectState name=${mDeviceData.name}")
        needRestart = connectState  // BLE 连着主动返回需要杀进程
        cleanUp()
        super.onBackPressed()
    }

    override fun onDestroy() {
        super.onDestroy()
        cleanUp()
    }

    // 点击 Status 时重置待配置参数为设备当前值
    private fun resetEventFromDevice() {
        mDeviceEvent.parkMode = mDeviceData.parkMode
        mDeviceEvent.reportPeriod = mDeviceData.reportPeriod
        mDeviceEvent.overflowLowThreshold = mDeviceData.overflowLowThreshold
        mDeviceEvent.overflowHighThreshold = mDeviceData.overflowHighThreshold
        mDeviceEvent.gpsPeriod = mDeviceData.gpsPeriod
        mDeviceEvent.volOut = mDeviceData.volOut
        mDeviceEvent.valveMode = mDeviceData.valveMode
        mDeviceEvent.buffetingDuration = mDeviceData.buffetingDuration
        mDeviceEvent.autoPower = mDeviceData.autoPower
        mDeviceEvent.timeZone = mDeviceData.timeZone
    }

    private fun cleanUp() {
        if (isCleanedUp) return
        isCleanedUp = true
        mDeviceData.name = DeviceNameEnum.VALUE_NULL.ordinal
        executorService.shutdown()
        disconnectHandler.removeCallbacksAndMessages(null)
        syncHandler?.removeCallbacksAndMessages(null)
        otaHandler?.removeCallbacksAndMessages(null)
        stopUpdatingTimestamp()
        connectState = false
        if (!needRestart) {
            stream?.interrupt()
        }
    }
}


class ProgressDialogFragment : DialogFragment() {

    private lateinit var progressBar: ProgressBar
    private lateinit var textView: TextView

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        // 加载你的布局文件
        return inflater.inflate(R.layout.fragment_progress_dialog, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        // 设置DialogFragment为非全屏，并且不可取消（可选）
        dialog?.setCancelable(false)
        dialog?.window?.setDimAmount(0.5f) // 设置背景透明度（可选）

        progressBar = view.findViewById<ProgressBar>(R.id.tv_ota_progress_bar)
        textView = view.findViewById<TextView>(R.id.tv_ota_text_view)
        progressBar.max = 100
        textView.text = "0%"
    }

    // 假设你有一个方法来更新进度
    fun updateProgress(progress: Int) {
        // 这里通常应该通过ViewModel或其他机制来更新进度
        // 但为了简单起见，我们直接更新进度条
        progressBar.progress = progress
        textView.text = "$progress%"
    }

    override fun onStart() {
        super.onStart()
        isRefreshPaused = true
        Log.i("onStart", "isRefreshPaused is true")
    }

    override fun onStop() {
        super.onStop()
        Handler(Looper.getMainLooper()).postDelayed({
            isRefreshPaused = false
            Log.i("onStop", "isRefreshPaused is false")
        }, 3000) // 延迟5秒后恢复周期性任务
    }
}
class ProcessDialogFragment : DialogFragment() {

    private lateinit var progressBar: ProgressBar

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        // 加载你的布局文件
        return inflater.inflate(R.layout.fragment_process_dialog, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        // 设置DialogFragment为非全屏，并且不可取消（可选）
        dialog?.setCancelable(false)
        dialog?.window?.setDimAmount(0.0f) // 设置背景透明度（可选）
        val semiTransparentWhite = Color.argb(128, 255, 255, 255) // 128是alpha值，范围0-255
        view.setBackgroundColor(semiTransparentWhite)
        progressBar = view.findViewById<ProgressBar>(R.id.tv_process_progress_bar)
        progressBar.max = 100
    }

    override fun onStart() {
        super.onStart()
        isRefreshPaused = true
        Log.i("onStart", "isRefreshPaused is true")
    }

    override fun onStop() {
        super.onStop()
        Handler(Looper.getMainLooper()).postDelayed({
            isRefreshPaused = false
            Log.i("onStop", "isRefreshPaused is false")
        }, 3000) // 延迟5秒后恢复周期性任务
    }
}
class DropDownActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_dropdown)

        // 使用ArrayList来存储HashMap，但明确HashMap的键值类型
        val list: ArrayList<MutableMap<String, String>> = ArrayList()

        // 使用循环或列表来添加每个星期的日子
        val daysOfWeek = listOf("Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday")
        daysOfWeek.forEach { day ->
            addWeekToMapAndList(day, list)
        }

        //下拉项选中状态
        val selected = BooleanArray(list.size)
        //下拉框数据源
        val str = arrayOfNulls<String>(list.size)
        //下拉项ID
        val id = IntArray(list.size)
        for (i in list.indices) {
            str[i] = list[i]!!["hobbies"].toString()
            //id[i] = (int)list.get(i).get("id");
            id[i] = list[i]!!["id"].toString().toInt()
            selected[i] = false
        }
        val edit = findViewById<EditText>(R.id.et_week_repeat)
        edit.setOnClickListener { checkboxEdit(edit, selected, str, id) }
    }

    // 定义一个函数来添加键值对到HashMap并添加到列表中
    private fun addWeekToMapAndList(weekDay: String, list: MutableList<MutableMap<String, String>>) {
        val map = HashMap<String, String>()
        map["week"] = weekDay
        list.add(map)
    }

    private fun checkboxEdit(
        edit: EditText,
        selected: BooleanArray,
        str: Array<String?>,
        id: IntArray
    ) {
        val builder: AlertDialog.Builder = AlertDialog.Builder(this)
        builder.setTitle("Repeat")
        val multiChoiceClickListener =
            OnMultiChoiceClickListener { dialog, which, isChecked -> selected[which] = isChecked }
        builder.setMultiChoiceItems(str, selected, multiChoiceClickListener)
        val clickListener =
            DialogInterface.OnClickListener { _, _ ->
                var selectStr = ""
                var ids = ""
                for (i in selected.indices) {
                    if (selected[i]) {
                        if (TextUtils.isEmpty(selectStr)) {
                            selectStr += str[i]
                            ids += id[i]
                        } else {
                            selectStr = selectStr + "," + str[i]
                            ids = ids + "," + id[i]
                        }
                    }
                }
                edit.setText(selectStr)
                Toast.makeText(this, ids, Toast.LENGTH_SHORT).show()
            }
        builder.setCancelable(false)
        builder.setNegativeButton("cancel", null)
        builder.setPositiveButton("ok", clickListener)
        val dialog: AlertDialog = builder.create()
        dialog.show()
    }
}
