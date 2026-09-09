import SwiftUI

@main
struct HKTBLEToolsApp: App {
    @State private var scanModel = ScanModel()

    var body: some Scene {
        WindowGroup {
            RootView()
                .environment(scanModel)
        }
    }
}
