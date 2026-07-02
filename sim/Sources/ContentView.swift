import SwiftUI

struct ContentView: View {
  @Environment(SimModel.self) private var model

  var body: some View {
    @Bindable var model = model

    let canvasPoints = canvasPointSize

    HStack(spacing: 0) {
      VStack(spacing: 12) {
        LEDCanvas(model: model)
          .frame(width: canvasPoints.width, height: canvasPoints.height)
          .clipShape(RoundedRectangle(cornerRadius: 8))
        SpectrumView(model: model)
          .frame(width: canvasPoints.width, height: 70)
      }
      .padding(16)
      .background(.black)

      Divider()

      Form {
        Section {
          Picker("Fixture", selection: fixtureBinding) {
            ForEach(Fixture.allCases) { fixture in
              Text(fixture.rawValue).tag(fixture)
            }
          }
          .pickerStyle(.segmented)

          if model.fixture == .ball {
            Text("Drag the ball to orbit; it spins on its own after 2s.")
              .font(.caption)
              .foregroundStyle(.secondary)
          }

          Picker("Sequence", selection: sequenceBinding) {
            ForEach(Array(model.sequenceNames.enumerated()), id: \.offset) { index, name in
              Text(name).tag(index)
            }
          }

          Picker("Audio", selection: audioBinding) {
            ForEach(AudioMode.allCases) { mode in
              Text(mode.rawValue).tag(mode)
            }
          }
          .pickerStyle(.segmented)

          if model.micDenied {
            Text("Microphone access denied — using synth beat.")
              .font(.caption)
              .foregroundStyle(.secondary)
          }
        }

        Section("Display") {
          LabeledContent("Exposure") {
            Slider(value: $model.exposure, in: 0.4...8)
          }
          Toggle("HDR glow (EDR)", isOn: $model.hdrEnabled)
          Toggle("Wide gamut (P3)", isOn: $model.wideGamut)
          LabeledContent("Headroom") {
            Text(String(format: "×%.1f", model.headroom))
              .monospacedDigit()
              .foregroundStyle(model.headroom > 1 ? .primary : .secondary)
          }
        }

        Section("DMX channels") {
          ForEach(0..<model.controlCount, id: \.self) { channel in
            LabeledContent("Ch \(channel)") {
              Slider(
                value: Binding(
                  get: { model.controlValues[channel] },
                  set: { model.setControl(channel, value: $0) }),
                in: 0...1)
            }
          }
          Text("Sliders stand in for the DMX console; controls hold firmware defaults until first touch.")
            .font(.caption)
            .foregroundStyle(.secondary)
        }

        Section {
          LabeledContent("Frame rate") {
            Text(String(format: "%.0f fps", model.fps)).monospacedDigit()
          }
        }
      }
      .formStyle(.grouped)
      .frame(width: 320)
    }
  }

  /// Canvas in points: bars at 0.75x pixels, ball square fit to same height budget.
  private var canvasPointSize: CGSize {
    switch model.fixture {
    case .bars: CGSize(width: 927, height: 312)
    case .ball: CGSize(width: 560, height: 560)
    }
  }

  private var fixtureBinding: Binding<Fixture> {
    Binding(
      get: { model.fixture },
      set: { model.selectFixture($0) })
  }

  private var sequenceBinding: Binding<Int> {
    Binding(
      get: { model.sequenceIndex },
      set: { model.selectSequence($0) })
  }

  private var audioBinding: Binding<AudioMode> {
    Binding(
      get: { model.audioMode },
      set: { model.setAudioMode($0) })
  }
}
