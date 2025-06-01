#include <Adafruit_Fingerprint.h>
#include <WiFi.h>        // For ESP32 Wi-Fi
#include <HTTPClient.h>  // For ESP32 HTTP requests

// --- Wi-Fi Configuration ---
const char* ssid = "IKO-WIFI";
const char* password = "12341234";

// --- Server Configuration ---
const char* serverHost = "192.168.1.12";
const int serverPort = 8000;
const char* serverLoginPath = "/fingerprint_login";
const char* serverEnrollPath = "/fingerprint_enroll";

// --- Hardware Pins & Settings ---
HardwareSerial mySerial(2); // RX = 16, TX = 17 for Serial2
Adafruit_Fingerprint finger(&mySerial);

const int feedbackLedPin = 4; // LED for Wi-Fi status, HTTP errors etc.
const int MAX_FINGERPRINTS = 127;
const int RELAY_PIN = 5;  // Pin relay

// --- Custom Error Codes (Optional) ---
#define FINGERPRINT_TIMEOUT 0xFE // Example custom error code for timeout

// --- Function Prototypes (optional, but good for organization) ---
void setupWifi();
void relayOn();
void relayOff();
int findNextAvailableID();
void enrollFingerprintAndNotify(int id);
void enrollFingerprint_original(int id);
void loginFingerprint();
void blinkLed(int pin, int times, int onDuration, int offDuration = -1);
uint8_t getFingerprintEnroll(int id); // Removed 'pass' parameter, not used
uint8_t getFingerprintImageAndConvertToTemplate(uint8_t templateSlot);


// --- Wi-Fi Setup ---
void setupWifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retries++;
    if (retries > 20) {
      Serial.println("\nFailed to connect to WiFi. Please check credentials or network.");
      blinkLed(feedbackLedPin, 5, 200, 200); // Blink 5 times for Wi-Fi failure
      return;
    }
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// --- LED Blinking Utility ---
void blinkLed(int pin, int times, int onDuration, int offDuration) {
  if (offDuration == -1) offDuration = onDuration;
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(onDuration);
    digitalWrite(pin, LOW);
    delay(offDuration);
  }
}

// --- Relay Control ---
void relayOn() {
  digitalWrite(RELAY_PIN, LOW);  // Relay ON (assuming active LOW)
  Serial.println("Relay ON");
}

void relayOff() {
  digitalWrite(RELAY_PIN, HIGH); // Relay OFF
  Serial.println("Relay OFF");
}

// --- Main Setup ---
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Wait for serial port to connect. Needed for native USB port only
  }
  delay(1000); // Allow Serial to stabilize

  pinMode(feedbackLedPin, OUTPUT);
  digitalWrite(feedbackLedPin, LOW);

  pinMode(RELAY_PIN, OUTPUT);
  relayOff(); // Ensure relay is off initially

  setupWifi(); // Connect to Wi-Fi

  // Initialize fingerprint sensor
  mySerial.begin(57600, SERIAL_8N1, 16, 17); // Pins for Serial2: RX=16, TX=17
  finger.begin(57600);

  Serial.println("\nInitializing fingerprint sensor...");
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor detected!");
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE); // Indicate sensor ready
    delay(500);
    finger.LEDcontrol(FINGERPRINT_LED_OFF);
  } else {
    Serial.println("Fingerprint sensor not found. Please check wiring.");
    blinkLed(feedbackLedPin, 10, 100, 100); // Fast blink for critical sensor error
    while (1) { delay(1); } // Halt execution
  }

  Serial.println("\nReady for commands:");
  Serial.println("  'a' - Auto enroll & notify server");
  Serial.println("  'd' - Manual enroll by ID (no server notify)");
  Serial.println("  'l' - Login & notify server");
}

