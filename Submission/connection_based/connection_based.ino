// COMP6733 PROJECT
// GROUP: FREE FOR ALL

// Code was inspired by https://github.com/arduino-libraries/ArduinoBLE/blob/master/examples/Central/Scan/Scan.ino
#include <ArduinoBLE.h>
#include <TimeLib.h>
#include <Arduino_LSM9DS1.h>
#include "Arduino_BMI270_BMM150.h"
#include <vector>
#include <array>


#define TIME_HEADER "T"
#define TIME_REQUEST 7

const uint samplingInterval = 1000;
const uint sampleSize = 15;
const uint contactDist = 200; // cm
const char * deviceName = "6733ContactTracing";

// Module control: turn these features on or off
const bool timestampingOn = false;
const bool buzzerOn = true;

const int buzzerPin = 10;
const int resetPin = 8;
const uint resetInterval = 10000;


// Manufacturer's Data
const uint8_t rssiAt2m = 67;
const uint8_t manufactData[1] = {rssiAt2m};

// Array for storing other device's manufacturer's data
uint8_t periphManuData[1];


// Used for looping frequency
unsigned long prevMillis = 0;

const uint adjustmentFactor = 1;
float m = 0.0806;
float c = 44.4;


uint errorCount = 0;
const uint maxErrors = 3;

bool isCentral = false;

// Keep a measure of the device's angle
int deg[2];

BLEService distService("cb73b405-7ed2-4f7c-a0ed-99150971e553");
BLEUnsignedIntCharacteristic rssiCalCharacteristic("7d15b7a8-9d0a-47cc-8a1b-695e9dbdbf30", BLERead | BLEWrite);


void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(resetPin, OUTPUT);

  digitalWrite(LED_BUILTIN, 0);
  digitalWrite(LEDR, 1);
  digitalWrite(LEDG, 1);
  digitalWrite(LEDB, 1);
  digitalWrite(resetPin, 1);
  noTone(buzzerPin);
  
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  // Set manufacturer's calibration for RSSI at 2m
  BLEAdvertisingData manufacturerData;
  manufacturerData.setManufacturerData(manufactData, sizeof(manufactData));
  BLE.setAdvertisingData(manufacturerData);


  BLE.setAdvertisedService(distService);
  distService.addCharacteristic(rssiCalCharacteristic);
  BLE.addService(distService);
  rssiCalCharacteristic.writeValue(0);

  // BLE.setEventHandler(BLEConnected, centralConnected);
  rssiCalCharacteristic.setEventHandler(BLEWritten, degCharacteristicWritten);

  // start scanning for peripheral
  BLE.setLocalName(deviceName);

  // Set the current time for contact time
  if (timestampingOn) {
    setCurrentTime();
  }

  BLE.advertise();
  BLE.scanForName(deviceName);

  Serial.println("BLE started");
  Serial.println("----------------------------------");
}

void loop() {
  BLE.poll();
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    Serial.println("----------------------------------------------------------");
    Serial.println("Found peripheral");
    BLE.stopScan();
    isCentral = true;

    uint measuredRssi = 100;
    uint manufacturerRssi = 100;
    if (peripheral.connect()) {
      Serial.println("Connected to peripheral: " + peripheral.address());
      peripheral.manufacturerData(periphManuData, 1);
      manufacturerRssi = periphManuData[0];
      Serial.print("Manufacturer's calibrated RSSI at 2m: ");
      Serial.println(periphManuData[0]);
      measuredRssi = rssiAvg(peripheral);

      if (peripheral.discoverAttributes()) {
        BLECharacteristic degPeriphCharacteristic = peripheral.characteristic("7d15b7a8-9d0a-47cc-8a1b-695e9dbdbf30");
        bool degStatus = degPeriphCharacteristic.writeValue(byte(rssiAt2m));
        Serial.print("DEG Characteristic write status: ");
        Serial.println(degStatus);
      } else {
        peripheral.disconnect();
      }

      // To avoid hanging here due to concurrency, set a time limit
      while(peripheral.connected()) {
        digitalWrite(LEDR, 0);
        if (millis() > resetInterval) {
          BLE.disconnect();
        }
      };
      digitalWrite(LEDR, 1);

    } else {
      measuredRssi = peripheral.rssi() * -1;
    }

    uint dist = distance(measuredRssi);
    Serial.println("Device: " + peripheral.address() + ", Measured RSSI: " + measuredRssi + ", Distance(approx.): " + dist);
    
    // checkContactWithDistance(dist, peripheral);
    checkContactWithManufacturerRSSI(manufacturerRssi, measuredRssi, peripheral);

    Serial.println("Disconnected from peripheral: " + peripheral.address());
    Serial.println("----------------------------------------------------------");

    BLE.scanForName(deviceName);
    isCentral = false;
  }
  
  if (millis() > resetInterval) {
    Serial.println("Resetting");
    digitalWrite(resetPin, 0);
  } else if (millis() - prevMillis > samplingInterval) {
    BLE.stopAdvertise();
    BLE.stopScan();
    BLE.advertise();
    BLE.scanForName(deviceName);
    prevMillis = millis();
  }
}

