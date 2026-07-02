import AppKit
import Metal
import QuartzCore
import SwiftUI
import sim_bridge

/// Hosts the CAMetalLayer the HDR framebuffer is presented on, and owns the
/// display link that paces the whole simulation.
struct LEDCanvas: NSViewRepresentable {
  let model: SimModel

  func makeNSView(context: Context) -> MetalLEDView {
    let view = MetalLEDView()
    view.model = model
    return view
  }

  func updateNSView(_ view: MetalLEDView, context: Context) {
    view.model = model
  }
}

final class MetalLEDView: NSView {
  var model: SimModel?
  private var displayLinkRef: CADisplayLink?

  override init(frame frameRect: NSRect) {
    super.init(frame: frameRect)
    wantsLayer = true
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("not used")
  }

  override func makeBackingLayer() -> CALayer {
    let metalLayer = CAMetalLayer()
    metalLayer.device = MTLCreateSystemDefaultDevice()
    metalLayer.pixelFormat = .rgba16Float
    metalLayer.framebufferOnly = false
    metalLayer.wantsExtendedDynamicRangeContent = true
    metalLayer.colorspace = CGColorSpace(name: CGColorSpace.extendedLinearDisplayP3)
    metalLayer.backgroundColor = .black
    return metalLayer
  }

  override func mouseDragged(with event: NSEvent) {
    model?.orbit(dx: event.deltaX, dy: event.deltaY)
  }

  override func viewDidMoveToWindow() {
    super.viewDidMoveToWindow()
    displayLinkRef?.invalidate()
    displayLinkRef = nil
    guard window != nil else { return }
    let link = displayLink(target: self, selector: #selector(step(_:)))
    link.add(to: .main, forMode: .common)
    displayLinkRef = link
  }

  @objc private func step(_ link: CADisplayLink) {
    guard let model, let metalLayer = layer as? CAMetalLayer else { return }
    model.frame(into: metalLayer, screen: window?.screen)
  }
}
