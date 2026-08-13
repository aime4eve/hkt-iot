package com.hkt.ble.bletools

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Rect
import android.graphics.Shader
import android.util.AttributeSet
import android.util.Log
import android.util.TypedValue
import com.journeyapps.barcodescanner.ViewfinderView
import java.io.Serializable


class ScanConfig : Serializable {
    /* 格式为#ARGB,默认为 '#4c000000' */
    var maskColor: String? = "#4c000000"

    /* 遮罩框相对比例，默认为宽度的0.68 */
    var maskRatio: Double = 0.68

    /* 返回按钮的样式, ReturnStyleExit(值为0) ReturnStyleCancel(值为1) */
    var returnStyle = 1

    /* 四个角的主色,默认为'#4bde2b' */
    var titleColor: String? = "#4bde2b"

    /* 标题文字,默认为扫一扫 */
    var title: String? = "Scan"

    /* 底部提示文字,默认为将二维码放入框内，即可自动扫描 */
    var hintString: String? = "Place the QR code in the box and scan"
}


class ScanFinderView(context: Context?, attrs: AttributeSet?) :
    ViewfinderView(context, attrs) {
    /**
     * 外部传入的配置项
     */
    private var config: ScanConfig? = null
    /* ******************************************    边角线相关属性    ************************************************/
    /**
     * "边角线长度/扫描边框长度"的占比 (比例越大，线越长)
     */
    private var mLineRate = 0.1f

    /**
     * 边角线厚度 (建议使用dp)
     */
    private var mLineDepth =
        TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 4f, resources.displayMetrics)

    /**
     * 边角线颜色
     */
    private var mLineColor = Color.parseColor("#ff4bde2b")
    /* *******************************************    扫描线相关属性    ************************************************/
    /**
     * 扫描线起始位置
     */
    private var mScanLinePosition = 0

    /**
     * 扫描线厚度
     */
    private var mScanLineDepth =
        TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 4f, resources.displayMetrics)

    /**
     * 扫描线每次重绘的移动距离
     */
    private var mScanLineDy =
        TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 3f, resources.displayMetrics)

    /**
     * 线性梯度
     */
    private var mLinearGradient: LinearGradient? = null

    /**
     * 线性梯度位置
     */
    private var mPositions = floatArrayOf(0f, 0.5f, 1f)

    /**
     * 线性梯度各个位置对应的颜色值
     */
    private var mScanLineColor = intArrayOf(0x004bde2b, Color.parseColor("#ff4bde2b"), 0x004bde2b)
    fun setConfig(config: ScanConfig) {
        this.config = config
        mLineColor = Color.parseColor(config.titleColor)
        mScanLineColor = intArrayOf(
            Color.parseColor("#00" + config.titleColor?.substring(1)),
            Color.parseColor("#ff" + config.titleColor?.substring(1)),
            Color.parseColor("#00" + config.titleColor?.substring(1))
        )
    }

    @SuppressLint("DrawAllocation")
    override fun onDraw(canvas: Canvas) {
        refreshSizes()
        val previewFramingRect: Rect? = cameraPreview.previewFramingRect
        if (framingRect == null || previewFramingRect == null) {
            Log.e("ScanFinderViewDebug", "framingRect or previewFramingRect is null")
            return
        }

        // 上移的偏移量，这里设置为30dp，你可以根据实际需求调整该数值
        val offset = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 50f, resources.displayMetrics).toInt()
        val frame = Rect(framingRect)
