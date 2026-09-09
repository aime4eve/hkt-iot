/// FeatureSurface 与 UI 显隐（SD §7；R-G2）。
/// 组合根构造后只读传递；运行期唯一可变项 `debugUI`（用户触发，持久化到 UserDefaults）。
/// 状态值按 2026-09-07/08 用户裁决：目标定位与扫码进 v1（翻案 Q4）、校准进 v1（Q1 三修）。
struct FeatureSurface: Sendable {
    var basicConfigUI = false      // M8 按 Q1 置 true
    var debugUI = false            // 关于页连点版本 7 次置 true；dev 构建初始 true
    var targetedScanUI = true      // 2026-09-07 用户裁决选 B：定位进 v1（翻案 Q4）；R-2/SP-2
    var barcodeUI = true           // 扫码定位依赖二维码/条码识别（需相机权限）
    var calibrationUI = true       // Q1 三修：校准进 v1（R-23）
    var languageSwitchUI = true
    var diagnosticLogUI = true
}
