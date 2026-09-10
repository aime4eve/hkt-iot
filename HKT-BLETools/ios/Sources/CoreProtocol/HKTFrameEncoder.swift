/// App-frame TX encoding: hkt(3) packNum(1) len(2 BE) cmd(1) data(n) crc(2 over cmd+data).
/// Firmware authority: */USER/Drive/communicate.c — hkt dispatch reads data[5]=lenLow, data[6]=cmd,
/// payload from data[7]; status query / power / cal / OTA-notify all carry a 4-byte filler payload
/// (power byte at payload[3], frame offset 10).
/// Vectors: shared/fixtures/app-frame-tx.json (TX-*).
import Foundation
public enum HKTFrameEncoder {
    public static let prefix: [UInt8] = [0x68, 0x6B, 0x74]

    public static func appFrame(packNum: UInt8, cmd: UInt8, data: Data) -> Data {
        var frame = Data(prefix)
        frame.append(packNum)
        let bodyLength = UInt16(1 + data.count)
        frame.append(UInt8(bodyLength >> 8))
        frame.append(UInt8(bodyLength & 0xFF))
        frame.append(cmd)
        frame.append(data)
        let crc = CRC16.ccitt(frame.suffix(1 + data.count))
        frame.append(UInt8(crc >> 8))
        frame.append(UInt8(crc & 0xFF))
        return frame
    }

    /// 4-byte filler payload used by 0xFF/0xFE/0xFD/0x01 (Android compatibility: 0xFF fill,
    /// power-on = 0x00000001). firmwareReference: communicate.c data[10] reads.
    public static func fillerPayload(fill: UInt8 = 0xFF) -> Data {
        Data(repeating: fill, count: 4)
    }

    public static func powerPayload(on: Bool) -> Data {
        Data([0x00, 0x00, 0x00, on ? 0x01 : 0x00])
    }

    // MARK: 0x02 写配置载荷（三家族）
    // 契约出处：Android Communicate.kt streamDevice(0x02)；固件核对 UDS data[5]==9 / DC data[5]==4 /
    // SVC data[5]==8 分支（载荷顺序与宽度逐字节一致）。任一参数非法固件整包拒绝（UDS 低/高阈值非法不回 ACK），
    // App 侧已按同规则前置校验。

    /// UDS100：上报周期 + GPS 周期 + 满溢低阈值 + 高阈值，各 u16 BE（8B，len=9）。
    public static func udsConfigPayload(reportMin: Int, gpsMin: Int, lowMM: Int, highMM: Int) -> Data {
        var data = Data()
        data.append(contentsOf: UInt16(reportMin).dataBE)
        data.append(contentsOf: UInt16(gpsMin).dataBE)
        data.append(contentsOf: UInt16(lowMM).dataBE)
        data.append(contentsOf: UInt16(highMM).dataBE)
        return data
    }

    /// DC200 家族：上报周期 u16 BE + 工作模式 u8（3B，len=4）。
    public static func dcConfigPayload(reportMin: Int, mode: Int) -> Data {
        var data = Data()
        data.append(contentsOf: UInt16(reportMin).dataBE)
        data.append(UInt8(mode))
        return data
    }

    /// SVC100：电压档 / 端口功能 / 稳定时长 / 自动开关机 / 时区（各 u8）+ 上报周期 u16 BE（7B，len=8）。
    public static func svcConfigPayload(volLevel: Int, port: Int, stableS: Int,
                                        autoPower: Int, timezone: Int, reportMin: Int) -> Data {
        var data = Data([UInt8(volLevel), UInt8(port), UInt8(stableS), UInt8(autoPower), UInt8(timezone)])
        data.append(contentsOf: UInt16(reportMin).dataBE)
        return data
    }

    /// Time-sync (0x06) frame — carries a wire quirk that all three firmwares depend on:
    /// the declared len is 4 (data length only, NOT cmd+data like every other command);
    /// the firmware guard is `data[5]==4` while the stamp is read from data[7..10].
    /// Android ships the same quirk (Communicate.kt streamDevice(0x06): %04X % 4).
    /// stamp == 0 is silently ignored by the firmware (no ACK).
    public static func timeSyncFrame(packNum: UInt8, stampBE: UInt32) -> Data {
        var frame = Data(prefix)
        frame.append(packNum)
        frame.append(contentsOf: [0x00, 0x04])
        let body = Data([CommandCode.timeSync]) + stampBE.dataBE
        frame.append(body)
        let crc = CRC16.ccitt(body)
        frame.append(UInt8(crc >> 8))
        frame.append(UInt8(crc & 0xFF))
        return frame
    }
}

/// Command codes shared by all three firmware families (communicate.h dispatch).
public enum CommandCode {
    public static let otaNotify: UInt8 = 0x01
    public static let config: UInt8 = 0x02
    public static let svcRealtimeTask: UInt8 = 0x03
    public static let svcTimedTask: UInt8 = 0x04
    public static let svcDeleteTask: UInt8 = 0x05
    public static let timeSync: UInt8 = 0x06
    public static let calibrate: UInt8 = 0xFD
    public static let power: UInt8 = 0xFE
    public static let query: UInt8 = 0xFF
}
