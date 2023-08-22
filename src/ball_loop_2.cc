// FFT Test
//
// Compute a 1024 point Fast Fourier Transform (spectrum analysis)
// on audio connected to the Left Line-In pin.  By changing code,
// a synthetic sine wave can be input instead.
//
// The first 40 (of 512) frequency analysis bins are printed to
// the Arduino Serial Monitor.  Viewing the raw data can help you
// understand how the FFT works and what results to expect when
// using the data to control LEDs, motors, or other fun things!
//
// This example code is in the public domain.

extern "C" int _kill(int pid, int sig) { return 0; }
extern "C" int _getpid(void) { return 1; }

#include <cstdint>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <HardwareSerial.h>
#include <usb_seremu.h>
#include <TeensyDMX.h>

#include <algorithm>

#include <usb_serial.h>

//#include </Applications/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/SPI/SPI.h>

#ifdef abs
#undef abs
#endif
#ifdef round
#undef round
#endif

#include <vector>
#include <memory>
#include "clock.h"
#include "Context.h"
#include "SequenceBase.h"
#include "Control.h"
#include "ExampleSequence.h"
#include "BurningFlambeosSequence.h"
#include "SinWaveSequence.h"
#include "ParticleEffectSequence.h"
#include "SoundHistogramSequence.h"
#include "SoundReactiveParticleEffectSequence.h"


namespace teensydmx = ::qindesign::teensydmx;

// Create the DMX receiver on Serial1.
teensydmx::Receiver dmxRx{Serial1};

using namespace std;


static const int segmentLength = 18;
static const int segmentCount = 30;

const int realStripLength = segmentLength * segmentCount;

const int stripCount = 1;
//const int stripCount = 1;
static const int totalLedCount = realStripLength * stripCount;
static ARGB leds[totalLedCount];
Clock sharedClock;


void writeStartFrame();

void writeColor(const ARGB &color);

void writeEndFrame(size_t ledCount);

void writeBuffer();


Context contexts[stripCount] = {
        Context(leds + realStripLength * 0, realStripLength, false),

};

std::mt19937 gen(0);


int currentSequenceIndex = -1;

bool seenNonZeroDmxValue = false; // Don't start setting controls until we see dmx is working, otherwise just use defaults
typedef Sequence *(*SequenceFactory)();

const SequenceFactory sequences[] = {
        []() -> Sequence * { return new SinWaveSequence(realStripLength, sharedClock, .5); },
        []() -> Sequence * { return new ParticleEffectSequence(&gen, realStripLength, sharedClock); },
//        []() -> Sequence * { return new BurningFlambeosSequence(realStripLength, sharedClock); },
//        []() -> Sequence * { return new HSVSequence(realStripLength, sharedClock); },
//        []() -> Sequence * { return new RGBSequence(realStripLength, sharedClock); },
};

const int sequenceCount = sizeof(sequences) / sizeof(SequenceFactory);

// we want to cache sequences after they've been initialized
// so we don't have to reinitialize them every time we switch to them. Mostly to work around a crash when deleting.
// Also if we don't realloc we wont fragment memory.
typedef Sequence *SequencesForAllStrips[stripCount];

SequencesForAllStrips instantiatedSequences[sequenceCount] = {{nullptr}};

LinearlyInterpolatedValueControl<int> visualizationControl(0, sequenceCount - 1, 0);


const int slaveSelectPin = 7;

const int myInput = AUDIO_INPUT_LINEIN;

AudioInputI2S audioInput;         // audio shield: mic or line-in
AudioAnalyzeFFT1024 myFFT;

// Connect either the live input or synthesized sine wave
__attribute__((unused)) AudioConnection patchCord1(audioInput, 0, myFFT, 0);

AudioControlSGTL5000 audioShield;


SoundDataBuffer soundDataBuffer;

const int8_t DMX_CHANNEL_COUNT = 16;

// Last 16 of 192 channels. 0-indexed.
const int DMX_START_CHANNEL = 0;

const int8_t DMX_VISUALIZATION_CONTROL_NUM = 7;

