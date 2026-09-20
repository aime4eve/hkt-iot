import AVFoundation
import SwiftUI

/// R-2 扫码真会话（2026-09-15 接入）：AVFoundation 元数据输出识别二维码/条码，
/// 全屏预览；识别文本经 onCode 主线程回调。相机权限=Info.plist NSCameraUsageDescription（工程已配）。
/// 识别规则在 LocateFlowView.handleQR：内容须为 16 位 hex DevEUI（Android isValidDevEui 同规）。
struct CameraScanner: UIViewRepresentable {
    enum RunState: Equatable { case running, denied, unavailable }
    var onCode: (String) -> Void
    @Binding var state: RunState

    func makeUIView(context: Context) -> ScannerView {
        let view = ScannerView()
        view.controller.onCode = onCode
        // controller.onState 从 sessionQueue 经主线程投递（见 CameraController），此处再转回绑定
        view.onStateChanged = { if state != $0 { state = $0 } }
        view.boot()
        return view
    }

    func updateUIView(_ uiView: ScannerView, context: Context) {}

    static func dismantleUIView(_ uiView: ScannerView, coordinator: ()) {
        uiView.controller.shutdown()   // 离开取景页（识别命中/取消）即停会话，避免后台占用相机
    }

    /// 预览视图：层=AVCaptureVideoPreviewLayer；会话状态由 CameraController（队列封闭）持有。
    final class ScannerView: UIView {
        let controller = CameraController()

        var onStateChanged: ((RunState) -> Void)?

        override static var layerClass: AnyClass { AVCaptureVideoPreviewLayer.self }
        var previewLayer: AVCaptureVideoPreviewLayer { layer as! AVCaptureVideoPreviewLayer }

        override init(frame: CGRect) {
            super.init(frame: frame)
            previewLayer.session = controller.session
            previewLayer.videoGravity = .resizeAspectFill
        }

        required init?(coder: NSCoder) { fatalError("not used") }

        func boot() {
            controller.requestAndRun()
        }
    }
}

/// 会话控制器（@unchecked Sendable：session/booted 仅 sessionQueue 触碰；onCode/onState 会话启动前设置）。
/// 兼任元数据代理，回调固定投递在 sessionQueue；节流后经 onState 约定回到主线程。
final class CameraController: NSObject, AVCaptureMetadataOutputObjectsDelegate, @unchecked Sendable {
    let session = AVCaptureSession()
    var onCode: ((String) -> Void)?
    var onState: ((CameraScanner.RunState) -> Void)?

    private let queue = DispatchQueue(label: "hkt.camera.scanner")
    private var booted = false
    private var lastDecode = TimeInterval(0)

    /// 相机权限三态：已授权直接启动；未决定弹系统授权；拒绝/受限报 denied（UI 引导去设置）。
    func requestAndRun() {
        switch AVCaptureDevice.authorizationStatus(for: .video) {
        case .authorized:
            configureAndRun()
        case .notDetermined:
            AVCaptureDevice.requestAccess(for: .video) { [weak self] granted in
                DispatchQueue.main.async {
                    if granted { self?.configureAndRun() } else { self?.onState?(.denied) }
                }
            }
        default:
            onState?(.denied)
        }
    }

    private func configureAndRun() {
        queue.async { [weak self] in
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
            output.setMetadataObjectsDelegate(self, queue: self.queue)
            // 二维码为主、常用一维条码兜底（SP-2「二维码/条码」）；内容规则仍=16 位 DevEUI
            let wanted: [AVMetadataObject.ObjectType] = [.qr, .code128, .code39, .code93, .ean13, .ean8, .pdf417]
            output.metadataObjectTypes = wanted.filter { output.availableMetadataObjectTypes.contains($0) }
            self.session.commitConfiguration()
            self.session.startRunning()
            DispatchQueue.main.async { self.onState?(.running) }
        }
    }

    func shutdown() {
        queue.async { [weak self] in
            guard let self, self.booted, self.session.isRunning else { return }
            self.session.stopRunning()
        }
    }

    // MARK: - AVCaptureMetadataOutputObjectsDelegate（sessionQueue）

    func metadataOutput(_ output: AVCaptureMetadataOutput, didOutput metadataObjects: [AVMetadataObject], from connection: AVCaptureConnection) {
        guard let text = metadataObjects.compactMap({ $0 as? AVMetadataMachineReadableCodeObject }).first?.stringValue else { return }
        let now = Date().timeIntervalSince1970
        guard now - lastDecode > 1.2 else { return }   // 同一张码持续在画面里的节流
        lastDecode = now
        DispatchQueue.main.async { [weak self] in self?.onCode?(text) }
    }
}
