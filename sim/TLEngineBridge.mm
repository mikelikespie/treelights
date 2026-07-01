#import "sim/TLEngineBridge.h"

#include <memory>

#include "sim/SimEngine.h"
#include "sim/SimRenderer.h"
#include "sim/SyntheticBeat.h"

@implementation TLEngine {
  std::unique_ptr<SimEngine> _engine;
  std::unique_ptr<SimRenderer> _renderer;
}

- (instancetype)init {
  return [self initWithFixture:TLFixtureBars];
}

- (instancetype)initWithFixture:(TLFixture)fixture {
  if ((self = [super init])) {
    _fixture = fixture;
    const SimFixture simFixture =
        fixture == TLFixtureBall ? SimFixture::kBall : SimFixture::kBars;
    _engine = std::make_unique<SimEngine>(simFixture);
    _renderer = std::make_unique<SimRenderer>(simFixture);
  }
  return self;
}

- (NSInteger)stripCount {
  return _engine->stripCount();
}

- (NSInteger)stripLength {
  return _engine->stripLength();
}

- (NSInteger)canvasWidth {
  return _renderer->width();
}

- (NSInteger)canvasHeight {
  return _renderer->height();
}

- (NSInteger)sequenceCount {
  return _engine->sequenceCount();
}

- (NSInteger)sequenceIndex {
  return _engine->sequenceIndex();
}

- (NSString *)sequenceNameAtIndex:(NSInteger)index {
  return @(_engine->sequenceName((int) index));
}

- (void)selectSequence:(NSInteger)index {
  _engine->setSequenceIndex((int) index);
}

- (NSInteger)controlCount {
  return _engine->controlCount();
}

- (void)setControl:(NSInteger)channel value:(float)value {
  _engine->setControlValue((int) channel, value);
}

- (void)tickAtMillis:(uint32_t)millis soundBins:(const float *)bins {
  _engine->tick(millis, bins);
}

- (void)renderWithExposure:(float)exposure
                  headroom:(float)headroom
                       yaw:(float)yaw
                     pitch:(float)pitch
                      into:(void *)rgba16f {
  _renderer->composite(_engine->leds(), _engine->totalLedCount(), yaw, pitch);
  _renderer->toneMapRGBA16F(exposure, headroom, (uint16_t *) rgba16f);
}

+ (void)syntheticBins:(double)tSeconds into:(float *)bins512 {
  SyntheticBeatBins(tSeconds, bins512);
}

@end
