import SwiftUI

/// Live log-spaced spectrum, hue-swept from bass (blue-violet) to treble (red).
struct SpectrumView: View {
  let model: SimModel

  private static let barCount = 64

  var body: some View {
    Canvas { context, size in
      let bins = model.spectrum
      guard bins.count >= 512 else { return }

      let barCount = Self.barCount
      let barWidth = size.width / Double(barCount)
      for bar in 0..<barCount {
        let lo = Int(pow(511.0, Double(bar) / Double(barCount)))
        let hi = max(lo + 1, Int(pow(511.0, Double(bar + 1) / Double(barCount))))
        var value: Float = 0
        for bin in lo..<min(hi, 512) {
          value = max(value, bins[bin])
        }

        let height = min(1.0, Double(value) * 1.4) * size.height
        let rect = CGRect(
          x: Double(bar) * barWidth + 1,
          y: size.height - height,
          width: barWidth - 2,
          height: height)
        let hue = 0.72 - 0.6 * Double(bar) / Double(barCount)
        context.fill(
          Path(roundedRect: rect, cornerRadius: 1.5),
          with: .color(Color(hue: hue, saturation: 0.85, brightness: 0.95)))
      }
    }
  }
}
