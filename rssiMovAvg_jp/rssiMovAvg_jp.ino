#include <ArduinoBLE.h>

const String commonBLEName = "contactTracing";

const int samplingInterval = 250;     // RSSI sample rate in ms
unsigned long prevMillis = 0;         // Helper for sampling rate

const unsigned int sampleSize = 5;      // Size our circular buffer
unsigned int rssiPool[sampleSize];      // Circular buffer (i.e. queue)
unsigned int curr;                      // Index into circular buffer
const unsigned int rssiStarting = 100;  // Starting RSSI (intentionally large)
unsigned int rssiMovSum;                 // Moving sum of RSSI's in circular buffer

// Contact tracing service
// BLEService contactTracingService("b2bb2fe8-b971-4afa-988f-e350f071f0ba");

void setup() {
  rssiValsInitialisation();

  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);       // Using the LED as notification of close contact

  BLE.begin();
  BLE.setLocalName("contactTracing");
  // BLE.setAdvertisedService(contactTracingService);
  // BLE.addService(contactTracingService);
  BLE.advertise();

  // Event handler for when peripheral is found with specific name
  // BLE.setEventHandler(BLEDiscovered, peripheralDiscoveredWithName);
  // BLE.scanForName(commonBLEName);

  // Event handlers for when a central connects.
  // Board acts as a peripheral when this happens.
  BLE.setEventHandler(BLEConnected, centralConnected);
  BLE.setEventHandler(BLEDisconnected, centralDisconnected);

  Serial.println("Peripheral is running");
}

void loop() {
  
  // BLE.scanForName(commonBLEName);
  // BLEDevice peripheral = BLE.available();
  // Connect to peripheral and perform logic as a central device

  BLE.poll();
}

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

  if (dist <= 30) {
    return true;
  } else if (dist > 60) {
    digitalWrite(LED_BUILTIN, 0);
  }
  return false;
}

// Compute the distance to the connected device.
// TODO: need a fast calibration process for demo day.
unsigned int distance(uint rssi) {
  uint dist;

  if (rssi < 45) {
    dist = 0;
  } else if (rssi < 53) {
    dist = 10;
  } else if (rssi < 55) {
    dist = 20;
  } else if (rssi < 57) {
    dist = 30;
  } else if (rssi < 63) {
    dist = 40;
  } else if (rssi < 70) {
    dist = 50;
  } else if (rssi < 76) {
    dist = 60;
  } else if (rssi < 82) {
    dist = 70;
  } else {
    dist = 80;
  }
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

// Old void loop
// voil loop() {
  // BLEDevice central = BLE.central();

  // if (central) {
  //   Serial.print("Connection from central device: ");
  //   Serial.println(central.address());

  //   while (central.connected()) {
  //     unsigned long currMillis = millis();
  //     if (currMillis - prevMillis >= samplingInterval) {
  //       checkContact();
  //       prevMillis = millis();
  //     }
  //   }

  //   Serial.print("Disconnection from central device: ");
  //   Serial.println(central.address());
  // }

// }