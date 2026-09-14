import Foundation
import CoreGraphics
import ImageIO

// App 图标生成器（1024×1024 sRGB 无透明）：
//   背景 = 品牌蓝对角三段渐变（#4CA2FF → #0A68D6(Theme.info) → #06479C）+ 左上柔光；
//   主体 = 白色蓝牙符文（Material 24 格几何，圆帽圆角 74px 描边，微投影）；
//   右上 = 两道广播弧（扫描语义，白 60%/34%）。
// 坐标系注意：CGBitmapContext 是 y-up 数学坐标（原点左下），PNG 视觉是 y-down。
// 本脚本全部用「视觉坐标」书写，经 V() 转换；勿直接写裸坐标。
// 用法：swift make_appicon.swift <输出目录>

let size = 1024
let outDir = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "."

func rgb(_ hex: UInt32, _ alpha: CGFloat = 1) -> CGColor {
    CGColor(srgbRed: CGFloat((hex >> 16) & 0xFF) / 255,
            green: CGFloat((hex >> 8) & 0xFF) / 255,
            blue: CGFloat(hex & 0xFF) / 255, alpha: alpha)
}

/// 视觉坐标（y 从顶部向下）→ 位图上下文数学坐标
func V(_ x: CGFloat, _ yTop: CGFloat) -> CGPoint { CGPoint(x: x, y: CGFloat(size) - yTop) }

let space = CGColorSpace(name: CGColorSpace.sRGB)!
let context = CGContext(data: nil, width: size, height: size,
                        bitsPerComponent: 8, bytesPerRow: 0, space: space,
                        bitmapInfo: CGImageAlphaInfo.noneSkipLast.rawValue)!

// 背景：视觉左上→右下对角渐变
let bg = CGGradient(colorsSpace: space,
                    colors: [rgb(0x4CA2FF), rgb(0x0A68D6), rgb(0x06479C)] as CFArray,
                    locations: [0, 0.55, 1])!
context.drawLinearGradient(bg, start: V(0, 0), end: V(CGFloat(size), CGFloat(size)), options: [])

// 左上柔光（径向白 22% → 0）
let sheen = CGGradient(colorsSpace: space,
                       colors: [rgb(0xFFFFFF, 0.22), rgb(0xFFFFFF, 0)] as CFArray,
                       locations: [0, 1])!
context.drawRadialGradient(sheen, startCenter: V(250, 210), startRadius: 0,
                           endCenter: V(250, 210), endRadius: 640, options: [])

// 蓝牙符文：Material bluetooth 24 格几何 → 缩放 30 → 居中（视觉坐标）
let s: CGFloat = 30
func pt(_ x: CGFloat, _ y: CGFloat) -> CGPoint { V(152 + x * s, 152 + y * s) }
func stroke(_ points: [CGPoint]) {
    let path = CGMutablePath()
    path.move(to: points[0])
    for p in points.dropFirst() { path.addLine(to: p) }
    context.addPath(path)
    context.strokePath()
}
context.setStrokeColor(rgb(0xFFFFFF))
context.setLineWidth(74)
context.setLineCap(.round)
context.setLineJoin(.round)
context.setShadow(offset: CGSize(width: 0, height: -16), blur: 34,
                  color: rgb(0x032F6B, 0.30))   // 数学坐标正 dy 向上，取负 = 视觉向下投影
stroke([pt(12, 2), pt(12, 22)])                 // 竖干
stroke([pt(12, 2), pt(17.5, 6.5), pt(6.5, 16.5)])   // 上折线（过中心交叉）
stroke([pt(12, 22), pt(17.5, 17.5), pt(6.5, 7.5)])  // 下折线（过中心交叉）

// 右上广播弧：圆心=符文顶端，两道，圆帽；扫角 15°→48°（完整落在画布内，距顶边 ≥26px）
context.setShadow(offset: .zero, blur: 0, color: rgb(0, 0))
let apex = pt(12, 2)
func arc(_ radius: CGFloat, _ alpha: CGFloat) {
    context.setStrokeColor(rgb(0xFFFFFF, alpha))
    context.setLineWidth(36)
    context.setLineCap(.round)
    context.addArc(center: apex, radius: radius,
                   startAngle: .pi / 12, endAngle: .pi * 4 / 15, clockwise: false)
    context.strokePath()
}
arc(170, 0.60)
arc(250, 0.34)

let image = context.makeImage()!
let file = URL(fileURLWithPath: outDir).appendingPathComponent("icon1024.png")
let dest = CGImageDestinationCreateWithURL(file as CFURL, "public.png" as CFString, 1, nil)!
CGImageDestinationAddImage(dest, image, nil)
guard CGImageDestinationFinalize(dest) else { fatalError("PNG write failed") }
print("wrote \(file.path)")
