// swift-tools-version:5.9
import PackageDescription

// Info.plist 经 __TEXT,__info_plist 链接段嵌入：裸 CLI 需要蓝牙用途声明，
// 否则 macOS 静默拒绝（CBCentralManager 代理回调永不到达——实测踩坑）。
let plist = "Info.plist"

let package = Package(
    name: "macble-bridge",
    platforms: [.macOS(.v13)],
    targets: [
        .executableTarget(
            name: "macble-bridge",
            path: "Sources/macble-bridge",
            linkerSettings: [
                .unsafeFlags([
                    "-Xlinker", "-sectcreate",
                    "-Xlinker", "__TEXT",
                    "-Xlinker", "__info_plist",
                    "-Xlinker", plist,
                ]),
            ],
        ),
    ],
)
