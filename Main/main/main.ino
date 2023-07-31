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
const uint adjustmentFactor = 1;
const uint contactDist = 200; // cm
const char * deviceName = "6733ContactTracing";

// Module control: turn these features on or off
const bool timestampingOn = false;
const bool buzzerOn = false;

const int buzzerPin = 10;
const int resetPin = 8;

// Used for looping frequency
unsigned long prevMillis = 0;

uint errorCount = 0;
const uint maxErrors = 3;

// Keep a measure of the device's angle
int deg[2];

BLEService distService("cb73b405-7ed2-4f7c-a0ed-99150971e553");
BLEIntCharacteristic degCharacteristic("7d15b7a8-9d0a-47cc-8a1b-695e9dbdbf30", BLERead | BLEWrite);
BLEUnsignedIntCharacteristic distCharacteristic("1c6ebb53-70d3-445a-a7af-46b97e2d3884", BLERead| BLEWrite | BLENotify);


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
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  BLE.setAdvertisedService(distService);
  distService.addCharacteristic(degCharacteristic);
  distService.addCharacteristic(distCharacteristic);
  BLE.addService(distService);
  degCharacteristic.writeValue(0);
  distCharacteristic.writeValue(0);

  // Line below commented out for battery power use
  // while (!Serial);

  if (!IMU.begin()) {
    Serial.println("Failed to initialise IMU!");
  }

  // BLE.setEventHandler(BLEConnected, centralConnected);
  degCharacteristic.setEventHandler(BLEWritten, degCharacteristicWritten);

  Serial.println("Bluetooth® Low Energy Central scan");
  Serial.println("----------------------------------");
  Serial.println("----------------------------------");
  Serial.println("----------------------------------");
  Serial.println("----------------------------------");


  // start scanning for peripheral
  BLE.setLocalName(deviceName);

  // Set the current time for contact time
  if (timestampingOn) {
    setCurrentTime();
  }

  BLE.advertise();
  // BLE.scanForName(deviceName);
}

void loop() {
  uint dist;
  bool contact;
  uint rssi;
  unsigned long currMillis = millis();
  BLE.poll();


  if (currMillis - prevMillis >= samplingInterval) {
    digitalWrite(LED_BUILTIN, 0);

    BLEDevice peripheral = BLE.available();

    BLE.stopScan();
    BLE.stopAdvertise();
    BLE.disconnect();
    Serial.println("Checkpoint: 2");

    while (peripheral && !BLE.connected()) {
      // If the device allows you to connect, take the average RSSI's
      Serial.println("Checkpoint: 3");

      if (peripheral.connect()) {
        Serial.println("Connected to peripheral: " + peripheral.address());
        rssi = rssiAvg(peripheral);

        returnAngle();
        if (peripheral.discoverAttributes()) {
          

          BLECharacteristic degPeriphCharacteristic = peripheral.characteristic("7d15b7a8-9d0a-47cc-8a1b-695e9dbdbf30");
          bool degStatus = degPeriphCharacteristic.writeValue(byte(deg[0]));
          Serial.print("DEG Characteristic write status: ");
          Serial.println(degStatus);
          
          BLECharacteristic distPeriphCharacteristic = peripheral.characteristic("1c6ebb53-70d3-445a-a7af-46b97e2d3884");
          bool distStatus = distPeriphCharacteristic.subscribe();
          Serial.print("DIST Characteristic subscribe status: ");
          Serial.println(distStatus);

          // if (distStatus) {
            // while (!distPeriphCharacteristic.written()) {
            for (int i = 0; i < 5; i++) {
              digitalWrite(LEDR, 0);
              delay(250);
              digitalWrite(LEDR, 1);
              delay(250);
            };

            byte tempDist;
            distPeriphCharacteristic.readValue(tempDist);
            Serial.print("Distance provided by peripheral: ");
            Serial.println((uint)tempDist);
          // }

          for (int i = 0; i < 5; i++) {
            digitalWrite(LEDB, 0);
            delay(1000);
            digitalWrite(LEDB, 1);
            delay(1000);
          };
          
        }

        peripheral.disconnect();
        Serial.println("Disconnected from peripheral: " + peripheral.address());

      // If the device does not allow you to connect, then take the advertised RSSI
      } else {
        rssi = peripheral.rssi() * -1;
      }
  
      dist = distance(rssi);
      Serial.println("Device: " + peripheral.address() + ", RSSI: " + rssi + ", Distance: " + dist);

      checkContact(dist, peripheral);
      // if (dist < contactDist) {
      //   alertContact();
      //   recordContact(peripheral);
      // } else {
      //   digitalWrite(LED_BUILTIN, 0);
      //   noTone(buzzerPin);
      // }

      // Move onto the next device that was scanned
      peripheral = BLE.available();
      Serial.println("Checkpoint: 4");
    }

    // Keep on advertising until it works.
    // RED LED indicates advertising has failed.
    while (!BLE.advertise()) {
      digitalWrite(LEDR, 0);
      Serial.println("Advertise: failure");
      BLE.stopAdvertise();
      errorCount++;
      if (errorCount >= maxErrors) {
        digitalWrite(resetPin, 0);
      }
    }
    errorCount = 0;
    digitalWrite(LEDR, 1);
    Serial.println("Advertise: success!");

    // Keep on scanning until it works
    // Blue LED indicates it has failed.
    while (!BLE.scanForName(deviceName)) {
      digitalWrite(LEDB, 0);
      Serial.println("Scan failed!");
      BLE.stopScan();
      delay(samplingInterval);
      errorCount++;
      if (errorCount > maxErrors) {
        digitalWrite(resetPin, 0);
      }
    }
    errorCount = 0;
    Serial.println("Scan success!");
    digitalWrite(LEDB, 1);
    
    prevMillis = millis();
  }
  
}

