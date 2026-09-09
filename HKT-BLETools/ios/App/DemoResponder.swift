import CoreBLE
import CoreProtocol
import Foundation

/// 演示模式（-mockble）仿真应答器：
/// - 0xFF 轮询 → 回该家族的标准快照夹具帧（与 DeviceSnapshotTests 同源字节）；
/// - 0xFE 开关机 → 改写夹具 0x8D 电源位，下次轮询生效（关机详情页早退视图可演示）；
/// - 0x06 对时等其余命令 → 静默（与真机行为一致）。
/// 夹具为固定字节，电源记录 0x8D 均为帧内首现（测试夹具已验证 power=1 可解码）。
final class DemoResponder: @unchecked Sendable {
    private var powerOn = true

    private let frames: [DeviceFamily: Data] = [
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
        case CommandCode.power:
            powerOn = frame.count > 7 && frame[7] == 0x01
            return nil
        default:
            return nil
        }
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
