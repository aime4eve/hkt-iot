package com.hkt.ble.bletools

import android.Manifest
import android.annotation.SuppressLint
import android.app.AlertDialog
import android.app.Application
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothManager
import android.bluetooth.le.BluetoothLeScanner
//import android.bluetooth.le.BluetoothLeScanner
//import android.bluetooth.le.ScanCallback
//import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import no.nordicsemi.android.support.v18.scanner.BluetoothLeScannerCompat
import no.nordicsemi.android.support.v18.scanner.ScanCallback
import no.nordicsemi.android.support.v18.scanner.ScanResult
//import no.nordicsemi.android.support.v18.scanner.ScanSettings
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
import android.util.Log
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.annotation.RequiresPermission
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
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
        fun instance(): Context? {
            return instance()
        }
        @SuppressLint("StaticFieldLeak")
        lateinit var context: Context
    }

    override fun onCreate() {
        super.onCreate()
        context = applicationContext
    }
}


class MainActivity : AppCompatActivity() , BleCallback.UiCallback {

    private lateinit var bluetoothAdapter: BluetoothAdapter
    private lateinit var bluetoothLeScanner: BluetoothLeScanner

//    private lateinit var swipeRefreshLayout: SwipeRefreshLayout
    private lateinit var adapterBluetoothList: RecyclerViewListAdapter

    // 缓存的 View 引用，避免在热路径中重复 findViewById
    private lateinit var btImage: ImageView
    private lateinit var btTip: TextView
    private lateinit var rvDeviceList: RecyclerView

    //蓝牙列表
    private var mList: MutableList<BleDevice> = ArrayList()

    //地址列表
    private var addressList: MutableList<String> = ArrayList()

    private var bleHandler: Handler? = Handler(Looper.getMainLooper())

    private var connectTime = 0

    //当前是否扫描
    private var isScanning = false

    //缓存的 SP 过滤值，在 scan() 时刷新，避免每次扫描结果都读 SP
    private var filterNullName = false
    private var filterRssi = -100

    // 扫描动画延迟 3 秒后再展示设备列表
    private var scanAnimationDelay = false

    // 想要连接的蓝牙名称
    private var bandNameDevice = ""
    private var isBand = false

    private val processDialogFragment = ProcessDialogFragment()

    companion object MainActivity{
        private lateinit var gatt: BluetoothGatt
        fun getGatt(): BluetoothGatt {
            return gatt
        }
    }

