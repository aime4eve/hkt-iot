package com.hkt.ble.bletools

import android.Manifest
import android.annotation.SuppressLint
import android.app.AlertDialog
import android.app.Application
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothManager
import no.nordicsemi.android.support.v18.scanner.BluetoothLeScannerCompat
import no.nordicsemi.android.support.v18.scanner.ScanCallback
import no.nordicsemi.android.support.v18.scanner.ScanResult
import no.nordicsemi.android.support.v18.scanner.ScanSettings
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.Looper
import android.os.PowerManager
import android.provider.Settings
import android.text.InputFilter
import android.text.InputType
import android.util.Log
import java.io.File
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.widget.EditText
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.annotation.RequiresPermission
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.os.LocaleListCompat
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.bottomsheet.BottomSheetDialog
import com.google.zxing.integration.android.IntentIntegrator
import com.google.zxing.integration.android.IntentResult
import com.hkt.ble.bletools.BleUuid.BleUuid.RSSI
import java.lang.Boolean.getBoolean
import kotlin.system.exitProcess


val bleCallback = BleCallback()

class WakeLockManager(context: Context) {
    private val powerManager: PowerManager = context.getSystemService(Context.POWER_SERVICE) as PowerManager
    private var wakeLock: PowerManager.WakeLock? = null

    @RequiresPermission(android.Manifest.permission.WAKE_LOCK)
    fun acquireWakeLock(tag: String) {
        if (wakeLock == null || !wakeLock!!.isHeld) {
            wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, tag)
            wakeLock?.acquire(10*60*1000L /*10 minutes*/)
        }
    }

    fun releaseWakeLock() {
        if (wakeLock != null && wakeLock!!.isHeld) {
            wakeLock?.release()
            wakeLock = null
        }
    }
}

class MyActivity : AppCompatActivity(){

    private val wakeLockManager: WakeLockManager = WakeLockManager(this)

    override fun onResume() {
        super.onResume()
        // 在Activity可见时获取WakeLock
        wakeLockManager.acquireWakeLock("MyActivity:WakeLockTag")
    }

    override fun onPause() {
        super.onPause()
        // 在Activity不可见时释放WakeLock
        wakeLockManager.releaseWakeLock()
    }

    // ... 其他代码 ...
}


class BaseApp : Application() {

    companion object {
        @SuppressLint("StaticFieldLeak")
        lateinit var context: Context
        
        fun instance(): Context {
            return context
        }
    }

    override fun onCreate() {
        super.onCreate()
        context = applicationContext
        AppCompatDelegate.setApplicationLocales(LocaleListCompat.forLanguageTags("en-US"))
        setupCrashHandler()
    }

    /**
     * 全局未捕获异常处理器：把崩溃线程名 + 堆栈写入文件，用于在无法连接 logcat 时定位闪退根因。
     * 写入位置（按优先级逐个尝试，任一失败不影响后续）：
     *   1. 公共 Download 目录：Download/BLETools_crash/crash_latest.txt（便于直接用文件管理器查看，需"所有文件访问权限"）
     *   2. 应用私有目录兜底：/Android/data/<包名>/files/crash/crash_latest.txt（无需存储权限，保证落盘）
     */
    private fun setupCrashHandler() {
        val previousHandler = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
            val log = "Thread: ${thread.name}\n" +
                    "Time(ms): ${System.currentTimeMillis()}\n\n" +
                    Log.getStackTraceString(throwable) + "\n"
            val stamp = System.currentTimeMillis()
            // 候选目录：Download 优先，私有目录兜底
            val targetDirs = mutableListOf<File>()
            try {
                targetDirs.add(
                    File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "BLETools_crash")
                )
            } catch (e: Exception) {
            }
            try {
                getExternalFilesDir(null)?.let { targetDirs.add(File(it, "crash")) }
            } catch (e: Exception) {
            }
            for (dir in targetDirs) {
                try {
                    if (!dir.exists()) dir.mkdirs()
                    // 覆盖写一份"最新"日志，便于快速查看
                    File(dir, "crash_latest.txt").writeText(log)
                    // 另存一份带时间戳的归档
                    File(dir, "crash_$stamp.txt").writeText(log)
                } catch (e: Exception) {
                    // 某个目录不可写（如未授权存储权限），忽略并继续尝试下一个目录
                }
            }
            // 交给默认处理器，保留系统默认退出行为（仍会闪退，但日志已落盘供定位）
            previousHandler?.uncaughtException(thread, throwable)
        }
    }
}


class MainActivity : AppCompatActivity() , BleCallback.UiCallback {
    
