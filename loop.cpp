#define USE_OCTOWS2811

#include <OctoWS2811.h>
#include <FastLED.h>
#include <arm_math.h>

#define NUM_STRIPS 8


// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5

static const int edgeCount = 10;
static const int pixelsPerEdge = 16;
static const int numLeds = edgeCount * pixelsPerEdge;

CRGB leds[NUM_STRIPS * numLeds];

static int frameNum = 0;

void setup() {
    LEDS.addLeds<OCTOWS2811>(leds, numLeds);
    LEDS.setBrightness(255);
    LEDS.setDither(BINARY_DITHER);

}

static const int TARGET_FRAME_LENGTH_MICROS = 1000000 / 400;

static int32_t lastNow = 0;

void loop() {
    frameNum++;

    float timePast = (millis()) / 2000.0;

    double period = 1.0;

    double sliceSizeMult = 0.032;
    double sliceSizePhase = fmod(timePast * sliceSizeMult, M_PI * 2);

    float colorSliceSizeMin = 60.0;
    float colorSliceSizeSize = 60.0;
    float colorSliceSize = colorSliceSizeMin + (sin(sliceSizePhase) * 0.5f + 0.5f) * colorSliceSizeSize;

    double sliceMult = 0.025;
    double slicePhase = fmod(timePast * sliceMult, M_PI * 2);
    float colorSliceStart = (sin(slicePhase) * 0.5f + 0.5f) * 360.0f - colorSliceSize * .5f + 360.0f;


    for (int p = 0; p < pixelsPerEdge; p++) {
        float mult = 1.0;

        float phase = fmod(timePast * mult + p / (float) (pixelsPerEdge), period) * M_PI * 2;
        float sliceOffset = sin(phase) * 0.5 + 0.5;

        uint8_t hue = uint32_t(colorSliceStart + sliceOffset * colorSliceSize) % 255;

        float mult2 = 1.123;
        float otherPhase = fmod(timePast * mult2 + p / (float) (pixelsPerEdge), period) * M_PI * 2;
        float v = sin(otherPhase) * 0.5 + 0.5;

        CRGB color = CHSV(hue, (uint8_t) (1.0 * 255), (uint8_t) (v * 255));

        for (int edge = 0; edge < edgeCount; edge++) {
            int pos = edge * pixelsPerEdge + p;
            int mappedPos = pos;
            for (int strip = 0; strip < NUM_STRIPS; strip++) {
                leds[mappedPos + (strip * numLeds)] = color;
            }
        }
    }

    uint32_t now = micros();

    uint32_t targetNow = lastNow + TARGET_FRAME_LENGTH_MICROS;
    lastNow = now;

    if (now < targetNow){
        Serial.print("delaying");
        Serial.println(targetNow - now);
        delayMicroseconds(targetNow - now);
    }

    LEDS.show();
    LEDS.countFPS();
    if (frameNum % 250 == 0) {
        Serial.print("FPS:");
        Serial.println(LEDS.getFPS());
    }
}