uint rssiAvg(BLEDevice device) {
  uint currSampleSize = 0;
  uint rssiSum = 0;

  // Collect rssi samples until the required sample size is reached
  while (currSampleSize < sampleSize) {
    if (millis() > resetInterval) {
      BLE.disconnect();
    }

    uint rssi = device.rssi() * - 1;
    if (rssi > 0) {
      rssiSum += rssi;
      currSampleSize++;
    }
  }

  return rssiSum / currSampleSize;
}

uint distance(uint rssi) {
  // 2PM Measurement: m = 9.909, c = 58.234
  uint dist = max((rssi - 58.234) / 9.909, 0) * 100 * adjustmentFactor;
  return dist;
}


void checkContactWithDistance(uint dist, BLEDevice device) {
  if (dist < contactDist) {
    alertContact();
    recordContact(device);
  } else {
    digitalWrite(LED_BUILTIN, 0);
    noTone(buzzerPin);
  }  
}


void checkContactWithManufacturerRSSI(uint manufacturerRSSI, uint measuredRSSI, BLEDevice device) {
  if (measuredRSSI <= manufacturerRSSI) {
    alertContact();
    recordContact(device);
  } else {
    digitalWrite(LED_BUILTIN, 0);
    noTone(buzzerPin);
  }
}



void alertContact() {
  digitalWrite(LED_BUILTIN, 1);
  
  // turn on buzzer if module switched on
  if (buzzerOn) {
    tone(buzzerPin, 100);
  }

  delay(50);
  digitalWrite(LED_BUILTIN, 0);
  noTone(buzzerPin);
}

void recordContact(BLEDevice device) {
  Serial.println("CONTACT WITH ADDRESS: " + device.address());
  if (timestampingOn) {
    Serial.print("Contact Time: ");
    digitalClockDisplay();
  }
}


//----------------Clock timing Stuff----------------//
void digitalClockDisplay() {
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.println(year());  
}

void printDigits(int digits){
  Serial.print(":");
  if (digits < 10){
    Serial.print('0');
  }
  Serial.print(digits);
}

void setCurrentTime(){
  unsigned long pcTime;
  // 1689225031 = 13/07/2023 3:10pm
  const unsigned long DEFAULT_TIME = 1689225031;

  Serial.println("Waiting for current time input:");
  while (true){
    if (Serial.find(TIME_HEADER)){
      pcTime = Serial.parseInt();
      setTime(pcTime);
      break;
    }
  }

}


void degCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  if (isCentral) {
    central.disconnect();
  } else {
    Serial.println("----------------------------------------------------------");
    Serial.println("Connected with central: " + central.address());


    uint centralManufacturerRssi = rssiCalCharacteristic.value();
    Serial.print("Manufacturer's calibrated RSSI at 2m: ");
    Serial.println(centralManufacturerRssi);
    
    uint measuredRssi = rssiAvg(central);
    uint dist = distance(measuredRssi);
    Serial.println("Central Device: " + central.address() + ", Central Measured RSSI: " + measuredRssi + ", Central Distance(approx.): " + dist);

    // checkContactWithDistance(dist, central);
    checkContactWithManufacturerRSSI(centralManufacturerRssi, measuredRssi, central);


    central.disconnect();
    Serial.println("Disconnected from central: " + central.address());
    Serial.println("----------------------------------------------------------");
  }
}
