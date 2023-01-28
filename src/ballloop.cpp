extern "C" int _kill(int pid, int sig) { return 0; }
extern "C" int _getpid(void) { return 1; }
//extern "C" int _init(void) { return 1; }

//#include <OctoWS2811.h>

//#include "FastLED.h"
#include <SPI.h>
#include <usb_serial.h>
#include <algorithm>
#include <RF24/RF24.h>

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = 0;


/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(47, 43);
/**********************************************************/

byte addresses[][6] = {"1Node", "2Node"};

using namespace std;

//#include "nanoflann.hpp"
#include </Applications/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/SPI/SPI.h>
#include </Applications/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/NXPMotionSense/NXPMotionSense.h>
#include <memory>
#include "clock.h"
#include "Context.h"
#include "SequenceBase.h"
#include "Control.h"
#include "ExampleSequence.h"
#include "ParticleEffectSequence.h"
#include "SinWaveSequence.h"

extern "C" uint32_t _ebss;

uint32_t _ebss = 0;

Clock sharedClock;


const int stripCount = 1;
int currentSequenceIndex = -1;
unique_ptr<Sequence> currentSequences[stripCount];

static const int segmentLength = 18;
static const int segmentCount = 30;
static const int ledCount = segmentLength * segmentCount;

static const int NUM_LEDS = ledCount;

const int stripLength = NUM_LEDS;

static ARGB leds[NUM_LEDS];

const int realStripLength = NUM_LEDS / 4;

IdentityValueControl brightnessControl;
std::mt19937 gen(0);
//SinWaveSequence sinWaveSequence(stripLength, sharedClock);
//ParticleEffectSequence particleEffectSequence(stripLength, sharedClock);
HSVSequence hsvSequence(stripLength, sharedClock);
RGBSequence rgbSequence(stripLength, sharedClock);

std::vector<Sequence *(*)()> sequences = {
//        [&]() -> Sequence * { return new SinWaveSequence(ledCount, sharedClock); },
};

const int SequenceBasesCount = sizeof(sequences) / sizeof(Sequence *);

LinearlyInterpolatedValueControl<int> visualizationControl(0, SequenceBasesCount - 1);


void writeEndFrame(size_t ledCount);

void writeStartFrame();


inline void writeColor(const ARGB &color);

void delayStart();

vector<Context> contexts{
        Context(leds, stripLength, false),
//        Context(leds + realStripLength * 1, realStripLength, false),
//        Context(leds + realStripLength * 2, realStripLength, true),
//        Context(leds + realStripLength * 3, realStripLength, false)

};

const int slaveSelectPin = 7;

SPISettings APA102(2200000, MSBFIRST, SPI_MODE0);

NXPMotionSense imu;
NXPSensorFusion filter;

void setup() {
  Serial.begin(115200);

  imu.begin();
  filter.begin(10);

  // set the slaveSelectPin as an output:
  pinMode(slaveSelectPin, OUTPUT);
  // initialize SPI:
  SPI.begin();

  digitalWrite(slaveSelectPin, HIGH);  // enable access to LEDs

  radio.begin();

  radio.setPALevel(RF24_PA_MAX);
  radio.openReadingPipe(1, addresses[1]);
  radio.startListening();
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
}

int cnt = 0;
int imu_cnt = 0;

int idx = 0;
float ax, ay, az;
float max_a_magnitude = 0;
float gx, gy, gz;
float mx, my, mz;
float roll = 0, pitch = 0, heading = 0;

boolean coolingDownCycle = false;

const float shockLowWatermark = 1.5;
const float shockHighWatermark = 4.0;

unsigned char dmxBuffer[32];

void loop() {
  sharedClock.tick();

  boolean shockTriggered = false;
  if (max_a_magnitude > shockHighWatermark) {
    if (!coolingDownCycle) {
      shockTriggered = true;
      coolingDownCycle = true;
    }
  } else if (max_a_magnitude < shockLowWatermark && coolingDownCycle) {
    coolingDownCycle = false;
  }

  brightnessControl.tick(sharedClock, 0);
  visualizationControl.tick(sharedClock, 0);

//    int newSequenceIndex = visualizationControl.value();

  int newSequenceIndex = std::max(0, (int) (shockTriggered ? (currentSequenceIndex + 1) % sequences.size()
                                                           : currentSequenceIndex));


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

    if (radio.available()) {
      // Variable for the received timestamp
      while (radio.available()) {                                   // While there is data ready
        radio.read(dmxBuffer, (int) sizeof(dmxBuffer));             // Get the payload

        Serial.println(F("*** Read data"));
      }

      Serial.println(F("*** Got dmx data\n"));

      for (size_t i = 0; i < sizeof(dmxBuffer); i++) {
        printf("%d=%d,", (int) (i + 1), (int) dmxBuffer[i]);
        if (i % 16 == 15) {
          Serial.println(F(""));
        }
      }
      Serial.println(F(""));
    }

    auto iter = currentControls->begin();
    if (currentControls->size()) {
//            ++iter;
      for (int i = 0; i < 16 && iter != currentControls->end(); ++i) {
        (*iter)->tick(sharedClock, dmxBuffer[i] / 255.0f);
        ++iter;
      }
      //            (*currentControls)[0]->tick(sharedClock, -ax + 0.5);
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
  for (size_t i = 0; i < 4; i++) {
    SPI.transfer(0xFF);
  }
}

