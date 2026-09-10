import CoreBLE
import CoreProtocol
import Foundation

/// 演示模式（-mockble）仿真应答器：
/// - 0xFF 轮询 → 回该家族的标准快照夹具帧（与 DeviceSnapshotTests 同源字节）；
/// - 0x02 写配置 → 回专用 ACK 帧（hkt 00 seq FF 值，固件 callback_BLEAck 同构），
///   并把载荷写进夹具（保存后下轮轮询即回新值，与真机一致）；
/// - 0xFD 校准 → 立即 ACK（完成文本 "Calibration Done" 由 App 层钩子延迟注入）；
/// - 0xFE 开关机 → 改写夹具 0x8D 电源位，下次轮询生效（关机详情页早退视图可演示）；
/// - 其余命令 → 静默（与真机行为一致）。
/// 夹具为固定字节，电源记录 0x8D 均为帧内首现（测试夹具已验证 power=1 可解码）。
final class DemoResponder: @unchecked Sendable {
    private var powerOn = true
    /// MOCK_CFG_ACK=0 时模拟固件静默拒绝（不回 ACK），用于验收配置页失败横幅。
    var configAcks = true
    /// MOCK_RT_BUSY=1 时模拟设备忙：0x03 静默忽略（无 ACK），验收任务页 busy 横幅。
    var realtimeBusy = false

    private var frames: [DeviceFamily: Data] = [
        // 夹具与 shared/fixtures/response-parse.json 同源（见 Tests/CoreProtocolTests）
        .dc200Family: DemoResponder.hex("686B740001010B1C8D0103573A013B0084008600145D00785EFF885F019060000A0014001E00280032003C00460050005A0064"),
        .uds100: DemoResponder.hex("686B74000201020A8D018B0FA0090061A80A002EE00E00144400280045003C46043847004801900BB886001E10010203041105060708"),
        .svc100: DemoResponder.hex("686B740003010D0D8D0103633C01000064010101F440024101420543018A1986001E"),
    ]

    func respond(to frame: Data, family: DeviceFamily?) -> Data? {
        guard frame.count > 6 else { return nil }
        switch frame[6] {
        case CommandCode.query:
            guard let family, var response = frames[family] else { return nil }
            if let index = response.firstIndex(of: 0x8D), response.count > index + 1 {
                response[index + 1] = powerOn ? 0x01 : 0x00
            }
            return response
        case CommandCode.config:
            guard let family else { return nil }
            applyConfig(frame, to: family)
            return configAcks ? Self.ackFrame : nil
        case CommandCode.svcRealtimeTask:
            return realtimeBusy ? nil : Self.ackFrame   // 设备忙=静默忽略，与固件一致
        case CommandCode.svcTimedTask, CommandCode.svcDeleteTask:
            return Self.ackFrame
        case CommandCode.power:
            powerOn = frame.count > 7 && frame[7] == 0x01
            return nil
        case CommandCode.calibrate:
            // 固件 0xFD：立即 ACK，设备端开始校准（完成文本由 App 层演示钩子延迟注入）
            return Self.ackFrame
        default:
            return nil
        }
    }

    /// 专用 ACK 帧（三家族同构）：hkt(3) + 0x00 + seq(1) + 0xFF 段（类型 + 1 字节值）。
    private static let ackFrame = DemoResponder.hex("686B740000FF00")

    /// 把 0x02 载荷按 TLV 偏移写进演示夹具（偏移对固定夹具字节逐位核对过）。
    private func applyConfig(_ request: Data, to family: DeviceFamily) {
        guard var response = frames[family], request.count >= 15 else { return }
        func put(_ offset: Int, _ bytes: [UInt8]) {
            for (index, byte) in bytes.enumerated() where offset + index < response.count {
                response[offset + index] = byte
            }
        }
        switch family {
        case .uds100:   // 载荷：report(2) gps(2) low(2) high(2)
            put(42, [request[7], request[8]])                                 // 0x86 上报周期
            put(29, [request[9], request[10]])                                // 0x45 GPS 周期
            put(37, [request[11], request[12], request[13], request[14]])     // 0x48 低/高阈值
        case .dc200Family:  // 载荷：report(2) mode(1)
            put(19, [request[7], request[8]])                                 // 0x86 上报周期
            put(15, [request[9]])                                             // 0x3B 工作模式
        case .svc100:   // 载荷：vol port stable autoPower tz(各1) report(2)
            put(22, [request[7]])                                             // 0x40 电压档
            put(24, [request[8]])                                             // 0x41 端口功能
            put(26, [request[9]])                                             // 0x42 稳定时长
            put(28, [request[10]])                                            // 0x43 自动开关机
            put(30, [request[11]])                                            // 0x8A 时区
            put(32, [request[12], request[13]])                               // 0x86 上报周期
        }
        frames[family] = response
    }

    private static func hex(_ hex: String) -> Data {
        var data = Data()
        var index = hex.startIndex
        while index < hex.endIndex {
            let next = hex.index(index, offsetBy: 2)
            data.append(UInt8(hex[index..<next], radix: 16)!)
            index = next
        }
        return data
    }
}
