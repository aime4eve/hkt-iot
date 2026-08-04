package com.hkt.ble.bletools

import android.bluetooth.BluetoothGatt
import android.os.Bundle
import android.text.SpannableStringBuilder
import android.view.View
import android.view.ViewTreeObserver
import android.widget.Button
import android.widget.EditText
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

var debugActivityPageRun = 0

class DebugActivity : AppCompatActivity(), BleCallback.UiCallback {

    private var gatt: BluetoothGatt? = com.hkt.ble.bletools.MainActivity.getGatt()
    //状态缓存
//    private var stringBuffer = StringBuffer()
    private var spannableStringBuilder = SpannableStringBuilder()
//    private val bleCallback = BleCallback()
    private lateinit var tvDebug: TextView
    private lateinit var tvScroll: ScrollView

    override fun onCreate(savedInstanceState: Bundle?) {

        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_debug)

        tvDebug = findViewById(R.id.tv_debug)
        tvScroll = findViewById(R.id.scroll)

        val btnSend = findViewById<Button>(R.id.bt_send_cmd)
        //发送指令
        btnSend.setOnClickListener {
            if(!connectState){
                showMsg("Bluetooth not connect")
            } else {
                var etString = findViewById<EditText>(R.id.et_command)
                var command = etString.text.toString().trim()
                if (command.isEmpty()) {
                    showMsg("Please Input Command")
                    return@setOnClickListener
                }
                gatt?.let { BleHelper.sendCommandString(it, command, false) }
            }
        }

        debugActivityPageRun = 1
        bleCallback.setUiDebugCallback(this@DebugActivity)

        // 设置滚动到底部的监听器（只添加一次）
        tvScroll.viewTreeObserver.addOnGlobalLayoutListener(object : ViewTreeObserver.OnGlobalLayoutListener {
            override fun onGlobalLayout() {
                // 确保只在布局完成后滚动一次
                tvScroll.post { tvScroll.fullScroll(View.FOCUS_DOWN) }
                // 移除监听器以避免重复添加
                tvScroll.viewTreeObserver.removeOnGlobalLayoutListener(this)
            }
        })

    }

    /**
     * Toast提示
     */
    private fun showMsg(msg: String) =
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()

    /**
     * 状态日志输出
     */
    override fun stateEvent(state: String) {
        runOnUiThread {
//            stringBuffer.append(state).append("\n")
//            tvDebug.text = stringBuffer.toString()

            // 使用 SpannableStringBuilder 追加新内容
            spannableStringBuilder.append(state).append("\n")
            // 更新 TextView 的文本
            tvDebug.text = spannableStringBuilder
            tvScroll.post { tvScroll.fullScroll(View.FOCUS_DOWN) }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        debugActivityPageRun = 0
    }
}