extern "C" int _kill(int pid, int sig) { return 0; }
extern "C" int _getpid(void) { return 1; }
//extern "C" int _init(void) { return 1; }

//#include <OctoWS2811.h>

//#include "FastLED.h"
#include <SPI.h>
#include <usb_serial.h>
#include <algorithm>

using namespace std;

//#include "nanoflann.hpp"
#include </Applications/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/SPI/SPI.h>
#include </Applications/Arduino.app/Contents/Java/hardware/teensy/avr/libraries/NXPMotionSense/NXPMotionSense.h>
#include "clock.h"
#include "Context.h"
#include "SequenceBase.h"
#include "Control.h"
#include "ExampleSequence.h"
#include "SinWaveSequence.h"

extern "C" uint32_t _ebss;

uint32_t _ebss = 0;

Clock sharedClock;


int currentSequenceIndex = -1;
Sequence *currentSequence = nullptr;
const std::vector<Control *> *currentControls = nullptr;

IdentityValueControl brightnessControl;

static const int NUM_LEDS = 5 * 60;

const int stripCount = 1;
const int stripLength = NUM_LEDS;

ExampleSequence exampleSequence(stripCount, stripLength, sharedClock, HCL { 1, 0, 1 });
ExampleSequence exampleSequence2(stripCount, stripLength, sharedClock, HCL { 0.5f, 0.5f, 0.5f });

SinWaveSequence sinWaveSequence(stripCount, stripLength, sharedClock);
HSVSequence hsvSequence(stripCount, stripLength, sharedClock);
RGBSequence rgbSequence(stripCount, stripLength, sharedClock);

Sequence *sequences[] = {
        &sinWaveSequence,
        &hsvSequence,
        &rgbSequence,
        &exampleSequence,
        &exampleSequence2,
};

const int SequenceBasesCount = sizeof(sequences)/sizeof(Sequence *);

LinearlyInterpolatedValueControl<int> visualizationControl( 0, SequenceBasesCount - 1);


void writeEndFrame(size_t ledCount);

void writeStartFrame();


inline void writeColor(const ARGB &color);

void delayStart();

static ARGB leds[NUM_LEDS];

Context context(leds, stripLength, stripCount);

const int slaveSelectPin = 7;

SPISettings APA102(18000000, MSBFIRST, SPI_MODE0);

NXPMotionSense imu;
NXPSensorFusion filter;

void setup() {
    Serial.begin(115200);

//    delayStart();


    imu.begin();
    filter.begin(100);

    // set the slaveSelectPin as an output:
    pinMode(slaveSelectPin, OUTPUT);
    // initialize SPI:
    SPI.begin();

    digitalWrite(slaveSelectPin, HIGH);  // enable access to LEDs

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

int cnt = 0;
int imu_cnt = 0;

int idx = 0;
float ax, ay, az;
float gx, gy, gz;
float mx, my, mz;
float roll = 0, pitch = 0, heading = 0;

void loop() {
    sharedClock.tick();

    brightnessControl.tick(sharedClock, 0);
    visualizationControl.tick(sharedClock, 0);

    int newSequenceIndex = visualizationControl.value();

    if (newSequenceIndex != currentSequenceIndex) {
        currentSequenceIndex = newSequenceIndex;
        currentSequence = sequences[currentSequenceIndex];
        currentSequence->initialize();
        currentControls = &currentSequence->controls();
    }

    auto iter = currentControls->begin();
    for (int i = 0; i < 16 && iter != currentControls->end(); ++i){
//        switch (i) {
//                // These are reserved
//            case 0:
//            case 7:
//                continue;
//            default:
//                break;
//        }

        (*iter)->tick(sharedClock, 0.5);
        ++iter;
    }

    if (imu.available()) {
        // Read the motion sensors
        imu.readMotionSensor(ax, ay, az, gx, gy, gz, mx, my, mz);

        // Update the SensorFusion filter
        filter.update(gx, gy, gz, ax, ay, az, mx, my, mz);

        // print the heading, pitch and roll
        roll = filter.getRoll();
        pitch = filter.getPitch();
        heading = filter.getYaw();

        if (imu_cnt % 1000 == 0) {
            Serial.print("Orientation: ");
            Serial.print(heading);
            Serial.print(" ");
            Serial.print(pitch);
            Serial.print(" ");
            Serial.println(roll);
        }

        imu_cnt++;
    }


    currentSequence->loop(&context);

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