    /**
     * 根据设备名称解析设备类型
     */
    private fun parseDeviceType(deviceName: String): Int {
        val result = when {
            deviceName.contains("UDS100") -> DeviceNameEnum.NAME_UDS100.ordinal
            deviceName.contains("SVC100") -> DeviceNameEnum.NAME_SVC100.ordinal
            // 地磁传感器：DC200 / EPS100 / MPS100 为同一系列，统一按地磁（DC200）处理
            deviceName.contains("DC200") ||
            deviceName.contains("EPS100") ||
            deviceName.contains("MPS100") -> DeviceNameEnum.NAME_DC200.ordinal
            else -> DeviceNameEnum.VALUE_NULL.ordinal
        }
        FileLogger.log("Scan", "parseDeviceType('$deviceName')=$result")
        return result
    }

    // 视图组件引用
    private lateinit var btImage: ImageView
    private lateinit var btTip: TextView
    private lateinit var rvDeviceList: RecyclerView
    
    private var bluetoothAdapter: BluetoothAdapter? = null
    private var bluetoothLeScanner: BluetoothLeScannerCompat? = null

//    private lateinit var swipeRefreshLayout: SwipeRefreshLayout
    private lateinit var adapterBluetoothList: RecyclerViewListAdapter

    //蓝牙列表
    private var mList: MutableList<BleDevice> = ArrayList()

    private var addressSet: HashSet<String> = HashSet()
    private var deviceIndexMap: HashMap<String, Int> = HashMap()

    private var bleHandler: Handler? = Handler(Looper.getMainLooper())

    private var connectTime = 0

    //当前是否扫描
    private var isScanning = false

    // 当前扫描轮次（0-2）
    private var scanCycleCount = 0

    //当前扫描设备是否过滤设备名称为Null的设备
    private var isScanNullNameDevice = true

    private var cachedRssiThreshold = -100

    // 想要连接的蓝牙名称
    private var bandNameDevice = ""
    private var isBand = false

    private var pendingUiUpdate = false

    private val uiUpdateRunnable = Runnable {
        if (mList.size > 0) {
            btImage.visibility = View.GONE
            btTip.visibility = View.GONE
            rvDeviceList.visibility = View.VISIBLE
            animationRunning = false
        } else {
            btImage.visibility = View.VISIBLE
            rvDeviceList.visibility = View.GONE
        }
        adapterBluetoothList.updateItems(mList.map { it.copy() })
        pendingUiUpdate = false
    }

    private val processDialogFragment = ProcessDialogFragment()

    companion object MainActivity{
        private var gatt: BluetoothGatt? = null
        const val DEFAULT_DEV_EUI_PREFIX = "0095690"

        fun getGatt(): BluetoothGatt? {
            return gatt
        }
        fun setGatt(value: BluetoothGatt) {
            gatt = value
        }

        /**
         * Determine whether to filter the device based on name and settings
         * @param isScanNullNameDevice Whether to filter null name devices
         * @param deviceName The name of the device
         * @return true if the device should be filtered (skipped), false otherwise
         */
        fun shouldFilterDevice(isScanNullNameDevice: Boolean, deviceName: String?): Boolean {
            return isScanNullNameDevice && deviceName == null
        }

        fun isValidDevEui(devEui: String): Boolean {
            return devEui.matches(Regex("""^[0-9A-Fa-f]{16}$"""))
        }

        fun getDevEuiMatchSuffix(devEui: String): String? {
            return if (isValidDevEui(devEui)) devEui.takeLast(6).uppercase() else null
        }

        fun isTargetDevice(deviceName: String?, matchSuffix: String?): Boolean {
            return matchSuffix != null && deviceName?.contains(matchSuffix, ignoreCase = true) == true
        }
    }

