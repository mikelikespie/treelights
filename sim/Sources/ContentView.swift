import SwiftUI

struct ContentView: View {
  @Environment(SimModel.self) private var model

  var body: some View {
    @Bindable var model = model

    HStack(spacing: 0) {
      VStack(spacing: 12) {
        LEDCanvas(model: model)
          .frame(width: 927, height: 312)
          .clipShape(RoundedRectangle(cornerRadius: 8))
        SpectrumView(model: model)
          .frame(width: 927, height: 70)
      }
      .padding(16)
      .background(.black)

      Divider()

      Form {
        Section {
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
