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

const int buzzerPin = 13; // Assuming buzzer is in D13

// Used for looping frequency
unsigned long prevMillis = 0;

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  noTone(buzzerPin);
  
  while (!Serial);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1);
  }

  Serial.println("Bluetooth® Low Energy Central scan");

  // start scanning for peripheral
  BLE.setLocalName(deviceName);

  // Set the current time for contact time
  setCurrentTime();

  BLE.advertise();
}

void loop() {
  uint dist;
  bool contact;
  uint rssi;
  unsigned long currMillis = millis();

  // Only scan for devices at this interval
  if (currMillis - prevMillis >= samplingInterval) {

    BLE.scan();
    // BLE.scanForName(deviceName);
    // BLE.scanForAddress("c5:f2:f4:40:ed:30");
    BLEDevice peripheral = BLE.available();
    while (peripheral) {
      // If the device allows you to connect, take the average RSSI's
      if (peripheral.connect()) {
        rssi = rssiAvg(peripheral);
        peripheral.disconnect();

      // If the device does not allow you to connect, then take the advertised RSSI
      } else {
        rssi = peripheral.rssi() * -1;
        peripheral.disconnect();
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

      // Move onto the next device that was scanned
      peripheral = BLE.available();
    }

    Serial.println("Out of loop");

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
  // turn on buzzer
  tone(buzzerPin, 110);
}

void recordContact(BLEDevice device) {
  Serial.println("Contact with address: " + device.address());
  Serial.print("Contact Time: ");
  digitalClockDisplay();
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
