/*
-   TESTING CODE - CENTRAL DEVICE
-   This code is written for the purposes of conducting our experiments.
-   It should be uploaded to the central device, i.e. the one that we 
    are moving around.
*/


#include <ArduinoBLE.h>

const char * nameThisDevice = "testing_device";

void setup() {
  Serial.begin(9600);

  pinMode(LEDG, OUTPUT);
  digitalWrite(LEDG, 1);


  if (!BLE.begin()) {
    Serial.println("Starting Bluetooth failed!");
    while(1);
  }

  BLE.setLocalName(nameThisDevice);
  BLE.advertise();
}

void loop() {
  digitalWrite(LEDG, 0);
}