uint8_t dmxValues[DMX_CHANNEL_COUNT] = {0};

// Log fps every 10 seconds
const unsigned long FPS_LOG_EVERY_MS = 10 * 1000;
elapsedMillis elapsed;
int frameCount = 0;

void setup() {
//  while (!Serial); // wait for serial monitor open
//  if (CrashReport) {
//    Serial.print(CrashReport);
//    delay(2);
//  }
  elapsed = elapsedMillis();

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);

  myFFT.windowFunction(AudioWindowHanning1024);

  // Start serial
  Serial.begin(115200);

  // set the slaveSelectPin as an output:
  pinMode(slaveSelectPin, OUTPUT);
  // initialize SPI:
  SPI.begin();

  digitalWrite(slaveSelectPin, HIGH);  // enable access to LEDs

  dmxRx.begin();

  // Delay a bit so we can have a dmx value by the time we start
  delay(500);
}


void loop() {
  sharedClock.tick();

  // Read DMX data
  // TODO: only read if there's new data
  for (int i = 0; i < DMX_CHANNEL_COUNT; i++) {
    uint8_t newValue = dmxRx.get(DMX_START_CHANNEL + i + 1);
    if (!seenNonZeroDmxValue && newValue != 0) {
      seenNonZeroDmxValue = true;
    }
//    if (newValue != dmxValues[i]) {
//      Serial.print("DMX ");
//      Serial.print(i);
//      Serial.print(" to ");
//      Serial.println(newValue);
//    }
    dmxValues[i] = newValue;
  }

  visualizationControl.tick(sharedClock,
                            (float) dmxValues[
                                    DMX_VISUALIZATION_CONTROL_NUM] / 255.0f);

  int newSequenceIndex = visualizationControl.value();

  if (newSequenceIndex != currentSequenceIndex) {
    Serial.print("x to ");
    Serial.print(newSequenceIndex);
    Serial.println(".");
    Serial.println("initializing");

    currentSequenceIndex = newSequenceIndex;
    auto &currentSequences = instantiatedSequences[currentSequenceIndex];

    for (auto &currentSequence: currentSequences) {
      if (currentSequence == nullptr) {
        auto newSequence = sequences[currentSequenceIndex]();
        currentSequence = newSequence;
        newSequence->initialize();
        continue;
      }
      currentSequence->updateSoundData(soundDataBuffer);
    }
  }


  if (myFFT.available()) {
    for (int i = 0; i < SOUND_BUFFER_BIN_COUNT; i++) {
      soundDataBuffer[i] = myFFT.read(i);
    }

    for (auto &currentSequence: instantiatedSequences[currentSequenceIndex]) {
      currentSequence->updateSoundData(soundDataBuffer);
    }
  }

  int contextIndex = 0;
  for (auto &currentSequence: instantiatedSequences[currentSequenceIndex]) {
    const std::vector<Control *> *currentControls = &currentSequence->controls();

    auto iter = currentControls->begin();
    for (int i = 0; i < DMX_CHANNEL_COUNT && iter != currentControls->end(); ++i, ++iter) {
      if (seenNonZeroDmxValue) {
        (*iter)->tick(sharedClock, (float) dmxValues[i] / 255.0f);
      } else {
        (*iter)->tick(sharedClock);
      }
    }

    currentSequence->loop(&contexts[contextIndex]);
    contextIndex++;
  }

  writeBuffer();

  frameCount++;
  unsigned long elapsedLong = elapsed;
  if (elapsedLong > FPS_LOG_EVERY_MS) {
    Serial.print("FPS: ");
    Serial.println((float) frameCount / (float) (elapsedLong) * 1000.0f);
    elapsed = elapsedMillis();
    frameCount = 0;
  }
}


SPISettings APA102(2800000 * 2, MSBFIRST, SPI_MODE0);

void writeBuffer() {
  SPI.beginTransaction(APA102);


  writeStartFrame();

  for (const auto &c: leds) {
    writeColor(c);
  }

  writeEndFrame(totalLedCount);

  SPI.endTransaction();
//    digitalWrite(slaveSelectPin, LOW);
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