// --- Main Loop ---
void loop() {
  if (Serial.available()) {
    char cmd = tolower(Serial.read()); // Read command, convert to lowercase
    while (Serial.available()) { Serial.read(); } // Clear any remaining characters in buffer

    if (cmd == 'a') {
      Serial.println("Starting automatic enrollment...");
      int newId = findNextAvailableID();
      if (newId == 0) {
        Serial.println("Enrollment failed: No available ID or database is full.");
        blinkLed(feedbackLedPin, 3, 300, 150); // Indicate failure
      } else {
        enrollFingerprintAndNotify(newId);
      }
    } else if (cmd == 'd') {
      Serial.println("Enter ID (1-" + String(MAX_FINGERPRINTS) + ") for manual enrollment:");
      while (!Serial.available()) { delay(10); } // Wait for user input
      String idStr = Serial.readStringUntil('\n');
      idStr.trim();
      int id = idStr.toInt();

      if (id < 1 || id > MAX_FINGERPRINTS) {
        Serial.println("Invalid ID.");
        blinkLed(feedbackLedPin, 2, 200, 100); // Indicate invalid input
        return;
      }
      enrollFingerprint_original(id);
    } else if (cmd == 'l') {
      loginFingerprint();
    } else {
      Serial.print("Unknown command: ");
      Serial.println(cmd);
    }
  }
}

// --- Find Next Available Fingerprint ID ---
int findNextAvailableID() {
  Serial.println("Searching for an available ID...");
  uint8_t p = finger.getTemplateCount(); // This updates finger.templateCount

  if (p != FINGERPRINT_OK) {
    Serial.print("Failed to get template count. Error code: 0x");
    Serial.println(p, HEX);
    return 0; // Indicate error
  }

  int current_template_count = finger.templateCount;
  Serial.print("Current number of stored templates: "); Serial.println(current_template_count);

  if (current_template_count >= MAX_FINGERPRINTS) {
    Serial.println("Fingerprint database is full.");
    return 0; // Indicate full
  }

  // This simple method finds the next sequential ID.
  // For finding gaps, a more complex check would be needed (e.g., trying to load each ID).
  int next_id = current_template_count + 1;
  Serial.print("Next available ID for enrollment: "); Serial.println(next_id);
  return next_id;
}


// --- Helper: Get Fingerprint Image and Convert to Template ---
uint8_t getFingerprintImageAndConvertToTemplate(uint8_t templateSlot) {
  uint8_t p = FINGERPRINT_NOFINGER; // Initialize p to ensure loop starts
  unsigned long startTime = millis();
  Serial.print("Place finger on sensor for image "); Serial.println(templateSlot);
  finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE); // Blue light for scanning

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (millis() - startTime > 10000) { // 10-second timeout
      Serial.println("Timeout: No fingerprint detected.");
      finger.LEDcontrol(FINGERPRINT_LED_OFF);
      return FINGERPRINT_TIMEOUT; 
    }
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken.");
        break;
      case FINGERPRINT_NOFINGER:
        delay(50); // Keep trying
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error.");
        finger.LEDcontrol(FINGERPRINT_LED_OFF);
        return p;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error.");
        finger.LEDcontrol(FINGERPRINT_LED_OFF);
        return p;
      default:
        // Don't print error here if it's still trying (e.g. FINGERPRINT_NOFINGER)
        // Only print if it's an unexpected error code that isn't FINGERPRINT_OK or FINGERPRINT_NOFINGER
        if (p != FINGERPRINT_NOFINGER) {
            Serial.print("Unknown error during getImage: 0x"); Serial.println(p, HEX);
            finger.LEDcontrol(FINGERPRINT_LED_OFF);
            return p;
        }
        break; // Continue loop if FINGERPRINT_NOFINGER
    }
  }
  // finger.LEDcontrol(FINGERPRINT_LED_OFF); // Turn off LED after image capture - moved to after image2Tz

  // Convert image to template
  p = finger.image2Tz(templateSlot);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.print("Image converted to template slot "); Serial.println(templateSlot);
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy."); finger.LEDcontrol(FINGERPRINT_LED_OFF); return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error during image2Tz."); finger.LEDcontrol(FINGERPRINT_LED_OFF); return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features."); finger.LEDcontrol(FINGERPRINT_LED_OFF); return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Invalid image."); finger.LEDcontrol(FINGERPRINT_LED_OFF); return p;
    default:
      Serial.print("Unknown error during image2Tz: 0x"); Serial.println(p, HEX); finger.LEDcontrol(FINGERPRINT_LED_OFF); return p;
  }
  finger.LEDcontrol(FINGERPRINT_LED_OFF); // Turn off LED after successful conversion or if an error occurred before this point
  return FINGERPRINT_OK;
}


