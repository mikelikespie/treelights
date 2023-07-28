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

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <HardwareSerial.h>
#include <usb_seremu.h>

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


using namespace std;

const int stripCount = 4;
const int realStripLength = 118;
static const int totalLedCount = realStripLength * stripCount;
static ARGB leds[totalLedCount];
unique_ptr<Sequence> currentSequences[stripCount];
Clock sharedClock;


void writeStartFrame();

void writeColor(const ARGB &color);

void writeEndFrame(size_t ledCount);

void writeBuffer();


vector<Context> contexts{
        Context(leds + realStripLength * 0, realStripLength, false),
        Context(leds + realStripLength * 1, realStripLength, false),
        Context(leds + realStripLength * 2, realStripLength, false),
        Context(leds + realStripLength * 3, realStripLength, false),
};

std::mt19937 gen(0);


int currentSequenceIndex = -1;


const std::vector<Sequence *(*)()> sequences = {
        []() -> Sequence * {
          return new SoundHistogramSequence(realStripLength, sharedClock);
          return new HSVSequence(realStripLength, sharedClock);
          return new ParticleEffectSequence(&gen, realStripLength, sharedClock);
        },
//        [&]() -> Sequence * { return new BurningFlambeosSequence(realStripLength, sharedClock); },
};

const int slaveSelectPin = 7;

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputI2S audioInput;         // audio shield: mic or line-in
AudioSynthWaveformSine sinewave;
AudioAnalyzeFFT1024 myFFT;
AudioOutputI2S audioOutput;        // audio shield: headphones & line-out

// Connect either the live input or synthesized sine wave
AudioConnection patchCord1(audioInput, 0, myFFT, 0);

AudioControlSGTL5000 audioShield;


SoundDataBuffer soundDataBuffer;

void setup() {

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);

  // Configure the window algorithm to use
  myFFT.windowFunction(AudioWindowHanning1024);
  //myFFT.windowFunction(NULL);

  // Create a synthetic sine wave, for testing
  // To use this, edit the connections above
  sinewave.amplitude(0.8);
  sinewave.frequency(1034.007);


  Serial.begin(115200);

  // set the slaveSelectPin as an output:
  pinMode(slaveSelectPin, OUTPUT);
  // initialize SPI:
  SPI.begin();

  digitalWrite(slaveSelectPin, HIGH);  // enable access to LEDs
}


void loop() {
  sharedClock.tick();

  int i;


  int newSequenceIndex = 0;


  if (newSequenceIndex != currentSequenceIndex) {
    Serial.print("x to ");
    Serial.print(newSequenceIndex);
    Serial.println(".");
    Serial.println("initializing");

    currentSequenceIndex = newSequenceIndex;
    for (auto &currentSequence: currentSequences) {
      Serial.println("resetting");

      currentSequence.reset(sequences[currentSequenceIndex]());
      currentSequence->initialize();
      currentSequence->updateSoundData(soundDataBuffer);
      Serial.println("reseted");
    }
    Serial.println("Initialized");
  }


  if (myFFT.available()) {
    // each time new FFT data is available
    // print it all to the Arduino Serial Monitor
//    Serial.print("FFT: ");
    // read the 512 FFT frequencies into 16 levels
    // music is heard in octaves, but the FFT data
    // is linear, so for the higher octaves, read
    // many FFT bins together.
    for (i = 0; i < SOUND_BUFFER_BIN_COUNT; i++) {
      soundDataBuffer[i] = myFFT.read(i);
    }

    for (auto &currentSequence: currentSequences) {
      currentSequence->updateSoundData(soundDataBuffer);
    }
  }


  int contextIndex = 0;
  for (auto &currentSequence: currentSequences) {
    const std::vector<Control *> *currentControls = &currentSequence->controls();

//    auto iter = currentControls->begin();
    if (currentControls->size()) {
//            ++iter;
//      for (int i = 0; i < 32 && iter != currentControls->end(); ++i) {
//        (*iter)->tick(sharedClock, fftLevels[i]);
//        ++iter;
//      }
    }

    currentSequence->loop(&contexts[contextIndex]);
    contextIndex++;
  }

  writeBuffer();

}


SPISettings APA102(2800000, MSBFIRST, SPI_MODE0);

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