    private val scanCallback = object : ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            super.onScanResult(callbackType, result)
            // 处理扫描结果，比如显示设备名
            val name = result.device.name ?: "N/A"
            addDeviceList(BleDevice(result.device, result.rssi, name))
            Log.d("BluetoothScan", "Found device: ${result.device.name} (${result.device.address})")
        }

        override fun onBatchScanResults(results: List<ScanResult>) {
            super.onBatchScanResults(results)
            // 处理批量扫描结果（如果需要）
        }

        override fun onScanFailed(errorCode: Int) {
            super.onScanFailed(errorCode)
            Log.e("BluetoothScan", "Scan failed with error code: $errorCode")
            Toast.makeText(this@MainActivity, "Scan failed with error code: $errorCode", Toast.LENGTH_SHORT).show()
        }
    }


    private val REQUEST_CODE_BLUETOOTH_PERMISSIONS = 1001
    private val REQUEST_CODE_MANAGE_STORAGE = 1002
    private val REQUEST_ENABLE_BT = 10

    // 基础权限列表（适配不同Android版本）
    private val REQUIRED_PERMISSIONS = mutableListOf(
        Manifest.permission.ACCESS_FINE_LOCATION,
        Manifest.permission.ACCESS_COARSE_LOCATION,
        Manifest.permission.BLUETOOTH,
        Manifest.permission.BLUETOOTH_ADMIN,
        Manifest.permission.WAKE_LOCK,
    ).apply {
        // 动态添加Android 12+的权限
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            addAll(listOf(
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.BLUETOOTH_SCAN,
            ))
        }
    }.toTypedArray()

    private fun checkAllPermissionsGranted(): Boolean {
        return REQUIRED_PERMISSIONS.all { permission ->
            ContextCompat.checkSelfPermission(this, permission) == PackageManager.PERMISSION_GRANTED
        }
    }

    private fun requestPermissions() {
        // 处理 Android 11+ 的 MANAGE_EXTERNAL_STORAGE
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R &&
            !Environment.isExternalStorageManager()) {
            showManageStorageDialog()
        }
        // 请求常规权限
        ActivityCompat.requestPermissions(
            this,
            REQUIRED_PERMISSIONS,
            REQUEST_CODE_BLUETOOTH_PERMISSIONS,
        )
    }

    @RequiresApi(Build.VERSION_CODES.R)
    private fun showManageStorageDialog() {
        AlertDialog.Builder(this)
            .setTitle(getString(R.string.storage_permission_title))
            .setMessage(getString(R.string.storage_permission_message))
            .setPositiveButton(getString(R.string.go_to_settings)) { _, _ ->
                startActivity(Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION))
            }
            .setNegativeButton(getString(R.string.deny)) { _, _ -> 
                // 不退出应用，只显示提示
                Toast.makeText(this, "部分功能可能受限", Toast.LENGTH_SHORT).show()
                // 继续初始化界面，即使存储权限被拒绝
                initView()
            }
            .setCancelable(false)
            .show()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        when (requestCode) {
            REQUEST_CODE_BLUETOOTH_PERMISSIONS -> {
                val deniedPermissions = permissions.zip(grantResults.toList())
                    .filter { it.second != PackageManager.PERMISSION_GRANTED }
                    .map { it.first }
                if (deniedPermissions.isEmpty()) {
                    // 所有权限已授予，初始化界面
                    Toast.makeText(this, getString(R.string.all_permissions_granted), Toast.LENGTH_SHORT).show()
                    initView()
                } else {
                    // 打印未通过的权限
                    val deniedPermissionsString = deniedPermissions.joinToString()
                    Log.e("PermissionDenied", getString(R.string.permission_denied_log, deniedPermissionsString))
                    handleDeniedPermissions(deniedPermissions)
                }
            }
        }
    }

    @RequiresApi(Build.VERSION_CODES.S)
    private fun handleDeniedPermissions(deniedPermissions: List<String>) {
        val criticalPermissions = listOf(
            Manifest.permission.BLUETOOTH_CONNECT,
            Manifest.permission.ACCESS_FINE_LOCATION
        )
        if (deniedPermissions.any { it in criticalPermissions }) {
            AlertDialog.Builder(this)
                .setTitle(getString(R.string.critical_permissions_denied_title))
                .setMessage(getString(R.string.critical_permissions_denied_message))
                .setPositiveButton(getString(R.string.go_to_settings)) { _, _ ->
                    startActivity(Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS).apply {
                        data = Uri.fromParts("package", packageName, null)
                    })
                }
                .setNegativeButton(getString(R.string.deny)) { _, _ -> 
                    // 不退出应用，只显示提示
                    Toast.makeText(this, "蓝牙功能可能受限", Toast.LENGTH_SHORT).show()
                    // 继续初始化界面，即使关键权限被拒绝
                    initView()
                }
                .show()
        } else {
            Toast.makeText(this, getString(R.string.partial_function_restricted), Toast.LENGTH_SHORT).show()
            // 即使部分权限被拒绝，也继续初始化界面
            initView()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val bluetoothManager = getSystemService(BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothAdapter = bluetoothManager.adapter
        
        // 检查蓝牙是否支持和已启用
        if (!packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, "Not support Bluetooth Low Energy", Toast.LENGTH_SHORT).show()
        }
        
        if (bluetoothAdapter == null) {
            // 设备不支持蓝牙
            Toast.makeText(this, "Bluetooth not supported on this device", Toast.LENGTH_SHORT).show()
        }
        
        if (!bluetoothAdapter!!.isEnabled) {
            // 如果蓝牙未启用，显示提示但不阻止应用启动
            Toast.makeText(this, "Bluetooth is not enabled. Some features may be limited.", Toast.LENGTH_LONG).show()
        }
        
        // 安全地获取 BluetoothLeScanner（如果蓝牙已启用）
        if (bluetoothAdapter != null && bluetoothAdapter!!.isEnabled) {
            val leScanner = BluetoothLeScannerCompat.getScanner()
            if (leScanner == null) {
                Toast.makeText(this, "Bluetooth LE scanner not available", Toast.LENGTH_SHORT).show()
            } else {
                bluetoothLeScanner = leScanner
                Log.d("BluetoothScan", "Scanner initialized: $bluetoothLeScanner")
            }
        }

        if (!checkAllPermissionsGranted()) {
            requestPermissions()
        } else {
            // 所有权限已授予，初始化界面
            initView()
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        
        // 处理蓝牙启用请求结果
        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == RESULT_OK) {
                // 蓝牙已成功启用，继续初始化
                try {
                    val leScanner = BluetoothLeScannerCompat.getScanner()
                    if (leScanner == null) {
                        Toast.makeText(this, "Bluetooth LE scanner not available", Toast.LENGTH_SHORT).show()
                        // 即使蓝牙扫描器不可用，也继续初始化界面
                        initView()
                        return
                    }
                    bluetoothLeScanner = leScanner
                    
                    if (!checkAllPermissionsGranted()) {
                        requestPermissions()
                    } else {
                        initView()
                    }
                } catch (e: Exception) {
                    Log.e("BluetoothError", "Failed to initialize Bluetooth after enable: ${e.message}")
                    Toast.makeText(this, "Failed to initialize Bluetooth: ${e.message}", Toast.LENGTH_SHORT).show()
                    // 即使蓝牙初始化失败，也继续显示界面
                    initView()
                }
            } else {
                // 用户拒绝启用蓝牙，显示提示但不退出应用
                Toast.makeText(this, "Bluetooth is not enabled. Some features may be limited.", Toast.LENGTH_LONG).show()
                // 继续初始化界面
                if (!checkAllPermissionsGranted()) {
                    requestPermissions()
                } else {
                    initView()
                }
            }
            return
        }

        val result: IntentResult = IntentIntegrator.parseActivityResult(requestCode, resultCode, data)
        if (result.contents == null) {
            Toast.makeText(this, "Cancelled", Toast.LENGTH_LONG).show()
        } else {
            if (isValidDevEui(result.contents)) {
                startTargetedScan(result.contents)
            } else {
                Toast.makeText(this, "Error: " + result.contents, Toast.LENGTH_LONG).show()
            }
        }
    }

    @SuppressLint("SetTextI18n")
    private fun showScanFilterDialog() =

            BottomSheetDialog(this).apply {
            setContentView(R.layout.dialog_scan_filter)

            val sbRssi = findViewById<SeekBar>(R.id.sb_rssi)
            val tvRssi = findViewById<TextView>(R.id.tv_rssi)
            val switchFilterNullNames = findViewById<android.widget.Switch>(R.id.switch_filter_null_names)
            val tvClose = findViewById<TextView>(R.id.tv_close)

            // 设置SeekBar的初始值
//            val initialRssi = 100 // 或者从SharedPreferences等地方获取
            val initialRssi = getInt(RSSI, 100)
            sbRssi?.progress = initialRssi
            tvRssi?.text = "-$initialRssi dBm"

            // 设置Switch的初始状态
            switchFilterNullNames?.isChecked = isScanNullNameDevice

            // 设置SeekBar的监听器
            sbRssi?.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    tvRssi?.text = "-$progress dBm"
                }

                override fun onStartTrackingTouch(seekBar: SeekBar) {}

                override fun onStopTrackingTouch(seekBar: SeekBar) {
                    seekBar.progress.putInt(RSSI)
                    cachedRssiThreshold = -seekBar.progress
                    filterDeviceList()
                    scheduleUiUpdate()
                }
            })

            switchFilterNullNames?.setOnCheckedChangeListener { _, isChecked ->
                isScanNullNameDevice = isChecked
                filterDeviceList()
                scheduleUiUpdate()
            }

            tvClose?.setOnClickListener { dismiss() }
            show()
        }

    private fun showManualEuiDialog() {
        val input = EditText(this).apply {
            hint = getString(R.string.manual_eui_hint)
            setSingleLine()
            inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS
            imeOptions = EditorInfo.IME_ACTION_DONE
            filters = arrayOf(
                InputFilter.LengthFilter(16),
                InputFilter { source, _, _, _, _, _ ->
                    val allowed = source.filter { char ->
                        char.isDigit() || char in 'A'..'F' || char in 'a'..'f'
                    }
                    if (allowed.length == source.length) null else allowed
                }
            )
            setText(DEFAULT_DEV_EUI_PREFIX)
            setSelection(length())
        }

        val dialog = AlertDialog.Builder(this)
            .setTitle(getString(R.string.manual_eui_title))
            .setView(input)
            .setPositiveButton(getString(R.string.connect), null)
            .setNegativeButton(getString(R.string.cancel), null)
            .create()

        input.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                confirmManualEui(input.text.toString().trim(), dialog)
                true
            } else {
                false
            }
        }

        dialog.setOnShowListener {
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener {
                confirmManualEui(input.text.toString().trim(), dialog)
            }
        }
        dialog.show()
    }

    private fun confirmManualEui(devEui: String, dialog: AlertDialog?) {
        if (!isValidDevEui(devEui)) {
            showMsg(getString(R.string.invalid_dev_eui))
            return
        }
        dialog?.dismiss()
        startTargetedScan(devEui)
    }

    private fun startTargetedScan(devEui: String) {
        val matchSuffix = getDevEuiMatchSuffix(devEui) ?: run {
            showMsg(getString(R.string.invalid_dev_eui))
            return
        }

        Toast.makeText(
            this,
            getString(R.string.target_scan_started),
            Toast.LENGTH_LONG
        ).show()
        scan(targetDeviceName = matchSuffix)
    }
    /**
     * 扫描蓝牙
     * @param isNewSession 是否是新的一轮扫描会话（如果是，则清空列表并重置计数）
     */
    @SuppressLint("MissingPermission")
    private fun scan(isNewSession: Boolean = true, targetDeviceName: String? = null) {

        val btImage = findViewById<ImageView>(R.id.bt_image)
        val btTip = findViewById<TextView>(R.id.bt_tip)
        val rvDeviceList = findViewById<RecyclerView>(R.id.rv_device_list)
        btTip.visibility  = View.GONE
        btImage.visibility  = View.VISIBLE
        rvDeviceList.visibility  = View.GONE

        if (isNewSession) {
            addressSet.clear()
            deviceIndexMap.clear()
            mList.clear()
            adapterBluetoothList.updateItems(ArrayList())
            scanCycleCount = 0
            cachedRssiThreshold = -getInt(RSSI, 100)
        }

        isScanning = true
        animationRunning = true
        isBand = targetDeviceName != null
        bandNameDevice = targetDeviceName.orEmpty()


        if (bluetoothAdapter?.isEnabled == false) {
            Toast.makeText(this, "Bluetooth not open", Toast.LENGTH_SHORT).show()
            val enableIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT)
            return
        }

        bluetoothLeScanner?.let { scanner ->
            scanner.stopScan(scanCallback)
            val scanSettings = ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .setCallbackType(ScanSettings.CALLBACK_TYPE_ALL_MATCHES)
                .setReportDelay(0) // Set to 0 for immediate results
                .build()
            scanner.startScan(null, scanSettings, scanCallback)

            // Auto stop scan after 10 seconds
            bleHandler?.postDelayed({
                if (isScanning) {
                    onScanCycleComplete()
                }
            }, 10000)
        } ?: run {
            Toast.makeText(this, "Bluetooth scanner not available", Toast.LENGTH_SHORT).show()
            isScanning = false
            animationRunning = false
        }
    }

    /**
     * 单次扫描周期完成
     */
    @SuppressLint("MissingPermission")
    private fun onScanCycleComplete() {
        // 停止当前扫描
        bluetoothLeScanner?.stopScan(scanCallback)
        
        scanCycleCount++
        Log.d("BluetoothScan", "Scan cycle $scanCycleCount complete")
        
        if (scanCycleCount < 3) {
            // 如果还未满3次，稍作延迟后继续下一次扫描（不清除列表）
            bleHandler?.postDelayed({
                if (isScanning) { // 确保用户没有手动停止
                    scan(
                        isNewSession = false,
                        targetDeviceName = if (isBand) bandNameDevice else null
                    )
                }
            }, 500)
        } else {
            // 已满3次，彻底停止
            val wasTargetedScan = isBand
            stopScan()
            val messageId = if (wasTargetedScan) R.string.target_device_not_found else R.string.scan_complete
            showMsg(getString(messageId))
        }
    }


    /**
     * 停止扫描
     */
    @SuppressLint("MissingPermission")
    private fun stopScan() {
        bleHandler?.removeCallbacksAndMessages(null)

        if (pendingUiUpdate) {
            pendingUiUpdate = false
            adapterBluetoothList.updateItems(mList.map { it.copy() })
        }

        scanCycleCount = 0

        btTip.visibility = View.VISIBLE
        btImage.visibility = View.GONE
        rvDeviceList.visibility = View.GONE

        isBand = false
        animationRunning = false
        if (isScanning) {
            isScanning = false
            bluetoothLeScanner?.stopScan(scanCallback)
        }
    }

    /**
     * 添加到设备列表
     */
    @SuppressLint("MissingPermission")
    private fun addDeviceList(bleDevice: BleDevice) {
        if (isBand) {
            if (!isTargetDevice(bleDevice.device.name, bandNameDevice)) {
                return
            }

            isBand = false
            runOnUiThread {
                mDeviceDataString.name = bleDevice.device.name
                mDeviceData.name = parseDeviceType(mDeviceDataString.name)
                onDeviceClicked(bleDevice.device)
            }
            return
        }

        if (MainActivity.shouldFilterDevice(isScanNullNameDevice, bleDevice.device.name)) {
            return
        }

        if (bleDevice.rssi < cachedRssiThreshold) {
            return
        }

        val address = bleDevice.device.address
        if (addressSet.contains(address)) {
            deviceIndexMap[address]?.let { index ->
                mList[index].rssi = bleDevice.rssi
                if (mList[index].name == "N/A" && bleDevice.device.name != null) {
                    mList[index].name = bleDevice.device.name
                }
            }
            scheduleUiUpdate()
        } else {
            addressSet.add(address)
            val index = mList.size
            mList.add(bleDevice)
            deviceIndexMap[address] = index

            scheduleUiUpdate()
        }
    }

    private fun scheduleUiUpdate() {
        if (!pendingUiUpdate) {
            pendingUiUpdate = true
            bleHandler?.postDelayed(uiUpdateRunnable, 300)
        }
    }

    /**
     * 过滤设备列表
     */
    @SuppressLint("MissingPermission")
    private fun filterDeviceList() {
        if (mList.size > 0) {
            val mIterator = mList.iterator()
            while (mIterator.hasNext()) {
                val next = mIterator.next()
                if (MainActivity.shouldFilterDevice(isScanNullNameDevice, next.device.name) || next.rssi < cachedRssiThreshold) {
                    addressSet.remove(next.device.address)
                    mIterator.remove()
                }
            }
            deviceIndexMap.clear()
            mList.forEachIndexed { index, bleDevice ->
                deviceIndexMap[bleDevice.device.address] = index
            }
        }
    }

    /**
     * 检查UUID
     */
    private fun checkUuid(): Boolean {

        val serviceUuid = BleUuid.SERVICE_UUID
        val descriptorUuid = BleUuid.DESCRIPTOR_UUID
        val writeUuid = BleUuid.CHARACTERISTIC_WRITE_UUID
        val indicateUuid = BleUuid.CHARACTERISTIC_INDICATE_UUID

        if (serviceUuid.isEmpty()) {
            showMsg("Please Input Service UUID")
            return false
        }
        if (serviceUuid.length < 32) {
            showMsg("Please Input Correct Service UUID")
            return false
        }
        if (descriptorUuid.isEmpty()) {
            showMsg("Please Input Descriptor UUID")
            return false
        }
        if (descriptorUuid.length < 32) {
            showMsg("Please Input Correct Descriptor UUID")
            return false
        }
        if (writeUuid.isEmpty()) {
            showMsg("Please Input Characteristic Write UUID")
            return false
        }
        if (writeUuid.length < 32) {
            showMsg("Please Input Correct Characteristic Write UUID")
            return false
        }
        if (indicateUuid.isEmpty()) {
            showMsg("Please Input Characteristic Indicate UUID")
            return false
        }
        if (indicateUuid.length < 32) {
            showMsg("Please Input Correct Characteristic Indicate UUID")
            return false
        }
        return true
    }

    /**
     * Toast提示
     */
    private fun showMsg(msg: String) =
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()

    /**
     * About：显示版本号 + 诊断日志（最多 256K）
     */
    private fun showAbout() {
        @Suppress("DEPRECATION")
        val pi = try { packageManager.getPackageInfo(packageName, 0) } catch (e: Exception) { null }
        val version = if (pi != null) "BLETools  v${pi.versionName} (${pi.versionCode})" else "BLETools  v?"
        val view = layoutInflater.inflate(R.layout.dialog_about, null)
        view.findViewById<TextView>(R.id.tv_about_version).text = version
        view.findViewById<TextView>(R.id.tv_about_log).text = FileLogger.lastFileLog()
        AlertDialog.Builder(this)
            .setTitle("About")
            .setView(view)
            .setPositiveButton("关闭", null)
            .show()
    }


    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            R.id.item_filter -> showScanFilterDialog()
            R.id.item_about -> {
                showAbout()
            }
            //扫描蓝牙
            R.id.item_bt_start -> {
                // 刷新菜单显示
                if (isScanning) {
                    stopScan()
                }
                else {
                    scan()
                }
                invalidateOptionsMenu()
            }
            R.id.item_manual_eui -> showManualEuiDialog()
            // 启动二维码扫描
            R.id.item_camera -> {

                val desiredFormats = arrayOf(
                    IntentIntegrator.QR_CODE,
                    IntentIntegrator.DATA_MATRIX,
                    IntentIntegrator.PDF_417,
                    IntentIntegrator.EAN_8
                )

                val integrator = IntentIntegrator(this)
//                integrator.setDesiredBarcodeFormats(*desiredFormats)
                integrator.setDesiredBarcodeFormats(IntentIntegrator.ALL_CODE_TYPES)
//                integrator.setPrompt("Place the QR code in the box and scan")
                integrator.setCameraId(0) // 使用默认摄像头

                integrator.setBeepEnabled(true)
                integrator.setBarcodeImageEnabled(true)
                integrator.captureActivity = (QrCodeActivity::class.java)
                integrator.initiateScan()
            }

