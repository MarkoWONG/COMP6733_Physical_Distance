// Code was inspired by https://github.com/arduino-libraries/ArduinoBLE/blob/master/examples/Central/Scan/Scan.ino
#include <ArduinoBLE.h>
#include <TimeLib.h>

#define TIME_HEADER "T"
#define TIME_REQUEST 7

const uint samplingInterval = 1000;
const uint sampleSize = 15;
const uint adjustmentFactor = 1;
const uint contactDist = 200; // cm
const char * deviceName = "6733ContactTracing";

uint errorCount = 0;
const uint maxErrors = 3;

// Module control: turn these features on or off
const bool timestampingOn = false;
const bool buzzerOn = false;

// buzzerPin D10 to avoid sharing the LED I/O
const int buzzerPin = 10;

const int resetPin = 8;

// Used for looping frequency
unsigned long prevMillis = 0;

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
  
  // Line below commented out for battery power use
  // while (!Serial);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

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

  if (currMillis - prevMillis >= samplingInterval) {
    digitalWrite(LED_BUILTIN, 0);
    Serial.println("Checkpoint: 1");
    
    BLEDevice peripheral = BLE.available();
    Serial.println("Checkpoint: 2");

    BLE.stopScan();
    Serial.println("Checkpoint: 3");
    
    BLE.stopAdvertise();
    BLE.disconnect();
    Serial.println("Checkpoint: 4");

    while (peripheral) {
      // If the device allows you to connect, take the average RSSI's
      Serial.println("Checkpoint: 5");

      if (peripheral.connect()) {
        Serial.println("Connected to: " + peripheral.address());
        rssi = rssiAvg(peripheral);
        peripheral.disconnect();
        Serial.println("Disconnected from: " + peripheral.address());

      // If the device does not allow you to connect, then take the advertised RSSI
      } else {
        rssi = peripheral.rssi() * -1;
      }
  
      dist = distance(rssi);
      Serial.println("Device: " + peripheral.address() + ", RSSI: " + rssi + ", Distance: " + dist);

      if (dist < contactDist) {
        alertContact();
        recordContact(peripheral);
      } else {
        digitalWrite(LED_BUILTIN, 0);
        noTone(buzzerPin);
      }
      Serial.println("Checkpoint: 6");

      // Move onto the next device that was scanned
      peripheral = BLE.available();
      Serial.println("Checkpoint: 7");
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

unsigned int rssiAvg(BLEDevice periph) {
  uint currSampleSize = 0;
  uint rssiSum = 0;

  // Collect rssi samples until the required sample size is reached
  while (currSampleSize < sampleSize) {
    uint rssi = periph.rssi() * - 1;
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

/////////////////////////////////////////////////////////////

// RANDOM JP NOTES
// 1. Randomise the interval to an extent so that devices don't clash
