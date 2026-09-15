import AVFoundation
import SwiftUI

/// R-2 扫码真会话（2026-09-14 接入）：AVFoundation 元数据输出识别二维码/条码，
/// 全屏预览；识别文本经 onCode 主线程回调。相机权限=Info.plist NSCameraUsageDescription（工程已配）。
/// 识别规则在 LocateFlowView.handleQR：内容须为 16 位 hex DevEUI（与 Android isValidDevEui 一致）。
struct CameraScanner: UIViewRepresentable {
    enum RunState: Equatable { case running, denied, unavailable }
    var onCode: (String) -> Void
    @Binding var state: RunState

    func makeUIView(context: Context) -> ScannerView {
        let view = ScannerView()
        view.onCode = onCode
        view.onState = { if state != $0 { state = $0 } }
        view.boot()
        return view
    }

    func updateUIView(_ uiView: ScannerView, context: Context) {}

    static func dismantleUIView(_ uiView: ScannerView, coordinator: ()) {
        uiView.shutdown()   // 离开取景页（识别命中/取消）即停会话，避免后台占用相机
    }

    final class ScannerView: UIView {
        var onCode: ((String) -> Void)? { didSet { relay.onCode = onCode } }
        var onState: ((RunState) -> Void)?
        let session = AVCaptureSession()
        private let sessionQueue = DispatchQueue(label: "hkt.camera.scanner")
        private let relay = CodeRelay()
        private var booted = false

        override static var layerClass: AnyClass { AVCaptureVideoPreviewLayer.self }
        var previewLayer: AVCaptureVideoPreviewLayer { layer as! AVCaptureVideoPreviewLayer }

        override init(frame: CGRect) {
            super.init(frame: frame)
            previewLayer.session = session
            previewLayer.videoGravity = .resizeAspectFill
        }

        required init?(coder: NSCoder) { fatalError("not used") }

        func boot() {
            switch AVCaptureDevice.authorizationStatus(for: .video) {
            case .authorized: configureAndRun()
            case .notDetermined:
                AVCaptureDevice.requestAccess(for: .video) { [weak self] granted in
                    DispatchQueue.main.async {
                        granted ? self?.configureAndRun() : self?.onState?(.denied)
                    }
                }
            default:
                onState?(.denied)
            }
        }

        private func configureAndRun() {
            sessionQueue.async { [weak self] in
                guard let self, !self.booted else { return }
                self.booted = true
                guard let device = AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .back),
                      let input = try? AVCaptureDeviceInput(device: device),
                      self.session.canAddInput(input) else {
                    DispatchQueue.main.async { self.onState?(.unavailable) }
                    return
                }
                let output = AVCaptureMetadataOutput()
                guard self.session.canAddOutput(output) else {
                    DispatchQueue.main.async { self.onState?(.unavailable) }
                    return
                }
                self.session.beginConfiguration()
                self.session.addInput(input)
                self.session.addOutput(output)
                output.setMetadataObjectsDelegate(self.relay, queue: self.sessionQueue)
                // 二维码为主、常用一维条码兜底（SP-2「二维码/条码」）；内容规则仍=16 位 DevEUI
                let wanted: [AVMetadataObject.ObjectType] = [.qr, .code128, .code39, .code93, .ean13, .ean8, .pdf417]
                output.metadataObjectTypes = wanted.filter { output.availableMetadataObjectTypes.contains($0) }
                self.session.commitConfiguration()
                self.session.startRunning()
                DispatchQueue.main.async { self.onState?(.running) }
            }
        }

        func shutdown() {
            sessionQueue.async { [weak self] in
                guard let self, self.booted, self.session.isRunning else { return }
                self.session.stopRunning()
            }
        }
    }
}

/// 元数据中继（非 UI 类，sessionQueue 上被回调）：同一张码节流后投递主线程。
/// Sendable 由约定保证：onCode 在会话启动前主线程设置一次，lastDecode 仅 sessionQueue 触碰。
final class CodeRelay: NSObject, AVCaptureMetadataOutputObjectsDelegate, @unchecked Sendable {
    var onCode: ((String) -> Void)?
    private var lastDecode = TimeInterval(0)

    func metadataOutput(_ output: AVCaptureMetadataOutput, didOutput metadataObjects: [AVMetadataObject], from connection: AVCaptureConnection) {
        guard let text = metadataObjects.compactMap({ $0 as? AVMetadataMachineReadableCodeObject }).first?.stringValue else { return }
        let now = Date().timeIntervalSince1970
        guard now - lastDecode > 1.2 else { return }   // 同一张码持续在画面里的节流
        lastDecode = now
        DispatchQueue.main.async { [weak self] in self?.onCode?(text) }
    }
}
