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
