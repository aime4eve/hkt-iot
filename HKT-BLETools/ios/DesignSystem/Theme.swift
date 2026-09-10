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

    /// 卡片圆角 10 / 控件圆角 8 / 对话框圆角 18（--r-card/--r-control/--r-dialog）。
    static let cardRadius: CGFloat = 10
    static let controlRadius: CGFloat = 8
    static let dialogRadius: CGFloat = 18
}

/// 双语开关：App 不带 .lproj，Locale.current 会回退开发语言；
/// 用系统首选语言列表（原型 ZH/EN 字典的运行时对应物）。
enum AppLocale {
    static var isZh: Bool {
        Locale.preferredLanguages.first?.hasPrefix("zh") ?? false
    }
}

// MARK: - 语言模式（P-07 设置页「跟随系统/简体中文/English」，切换全 App 即时生效）

enum LanguageMode: Equatable {
    case system, zh, en
}

/// 可观察的语言存储：视图经 @Environment 持有，`mode` 变化即重渲染。
@MainActor
@Observable
final class LanguageStore {
    static let shared = LanguageStore()

    var mode: LanguageMode = .system

    var isZh: Bool {
        switch mode {
        case .system: return Locale.preferredLanguages.first?.hasPrefix("zh") ?? false
        case .zh: return true
        case .en: return false
        }
    }

    var label: String {
        switch mode {
        case .system: return isZh ? "跟随系统" : "Follow System"
        case .zh: return "简体中文"
        case .en: return "English"
        }
    }

    /// 原型 cycleLang：跟随系统 → 简体中文 → English → 跟随系统。
    func cycle() {
        mode = switch mode {
        case .system: .zh
        case .zh: .en
        case .en: .system
        }
    }
}

extension Color {
    init(light: String, dark: String) {
        self.init(uiColor: UIColor { traits in
            traits.userInterfaceStyle == .dark
                ? UIColor(hexString: dark)
                : UIColor(hexString: light)
        })
    }

    /// "#RRGGBB" 单值初始化（相机取景等固定色）。
    init(hex: String) {
        self.init(uiColor: UIColor(hexString: hex))
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