//        frame.top -= offset // 将顶部上移指定的偏移量
//        frame.bottom -= offset // 将顶部上移指定的偏移量

        val previewFrame: Rect = previewFramingRect
        val width = width
        val height = height

        //绘制4个角
        paint.color = mLineColor // 定义画笔的颜色

        canvas.drawRect(
            frame.left.toFloat(),
            frame.top.toFloat(),
            frame.left + frame.width() * mLineRate,
            frame.top + mLineDepth,
            paint
        )
        canvas.drawRect(
            frame.left.toFloat(),
            frame.top.toFloat(),
            frame.left + mLineDepth,
            frame.top + frame.height() * mLineRate,
            paint
        )
        canvas.drawRect(
            frame.right - frame.width() * mLineRate,
            frame.top.toFloat(),
            frame.right.toFloat(),
            frame.top + mLineDepth,
            paint
        )
        canvas.drawRect(
            frame.right - mLineDepth,
            frame.top.toFloat(),
            frame.right.toFloat(),
            frame.top + frame.height() * mLineRate,
            paint
        )
        canvas.drawRect(
            frame.left.toFloat(),
            frame.bottom - mLineDepth,
            frame.left + frame.width() * mLineRate,
            frame.bottom.toFloat(),
            paint
        )
        canvas.drawRect(
            frame.left.toFloat(),
            frame.bottom - frame.height() * mLineRate,
            frame.left + mLineDepth,
            frame.bottom.toFloat(),
            paint
        )
        canvas.drawRect(
            frame.right - frame.width() * mLineRate,
            frame.bottom - mLineDepth,
            frame.right.toFloat(),
            frame.bottom.toFloat(),
            paint
        )
        canvas.drawRect(
            frame.right - mLineDepth,
            frame.bottom - frame.height() * mLineRate,
            frame.right.toFloat(),
            frame.bottom.toFloat(),
            paint
        )

        // Draw the exterior (i.e. outside the framing rect) darkened
        paint.color = if (resultBitmap != null) resultColor else maskColor
        canvas.drawRect(0f, 0f, width.toFloat(), frame.top.toFloat(), paint)
        canvas.drawRect(
            0f,
            frame.top.toFloat(),
            frame.left.toFloat(),
            (frame.bottom + 1).toFloat(),
            paint
        )
        canvas.drawRect(
            (frame.right + 1).toFloat(),
            frame.top.toFloat(),
            width.toFloat(),
            (frame.bottom + 1).toFloat(),
            paint
        )
        canvas.drawRect(0f, (frame.bottom + 1).toFloat(), width.toFloat(), height.toFloat(), paint)
        if (resultBitmap != null) {
            // Draw the opaque result bitmap over the scanning rectangle
            paint.alpha = CURRENT_POINT_OPACITY
            canvas.drawBitmap(resultBitmap, null, frame, paint)
        } else {
            // 绘制扫描线
            mScanLinePosition += mScanLineDy.toInt()
            if (mScanLinePosition > frame.height()) {
                mScanLinePosition = 0
            }
            mLinearGradient = LinearGradient(
                frame.left.toFloat(),
                (frame.top + mScanLinePosition).toFloat(),
                frame.right.toFloat(),
                (frame.top + mScanLinePosition).toFloat(),
                mScanLineColor,
                mPositions,
                Shader.TileMode.CLAMP
            )
            paint.shader = mLinearGradient
            canvas.drawRect(
                frame.left.toFloat(),
                (frame.top + mScanLinePosition).toFloat(),
                frame.right.toFloat(),
                frame.top + mScanLinePosition + mScanLineDepth,
                paint
            )
            paint.shader = null
            val scaleX = frame.width() / previewFrame.width().toFloat()
            val scaleY = frame.height() / previewFrame.height().toFloat()
            val currentPossible = possibleResultPoints
            val currentLast = lastPossibleResultPoints
            val frameLeft = frame.left
            val frameTop = frame.top
            if (currentPossible.isEmpty()) {
                lastPossibleResultPoints = null
            } else {
                possibleResultPoints = ArrayList(5)
                lastPossibleResultPoints = currentPossible
                paint.alpha = CURRENT_POINT_OPACITY
                paint.color = resultPointColor
                for (point in currentPossible) {
                    canvas.drawCircle(
                        (frameLeft + (point.x * scaleX).toInt()).toFloat(),
                        (
                                frameTop + (point.y * scaleY).toInt()).toFloat(),
                        POINT_SIZE.toFloat(), paint
                    )
                }
            }
            if (currentLast != null) {
                paint.alpha = CURRENT_POINT_OPACITY / 2
                paint.color = resultPointColor
                val radius = POINT_SIZE / 2.0f
                for (point in currentLast) {
                    canvas.drawCircle(
                        (frameLeft + (point.x * scaleX).toInt()).toFloat(),
                        (
                                frameTop + (point.y * scaleY).toInt()).toFloat(),
                        radius, paint
                    )
                }
            }
        }

        // Request another update at the animation interval, but only repaint the laser line,
        // not the entire viewfinder mask.
        postInvalidateDelayed(
            CUSTOME_ANIMATION_DELAY,
            frame.left,
            frame.top,
            frame.right,
            frame.bottom
        )
    }

    companion object {
        /**
         * 重绘时间间隔
         */
        const val CUSTOME_ANIMATION_DELAY: Long = 16
    }
}