//            else -> showMsg("Do nothing...")
        }
        return super.onOptionsItemSelected(item)
    }

    override fun onPrepareOptionsMenu(menu: Menu?): Boolean {
        // 检查是否需要更新图标
        val settingsItem = menu?.findItem(R.id.item_bt_start)
        if (settingsItem != null) {
            if (isScanning) {
                settingsItem.setIcon(R.drawable.ic_ble_stop)
            }
            else {
                settingsItem.setIcon(R.drawable.ic_ble_start)
            }
//            invalidateOptionsMenu()
        }
        return super.onPrepareOptionsMenu(menu)
    }

    @SuppressLint("MissingPermission")
    private fun connectToDevice(device: BluetoothDevice) {
        setGatt(device.connectGatt(this@MainActivity, false, bleCallback))
        bleCallback.setUiCallback(this@MainActivity)
        // 可以在这里添加连接状态的监听逻辑
    }

    private fun navigateToDeviceActivity(device: BluetoothDevice) {
        // 清除列表和隐藏视图的操作可以在这里进行，或者在新Activity启动后通过回调进行
        // ... 可能的视图操作 ...

        val intent = Intent(this@MainActivity, DeviceActivity::class.java)
//        // 可以将设备信息传递给新Activity，如果需要的话
        intent.putExtra("device_address", device.address)
        startActivity(intent)
    }

    // 使用lambda表达式作为Runnable
    private fun startConnectionCheck(device: BluetoothDevice) {

        bleHandler?.postDelayed({
            if (connectState) {
                // 连接成功后执行的代码...
                Toast.makeText(this@MainActivity, "Bluetooth connection successfully", Toast.LENGTH_SHORT).show()
                navigateToDeviceActivity(device)
                processDialogFragment.dismiss()
                btTip.visibility = View.VISIBLE
                btImage.visibility = View.GONE
                rvDeviceList.visibility = View.GONE

                addressSet.clear()
                deviceIndexMap.clear()
                mList.clear()
                // 取消所有回调（如果需要）
                bleHandler?.removeCallbacksAndMessages(null)
            } else {
                // 如果还没有连接，则再次检查（递归调用）
                if(connectTime++ >=15){
                    rvDeviceList.visibility = View.VISIBLE
                    processDialogFragment.dismiss()
                    Toast.makeText(this@MainActivity, "Bluetooth connection failed", Toast.LENGTH_SHORT).show()
                }else{
                    startConnectionCheck(device) // 注意这里调用的是函数本身，而不是Handler的postDelayed
                }
            }
        }, 1000) // 延迟1秒执行
    }

    override fun onDestroy() {
        super.onDestroy()
        bleHandler?.removeCallbacksAndMessages(null)
    }


    @SuppressLint("MissingPermission")
    fun onDeviceClicked(device: BluetoothDevice) {
        if (isScanning) {
            stopScan()
            invalidateOptionsMenu()
        }

        if (checkUuid()) {
            animationRunning = false
            btImage.visibility = View.GONE
            btTip.visibility = View.GONE

            processDialogFragment.show(supportFragmentManager, "")

            connectTime = 0
            connectToDevice(device)
            startConnectionCheck(device)
        }
    }

    @SuppressLint("MissingPermission")
    fun initView() {
        // 初始化视图组件引用
        btImage = findViewById(R.id.bt_image)
        btTip = findViewById(R.id.bt_tip)
        rvDeviceList = findViewById(R.id.rv_device_list)
//        swipeRefreshLayout = findViewById(R.id.swipe_refresh_layout)

        btImage.visibility = View.GONE
        btTip.visibility = View.VISIBLE
        rvDeviceList.visibility = View.GONE

        rvDeviceList.layoutManager = LinearLayoutManager(this)
        adapterBluetoothList = RecyclerViewListAdapter(mList)
        adapterBluetoothList.setOnItemClickListener(object : RecyclerViewListAdapter.OnItemClickListener {
            override fun onItemClicked(position: Int) {
//                Toast.makeText(this@MainActivity, "Item click: $position", Toast.LENGTH_SHORT).show()
//                Log.w("setOnItemClickListener", "recyclerView onItemClicked")
                onDeviceClicked(mList[position].device)
                mDeviceDataString.name = mList[position].device.name
                mDeviceData.name = parseDeviceType(mDeviceDataString.name)
            }
        })
        rvDeviceList.adapter = adapterBluetoothList

//        // 设置刷新时显示的动画颜色（可选）
//        swipeRefreshLayout.setColorSchemeColors(Color.BLUE, Color.GREEN, Color.RED, Color.YELLOW)
//        // 设置刷新监听器
//        swipeRefreshLayout.setOnRefreshListener {
//            // 这里执行刷新数据的逻辑，比如从网络获取新数据
//            // 使用异步操作（如Retrofit）来加载数据，并在完成后调用 swipeRefreshLayout.isRefreshing = false
//
//            invalidateOptionsMenu()
//            addressList.clear()
//            mList.clear()
//            scan()
//
//            // 模拟异步刷新
//            Handler(Looper.getMainLooper()).postDelayed({
//                // 假设这里完成了数据的加载和UI的更新
//                swipeRefreshLayout.isRefreshing = false
//            }, 1500) // 2秒后停止刷新动画
//        }
    }

    /**
     * 状态日志输出
     */
    override fun stateEvent(state: String) = runOnUiThread {
//        Toast.makeText(this, state, Toast.LENGTH_SHORT).show()
    }
}

