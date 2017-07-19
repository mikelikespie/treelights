
#import "Context.h"
#include "clock.h"
#include "ExampleSequence.h"
#include "Control.h"
#include "SinWaveSequence.h"

static const int stripCount = 8;
static const int stripLength = 64;
static const int dmxChannels = 16;

/// 16 channels of DMX values
uint8_t dmxValues[dmxChannels] = {0};



// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5
static const int numLeds = stripCount * stripLength;

ARGB leds[numLeds] = {0};

static int frameNum = 0;

void setupDMX();
void loopDMX();

void setup() {
    LEDS.addLeds<OCTOWS2811>(leds, stripLength);
    LEDS.setDither(BINARY_DITHER);

    setupDMX();
}

uint8_t lastDitherMode = BINARY_DITHER;

Clock sharedClock;
Context context(leds, stripLength, stripCount);


int currentSequenceIndex = -1;
Sequence *currentSequence = nullptr;
const std::vector<Control *> *currentControls = nullptr;

IdentityValueControl brightnessControl;

ExampleSequence exampleSequence(stripLength, sharedClock, HCL(255, 0, 255));
ExampleSequence exampleSequence2(stripLength, sharedClock, HCL(127, 127, 127));

SinWaveSequence sinWaveSequence(stripLength, sharedClock);
HSVSequence hsvSequence(stripLength, sharedClock);
RGBSequence rgbSequence(stripLength, sharedClock);

Sequence *sequences[] = {
    &hsvSequence,
    &rgbSequence,
    &exampleSequence,
    &exampleSequence2,
    &sinWaveSequence,
};

const int SequenceBasesCount = sizeof(sequences)/sizeof(Sequence *);

LinearlyInterpolatedValueControl<int> visualizationControl( 0, SequenceBasesCount - 1);


extern "C" int _kill(int pid, int sig) {return 0;}
extern "C" int _getpid(void) { return 1;}


void loop() {
    sharedClock.tick();

    brightnessControl.tick(sharedClock, dmxValues[0]);
    visualizationControl.tick(sharedClock, dmxValues[7]);
    
    if (brightnessControl.didChange()) {
        LEDS.setBrightness(brightnessControl.value());
    }
    
    int newSequenceIndex = visualizationControl.value();
    
     
    if (newSequenceIndex != currentSequenceIndex) {
        currentSequenceIndex = newSequenceIndex;
        currentSequence = sequences[currentSequenceIndex];
        currentSequence->initialize();
        currentControls = &currentSequence->controls();
    }
    
//    auto iter = currentControls->begin();
//    for (int i = 0; i < dmxChannels && iter != currentControls->end(); ++i){
//        switch (i) {
//                // These are reserved
//            case 0:
//            case 7:
//                continue;
//            default:
//                break;
//        }
//
//        (*iter)->tick(sharedClock, dmxValues[i]);
//        ++iter;
//    }
//
    currentSequence->loop(&context);
    
    LEDS.show();
    LEDS.countFPS();
    frameNum++;
    if (frameNum % 3000 == 0) {
        Serial.print("FPS:");
        Serial.println(LEDS.getFPS());
    }
}


uint16_t dmxStartAddress = 65;

#include <DmxReceiver.h>

DmxReceiver dmx;
IntervalTimer dmxTimer;

int led = 0;
elapsedMillis elapsed;