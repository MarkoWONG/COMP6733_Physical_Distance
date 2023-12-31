#include <ArduinoBLE.h>
#include <TimeLib.h>

#define TIME_HEADER "T"
#define TIME_REQUEST 7

const String commonBLEName = "contactTracing";

const int adjustmentFactor = 1;
const int closeContactDist = 200;   // in mm

const int samplingInterval = 100;       // RSSI sample rate in ms
unsigned long prevMillis = 0;           // Helper for sampling rate

const unsigned int sampleSize = 15;      // Size our circular buffer
unsigned int rssiPool[sampleSize];      // Circular buffer (i.e. queue)
unsigned int curr;                      // Index into circular buffer
const unsigned int rssiStarting = 100;  // Starting RSSI (intentionally large)
unsigned int rssiMovSum;                // Moving sum of RSSI's in circular buffer

const int buzzerPin = 13; // Assuming buzzer is in D13

// Time of day profile
const float c_profile_timeOfDay[3] = {50,58.234,58.234};
const float m_profile_timeOfDay[3] = {8.01,9.909,9.909};

// For tester 
BLEService testerService("4A981234-1CC4-E7C1-C757-F1267DD021E8");
BLEByteCharacteristic distanceReadingCharacteristic("4A981236-1CC4-E7C1-C757-F1267DD021E8", BLENotify | BLERead | BLEWrite);

void setup() {
  rssiValsInitialisation();

  Serial.begin(9600);
  
  // pinMode(buzzerPin, OUTPUT);
  noTone(buzzerPin);

  // IMPORTANT: ENTER START TIME BEFORE CONNECTING TO OTHER THINGS
  setSyncProvider(requestSync);

  // Service to see distance reading on central devices (clients)
  // BLE.setAdvertisedService(testerService);
  BLE.addService(testerService);
  testerService.addCharacteristic(distanceReadingCharacteristic);
  distanceReadingCharacteristic.writeValue(0);
  // Begin BLE connections
  BLE.begin();
  BLE.setLocalName("contactTracing");
  BLE.advertise();

  // Event handlers for when a central connects.
  // Board acts as a peripheral when this happens.
  BLE.setEventHandler(BLEConnected, centralConnected);
  BLE.setEventHandler(BLEDisconnected, centralDisconnected);

}

void loop() {
  // Need to enter the current time first E.g: "T1689223105"
  if (Serial.available()){
    processSyncMessage();
  }

  BLE.poll();
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

void processSyncMessage(){
  unsigned long pcTime;
  // 1689225031 = 13/07/2023 3:10pm
  const unsigned long DEFAULT_TIME = 1689225031;

  if (Serial.find(TIME_HEADER)){
    pcTime = Serial.parseInt();
    if (pcTime >= DEFAULT_TIME) {
      setTime(pcTime);
    }
  }
}

time_t requestSync(){
  Serial.write(TIME_REQUEST);
  return 0;
}
/////////////////////////////////////////////////////////////

void centralConnected(BLEDevice central) {
  Serial.print("Event handler: Connection from central: ");
  Serial.println(central.address());
  while (BLE.connected()) {
    unsigned long currMillis = millis();
    if (currMillis - prevMillis >= samplingInterval) {
      if (checkContact()) {
        alertContact();
        recordContact(central);
      }
      prevMillis = millis();
    }
  }
}

void centralDisconnected(BLEDevice central) {
  Serial.print("Event handler: Disconnection from central: ");
  Serial.println(central.address());
  rssiValsInitialisation();
}

void peripheralDiscoveredWithName(BLEDevice peripheral) {
  Serial.print("Event handler: Discovered peripheral with address: ");
  Serial.println(peripheral.address());  
}

void rssiValsInitialisation() {
  rssiMovSum = rssiStarting * sampleSize;
  curr = 0;

  // Set starting RSSI values
  for (int i = 0; i < sampleSize; i++) {
    rssiPool[i] = rssiStarting;
  }  
}

// Check if close contact has occurred. 
// Written this way so that we can add / change to other strategies.
bool checkContact() {
  return checkContactRssiStrategy();
}

bool checkContactRssiStrategy() {
  uint rssi = rssiMovAvg();
  uint dist = distance(rssi);

  if (dist <= closeContactDist) {
    return true;
  } else if (dist > closeContactDist) {
    digitalWrite(LED_BUILTIN, 0);
    noTone(buzzerPin);
  }
  return false;
}

// Compute the distance to the connected device.
// TODO: need a fast calibration process for demo day.
unsigned int distance(uint rssi) {

  float m = 9.909;
  float c = 58.234;
  // Morning
  if (hour() < 10){
    m = m_profile_timeOfDay[0];
    c = c_profile_timeOfDay[0];
  }
  // Mid-day
  else if (hour() >= 10 && hour() <= 17){
    m = m_profile_timeOfDay[1];
    c = c_profile_timeOfDay[1];
  }
  // Arvo
  else {
    m = m_profile_timeOfDay[2];
    c = c_profile_timeOfDay[1];
  }
  
  uint dist = max((rssi - c) / m, 0) * 100 * adjustmentFactor;

  distanceReadingCharacteristic.writeValue(dist);
  Serial.print("Distance: ");
  Serial.println(dist);
  return dist;
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

// Alert method: update to preferred method (e.g. buzzer)
void alertContact() {
  // turn on led
  digitalWrite(LED_BUILTIN, 1);

  // turn on buzzer
  tone(buzzerPin, 110);
}

void recordContact(BLEDevice central) {
  // print contact details
  Serial.print("Contact with address: ");
  Serial.println(central.address());
  Serial.print("Contact Time: ");
  digitalClockDisplay();
}
