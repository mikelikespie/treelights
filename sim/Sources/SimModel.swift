import AppKit
import Metal
import Observation
import QuartzCore
import sim_bridge

enum Fixture: String, CaseIterable, Identifiable {
  case bars = "Light Bars"
  case ball = "Icosahedron"
  var id: String { rawValue }

  var tlFixture: TLFixture {
    switch self {
    case .bars: .bars
    case .ball: .ball
    }
  }
}

/// Drives the simulation: one display-link frame pulls audio bins, ticks the
/// firmware engine, composites the HDR framebuffer, and presents it.
@MainActor
@Observable
final class SimModel {
  private(set) var engine: TLEngine
  private(set) var fixture = Fixture.bars
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
  @ObservationIgnored private var framebuffer: UnsafeMutableRawPointer?
  @ObservationIgnored private var framebufferBytes = 0
  @ObservationIgnored private var commandQueue: MTLCommandQueue?
  @ObservationIgnored private let startTime = CACurrentMediaTime()
  @ObservationIgnored private var framesSinceFpsUpdate = 0
  @ObservationIgnored private var lastFpsUpdate = CACurrentMediaTime()

  // Ball orbit state.
  @ObservationIgnored private var yaw: Float = 0.6
  @ObservationIgnored private var pitch: Float = 0.35
  @ObservationIgnored private var lastFrameTime = CACurrentMediaTime()
  @ObservationIgnored private var lastDragTime = 0.0

  init() {
    engine = TLEngine(fixture: .bars)
    sequenceNames = (0..<engine.sequenceCount).map { engine.sequenceName(at: $0) }
    controlCount = engine.controlCount
  }

  var canvasSize: CGSize {
    CGSize(width: engine.canvasWidth, height: engine.canvasHeight)
  }

  func selectFixture(_ newFixture: Fixture) {
    guard newFixture != fixture else { return }
    fixture = newFixture
    let index = sequenceIndex
    engine = TLEngine(fixture: newFixture.tlFixture)
    engine.selectSequence(index)
    sequenceIndex = engine.sequenceIndex
    controlCount = engine.controlCount
    if pushedControls {
      for (i, v) in controlValues.enumerated() {
        engine.setControl(i, value: Float(v))
      }
    }
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

  /// Drag-to-orbit for the ball fixture.
  func orbit(dx: Double, dy: Double) {
    yaw += Float(dx) * 0.010
    pitch += Float(dy) * 0.010
    pitch = min(1.45, max(-1.45, pitch))
    lastDragTime = CACurrentMediaTime()
  }

  /// One display-link frame.
  func frame(into layer: CAMetalLayer, screen: NSScreen?) {
    let now = CACurrentMediaTime()
    let elapsed = now - startTime
    let frameDelta = min(now - lastFrameTime, 0.1)
    lastFrameTime = now

    // Auto-spin the ball, pausing briefly after a drag.
    if fixture == .ball && now - lastDragTime > 2.0 {
      yaw += Float(frameDelta) * 0.25
    }

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

    let width = engine.canvasWidth
    let height = engine.canvasHeight
    let neededBytes = width * height * 8
    if framebuffer == nil || framebufferBytes < neededBytes {
      framebuffer?.deallocate()
      framebuffer = .allocate(byteCount: neededBytes, alignment: 16)
      framebufferBytes = neededBytes
    }
    guard let framebuffer else { return }

    engine.render(
      withExposure: Float(exposure),
      headroom: Float(maxComponent),
      yaw: yaw,
      pitch: pitch,
      into: framebuffer)

    let drawableSize = CGSize(width: width, height: height)
    if layer.drawableSize != drawableSize {
      layer.drawableSize = drawableSize
    }
    guard let drawable = layer.nextDrawable() else { return }
    drawable.texture.replace(
      region: MTLRegionMake2D(0, 0, width, height),
      mipmapLevel: 0,
      withBytes: framebuffer,
      bytesPerRow: width * 8)
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
    if now - lastFpsUpdate >= 1 {
      fps = Double(framesSinceFpsUpdate) / (now - lastFpsUpdate)
      framesSinceFpsUpdate = 0
      lastFpsUpdate = now
    }
  }
}
