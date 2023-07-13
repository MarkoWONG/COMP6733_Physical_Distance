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

void setup() {
  rssiValsInitialisation();

  Serial.begin(9600);
  
  pinMode(buzzerPin, OUTPUT);

  // IMPORTANT: ENTER START TIME BEFORE CONNECTING TO OTHER THINGS
  setSyncProvider(requestSync);

  BLE.begin();
  BLE.setLocalName("contactTracing");
  BLE.advertise();

  // // Event handler for when peripheral is found with specific name
  // // BLE.setEventHandler(BLEDiscovered, peripheralDiscoveredWithName);
  // // BLE.scanForName(commonBLEName);

  // Event handlers for when a central connects.
  // Board acts as a peripheral when this happens.
  BLE.setEventHandler(BLEConnected, centralConnected);
  BLE.setEventHandler(BLEDisconnected, centralDisconnected);

  // Serial.println("Peripheral is running");
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
  }
  return false;
}

// Compute the distance to the connected device.
// TODO: need a fast calibration process for demo day.
unsigned int distance(uint rssi) {

   // 2PM Measurement: m = 9.909, c = 58.234
  uint dist = max((rssi - 58.234) / 9.909, 0) * 100 * adjustmentFactor;

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
  digitalWrite(LED_BUILTIN, 1);

  // activate buzzer for 3 times
  for (auto sec = 0; sec < 3; sec++){
    digitalWrite(buzzerPin, HIGH);       // sets the digital pin 13 on
    delay(500);                  // waits for a second
    digitalWrite(buzzerPin, LOW);        // sets the digital pin 13 off
    delay(500);                  // waits for a second
  }
  
}

void recordContact(BLEDevice central) {
  Serial.print("Contact with address: ");
  Serial.println(central.address());
  Serial.print("Contact Time: ")
  digitalClockDisplay();
}
