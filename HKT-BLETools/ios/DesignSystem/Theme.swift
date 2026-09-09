import SwiftUI
import UIKit

/// R-G6 工业遥测视觉语言 Token（与已确认原型的 CSS 变量一一对应，浅/深双外观）。
enum Theme {
    static let bg = Color(light: "#EEF1F4", dark: "#000000")
    static let card = Color(light: "#FFFFFF", dark: "#16181C")
    static let card2 = Color(light: "#F5F7F9", dark: "#222529")
    static let text = Color(light: "#151A20", dark: "#F3F5F7")
    static let text2 = Color(light: "#5F6B77", dark: "#A2AAB3")
    static let line = Color(light: "#D9E0E7", dark: "#35383F")
    static let ok = Color(light: "#178A44", dark: "#33D269")
    static let warn = Color(light: "#B65F00", dark: "#FFB340")
    static let err = Color(light: "#C81E1E", dark: "#FF6B60")
    static let info = Color(light: "#0A68D6", dark: "#63AAFF")
    static let fill = Color(light: "#E8ECF0", dark: "#292C31")

    /// 卡片圆角 10 / 控件圆角 8（8pt 网格）。
    static let cardRadius: CGFloat = 10
    static let controlRadius: CGFloat = 8
}

extension Color {
    init(light: String, dark: String) {
        self.init(uiColor: UIColor { traits in
            traits.userInterfaceStyle == .dark
                ? UIColor(hexString: dark)
                : UIColor(hexString: light)
        })
    }
}

extension UIColor {
    convenience init(hexString: String) {
        var value: UInt64 = 0
        Scanner(string: hexString.trimmingCharacters(in: .alphanumerics.inverted))
            .scanHexInt64(&value)
        let r = CGFloat((value >> 16) & 0xFF) / 255
        let g = CGFloat((value >> 8) & 0xFF) / 255
        let b = CGFloat(value & 0xFF) / 255
        self.init(red: r, green: g, blue: b, alpha: 1)
    }
}
