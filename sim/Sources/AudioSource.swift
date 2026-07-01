import AVFoundation
import Accelerate
import sim_bridge

enum AudioMode: String, CaseIterable, Identifiable {
  case synthetic = "Synth beat"
  case microphone = "Microphone"
  var id: String { rawValue }
}

/// Produces 512 FFT bins shaped like the Teensy AudioAnalyzeFFT1024 output
/// (~43 Hz per bin), either from the microphone or a synthetic beat.
final class AudioSource: @unchecked Sendable {
  private let lock = NSLock()
  private var ring = [Float](repeating: 0, count: 4096)
  private var writeIndex = 0
  private var engine: AVAudioEngine?
  private(set) var micRunning = false

  private let fft = vDSP.FFT(log2n: 10, radix: .radix2, ofType: DSPSplitComplex.self)
  private let window = vDSP.window(
    ofType: Float.self, usingSequence: .hanningDenormalized, count: 1024, isHalfWindow: false)
  private var peak: Float = 0.05

  func startMicrophone() async -> Bool {
    guard await AVCaptureDevice.requestAccess(for: .audio) else { return false }
    return await MainActor.run { startEngine() }
  }

  private func startEngine() -> Bool {
    if micRunning { return true }
    let engine = AVAudioEngine()
    let input = engine.inputNode
    let format = input.inputFormat(forBus: 0)
    guard format.sampleRate > 0 else { return false }
    input.installTap(onBus: 0, bufferSize: 1024, format: format) { [weak self] buffer, _ in
      self?.append(buffer)
    }
    do {
      try engine.start()
    } catch {
      return false
    }
    self.engine = engine
    micRunning = true
    return true
  }

  private func append(_ buffer: AVAudioPCMBuffer) {
    guard let data = buffer.floatChannelData?[0] else { return }
    let count = Int(buffer.frameLength)
    lock.lock()
    for i in 0..<count {
      ring[(writeIndex + i) % ring.count] = data[i]
    }
    writeIndex = (writeIndex + count) % ring.count
    lock.unlock()
  }

  /// FFT of the most recent 1024 mic samples, or nil if the mic isn't running.
  func microphoneBins() -> [Float]? {
    guard micRunning, let fft else { return nil }

    var samples = [Float](repeating: 0, count: 1024)
    lock.lock()
    var index = (writeIndex - 1024 + ring.count * 2) % ring.count
    for i in 0..<1024 {
      samples[i] = ring[index]
      index = (index + 1) % ring.count
    }
    lock.unlock()

    var windowed = [Float](repeating: 0, count: 1024)
    vDSP.multiply(samples, window, result: &windowed)

    var real = [Float](repeating: 0, count: 512)
    var imag = [Float](repeating: 0, count: 512)
    var bins = [Float](repeating: 0, count: 512)
    real.withUnsafeMutableBufferPointer { realPtr in
      imag.withUnsafeMutableBufferPointer { imagPtr in
        var split = DSPSplitComplex(
          realp: realPtr.baseAddress!, imagp: imagPtr.baseAddress!)
        windowed.withUnsafeBytes { raw in
          vDSP_ctoz(
            raw.bindMemory(to: DSPComplex.self).baseAddress!, 2, &split, 1, 512)
        }
        fft.forward(input: split, output: &split)
        vDSP.squareMagnitudes(split, result: &bins)
      }
    }
    bins = vForce.sqrt(bins)
    vDSP.multiply(1.0 / 1024.0, bins, result: &bins)
    bins[0] = 0

    // Gentle autogain so quiet rooms and loud parties both animate.
    peak = max(vDSP.maximum(bins), peak * 0.995, 0.02)
    vDSP.multiply(0.55 / peak, bins, result: &bins)
    return bins
  }

  /// Deterministic four-on-the-floor groove for hardware-free demos.
  /// Implemented once in C++ (shared with the render CLI and snapshot tests).
  func syntheticBins(at t: Double) -> [Float] {
    var bins = [Float](repeating: 0, count: 512)
    TLEngine.syntheticBins(at: t, into: &bins)
    return bins
  }
}
