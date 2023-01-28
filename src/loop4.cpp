extern "C" int _kill(int pid, int sig) { return 0; }
extern "C" int _getpid(void) { return 1; }

#include <algorithm>

#include <SPI.h>
#include <usb_serial.h>

#include </Applications/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/SPI/SPI.h>

#ifdef abs
#undef abs
#endif

#include <memory>
#include "clock.h"
#include "Context.h"
#include "SequenceBase.h"
#include "Control.h"
#include "ExampleSequence.h"
#include "BurningFlambeosSequence.h"
#include "SinWaveSequence.h"
#include "ParticleEffectSequence.h"

using namespace std;

extern "C" uint32_t _ebss;

uint32_t _ebss = 0;

Clock sharedClock;


const int stripCount = 6 * 4;
int currentSequenceIndex = -1;
unique_ptr<Sequence> currentSequences[stripCount];

static const int NUM_LEDS = 60 * 5 * 4;

const int stripLength = NUM_LEDS;

float max_a_magnitude = 0;

static ARGB leds[NUM_LEDS];

const int realStripLength = NUM_LEDS / stripCount;

IdentityValueControl brightnessControl;
std::mt19937 gen(0);

std::vector<Sequence *(*)()> sequences = {
//        [&]()-> Sequence *{ return new ParticleEffectSequence(&gen, realStripLength, sharedClock); },
//        [&]() -> Sequence * { return new BurningFlambeosSequence(realStripLength, sharedClock); },
};

const int SequenceBasesCount = sizeof(sequences) / sizeof(Sequence *);

LinearlyInterpolatedValueControl<int> visualizationControl(0, SequenceBasesCount - 1);


void writeEndFrame(size_t ledCount);

void writeStartFrame();


inline void writeColor(const ARGB &color);

void delayStart();

vector<Context> contexts{
        Context(leds + realStripLength * 0, realStripLength, true),
        Context(leds + realStripLength * 1, realStripLength, false),
        Context(leds + realStripLength * 2, realStripLength, true),
        Context(leds + realStripLength * 3, realStripLength, false),
        Context(leds + realStripLength * 4, realStripLength, true),
        Context(leds + realStripLength * 5, realStripLength, false),
        Context(leds + realStripLength * 6, realStripLength, true),
        Context(leds + realStripLength * 7, realStripLength, false),
        Context(leds + realStripLength * 8, realStripLength, true),
        Context(leds + realStripLength * 9, realStripLength, false),
        Context(leds + realStripLength * 10, realStripLength, true),
        Context(leds + realStripLength * 11, realStripLength, false),
        Context(leds + realStripLength * 12, realStripLength, true),
        Context(leds + realStripLength * 13, realStripLength, false),
        Context(leds + realStripLength * 14, realStripLength, true),
        Context(leds + realStripLength * 15, realStripLength, false),
        Context(leds + realStripLength * 16, realStripLength, true),
        Context(leds + realStripLength * 17, realStripLength, false),
        Context(leds + realStripLength * 18, realStripLength, true),
        Context(leds + realStripLength * 19, realStripLength, false),
        Context(leds + realStripLength * 20, realStripLength, true),
        Context(leds + realStripLength * 21, realStripLength, false),
        Context(leds + realStripLength * 22, realStripLength, true),
        Context(leds + realStripLength * 23, realStripLength, false),
};

const int slaveSelectPin = 7;

SPISettings APA102(2800000, MSBFIRST, SPI_MODE0);

void setup() {
  Serial.begin(115200);

  // set the slaveSelectPin as an output:
  pinMode(slaveSelectPin, OUTPUT);
  // initialize SPI:
  SPI.begin();

  digitalWrite(slaveSelectPin, HIGH);  // enable access to LEDs
//  delayStart();

}

void delayStart() {
  delay(1000);
  Serial.println("5");

  delay(1000);
  Serial.println("4");

  delay(1000);
  Serial.println("3");

  delay(1000);
  Serial.println("2");

  delay(1000);
  Serial.println("1");
}

void writeBuffer() {
  SPI.beginTransaction(APA102);


  writeStartFrame();

  for (const auto &c : leds) {
    writeColor(c);
  }

  writeEndFrame(NUM_LEDS);

  SPI.endTransaction();
//    digitalWrite(slaveSelectPin, LOW);
}


boolean coolingDownCycle = false;

const float shockLowWatermark = 1.5;
const float shockHighWatermark = 4.0;

void loop() {
  sharedClock.tick();

  brightnessControl.tick(sharedClock, 0);
  visualizationControl.tick(sharedClock, 0);

  int newSequenceIndex = 0;


  if (newSequenceIndex != currentSequenceIndex) {
    Serial.print("Switching sequence to ");
    Serial.print(newSequenceIndex);
    Serial.println(".");

    currentSequenceIndex = newSequenceIndex;
    for (auto &currentSequence : currentSequences) {
      currentSequence.reset(sequences[currentSequenceIndex]());
      currentSequence->initialize();
    }
    Serial.println("Initialized");
  }

  int contextIndex = 0;
  for (auto &currentSequence : currentSequences) {
    const std::vector<Control *> *currentControls = &currentSequence->controls();

    auto iter = currentControls->begin();
    if (currentControls->size()) {
//            ++iter;
      for (int i = 0; i < 16 && iter != currentControls->end(); ++i) {
        (*iter)->tick(sharedClock);
        ++iter;
      }

//            (*currentControls)[0]->tick(sharedClock, 0.4);
    }

    currentSequence->loop(&contexts[contextIndex]);
    contextIndex++;
  }

  writeBuffer();
}

inline void writeColor(const ARGB &color) {
  uint8_t brightness = 31;
  if (color.a < 31) {
    brightness = color.a;
  }

  SPI.transfer(0xE0 | brightness);
  SPI.transfer(color.b);
  SPI.transfer(color.g);
  SPI.transfer(color.r);
}

void writeStartFrame() {
  for (int i = 0; i < 4; i++) {
    SPI.transfer(0x00);
  }
}

void writeEndFrame(size_t ledCount) {
  for (size_t i = 0; i < ledCount / 8 + 2; i++) {
    SPI.transfer(0xFF);
  }
}