    private val scanCallback = object : android.bluetooth.le.ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: android.bluetooth.le.ScanResult) {
            super.onScanResult(callbackType, result)
            // 处理扫描结果，比如显示设备名
            val name = result.device.name ?: "N/A"
            addDeviceList(BleDevice(result.device, result.rssi, name))
//            Log.d("BluetoothScan", "Found device: ${result.device.name} (${result.device.address})")
        }

        override fun onBatchScanResults(results: List<android.bluetooth.le.ScanResult>) {
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
            .setNegativeButton(getString(R.string.deny)) { _, _ -> finish() }
            .setCancelable(false)
            .show()
    }


    @RequiresApi(Build.VERSION_CODES.S)
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
                    // 所有权限已授予
                    Toast.makeText(this, getString(R.string.all_permissions_granted), Toast.LENGTH_SHORT).show()
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
                .setNegativeButton(getString(R.string.deny)) { _, _ -> finish() }
                .show()
        } else {
            Toast.makeText(this, getString(R.string.partial_function_restricted), Toast.LENGTH_SHORT).show()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val bluetoothManager = getSystemService(BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothAdapter = bluetoothManager.adapter
        bluetoothLeScanner = bluetoothAdapter.bluetoothLeScanner!!

        if (!packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, "Not support Bluetooth Low Energy", Toast.LENGTH_SHORT).show()
            return
        }

        if (!checkAllPermissionsGranted()) {
            requestPermissions()
        }
        initView()
    }

    override fun onResume() {
        super.onResume()
        // 从 DeviceActivity 返回时刷新工具栏扫描图标状态
        invalidateOptionsMenu()
    }

    private fun isValidHex16(hexString: String): Boolean {
        return hexString.matches(Regex("""^[0-9A-Fa-f]{16}$"""))
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        val result: IntentResult =
            IntentIntegrator.parseActivityResult(requestCode, resultCode, data)
        if (result.contents == null) {
            Toast.makeText(this, "Cancelled", Toast.LENGTH_LONG).show()
        } else {
            if (isValidHex16(result.contents))
            {
                Toast.makeText(this, "Scanned: " + result.contents, Toast.LENGTH_LONG).show()

                val locale =
                    resources.configuration.locales.get(0)
                val tipText = when (locale.language) {
                    "zh" -> "请等待蓝牙扫描完成并自动连接"
                    "en" -> "Please wait for the Bluetooth scan to complete and automatically connect"
                    // 可以添加更多语言判断分支，如日语等
                    else -> "Please wait for the Bluetooth scan to complete and automatically connect" // 默认显示英文或其他合适的提示语
                }
                Toast.makeText(this, tipText, Toast.LENGTH_LONG).show()

//                    bandNameDevice = result.contents
                // 提取后6位作为bandNameDevice
                bandNameDevice = result.contents.substring(10)
                isBand = true
                // 在这里处理扫描结果，例如显示在UI上
                Log.d("bandNameDevice", bandNameDevice)
            }
        else {
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
            val tvClose = findViewById<TextView>(R.id.tv_close)

            // 设置SeekBar的初始值
//            val initialRssi = 100 // 或者从SharedPreferences等地方获取
            val initialRssi = getInt(RSSI, 100)
            sbRssi?.progress = initialRssi
            tvRssi?.text = "-$initialRssi dBm"

            // 设置SeekBar的监听器
            sbRssi?.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    tvRssi?.text = "-$progress dBm"
                }

                override fun onStartTrackingTouch(seekBar: SeekBar) {}

                override fun onStopTrackingTouch(seekBar: SeekBar) {
                    // 这里可以保存progress到SharedPreferences或其他地方
                    seekBar.progress.putInt(RSSI)
                    // 更新缓存的过滤值并重新过滤列表
                    filterRssi = -seekBar.progress
                    filterDeviceList()
                    adapterBluetoothList.updateItems(mList)
                }
            })
            tvClose?.setOnClickListener { dismiss() }
            show()
        }
    /**
     * 扫描蓝牙
     */
    @SuppressLint("MissingPermission")
    private fun scan() {

        btTip.visibility = View.GONE
        btImage.visibility = View.VISIBLE
        rvDeviceList.visibility = View.GONE

        addressList.clear()
        mList.clear()

        // 缓存 SP 过滤值，避免每个扫描结果都读 SP
        filterNullName = getBoolean("N/A")
        filterRssi = -getInt(RSSI, 100)

        isScanning = true
        animationRunning = true
        isBand = false
        scanAnimationDelay = true

        // 3 秒动画延迟后展示设备列表
        bleHandler?.postDelayed({
            scanAnimationDelay = false
            if (mList.size > 0) {
                btImage.visibility = View.GONE
                btTip.visibility = View.GONE
                rvDeviceList.visibility = View.VISIBLE
                animationRunning = false
                adapterBluetoothList.updateItems(mList)
            }
        }, 1500)


        if (bluetoothAdapter?.isEnabled == false) {
            Toast.makeText(this, "Bluetooth not open", Toast.LENGTH_SHORT).show()
            val enableIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT)
            return
        }

        // 关闭上一次连接的 GATT（此时 StreamThread 已退出，安全）
        try { getGatt().close() } catch (_: Exception) {}

        bluetoothLeScanner.let { scanner ->
            scanner.stopScan(scanCallback)
            val scanSettings = ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .setCallbackType(ScanSettings.CALLBACK_TYPE_ALL_MATCHES)
                .build()
            scanner.startScan(null, scanSettings, scanCallback)
        }
    }


    /**
     * 停止扫描
     */
    @SuppressLint("MissingPermission")
    private fun stopScan() {

        btTip.visibility = View.VISIBLE
        btImage.visibility = View.GONE
        rvDeviceList.visibility = View.GONE

        isBand = false
        animationRunning = false
        scanAnimationDelay = false
        if (isScanning) {
            isScanning = false
            bluetoothLeScanner.stopScan(scanCallback)
        }
    }

    /**
     * 添加到设备列表
     */
    @SuppressLint("MissingPermission")
    private fun addDeviceList(bleDevice: BleDevice) {
        //过滤设备名为null的设备
        if (bleDevice.device.name == null) {
            return
        }

        if (bleDevice.rssi < filterRssi) {
            return
        }

        //检查之前所添加的设备地址是否存在当前地址列表
        val address = bleDevice.device.address
        if (!addressList.contains(address)) {
            addressList.add(address)
            mList.add(bleDevice)

            // 仅在新增设备且动画延迟结束后才刷新列表
            if (!scanAnimationDelay && mList.size > 0) {
                if (mList.size == 1) {
                    btImage.visibility = View.GONE
                    btTip.visibility = View.GONE
                    rvDeviceList.visibility = View.VISIBLE
                    animationRunning = false
                }
                adapterBluetoothList.updateItems(mList)
            }
        }

        if(isBand){
            if(bleDevice.device.name.contains(bandNameDevice)){
                isBand = false
                onDeviceClicked(bleDevice.device)
                mDeviceDataString.name = bleDevice.device.name
                when {
                    mDeviceDataString.name.contains("UDS100") -> mDeviceData.name = DeviceNameEnum.NAME_UDS100.ordinal
                    mDeviceDataString.name.contains("SVC100") -> mDeviceData.name = DeviceNameEnum.NAME_SVC100.ordinal
                    mDeviceDataString.name.contains("DC200") || mDeviceDataString.name.contains("DC201") ||
                    mDeviceDataString.name.contains("EPS100") || mDeviceDataString.name.contains("MPS100") ||
                    mDeviceDataString.name.contains("E_") || mDeviceDataString.name.contains("M_") ||
                    mDeviceDataString.name.contains("PS100") -> mDeviceData.name = DeviceNameEnum.NAME_DC200.ordinal
                    else -> mDeviceData.name = DeviceNameEnum.VALUE_NULL.ordinal
                }
            }
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
                if ((filterNullName && next.device.name == null) || next.rssi < filterRssi) {
                    addressList.remove(next.device.address)
                    mIterator.remove()
                }
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


    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            R.id.item_filter -> showScanFilterDialog()
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

                scan()
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
        gatt = device.connectGatt(this@MainActivity, false, bleCallback)
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

                addressList.clear()
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


    // 处理设备点击事件的方法
    @SuppressLint("MissingPermission")
    fun onDeviceClicked(device: BluetoothDevice) {

        // 在这里处理设备点击后的逻辑，比如停止扫描、连接设备等
        if (checkUuid()) {

            isScanning = false
            stopScan()
            bluetoothLeScanner.stopScan(scanCallback)
            animationRunning = false

            btImage.visibility = View.GONE
            btTip.visibility = View.GONE

            processDialogFragment.show(supportFragmentManager, "")

            // 异步连接蓝牙设备
            connectToDevice(device)
            startConnectionCheck(device)
        }
    }

    @SuppressLint("MissingPermission")
    fun initView() {

        btImage = findViewById<ImageView>(R.id.bt_image)
        btTip = findViewById<TextView>(R.id.bt_tip)
        rvDeviceList = findViewById<RecyclerView>(R.id.rv_device_list)
//        swipeRefreshLayout = findViewById(R.id.swipe_refresh_layout)

        btImage.visibility = View.GONE
        btTip.visibility = View.VISIBLE
        rvDeviceList.visibility = View.GONE

        rvDeviceList.layoutManager = LinearLayoutManager(this)
        adapterBluetoothList = RecyclerViewListAdapter(mList)
        adapterBluetoothList.setOnItemClickListener(object : RecyclerViewListAdapter.OnItemClickListener {
            override fun onItemClicked(position: Int) {
                onDeviceClicked(mList[position].device)
                mDeviceDataString.name = mList[position].device.name
                when {
                    mDeviceDataString.name.contains("UDS100") -> mDeviceData.name = DeviceNameEnum.NAME_UDS100.ordinal
                    mDeviceDataString.name.contains("SVC100") -> mDeviceData.name = DeviceNameEnum.NAME_SVC100.ordinal
                    mDeviceDataString.name.contains("DC200") || mDeviceDataString.name.contains("DC201") ||
                    mDeviceDataString.name.contains("EPS100") || mDeviceDataString.name.contains("MPS100") ||
                    mDeviceDataString.name.contains("E_") || mDeviceDataString.name.contains("M_") ||
                    mDeviceDataString.name.contains("PS100") -> mDeviceData.name = DeviceNameEnum.NAME_DC200.ordinal
                    else -> mDeviceData.name = DeviceNameEnum.VALUE_NULL.ordinal
                }
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
        val newList = newItems.toList()  // 拷贝一份，避免与 mList 同引用导致 DiffUtil 无变化
        val diffCallback = BleDeviceDiffCallback(this.items, newList)
        val diffResult = DiffUtil.calculateDiff(diffCallback)
        this.items = newList
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