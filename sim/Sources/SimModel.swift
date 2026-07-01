import AppKit
import Metal
import Observation
import QuartzCore
import sim_bridge

/// Drives the simulation: one display-link frame pulls audio bins, ticks the
/// firmware engine, composites the HDR framebuffer, and presents it.
@MainActor
@Observable
final class SimModel {
  let engine = TLEngine()

  private(set) var sequenceNames: [String] = []
  private(set) var sequenceIndex = 0
  private(set) var controlCount = 0
  private(set) var headroom = 1.0
  private(set) var fps = 0.0
  private(set) var spectrum = [Float](repeating: 0, count: 512)
  private(set) var micDenied = false

  var controlValues = [Double](repeating: 0.5, count: 16)
  var exposure = 2.2
  var hdrEnabled = true
  var audioMode = AudioMode.synthetic

  @ObservationIgnored private let audio = AudioSource()
  @ObservationIgnored private var pushedControls = false
  @ObservationIgnored private var framebuffer: UnsafeMutableRawPointer
  @ObservationIgnored private var commandQueue: MTLCommandQueue?
  @ObservationIgnored private let startTime = CACurrentMediaTime()
  @ObservationIgnored private var framesSinceFpsUpdate = 0
  @ObservationIgnored private var lastFpsUpdate = CACurrentMediaTime()

  init() {
    framebuffer = .allocate(
      byteCount: TLEngine.canvasWidth * TLEngine.canvasHeight * 8, alignment: 16)
    sequenceNames = (0..<engine.sequenceCount).map { engine.sequenceName(at: $0) }
    controlCount = engine.controlCount
  }

  func selectSequence(_ index: Int) {
    guard sequenceNames.indices.contains(index) else { return }
    engine.selectSequence(index)
    sequenceIndex = index
    controlCount = engine.controlCount
  }

  /// Moving any slider flips the engine into DMX mode (like the firmware
  /// seeing its first nonzero DMX byte), so push every channel on first touch.
  func setControl(_ channel: Int, value: Double) {
    controlValues[channel] = value
    if pushedControls {
      engine.setControl(channel, value: Float(value))
    } else {
      pushedControls = true
      for (i, v) in controlValues.enumerated() {
        engine.setControl(i, value: Float(v))
      }
    }
  }

  func setAudioMode(_ mode: AudioMode) {
    audioMode = mode
    guard mode == .microphone else { return }
    Task {
      let started = await audio.startMicrophone()
      if !started {
        micDenied = true
        audioMode = .synthetic
      }
    }
  }

  /// One display-link frame.
  func frame(into layer: CAMetalLayer, screen: NSScreen?) {
    let elapsed = CACurrentMediaTime() - startTime
    let bins =
      audioMode == .microphone
      ? audio.microphoneBins()
      : audio.syntheticBins(at: elapsed)
    if let bins {
      spectrum = bins
    }

    let millis = UInt32(truncatingIfNeeded: Int(elapsed * 1000) + 1000)
    if let bins {
      bins.withUnsafeBufferPointer {
        engine.tick(atMillis: millis, soundBins: $0.baseAddress)
      }
    } else {
      engine.tick(atMillis: millis, soundBins: nil)
    }

    var maxComponent = 1.0
    if hdrEnabled, let screen {
      maxComponent = max(1.0, screen.maximumExtendedDynamicRangeColorComponentValue)
    }
    headroom = maxComponent

    engine.render(
      withExposure: Float(exposure), headroom: Float(maxComponent), into: framebuffer)

    guard let drawable = layer.nextDrawable() else { return }
    drawable.texture.replace(
      region: MTLRegionMake2D(0, 0, TLEngine.canvasWidth, TLEngine.canvasHeight),
      mipmapLevel: 0,
      withBytes: framebuffer,
      bytesPerRow: TLEngine.canvasWidth * 8)
    if commandQueue == nil {
      commandQueue = layer.device?.makeCommandQueue()
    }
    if let commandBuffer = commandQueue?.makeCommandBuffer() {
      commandBuffer.present(drawable)
      commandBuffer.commit()
    } else {
      drawable.present()
    }

    framesSinceFpsUpdate += 1
    let now = CACurrentMediaTime()
    if now - lastFpsUpdate >= 1 {
      fps = Double(framesSinceFpsUpdate) / (now - lastFpsUpdate)
      framesSinceFpsUpdate = 0
      lastFpsUpdate = now
    }
  }
}