class RecyclerViewListAdapter(mList: MutableList<BleDevice>) : RecyclerView.Adapter<RecyclerViewListAdapter.ViewHolder>() {
    // 添加一个var来存储数据列表
    private var items: List<BleDevice> = emptyList()
    private var onItemClickListener: OnItemClickListener? = null

    interface OnItemClickListener {
        fun onItemClicked(position: Int)
    }

    // 在Adapter中提供一个方法来设置监听器
    fun setOnItemClickListener(listener: OnItemClickListener?) {
        this.onItemClickListener = listener
    }


    @SuppressLint("NotifyDataSetChanged")
    fun updateItems(newItems: List<BleDevice>) {
//        Log.d("RecyclerViewListAdapter", "Updating items with size: ${newItems.size}")
        val diffCallback = BleDeviceDiffCallback(this.items, newItems)
        val diffResult = DiffUtil.calculateDiff(diffCallback)
        this.items = newItems // 在这里更新数据列表
        // 使用DiffUtil高效更新，避免使用notifyDataSetChanged
        diffResult.dispatchUpdatesTo(this)
    }

    override fun getItemCount(): Int = items.size

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val itemView = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_bluetooth, parent, false)
        return ViewHolder(itemView)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val item = items[position]
//        Log.d("RecyclerViewListAdapter", "Binding item at position: $position with name: ${item.name}")
        holder.bind(item)

        // 设置点击监听器
        holder.itemView.setOnClickListener {
            Log.d("onBindViewHolder", "setOnClickListener: $position with name: ${item.name}")
            onItemClickListener?.onItemClicked(position)
        }
    }

    // ViewHolder类
    class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val deviceTextView: TextView = itemView.findViewById(R.id.tv_device)
        private val macAddressTextView: TextView = itemView.findViewById(R.id.tv_mac_address)
        private val rssiTextView: TextView = itemView.findViewById(R.id.tv_rssi)

        @SuppressLint("SetTextI18n")
        fun bind(item: BleDevice) {
            deviceTextView.text = item.name
            macAddressTextView.text = item.device.address
            rssiTextView.text = "${item.rssi} dBm"
        }
    }
}

class BleDeviceDiffCallback(
    private val oldList: List<BleDevice>,
    private val newList: List<BleDevice>
) : DiffUtil.Callback() {

    override fun getOldListSize(): Int = oldList.size

    override fun getNewListSize(): Int = newList.size

    override fun areItemsTheSame(oldItemPosition: Int, newItemPosition: Int): Boolean {
        // 这里通常比较唯一标识符，比如ID
        return oldList[oldItemPosition].device.address == newList[newItemPosition].device.address
    }

    override fun areContentsTheSame(oldItemPosition: Int, newItemPosition: Int): Boolean {
        // 这里比较数据项的内容是否相同
        return oldList[oldItemPosition] == newList[newItemPosition]
//        return oldList[oldItemPosition].device == newList[newItemPosition].device
    }
    // 如果你需要处理更复杂的差异（比如插入、删除和移动项），
    // 你还需要实现 getChangePayload 方法，但在这里我们为了简单起见省略了。
}
