#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Which physical build to emulate.
typedef NS_ENUM(NSInteger, TLFixture) {
  /// particles_with_sound.cc: 8 strips x 118 LEDs, rendered as light bars.
  TLFixtureBars = 0,
  /// ball_loop_2.cc: 540 LEDs along the 30 edges of an icosahedron,
  /// rendered as a spinning 3D ball.
  TLFixtureBall = 1,
};

/// Objective-C facade over the C++ SimEngine plus the CPU glow compositor,
/// so the SwiftUI app can drive frames without C++ interop.
@interface TLEngine : NSObject

- (instancetype)initWithFixture:(TLFixture)fixture NS_DESIGNATED_INITIALIZER;

@property(nonatomic, readonly) TLFixture fixture;

/// Strip geometry (matches the firmware for the chosen fixture).
@property(nonatomic, readonly) NSInteger stripCount;
@property(nonatomic, readonly) NSInteger stripLength;

/// Pixel dimensions of the composited framebuffer for this fixture.
@property(nonatomic, readonly) NSInteger canvasWidth;
@property(nonatomic, readonly) NSInteger canvasHeight;

@property(nonatomic, readonly) NSInteger sequenceCount;
@property(nonatomic, readonly) NSInteger sequenceIndex;

- (NSString *)sequenceNameAtIndex:(NSInteger)index NS_SWIFT_NAME(sequenceName(at:));
- (void)selectSequence:(NSInteger)index NS_SWIFT_NAME(selectSequence(_:));

/// Number of DMX-mapped controls on the current sequence.
@property(nonatomic, readonly) NSInteger controlCount;

/// Sets a DMX channel input in 0..1. Until the first call, controls hold
/// their firmware defaults (mirrors seenNonZeroDmxValue).
- (void)setControl:(NSInteger)channel value:(float)value;

/// Advances the simulation. bins may be NULL (no fresh audio this frame);
/// otherwise it must point at 512 floats shaped like the Teensy FFT output.
- (void)tickAtMillis:(uint32_t)millis
           soundBins:(nullable const float *)bins NS_SWIFT_NAME(tick(atMillis:soundBins:));

/// Composites the LEDs with additive glow into an RGBA16Float buffer of
/// canvasWidth x canvasHeight pixels, in *linear* extended-range sRGB.
/// Values are scaled by `exposure` and soft-clamped to `headroom`
/// (1.0 = SDR white; >1.0 = EDR nits on HDR displays).
/// yaw/pitch (radians) orbit the camera for the ball fixture; ignored for bars.
- (void)renderWithExposure:(float)exposure
                  headroom:(float)headroom
                       yaw:(float)yaw
                     pitch:(float)pitch
                      into:(void *)rgba16f;

@end

NS_ASSUME_NONNULL_END
