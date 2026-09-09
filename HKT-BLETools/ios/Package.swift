// swift-tools-version:6.0
// HKT BLETools iOS core packages. The Xcode app project (HKTBLETools.xcodeproj)
// lives beside this file and depends on these targets (M4+).
import PackageDescription

let package = Package(
    name: "HKTBLEToolsCore",
    products: [
        .library(name: "CoreProtocol", targets: ["CoreProtocol"]),
        .library(name: "CoreDevice", targets: ["CoreDevice"]),
        .library(name: "CoreOTA", targets: ["CoreOTA"]),
        .library(name: "CoreBLE", targets: ["CoreBLE"]),
    ],
    targets: [
        .target(name: "CoreProtocol"),
        .target(name: "CoreDevice", dependencies: ["CoreProtocol"]),
        .target(name: "CoreOTA", dependencies: ["CoreProtocol"]),
        // M4：蓝牙可用性/发现模型/传输端口协议/可编程假蓝牙源（无真机可测）。
        .target(name: "CoreBLE"),
        .testTarget(name: "CoreProtocolTests", dependencies: ["CoreProtocol"]),
        .testTarget(name: "CoreOTATests", dependencies: ["CoreOTA"]),
        .testTarget(name: "CoreDeviceTests", dependencies: ["CoreDevice"]),
        .testTarget(name: "CoreBLETests", dependencies: ["CoreBLE"]),
    ]
)
