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

// Used for looping frequency
unsigned long prevMillis = 0;


//-----------------------------CALIBRATION---------------------------//
const uint adjustmentFactor = 1;
// float m = 0.147; // Control Nano-Nano
float m = 0.140; // 


// float c = 41.5;   // Control Nano-Nano
float c = 27.3;   //

BLEService contactService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEBoolCharacteristic contactCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);

//-----------------------------SETUP---------------------------//
void setup() {
  Serial.begin(9600);

  rssiValsInitialisation();

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
  
  BLE.setAdvertisedService(contactService);
  contactService.addCharacteristic(contactCharacteristic);
  BLE.addService(contactService);

  BLE.setEventHandler(BLEConnected, centralConnected);
  contactCharacteristic.writeValue(0);

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
}


//-----------------------------MEASUREMENT CALCULATIONS---------------------------//
// Initialise moving average
void rssiValsInitialisation() {
  rssiMovSum = rssiStarting * sampleSize;
  curr = 0;

  // Set starting RSSI values
  for (int i = 0; i < sampleSize; i++) {
    rssiPool[i] = rssiStarting;
  }  
}
// Computes RSSI of the connected central device
unsigned int rssiMovAvg() {
  uint rssi = BLE.rssi() * -1;
  Serial.print("Instant RSSI: ");
  Serial.println(rssi);

  // Update the moving sum of RSSI values, only
  // if the rssi value is not zero.
  if (abs(rssi) > 0) {
    rssiMovSum -= rssiPool[curr];
    rssiPool[curr] = rssi;
    rssiMovSum += rssiPool[curr];
    curr = (curr + 1) % sampleSize;
  }

  // Calculate the moving average of RSSI's
  uint rssiMovAvg = rssiMovSum / sampleSize;
  Serial.print("Moving Avg RSSI: ");
  Serial.println(rssiMovAvg);

  return rssiMovAvg;
}

uint distance(uint rssi) {
  uint dist = max((rssi - c) / m, 0) * adjustmentFactor;
  return dist;
}



//-----------------------------EVENT HANDLERS---------------------------//
void centralConnected(BLEDevice central) {
  Serial.print("Connection from: " + central.address());

  while (central.connected()) {
    uint rssi = rssiMovAvg();
    uint dist = distance(rssi);
    Serial.print("Distance: ");
    Serial.println(dist);
    checkContact(dist, central);
    delay(100);
  }

  Serial.println("Disconnection from: " + central.address());
}


//-----------------------------ALERTS AND NOTIFICATIONS---------------------------//
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
    tone(buzzerPin, 100);
    delay(100);
    noTone(buzzerPin);
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
