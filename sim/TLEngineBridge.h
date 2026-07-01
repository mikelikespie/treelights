#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Objective-C facade over the C++ SimEngine plus the CPU glow compositor,
/// so the SwiftUI app can drive frames without C++ interop.
@interface TLEngine : NSObject

/// Strip geometry (matches the firmware).
@property(class, readonly) NSInteger stripCount;
@property(class, readonly) NSInteger stripLength;

/// Pixel dimensions of the composited framebuffer.
@property(class, readonly) NSInteger canvasWidth;
@property(class, readonly) NSInteger canvasHeight;

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
- (void)renderWithExposure:(float)exposure
                  headroom:(float)headroom
                      into:(void *)rgba16f;

@end

NS_ASSUME_NONNULL_END
