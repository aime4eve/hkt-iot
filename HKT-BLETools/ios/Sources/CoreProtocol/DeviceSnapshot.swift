import Foundation

/// 详情页/界面共用的设备状态快照（R-5 字段全集）。
/// 由 TLV 记录解码（DeviceSnapshotDecoder）；轮询每秒整体刷新，字段保留上次有效值（R-7）。
public struct DeviceSnapshot: Equatable, Sendable {
    public var family: DeviceFamily
    public var hardwareVersion: Int = 0
    public var softwareVersion: Int = 0
    /// 0x8D：1 开 / 0 关
    public var power: Int = 0
    /// DC200Family/SVC100 估算电量（0x03，1B 百分比）
    public var batteryPercent: Int?
    /// UDS100 电压（0x8B，2B mV）
    public var batteryVoltageMV: Int?
    /// 上报周期（分钟，0x86）
    public var reportPeriodMin: Int?

    // UDS100（0x09 温度 /0x0A 湿度 存毫度；0x46 距离 mm；0x47 满溢四态；0x0E 倾角=原始值/100）
    public var temperatureMilli: Int?
    public var humidityMilli: Int?
    public var distanceMM: Int?
    public var overflowState: Int?
    public var lowThresholdMM: Int?
    public var highThresholdMM: Int?
    public var angleCenti: Int?
    public var slant: Int?
    public var htAlarm: Int?
    public var gpsPeriodMin: Int?
    public var latitude: Double?
    public var longitude: Double?

    // DC200Family（0x3A 车位 0/1/0xFF 遮挡；0x3B 模式 0/1/2；0x84 防拆；0x5D/5E/5F 地磁；0x60 雷达 10 段）
    public var parkState: Int?
    public var parkMode: Int?
    public var tamper: Int?
    public var magX: Int?
    public var magY: Int?
    public var magZ: Int?
    public var radarSpectrum: [Int]?

    // SVC100（0x3C 阀 9B；0x40 电压档 0/1/2=12/9/5V；0x41 端口功能 7 值；0x42 稳定 s；
    // 0x43 智能电源 1 自动；0x8A 时区 0-26）
    public var valve1State: Int?
    public var valve1Inserted: Int?
    public var valve1Pulse: Int?
    public var valve2State: Int?
    public var valve2Inserted: Int?
    public var valve2Pulse: Int?
    public var voltageLevel: Int?
    public var portFunction: Int?
    public var stableTimeS: Int?
    public var smartPower: Int?
    public var timezone: Int?

    public init(family: DeviceFamily) {
        self.family = family
    }
}

/// TLV 记录 → 快照字段。字段-类型-尺寸表以固件 setDataPackage/callback_BLEQuery 为唯一出处
/// （firmware-traceability §2.1；Android streamRev 对照）。未知类型由 HKTResponseParser 报
/// unknownTail，这里不参与解码（R-8/S-6）。
public enum DeviceSnapshotDecoder {
    /// 将记录解码进快照（保留未出现字段的上次有效值——轮询整体刷新语义）。
    public static func decode(_ entries: [TLVEntry], family: DeviceFamily, into snapshot: inout DeviceSnapshot) {
        for entry in entries {
            let v = entry.value
            switch entry.type {
            case 0x01:
                if v.count >= 2 { snapshot.hardwareVersion = Int(v[v.startIndex]); snapshot.softwareVersion = Int(v[v.startIndex + 1]) }
            case 0x03: if v.count >= 1 { snapshot.batteryPercent = Int(v[v.startIndex]) }
            case 0x09: if v.count >= 3 { snapshot.temperatureMilli = BEValue.i24(v) }
            case 0x0A: if v.count >= 3 { snapshot.humidityMilli = BEValue.i24(v) }
            case 0x0E: if v.count >= 2 { snapshot.angleCenti = BEValue.u16(v) }
            case 0x10: if v.count >= 4 { snapshot.latitude = Double(BEValue.i32(v)) / 1_000_000 }
            case 0x11: if v.count >= 4 { snapshot.longitude = Double(BEValue.i32(v)) / 1_000_000 }
            case 0x28: if v.count >= 1 { snapshot.htAlarm = Int(v[v.startIndex]) }
            case 0x3A: if v.count >= 1 { snapshot.parkState = Int(v[v.startIndex]) }
            case 0x3B: if v.count >= 1 { snapshot.parkMode = Int(v[v.startIndex]) }
            case 0x3C:
                if v.count >= 8 {
                    snapshot.valve1State = Int(v[v.startIndex])
                    snapshot.valve1Inserted = Int(v[v.startIndex + 1])
                    snapshot.valve1Pulse = BEValue.u16(v.advanced(by: 2))
                    snapshot.valve2State = Int(v[v.startIndex + 4])
                    snapshot.valve2Inserted = Int(v[v.startIndex + 5])
                    snapshot.valve2Pulse = BEValue.u16(v.advanced(by: 6))
                }
            case 0x40: if v.count >= 1 { snapshot.voltageLevel = Int(v[v.startIndex]) }
            case 0x41: if v.count >= 1 { snapshot.portFunction = Int(v[v.startIndex]) }
            case 0x42: if v.count >= 1 { snapshot.stableTimeS = Int(v[v.startIndex]) }
            case 0x43: if v.count >= 1 { snapshot.smartPower = Int(v[v.startIndex]) }
            case 0x44: if v.count >= 1 { snapshot.slant = Int(v[v.startIndex]) }
            case 0x45: if v.count >= 2 { snapshot.gpsPeriodMin = BEValue.u16(v) }
            case 0x46: if v.count >= 2 { snapshot.distanceMM = BEValue.u16(v) }
            case 0x47: if v.count >= 1 { snapshot.overflowState = Int(v[v.startIndex]) }
            case 0x48:
                if v.count >= 4 {
                    snapshot.lowThresholdMM = BEValue.u16(v)
                    snapshot.highThresholdMM = BEValue.u16(v.advanced(by: 2))
                }
            case 0x84: if v.count >= 1 { snapshot.tamper = Int(v[v.startIndex]) }
            case 0x86: if v.count >= 2 { snapshot.reportPeriodMin = BEValue.u16(v) }
            case 0x8A: if v.count >= 1 { snapshot.timezone = Int(v[v.startIndex]) }
            case 0x8B: if v.count >= 2 { snapshot.batteryVoltageMV = BEValue.u16(v) }
            case 0x8D: if v.count >= 1 { snapshot.power = Int(v[v.startIndex]) }
            case 0x5D: if v.count >= 2 { snapshot.magX = BEValue.i16(v) }
            case 0x5E: if v.count >= 2 { snapshot.magY = BEValue.i16(v) }
            case 0x5F: if v.count >= 2 { snapshot.magZ = BEValue.i16(v) }
            case 0x60:
                if v.count >= 20 {
                    snapshot.radarSpectrum = (0..<10).map { BEValue.u16(v.advanced(by: $0 * 2)) }
                }
            default:
                break   // 已由 parser 标记 unknownTail
            }
        }
    }

    /// 一次解码：空快照 + 记录 → 完整快照。
    public static func snapshot(from entries: [TLVEntry], family: DeviceFamily) -> DeviceSnapshot {
        var snapshot = DeviceSnapshot(family: family)
        decode(entries, family: family, into: &snapshot)
        return snapshot
    }
}
