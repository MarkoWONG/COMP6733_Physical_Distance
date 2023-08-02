/*
-   TESTING CODE - PERIPHERAL DEVICE
-   This code is written for the purposes of conducting our experiments.
-   It should be uploaded to the peripheral device, i.e. the one that is
    connected via USB to our laptops in which we will be recording the
    measurements.
*/

#include <ArduinoBLE.h>

const char * nameThisDevice = "testing_device";
const char * nameOtherDevice = "testing_device";

const int adjustmentFactor = 1;
const int closeContactDist = 200;   // in mm

const int samplingInterval = 0;       // RSSI sample rate in ms
unsigned long prevMillis = 0;           // Helper for sampling rate

const unsigned int sampleSize = 15;      // Size our circular buffer
unsigned int rssiPool[sampleSize];      // Circular buffer (i.e. queue)
unsigned int curr;                      // Index into circular buffer
const unsigned int rssiStarting = 100;  // Starting RSSI (intentionally large)
unsigned int rssiMovSum;                // Moving sum of RSSI's in circular buffer

const int buzzerPin = 10;

float m = 0.0954; // Control
// float m = 0.0643; // Pocket
// float m = 0.112; // Wrist

float c = 23.5; // Control
// float c = 43; // Pocket
// float c = 37.8; // Wrist



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

  BLE.setLocalName(nameThisDevice);
  BLE.advertise();

  BLE.scanForName(nameThisDevice);

  Serial.println("Peripheral is running");
}

void loop() {
  uint currMillis = millis();
  if (currMillis - prevMillis > samplingInterval) {
    BLE.stopAdvertise();
    BLEDevice otherDevice = BLE.available();
    
    if (otherDevice) {
      BLE.stopScan();
      uint rssi = otherDevice.rssi() * -1;
      checkContact(otherDevice, rssi);
      BLE.scanForName(nameOtherDevice);
    } else {
      BLE.scanForName(nameOtherDevice);
    }

    BLE.advertise();
    prevMillis = millis();
  }

}


void rssiValsInitialisation() {
  rssiMovSum = rssiStarting * sampleSize;
  curr = 0;

  // Set starting RSSI values
  for (int i = 0; i < sampleSize; i++) {
    rssiPool[i] = rssiStarting;
  }  
}


bool checkContact(BLEDevice device, uint rssiInstant) {
  uint rssiAvg = rssiMovAvg(rssiInstant);
  uint dist = distance(rssiAvg);

  if (dist <= closeContactDist) {
    alertContact();
    recordContact(device);
  } else if (dist > closeContactDist) {
    digitalWrite(LED_BUILTIN, 0);
  }
  return false;
}

// Compute the distance to the connected device.
unsigned int distance(uint rssi) {
  // In cm
  uint dist = max((rssi - c) / m, 0) * 100 * adjustmentFactor;
  Serial.print("Distance: ");
  Serial.println(dist);
  return dist;
}


// Computes RSSI moving average
unsigned int rssiMovAvg(uint rssi) {
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
  Serial.print("Averaged RSSI: ");
  Serial.println(rssiMovAvg);

  return rssiMovAvg;
}

// Alert method: update to preferred method (e.g. buzzer)
void alertContact() {
  digitalWrite(LED_BUILTIN, 1);
  tone(buzzerPin, 100);
  delay(50);
  noTone(buzzerPin);
}

void recordContact(BLEDevice central) {
  Serial.print("Contact with address: ");
  Serial.println(central.address());
}