uint rssiAvg(BLEDevice device) {
  uint currSampleSize = 0;
  uint rssiSum = 0;

  // Collect rssi samples until the required sample size is reached
  while (currSampleSize < sampleSize) {
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


void checkContact(uint dist, BLEDevice device) {
  if (dist < contactDist) {
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
    tone(buzzerPin, 25);
  }
}

void recordContact(BLEDevice device) {
  Serial.println("Contact with address: " + device.address());
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

// Updates the angle of the device in an array [degX, degY]
void returnAngle() {
  float x, y, z;
  deg[0] = 0;
  deg[1] = 0;

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
  }

  if (x > 0.1) {
    x = 100 * x;
    deg[0] = map(x, 0, 97, 0, 90);
  } else if (x < 0.1) {
    x = 100 * x;
    deg[0] = map(x , 0, -100, 0, 90) * -1;
  }

  if (y > 0.1) {
    y = 100 * y;
    deg[1] = map(y, 0, 97, 0, 90);
  } else if (y < -0.1) {
    y = 100 * y;
    deg[1] = map(y, 0, -100, 0, 90) * -1;
  }
}

void centralConnected(BLEDevice central) {
  Serial.println("Central connected: " + central.address());

  while (central.connected()) {
    if (degCharacteristic.written()) {
      tone(buzzerPin, 50);
      delay(1000);
      noTone(buzzerPin);
      Serial.print("DEG characteristic written by connected central: ");
      Serial.println(degCharacteristic.value());

      break;
    }
  }

  Serial.println("Central disconnected: " + central.address());
}

void degCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  Serial.print("DEG characteristic: ");
  Serial.println(degCharacteristic.value());
  Serial.print("Written by: ");
  Serial.println(central.address());

  uint rssi = rssiAvg(central);
  uint dist = distance(rssi);
  Serial.print("Distance from within characteristic: ");
  Serial.println(dist);

  for (int i = 0; i < 5; i++) {
    digitalWrite(LEDG, 0);
    delay(1000);
    digitalWrite(LEDG, 1);
    delay(1000);
  }

  distCharacteristic.writeValue(dist);
  checkContact(dist, central);

}



/////////////////////////////////////////////////////////////

