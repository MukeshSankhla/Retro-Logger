/*
Project: Production Line Data-Logger
Author: Mukesh Sankhla (Modified by Claude)
Website: https://www.makerbrains.com
GitHub: https://github.com/MukeshSankhla
*/

#include <Arduino.h>
#include <ArduinoJson.h>  // Include the ArduinoJson library

// Timestamps to track last simulation time for each station
unsigned long lastWinding = 0;
unsigned long lastAssembling = 0;
unsigned long lastTesting = 0;
unsigned long lastError = 0;

// Time intervals (in milliseconds) for each process simulation
const unsigned long cycleWinding = 10000;    // Winding every 10 seconds
const unsigned long cycleAssembling = 7000;  // Assembling every 7 seconds
const unsigned long cycleTesting = 12000;    // Testing every 12 seconds
const unsigned long errorInterval = 30000;   // Error simulation check every 30 seconds

// Counters for each process to generate unique serial IDs
int windingCounter = 1;
int assemblingCounter = 1;
int testingCounter = 1;

// Function to generate serial ID for each process
// Format: 120425S1[process char]N[zero-padded 5-digit number]
String makeID(int counter, char process) {
  char buf[30];
  sprintf(buf, "120425S1%cN%05d", process, counter);
  return String(buf);
}

// Returns 1 with the given probability (default 50%)
int randBool(float chance = 0.5) {
  return random(100) < (chance * 100) ? 1 : 0;
}

// Simulates the winding process
void simulateWinding() {
  String id = makeID(windingCounter, 'W');
  
  // Generate wire and drum thickness with tight tolerances
  float wireThk = random(195, 225) / 100.0;  // 1.95 - 2.25 mm
  float drumThk = random(115, 130) / 10.0;   // 11.5 - 13.0 mm
  
  // 10% chance to generate outlier values
  if (random(100) < 10) {
    wireThk = random(180, 240) / 100.0;
    drumThk = random(110, 135) / 10.0;
  }
  
  // Check if values are within acceptable range
  int wireOK = (wireThk >= 1.90 && wireThk <= 2.30) ? 1 : 0;
  int drumOK = (drumThk >= 11.0 && drumThk <= 13.5) ? 1 : 0;
  
  // Pass/fail status
  int pass = (wireOK && drumOK) ? 1 : 0;
  int fail = !pass;
  
  // Reason for failure if any
  String reason = "-";
  if (fail) {
    if (!wireOK && !drumOK) reason = "Both thicknesses out of range";
    else if (!wireOK) reason = "Wire thickness out of range";
    else reason = "Drum thickness out of range";
  }

  // Create JSON document
  StaticJsonDocument<256> doc;
  JsonObject winding = doc.createNestedObject("winding");
  JsonObject data = winding.createNestedObject(id);
  
  data["wire_thickness"] = wireThk;
  data["drum_thickness"] = drumThk;
  data["wire_ok"] = wireOK;
  data["drum_ok"] = drumOK;
  data["pass"] = pass;
  data["fail"] = fail;
  data["reason"] = reason;

  // Serialize JSON to Serial
  serializeJson(doc, Serial);
  Serial.println();

  // Increment counter for unique ID generation
  windingCounter++;
}

// Simulates the assembling process
void simulateAssembling() {
  String id = makeID(assemblingCounter, 'A');
  
  // Randomly determine assembly count (1 to 3 units per assembly)
  int assemblyCount;
  int r = random(100);
  if (r < 60) assemblyCount = 2;      // Most often 2 (60%)
  else if (r < 85) assemblyCount = 1; // Occasionally 1 (25%)
  else assemblyCount = 3;             // Least often 3 (15%)
  
  // Create JSON document
  StaticJsonDocument<128> doc;
  JsonObject assembling = doc.createNestedObject("assembling");
  JsonObject data = assembling.createNestedObject(id);
  
  data["assembly_count"] = assemblyCount;

  // Serialize JSON to Serial
  serializeJson(doc, Serial);
  Serial.println();
  
  // Increment counter for unique ID generation
  assemblingCounter++;
}

