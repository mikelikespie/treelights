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
unique_ptr <Sequence> currentSequences[stripCount];
Clock sharedClock;


void writeStartFrame();

void writeColor(const ARGB &color);

void writeEndFrame(size_t ledCount);

void writeBuffer();


vector <Context> contexts{
        Context(leds + realStripLength * 0, realStripLength, false),
        Context(leds + realStripLength * 1, realStripLength, true),
        Context(leds + realStripLength * 2, realStripLength, false),
        Context(leds + realStripLength * 3, realStripLength, true),
};

std::mt19937 gen(0);


float fftLevels[32] = {0};

int currentSequenceIndex = -1;


const std::vector<Sequence *(*)()> sequences = {
        [&gen, realStripLength, &sharedClock]() -> Sequence * {
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

  float n;
  int i;

  if (myFFT.available()) {
    // each time new FFT data is available
    // print it all to the Arduino Serial Monitor
//    Serial.print("FFT: ");
    // read the 512 FFT frequencies into 16 levels
    // music is heard in octaves, but the FFT data
    // is linear, so for the higher octaves, read
    // many FFT bins together.
    fftLevels[0] = myFFT.read(0);
    fftLevels[1] = myFFT.read(1);
    fftLevels[2] = myFFT.read(2);
    fftLevels[3] = myFFT.read(3, 4);
    fftLevels[4] = myFFT.read(5, 6);
    fftLevels[5] = myFFT.read(7, 8);
    fftLevels[6] = myFFT.read(9, 11);
    fftLevels[7] = myFFT.read(12, 14);
    fftLevels[8] = myFFT.read(15, 18);
    fftLevels[9] = myFFT.read(19, 22);
    fftLevels[10] = myFFT.read(23, 27);
    fftLevels[11] = myFFT.read(28, 33);
    fftLevels[12] = myFFT.read(34, 40);
    fftLevels[13] = myFFT.read(41, 48);
    fftLevels[14] = myFFT.read(49, 57);
    fftLevels[15] = myFFT.read(58, 67);
    fftLevels[16] = myFFT.read(68, 78);
    fftLevels[17] = myFFT.read(79, 91);
    fftLevels[18] = myFFT.read(92, 105);
    fftLevels[19] = myFFT.read(106, 121);
    fftLevels[20] = myFFT.read(122, 139);
    fftLevels[21] = myFFT.read(140, 159);
    fftLevels[22] = myFFT.read(160, 182);
    fftLevels[23] = myFFT.read(183, 208);
    fftLevels[24] = myFFT.read(209, 238);
    fftLevels[25] = myFFT.read(239, 272);
    fftLevels[26] = myFFT.read(273, 310);
    fftLevels[27] = myFFT.read(311, 353);
    fftLevels[28] = myFFT.read(354, 403);
    fftLevels[29] = myFFT.read(404, 459);
    fftLevels[30] = myFFT.read(460, 500);
    fftLevels[31] = myFFT.read(500, 511);
//    for (i = 0; i < 32; i++) {
//      n = fftLevels[i];
//      if (n >= 0.01) {
//        Serial.print(n);
//        Serial.print(" ");
//      } else {
//        Serial.print("  -  "); // don't print "0.00"
//      }
//    }
//    Serial.println();
  }


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
      Serial.println("reseted");
    }
    Serial.println("Initialized");
  }

  int contextIndex = 0;
  for (auto &currentSequence: currentSequences) {
    const std::vector<Control *> *currentControls = &currentSequence->controls();

    auto iter = currentControls->begin();
    if (currentControls->size()) {
//            ++iter;
      for (int i = 0; i < 32 && iter != currentControls->end(); ++i) {
        (*iter)->tick(sharedClock, fftLevels[i]);
        ++iter;
      }
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

