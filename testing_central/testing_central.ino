/*
-   TESTING CODE - CENTRAL DEVICE
-   This code is written for the purposes of conducting our experiments.
-   It should be uploaded to the central device, i.e. the one that we 
    are moving around.
*/


#include <ArduinoBLE.h>

const char * periphName = "testing_periph";

BLEDevice periph;

void setup() {
  Serial.begin(9600);

  pinMode(LEDG, OUTPUT);
  digitalWrite(LEDG, 1);


  if (!BLE.begin()) {
    Serial.println("Starting Bluetooth failed!");
    while(1);
  }

  // Scan for peripheral device which will be connected to the laptop
  BLE.scanForName(periphName);
  Serial.println("Scanning for peripheral");
}

void loop() {
  BLEDevice periph = BLE.available();

  if (periph) {
    Serial.println("Found peripheral: " + periph.address());
    while (1) {
      while (!periph.connected()) {
        digitalWrite(LEDR, 0);
        digitalWrite(LEDG, 1);
        periph.connect();
        Serial.println("Connecting to peripheral: " + periph.address());
      }
      digitalWrite(LEDR, 1);
      digitalWrite(LEDG, 0);
    }
  } else {
    BLE.scanForName(periphName);
  }
}