// Simulates the testing process
void simulateTesting() {
  String id = makeID(testingCounter, 'T');
  
  // Random test results (pass rates 90-95%)
  int working = randBool(0.95);
  int lever = randBool(0.93);
  int voltTest = randBool(0.92);
  
  // Generate maximum current value mostly within range
  float maxCurrent;
  if (random(100) < 90) {
    maxCurrent = random(980, 1050) / 100.0;
  } else {
    maxCurrent = random(950, 1100) / 100.0;
  }

  // Generate random tolerance (0.1 to 5.5)
  float tolerance = random(10, 55) / 10.0;
  int thermal = randBool(0.96); // 96% chance to pass thermal test

  // Generate magnetic field (most between 150-200)
  float magField;
  if (random(100) < 90) {
    magField = random(150, 200);
  } else {
    magField = random(100, 220);
  }

  // Polarity check (98% pass rate)
  int polarity = randBool(0.98);
  int magOK = (magField >= 140 && magField <= 210 && polarity) ? 1 : 0;

  // Determine overall pass/fail status
  int pass = (working && lever && voltTest && (maxCurrent >= 9.7 && maxCurrent <= 10.7) &&
             (tolerance <= 5.5) && thermal && magOK) ? 1 : 0;
  int fail = !pass;

  // Determine reason for failure
  String reason = "-";
  if (fail) {
    if (!working) reason = "Not working";
    else if (!lever) reason = "Lever fail";
    else if (!voltTest) reason = "Voltage test fail";
    else if (maxCurrent < 9.7 || maxCurrent > 10.7) reason = "Current out of range";
    else if (tolerance > 5.5) reason = "Tolerance too high";
    else if (!thermal) reason = "Thermal test failed";
    else if (!magOK) reason = "Magnetic test fail";
  }

  // Create JSON document
  StaticJsonDocument<384> doc;
  JsonObject testing = doc.createNestedObject("testing");
  JsonObject data = testing.createNestedObject(id);
  
  data["working"] = working;
  data["lever"] = lever;
  data["volt_test"] = voltTest;
  data["max_current"] = maxCurrent;
  data["tolerance"] = tolerance;
  data["thermal"] = thermal;
  data["mag_field"] = magField;
  data["polarity_ok"] = polarity;
  data["mag_test_ok"] = magOK;
  data["pass"] = pass;
  data["fail"] = fail;
  data["reason"] = reason;

  // Serialize JSON to Serial
  serializeJson(doc, Serial);
  Serial.println();

  // Increment counter for unique ID generation
  testingCounter++;
}

// Simulates occasional random machine errors
void simulateErrors() {
  // List of station names
  const char* stations[] = {"winding", "assembling", "testing"};

  // List of possible machine errors
  const char* errors[] = {
    "Sensor Fault", "Motor Jam", "Voltage Drop", "Feeder Error", 
    "Thermal Probe Error", "Conveyor Jam", "Connection Error", 
    "Calibration Issue", "Drive Failure", "Control System Error"
  };

  // Random station and error selection
  int stationIndex = random(0, 3);
  String station = stations[stationIndex];
  int errorIndex = random(0, 10);
  String error = errors[errorIndex];

  // Slightly randomized timestamp
  unsigned long timeStamp = millis() + random(1000);

  // Create JSON document
  StaticJsonDocument<192> doc;
  JsonObject machineError = doc.createNestedObject("machine_error");
  
  // Use timestamp as a string key
  char timeStampStr[16];
  sprintf(timeStampStr, "%lu", timeStamp);
  JsonObject data = machineError.createNestedObject(timeStampStr);
  
  data["station"] = station;
  data["error"] = error;

  // Serialize JSON to Serial
  serializeJson(doc, Serial);
  Serial.println();
}

// Arduino setup function, runs once at startup
void setup() {
  Serial.begin(115200);           // Initialize serial communication at 115200 baud
  randomSeed(analogRead(0));      // Seed random number generator using analog noise
  
  // Wait for serial to be ready
  delay(1000);
  Serial.println("Production Line Simulator Started");
}

// Main loop, repeatedly runs simulations based on timing
void loop() {
  unsigned long now = millis();   // Get current time

  // Simulate winding if enough time has passed
  if (now - lastWinding >= cycleWinding + random(-500, 500)) {
    simulateWinding();
    lastWinding = now;
  }

  // Simulate assembling if enough time has passed
  if (now - lastAssembling >= cycleAssembling + random(-300, 300)) {
    simulateAssembling();
    lastAssembling = now;
  }

  // Simulate testing if enough time has passed
  if (now - lastTesting >= cycleTesting + random(-600, 600)) {
    simulateTesting();
    lastTesting = now;
  }

  // Occasionally simulate a random error
  if (now - lastError >= errorInterval && random(100) < 5) {
    simulateErrors();
    lastError = now;
  }
}