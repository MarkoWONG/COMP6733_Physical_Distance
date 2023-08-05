// COMP6733 PROJECT
// GROUP: FREE FOR ALL

/*
-   TESTING CODE - PERIPHERAL DEVICE
-   This code is written for the purposes of conducting our experiments.
-   It should be uploaded to the peripheral device, i.e. the one that is
    connected via USB to our laptops in which we will be recording the
    measurements.
*/

#include <ArduinoBLE.h>

const char * deviceName = "testing_periph";

const int adjustmentFactor = 1;
const int closeContactDist = 200;   // in mm

const int samplingInterval = 250;       // RSSI sample rate in ms
unsigned long prevMillis = 0;           // Helper for sampling rate

const unsigned int sampleSize = 15;      // Size our circular buffer
unsigned int rssiPool[sampleSize];      // Circular buffer (i.e. queue)
unsigned int curr;                      // Index into circular buffer
const unsigned int rssiStarting = 100;  // Starting RSSI (intentionally large)
unsigned int rssiMovSum;                // Moving sum of RSSI's in circular buffer

const int buzzerPin = 10;


//-----------------------------CALIBRATION---------------------------//
float m = 0.0954;     // Control
// float m = 0.0544;  // X 45-deg down
// float m = 0.0488;  // X 90-deg down
// float m = 0.0672;  // X 45-deg up
// float m = 0.0264;  // X 90-deg up
// float m = 0.0775;  // Y 45-deg left
// float m = 0.083;   // Y 90-deg left
// float m = 0.0405;  // Y 45-deg right
// float m = 0.0614;  // Z 45-deg CW
// float m = 0.0752;  // Z 90-deg CW
// float m = 0.0757;  // Z 135-deg CW
// float m = 0.0787;  // Z 180-deg CW
// float m = 0.180;   // Control for Pocket/Wrist
// float m = 0.0643;  // Pocket adjusted
// float m = 0.112;   // Wrist adjusted

float c = 23.5;       // Control
// float c = 29.9;    // X 45-deg down
// float c = 40.5;    // X 90-deg down
// float c = 31.9;    // X 45-deg up
// float c = 39.2;    // X 90-deg up
// float c = 28.6;    // Y 45-deg left
// float c = 37;      // Y 90-deg left
// float c = 40.7;    // Y 45-deg right
// float c = 32.3;    // Z 45-deg CW
// float c = 30.2;    // Z 90-deg CW
// float c = 29.1;    // Z 135-deg CW
// float c = 32.1;    // Z 180 deg CW
// float c = 28.9;    // Control for Pocket/Wrist
// float c = 43;      // Pocket adjusted
// float c = 37.8;    // Wrist adjusted


//-----------------------------SETUP---------------------------//
void setup() {
  rssiValsInitialisation();

  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);

  digitalWrite(LEDR, 1);
  digitalWrite(LEDG, 1);
  noTone(buzzerPin);


  if (!BLE.begin()) {
    Serial.println("Begin did not work");
  }

  BLE.setLocalName(deviceName);
  BLE.advertise();

  // Event handlers for when a central connects and disconnects
  BLE.setEventHandler(BLEConnected, centralConnected);
  BLE.setEventHandler(BLEDisconnected, centralDisconnected);

  Serial.println("Peripheral is running");
}


//-----------------------------LOOP---------------------------//
void loop() {
  BLE.poll();
}


//-----------------------------EVENT HANDLERS---------------------------//
void centralConnected(BLEDevice central) {
  digitalWrite(LEDR, 1);
  digitalWrite(LEDG, 0);
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
  digitalWrite(LEDR, 0);
  digitalWrite(LEDG, 1);
  digitalWrite(LED_BUILTIN, 0);
  Serial.print("Event handler: Disconnection from central: ");
  Serial.println(central.address());
  rssiValsInitialisation();
}


//-----------------------------MEASUREMENT CALCULATIONS---------------------------//
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
unsigned int distance(uint rssi) {
  uint dist; // in cm

  // 2PM Measurement: m = 9.909, c = 58.234
  dist = max((rssi - c) / m, 0) * adjustmentFactor;

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
  tone(buzzerPin, 100);
  delay(100);
  noTone(buzzerPin);
}

void recordContact(BLEDevice central) {
  Serial.print("Contact with address: ");
  Serial.println(central.address());
}

