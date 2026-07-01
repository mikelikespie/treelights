import SwiftUI

@main
struct SimulatorApp: App {
  @State private var model = SimModel()

  var body: some Scene {
    WindowGroup("Treelights Simulator") {
      ContentView()
        .environment(model)
    }
    .windowResizability(.contentSize)
    .commands {
      CommandMenu("Sequence") {
        ForEach(Array(model.sequenceNames.enumerated()), id: \.offset) { index, name in
          Button(name) {
            model.selectSequence(index)
          }
          .keyboardShortcut(KeyEquivalent(Character("\(index + 1)")), modifiers: .command)
        }
      }
    }
  }
}
