#include <ArduinoBLE.h>

const int samplingInterval = 5;
unsigned long prevMillis = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1);
  }

  Serial.println("Bluetooth® Low Energy Central scan");
  

  // start scanning for peripheral
  BLE.setLocalName("6733ContactTracing");
  BLE.advertise();
  BLE.scan();

  
}

void loop() {

  unsigned long currMillis = millis();
  if (currMillis - prevMillis >= samplingInterval) {
    // check if a peripheral has been discovered
    BLEDevice peripheral = BLE.available();

    if (peripheral) {
      // discovered a peripheral
      Serial.println("Discovered a peripheral");
      Serial.println("-----------------------");

      // print address
      Serial.print("Address: ");
      Serial.println(peripheral.address());

      // print the local name, if present
      if (peripheral.hasLocalName()) {
        Serial.print("Local Name: ");
        Serial.println(peripheral.localName());
      }

      // print the advertised service UUIDs, if present
      if (peripheral.hasAdvertisedServiceUuid()) {
        Serial.print("Service UUIDs: ");
        for (int i = 0; i < peripheral.advertisedServiceUuidCount(); i++) {
          Serial.print(peripheral.advertisedServiceUuid(i));
          Serial.print(" ");
        }
        Serial.println();
      }

      // print the RSSI
      Serial.print("RSSI: ");
      Serial.println(peripheral.rssi());

      Serial.println();
    }
    prevMillis = millis();
  }
  
}