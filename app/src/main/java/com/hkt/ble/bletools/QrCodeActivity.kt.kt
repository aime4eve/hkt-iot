package com.hkt.ble.bletools

import android.annotation.SuppressLint
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.KeyEvent
import android.view.View
import android.widget.ImageButton
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.journeyapps.barcodescanner.CaptureManager
import com.journeyapps.barcodescanner.DecoratedBarcodeView


class QrCodeActivity: AppCompatActivity() {
    private var decoratedBarcodeView: DecoratedBarcodeView? = null
    private var captureManager: CaptureManager? = null
    private var config: ScanConfig? = ScanConfig()
    private var scanFinderView: ScanFinderView? = null

    @SuppressLint("MissingInflatedId")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val intent = intent
//        config = intent.getSerializableExtra("ScanConfig") as ScanConfig?
        // 设置配置参数
        config?.maskColor = "#4c000000"  // 实际上这里已经是默认值，可以省略
        config?.maskRatio = 0.68  // 正确设置遮罩框比例
        config?.returnStyle = 1  // 设置返回按钮样式为取消
        config?.titleColor = "#23ADE5"  // 设置四个角的主色
        config?.title = "Scan"  // 设置标题为"Scan"
        config?.hintString = "Place the QR code in the box and scan"

        setContentView(R.layout.activity_scan)
        decoratedBarcodeView = findViewById(R.id.dbv_custom)

        var titleView = findViewById<View>(R.id.scan_view_title) as TextView
        titleView.text = config?.title
//        var statusView = findViewById<View>(R.id.zxing_status_view) as TextView
//        statusView.text = config?.hintString

        scanFinderView = decoratedBarcodeView!!.viewFinder as ScanFinderView
//        var scanFinderView = findViewById<ScanFinderView>(R.id.zxing_viewfinder_view)
        config?.let { scanFinderView!!.setConfig(it) }

        val exitButton = findViewById<ImageButton>(R.id.scan_exit_button)
//        exitButton.setImageResource(R.drawable.sd_scan_exit_icon)
        exitButton.setImageResource(R.drawable.sd_scan_cancle_icon)
        exitButton.setOnClickListener { finish() }

        //重要代码，初始化捕获
        captureManager = CaptureManager(this, decoratedBarcodeView)
        captureManager!!.initializeFromIntent(intent, savedInstanceState)
        captureManager!!.decode()
    }


    fun onTorchOn() {}
    fun onTorchOff() {}

    override fun onResume() {
        super.onResume()
        captureManager!!.onResume()
    }

    override fun onPause() {
        super.onPause()
        captureManager!!.onPause()
    }

    override fun onDestroy() {
        super.onDestroy()
        captureManager!!.onDestroy()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String?>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        captureManager!!.onRequestPermissionsResult(requestCode, permissions, grantResults)
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        return decoratedBarcodeView!!.onKeyDown(keyCode, event) || super.onKeyDown(keyCode, event)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
    }
}
