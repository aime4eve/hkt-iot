// swift-tools-version:6.0
// HKT BLETools iOS core packages. The Xcode app project (HKTBLETools.xcodeproj)
// will live beside this file and depend on these targets (M4+).
import PackageDescription

let package = Package(
    name: "HKTBLEToolsCore",
    products: [
        .library(name: "CoreProtocol", targets: ["CoreProtocol"]),
        .library(name: "CoreDevice", targets: ["CoreDevice"]),
        .library(name: "CoreOTA", targets: ["CoreOTA"]),
    ],
    targets: [
        .target(name: "CoreProtocol"),
        .target(name: "CoreDevice", dependencies: ["CoreProtocol"]),
        .target(name: "CoreOTA", dependencies: ["CoreProtocol"]),
        .testTarget(name: "CoreProtocolTests", dependencies: ["CoreProtocol"]),
        .testTarget(name: "CoreOTATests", dependencies: ["CoreOTA"]),
        .testTarget(name: "CoreDeviceTests", dependencies: ["CoreDevice"]),
    ]
)
