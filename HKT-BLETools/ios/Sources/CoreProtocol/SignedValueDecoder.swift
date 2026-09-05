/// Signed-value decoding, preserving the Android sign-bit rules
/// (24-bit `> 0x800000`, 32-bit `> 0x80000000`, 16-bit `> 0x8000`).
/// Used for e.g. UDS100 temperature (×1000, s24 BE) and DC200Family mag axes (s16 BE).
import Foundation
public enum SignedValueDecoder {
    public static func s16(_ data: Data) -> Int {
        let v = BEValue.u16(data)
        return v > 0x8000 ? v - 0x10000 : v
    }

    public static func s24(_ data: Data) -> Int {
        let v = BEValue.u24(data)
        return v > 0x800000 ? v - 0x1000000 : v
    }

    public static func s32(_ data: Data) -> Int {
        let v = BEValue.u32(data)
        return v > 0x80000000 ? v - 0x100000000 : v
    }
}
