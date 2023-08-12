#include <SPI.h>
#include <usb_serial.h>

#include <TeensyDMX.h>


namespace teensydmx = ::qindesign::teensydmx;

// Create the DMX receiver on Serial1.
teensydmx::Receiver dmxRx{Serial1};


// The last values received on channels 10-12, initialized to zero.
uint8_t rgb[3]{0};


const uint8_t data[16] = {0};

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {
    // Wait for initialization to complete or a time limit
  }
  delay(1000);  // Wait for the serial port to settle
  Serial.println("Starting BasicReceive.");

  // Start the receiver
  dmxRx.begin();


}


void loop() {
  // put your main code here, to run repeatedly:
  bool hasChange = false;
  for (int i = 0; i < 16; i++) {
    int lastValue = dmxRx.get(i);
    if (lastValue != data[i]) {
      hasChange = true;
      data[i] = lastValue;
    }
  }

  if (hasChange) {
    Serial.print("change");
    for (unsigned char i: data) {
      Serial.printf("%d ", i);
    }
    Serial.println();
  }

//  delay(1000);

}
