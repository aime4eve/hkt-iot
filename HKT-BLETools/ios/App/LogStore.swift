import Foundation
import Observation

/// P-07 诊断日志存储（原型 S.logs）：App 层事件（扫描启停/发现/连接/断开）追加，
/// 界面只读；环形上限防止长会话膨胀。诊断导出随诊断里程碑接入。
@MainActor
@Observable
final class LogStore {
    static let shared = LogStore()

    struct Entry: Identifiable, Equatable {
        let id = UUID()
        let timestamp: String    // HH:mm:ss
        let level: String        // INFO / WARN / ERR
        let message: String
    }

    private(set) var entries: [Entry] = []
    private let capacity = 500

    func log(_ level: String, _ message: String) {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm:ss"
        entries.append(Entry(timestamp: formatter.string(from: Date()),
                             level: level,
                             message: message))
        if entries.count > capacity {
            entries.removeFirst(entries.count - capacity)
        }
    }

    func info(_ message: String) { log("INFO", message) }
    func warn(_ message: String) { log("WARN", message) }
    func error(_ message: String) { log("ERR", message) }

    /// 导出文本（时间 级别 消息，倒序=最新在上），供失败页/日志页「导出日志」分享。
    var exportText: String {
        let header = "HKT BLETools 诊断日志 · \(entries.count) 条 · 导出于 \(Date().formatted())\n"
        return header + entries.reversed()
            .map { "\($0.timestamp) [\($0.level)] \($0.message)" }
            .joined(separator: "\n")
    }
}
