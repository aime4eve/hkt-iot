package com.hkt.ble.bletools

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.util.AttributeSet
import android.view.View

var animationRunning:Boolean = false

class RippleView : View {
    private var mContext: Context? = null
    // 画笔对象
    private var mPaint: Paint? = null
    // View宽
    private var mWidth = 0f

    // View高
    private var mHeight = 0f

    // 声波的圆圈集合
    private var mRipples: MutableList<RippleView.Circle>? = null

    // 圆圈扩散的速度
    private var mSpeed: Int = 0

    // 圆圈之间的密度
    private var mDensity: Int = 0

    // 圆圈的颜色
    private var mColor: Int = 0

    // 圆圈是否为填充模式
    private var mIsFill: Boolean = false

    // 圆圈是否为渐变模式
    private var mIsAlpha: Boolean = false

    constructor(context: Context?) : super(context) {
        init()
    }
    @SuppressLint("CustomViewStyleable")
    constructor(context: Context?, attrs: AttributeSet?) : super(context, attrs) {
        // 获取用户配置属性
        val tya = context?.obtainStyledAttributes(attrs, R.styleable.mRippleView)
        if (tya != null) {
            this.mColor = tya.getColor(R.styleable.mRippleView_cColor, Color.BLUE)
            this.mSpeed = tya.getInt(R.styleable.mRippleView_cSpeed, 3)
            this.mDensity = tya.getInt(R.styleable.mRippleView_cDensity, 15)
            this.mIsFill = tya.getBoolean(R.styleable.mRippleView_cIsFill, false)
            this.mIsAlpha = tya.getBoolean(R.styleable.mRippleView_cIsAlpha, true)
            tya.recycle()
        }
        init()
    }

    private fun init() {
        mContext = context

        // 设置画笔样式
        mPaint = Paint()
        mPaint!!.color = mColor
        mPaint!!.strokeWidth = DensityUtil.dip2px(context, 1).toFloat()
        if (mIsFill) {
            mPaint!!.style = Paint.Style.FILL
        } else {
            mPaint!!.style = Paint.Style.STROKE
        }
        mPaint!!.strokeCap = Paint.Cap.ROUND
        mPaint!!.isAntiAlias = true

        // 添加第一个圆圈
        mRipples = ArrayList()
        val c: RippleView.Circle = Circle(0, 255)
        (mRipples as ArrayList<RippleView.Circle>).add(c)
        mDensity = DensityUtil.dip2px(context, mDensity)

        // 设置View的圆为半透明
        setBackgroundColor(Color.TRANSPARENT)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        // 内切正方形
        drawInCircle(canvas)
    }

    /**
     * 圆到宽度
     *class RippleView
     * @param canvas
     */
    private fun drawInCircle(canvas: Canvas) {
        canvas.save()

        if(animationRunning) {
            // 处理每个圆的宽度和透明度
            for (i in mRipples!!.indices) {
                val c = mRipples!![i]
                mPaint!!.alpha = c.alpha // （透明）0~255（不透明）
                canvas.drawCircle(
                    mWidth / 2, mHeight / 2, c.width - mPaint!!.strokeWidth,
                    mPaint!!
                )

                // 当圆超出View的宽度后删除
                if (c.width > mWidth / 2) {
//                mRipples!!.removeAt(i)
                } else {
                    // 计算不透明的数值，这里有个小知识，就是如果不加上double的话，255除以一个任意比它大的数都将是0
                    if (mIsAlpha) {
                        val alpha = 255 - c.width * (255 / (mWidth.toDouble() / 2))
                        c.alpha = alpha.toInt()
                    }
                    // 修改这个值控制速度
                    c.width += mSpeed
                }
            }

            // 里面添加圆
            if (mRipples!!.size > 0) {
                // 控制第二个圆出来的间距
                if (mRipples!![mRipples!!.size - 1].width > DensityUtil.dip2px(context, mDensity)) {
                    mRipples!!.add(Circle(0, 255))
                }
            }
        }

        invalidate()
        canvas.restore()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        val myWidthSpecMode = MeasureSpec.getMode(widthMeasureSpec)
        val myWidthSpecSize = MeasureSpec.getSize(widthMeasureSpec)
        val myHeightSpecMode = MeasureSpec.getMode(heightMeasureSpec)
        val myHeightSpecSize = MeasureSpec.getSize(heightMeasureSpec)

        // 获取宽度
        mWidth = if (myWidthSpecMode == MeasureSpec.EXACTLY) {
            // match_parent
            myWidthSpecSize.toFloat()
        } else {
            // wrap_content
            DensityUtil.dip2px(context, 120).toFloat()
        }

        // 获取高度
        mHeight = if (myHeightSpecMode == MeasureSpec.EXACTLY) {
            myHeightSpecSize.toFloat()
        } else {
            // wrap_content
            DensityUtil.dip2px(context, 120).toFloat()
        }

        // 设置该view的宽高
        setMeasuredDimension(mWidth.toInt(), mHeight.toInt())
    }

    override fun setBackgroundColor(color: Int) {
        invalidate() // 重新绘制 View，使背景颜色生效
    }
    internal inner class Circle(var width: Int, var alpha: Int)
}
