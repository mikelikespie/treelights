extern "C" int _kill(int pid, int sig) { return 0; }
extern "C" int _getpid(void) { return 1; }
//extern "C" int _init(void) { return 1; }

//#include <OctoWS2811.h>

//#include "FastLED.h"
#include <SPI.h>

//#include "nanoflann.hpp"
//#include </Applications/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/SPI/SPI.h>

extern "C" uint32_t _ebss;

uint32_t _ebss = 0;


static const int NUM_LEDS = 5 * 60;

inline static uint32_t h2rgb(unsigned int v1, unsigned int v2, unsigned int hue);


void writeEndFrame(size_t ledCount);

void writeStartFrame();

struct ARGB {
    uint8_t a;
    // We actually only get 5 bits
    uint8_t r, g, b;
};

inline void writeColor(const ARGB &color);

static ARGB leds[NUM_LEDS];

const int slaveSelectPin = 7;


SPISettings APA102(24000000, MSBFIRST, SPI_MODE0);


void setup() {
    Serial.begin(115200);

    // set the slaveSelectPin as an output:
    pinMode(slaveSelectPin, OUTPUT);
    // initialize SPI:
    SPI.begin();
}

void writeBuffer() {
    SPI.beginTransaction(APA102);
    digitalWrite(slaveSelectPin, HIGH);  // enable access to LEDs


    writeStartFrame();

    for (const auto &c : leds) {
        writeColor(c);
    }

    writeEndFrame(NUM_LEDS);

    digitalWrite(slaveSelectPin, LOW);
    SPI.endTransaction();
}

int cnt = 0;
void loop() {
    for (auto &led : leds) {
        led.a = 31;
        led.b = 255;
    }
    cnt++;
    if ((cnt % 10000) == 0) {
        Serial.println("hi");

    }
    //   writeBuffer();
//    delay(500);
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
    for (int i = 0; i < ledCount / 8 + 2; i++) {
        SPI.transfer(0xFF);
    }
}