// --- Helper: Perform the two-stage enrollment process ---
uint8_t getFingerprintEnroll(int id) {
  uint8_t p;

  // --- First Finger Scan ---
  p = getFingerprintImageAndConvertToTemplate(1);
  if (p != FINGERPRINT_OK) return p;

  Serial.println("Remove finger.");
  finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 200, FINGERPRINT_LED_PURPLE); // Purple flashing to indicate remove finger
  delay(1000); // Give user time to see message/LED
  unsigned long start_remove_time = millis();
  // Wait for finger to be removed
  // Initialize getImage_status to something other than FINGERPRINT_NOFINGER to enter the loop
  uint8_t getImage_status = FINGERPRINT_OK; 
  while (getImage_status != FINGERPRINT_NOFINGER) {
    getImage_status = finger.getImage();
    delay(50);
    if (millis() - start_remove_time > 5000) { // 5 sec timeout to remove finger
        Serial.println("Timeout waiting for finger removal.");
        finger.LEDcontrol(FINGERPRINT_LED_OFF);
        return FINGERPRINT_TIMEOUT; 
    }
  }
  Serial.println("Finger removed.");
  finger.LEDcontrol(FINGERPRINT_LED_OFF);

  // --- Second Finger Scan ---
  p = getFingerprintImageAndConvertToTemplate(2);
  if (p != FINGERPRINT_OK) return p;

  // --- Create Model ---
  Serial.print("Creating model for ID #"); Serial.println(id);
  finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE); // Blue during processing
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Fingerprints matched! Model created.");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error while creating model.");
    finger.LEDcontrol(FINGERPRINT_LED_OFF); return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match. Please try again.");
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED); delay(1000); 
    finger.LEDcontrol(FINGERPRINT_LED_OFF); return p;
  } else {
    Serial.print("Unknown error while creating model: 0x"); Serial.println(p, HEX);
    finger.LEDcontrol(FINGERPRINT_LED_OFF); return p;
  }
  finger.LEDcontrol(FINGERPRINT_LED_OFF);
  return FINGERPRINT_OK;
}


// --- Manual Fingerprint Enrollment (no server notification) ---
void enrollFingerprint_original(int id) {
  Serial.print("Starting manual enrollment for ID #"); Serial.println(id);

  uint8_t enroll_status = getFingerprintEnroll(id); // Use the helper function

  if (enroll_status == FINGERPRINT_OK) {
    Serial.print("Storing model at ID #"); Serial.println(id);
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE); // Blue during storage
    uint8_t p = finger.storeModel(id);
    if (p == FINGERPRINT_OK) {
      Serial.println("Enrollment successful (manual)!");
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE); // Solid BLUE for success (GREEN not available)
      delay(1500);
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error while storing model.");
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED); delay(1000);
    } else if (p == FINGERPRINT_BADLOCATION) {
      Serial.println("Invalid storage location (ID may be out of range or already taken).");
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED); delay(1000);
    } else if (p == FINGERPRINT_FLASHERR) {
      Serial.println("Error writing to flash memory.");
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED); delay(1000);
    } else {
      Serial.print("Unknown error while storing model: 0x"); Serial.println(p, HEX);
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED); delay(1000);
    }
  } else {
    Serial.println("Enrollment process failed. Please try again.");
    // LED feedback for enroll_status failure already handled in getFingerprintEnroll or getFingerprintImageAndConvertToTemplate
  }
  finger.LEDcontrol(FINGERPRINT_LED_OFF); // Ensure LED is off
}


