// COMP6733 PROJECT
// GROUP: FREE FOR ALL

// Code was inspired by https://github.com/arduino-libraries/ArduinoBLE/blob/master/examples/Central/Scan/Scan.ino
#include <ArduinoBLE.h>
#include <TimeLib.h>

#define TIME_HEADER "T"
#define TIME_REQUEST 7

const uint samplingInterval = 250;
const uint sampleSize = 15;
unsigned int rssiPool[sampleSize];      // Circular buffer (i.e. queue)
unsigned int curr;                      // Index into circular buffer
const unsigned int rssiStarting = 100;  // Starting RSSI (intentionally large)
unsigned int rssiMovSum;                // Moving sum of RSSI's in circular buffer

const uint contactDist = 200; // cm
const char * deviceName = "6733ContactTracing";

// Module control: turn these features on or off
const bool timestampingOn = false;
const bool buzzerOn = true;

// Hardware
const int buzzerPin = 10;
const int resetPin = 8;
const uint resetInterval = 10000;

// Manufacturer's Data
const uint8_t rssiAt2m = 71;
const uint8_t manufactData[1] = {rssiAt2m};

// Array for storing other device's manufacturer's data
uint8_t periphManuData[1];

// Used for looping frequency
unsigned long prevMillis = 0;


//-----------------------------CALIBRATION---------------------------//
const uint adjustmentFactor = 1;
float m = 0.147; // Control Nano-Nano


float c = 41.5;   // Control Nano-Nano


//-----------------------------SETUP---------------------------//
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
  
  if (!BLE.begin()) {
    Serial.println("Failed to start BLE!");
    while (1);
  }
  
  // Set manufacturer's calibration for RSSI at 2m
  BLEAdvertisingData manufacturerData;
  manufacturerData.setManufacturerData(manufactData, sizeof(manufactData));
  BLE.setAdvertisingData(manufacturerData);

  // Set the current time for contact time
  if (timestampingOn) {
    setCurrentTime();
  }

  BLE.setLocalName(deviceName);
  BLE.advertise();

  Serial.println("Peripheral started!");
}


//-----------------------------LOOP---------------------------//
void loop() {
  BLE.poll();
  unsigned long currMillis = millis();

  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    peripheral.manufacturerData(periphManuData, 1);
    uint manufacturerRssi = periphManuData[0];
    uint measuredRssi = peripheral.rssi() * -1;
    uint dist = distance(measuredRssi);

    Serial.println("------------------------------------------------------");
    Serial.print("Peripheral's calibrated RSSI @ 2m: ");
    Serial.println(manufacturerRssi);
    Serial.println("Device: " + peripheral.address() + ", Measured RSSI: " + measuredRssi + ", Distance(est.): " + dist);

    // Check contact based on distance equation or peripheral's manufacturer data.
    // checkContactWithDistance(dist, peripheral);
    checkContactWithManufacturerRSSI(manufacturerRssi, measuredRssi, peripheral);
    Serial.println("------------------------------------------------------");
  }

  // Devices stop advertising after a while so perform a hard reset at given interval
  if (currMillis > resetInterval) {
    Serial.println("Resetting...");
    digitalWrite(resetPin, 0);
  // Else regularly re-advertise and re-scan to get up-to-date info
  } else if (currMillis - prevMillis > samplingInterval) {
    BLE.stopAdvertise();
    BLE.stopScan();
    BLE.advertise();
    BLE.scanForName(deviceName);
    prevMillis = millis();
  }
}


//-----------------------------MEASUREMENT CALCULATIONS---------------------------//
uint distance(uint rssi) {
  uint dist = max((rssi - c) / m, 0) * adjustmentFactor;
  return dist;
}


//-----------------------------ALERTS AND NOTIFICATIONS---------------------------//
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

  delay(100);
  digitalWrite(LED_BUILTIN, 0);
  noTone(buzzerPin);
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
