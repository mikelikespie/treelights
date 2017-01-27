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

static const int NUM_LEDS = 1 * 60;

const int stripCount = 1;
const int stripLength = NUM_LEDS;

ExampleSequence exampleSequence(stripCount, stripLength, sharedClock, HCL { 1, 0, 1 });
ExampleSequence exampleSequence2(stripCount, stripLength, sharedClock, HCL { 0.5f, 0.5f, 0.5f });

SinWaveSequence sinWaveSequence(stripCount, stripLength, sharedClock);
SinWaveSequence sinWaveSequence2(stripCount, stripLength, sharedClock);
HSVSequence hsvSequence(stripCount, stripLength, sharedClock);
RGBSequence rgbSequence(stripCount, stripLength, sharedClock);

std::vector<Sequence *>sequences = {
        &sinWaveSequence,
        &sinWaveSequence2,
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
    filter.begin(10);

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
float max_a_magnitude = 0;
float gx, gy, gz;
float mx, my, mz;
float roll = 0, pitch = 0, heading = 0;

boolean coolingDownCycle = false;

const float shockLowWatermark = 1.5;
const float shockHighWatermark = 2.0;
void loop() {
    sharedClock.tick();

    boolean shockTriggered = false;
    if (max_a_magnitude > shockHighWatermark ) {
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

    int newSequenceIndex = std::max(0, (int)(shockTriggered ? (currentSequenceIndex + 1) % sequences.size() : currentSequenceIndex));

    if (newSequenceIndex != currentSequenceIndex) {
        Serial.print("Switching sequence to ");
        Serial.print(newSequenceIndex);
        Serial.println(".");

        currentSequenceIndex = newSequenceIndex;
        currentSequence = sequences[currentSequenceIndex];

        currentSequence->initialize();
        Serial.println("Initialized");
        currentControls = &currentSequence->controls();
    }


    auto iter = currentControls->begin();
    if (iter != currentControls->end()) {

        ++iter;
        for (int i = 0; i < 16 && iter != currentControls->end(); ++i) {
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
        (*currentControls)[0]->tick(sharedClock, -ax + 0.5);
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

        float g_magnitude = sqrtf(gx * gx + gy * gy + gz * gz);
        float a_magnitude = sqrtf(ax * ax + ay * ay + az * az);

        max_a_magnitude *= 0.99;
        max_a_magnitude = std::max(max_a_magnitude, a_magnitude);

        if (imu_cnt % 100 == 0) {
            Serial.print("Orientation: ax");
            Serial.print(ax);
            Serial.print(" ay:");
            Serial.print(ay);
            Serial.print(" az:");
            Serial.print(az);
            Serial.printf(" a_magnitude:");
            Serial.print(a_magnitude);
            Serial.printf(" max_a_magnitude:");
            Serial.print(max_a_magnitude);

            Serial.print(" gx:");
            Serial.print(gx);
            Serial.print(" gy:");
            Serial.print(gy);
            Serial.print(" gz:");
            Serial.print(gz);

            Serial.println("");
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