// --- Auto Fingerprint Enrollment with Server Notification ---
void enrollFingerprintAndNotify(int id) {
  Serial.print("Starting automatic enrollment and notification for ID #"); Serial.println(id);

  uint8_t enroll_status = getFingerprintEnroll(id); // Use the helper function

  if (enroll_status == FINGERPRINT_OK) {
    Serial.print("Storing model at ID #"); Serial.println(id);
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE); // Blue during storage
    uint8_t p_store = finger.storeModel(id);

    if (p_store == FINGERPRINT_OK) {
      Serial.println("Enrollment successful! Sending data to server...");
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE); // BLUE for local success (GREEN not available)
      delay(1000);
      finger.LEDcontrol(FINGERPRINT_LED_OFF);

      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.setTimeout(10000); // 10 seconds timeout

        String serverUrl = "http://" + String(serverHost) + ":" + String(serverPort) + String(serverEnrollPath);
        Serial.print("Sending POST request (enroll) to: "); Serial.println(serverUrl);

        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        String jsonPayload = "{\"status\":\"enrolled\", \"fingerprint_id_real\":\"" + String(id) + "\"}";
        Serial.print("Payload: "); Serial.println(jsonPayload);

        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
          String response = http.getString();
          Serial.print("HTTP Response code (enroll): "); Serial.println(httpResponseCode);
          Serial.print("Response: "); Serial.println(response);
          blinkLed(feedbackLedPin, 2, 100, 50); // Optional: Blink external LED green for server success
        } else {
          Serial.print("Error on sending POST (enroll): "); Serial.println(httpResponseCode);
          Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
          blinkLed(feedbackLedPin, 3, 300, 150); 
        }
        http.end();
      } else {
        Serial.println("WiFi not connected. Failed to send enrollment data to server.");
        blinkLed(feedbackLedPin, 3, 300, 150);
      }
    } else { 
        Serial.print("Failed to store model. Error: 0x"); Serial.println(p_store, HEX);
        if (p_store == FINGERPRINT_PACKETRECIEVEERR) Serial.println("Communication error while storing.");
        else if (p_store == FINGERPRINT_BADLOCATION) Serial.println("Bad storage location.");
        else if (p_store == FINGERPRINT_FLASHERR) Serial.println("Flash write error.");
        else Serial.println("Unknown error storing model.");
        finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED); delay(1000); 
        finger.LEDcontrol(FINGERPRINT_LED_OFF);
    }
  } else {
    Serial.println("Enrollment process failed. Please try again.");
  }
  finger.LEDcontrol(FINGERPRINT_LED_OFF); 
}

// --- Fingerprint Login ---
void loginFingerprint() {
  Serial.println("Starting login process. Place finger on sensor...");

  uint8_t p = -1;
  p = getFingerprintImageAndConvertToTemplate(1); 

  if (p != FINGERPRINT_OK) {
    Serial.println("Login failed: Could not get valid fingerprint image.");
    return;
  }

  Serial.println("Searching for match...");
  finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE); 
  p = finger.fingerFastSearch();
  finger.LEDcontrol(FINGERPRINT_LED_OFF);

  if (p == FINGERPRINT_OK) {
    Serial.print("Fingerprint match found! ID: "); Serial.println(finger.fingerID);
    Serial.print("Confidence: "); Serial.println(finger.confidence);
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE); // BLUE for successful match (GREEN not available)
    delay(1000);
    finger.LEDcontrol(FINGERPRINT_LED_OFF);

    relayOn();
    delay(3000); 
    relayOff();

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.setTimeout(10000); 

      String serverUrl = "http://" + String(serverHost) + ":" + String(serverPort) + String(serverLoginPath);
      Serial.print("Sending POST request (login) to: "); Serial.println(serverUrl);

      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      String jsonPayload = "{\"status\":\"login\", \"fingerprint_id_real\":\"" + String(finger.fingerID) + "\"}";
      Serial.print("Payload: "); Serial.println(jsonPayload);

      int httpResponseCode = http.POST(jsonPayload);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("HTTP Response code (login): "); Serial.println(httpResponseCode);
        Serial.print("Response: "); Serial.println(response);
      } else {
        Serial.print("Error on sending POST (login): "); Serial.println(httpResponseCode);
        Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
        blinkLed(feedbackLedPin, 3, 300, 150); 
      }
      http.end();
    } else {
      Serial.println("WiFi not connected. Failed to send login data to server.");
      blinkLed(feedbackLedPin, 3, 300, 150);
    }
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Fingerprint not found in database.");
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED); 
    delay(1000);
    finger.LEDcontrol(FINGERPRINT_LED_OFF);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error during search.");
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED); delay(1000); finger.LEDcontrol(FINGERPRINT_LED_OFF);
  } else {
    Serial.print("Unknown error during search: 0x"); Serial.println(p, HEX);
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED); delay(1000); finger.LEDcontrol(FINGERPRINT_LED_OFF);
  }
}

