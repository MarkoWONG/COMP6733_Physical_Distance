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


// Contact tracing service
// BLEService contactTracingService("b2bb2fe8-b971-4afa-988f-e350f071f0ba");

void setup() {
  rssiValsInitialisation();

  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);

  digitalWrite(LEDR, 1);
  digitalWrite(LEDG, 1);

  if (!BLE.begin()) {
    Serial.println("Begin did not work");
  }

  BLE.setLocalName(deviceName);
  BLE.advertise();

  // Event handlers for when a central connects.
  BLE.setEventHandler(BLEConnected, centralConnected);
  BLE.setEventHandler(BLEDisconnected, centralDisconnected);

  Serial.println("Peripheral is running");
}

void loop() {
  BLE.poll();
}

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
  uint dist; // in cm

  // 2PM Measurement: m = 9.909, c = 58.234
  dist = max((rssi - 58.234) / 9.909, 0) * 100 * adjustmentFactor;

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
}

void recordContact(BLEDevice central) {
  Serial.print("Contact with address: ");
  Serial.println(central.address());
}
