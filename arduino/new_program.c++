#include <Adafruit_Fingerprint.h>
#include <WiFi.h>        // For ESP32 Wi-Fi
#include <HTTPClient.h>  // For ESP32 HTTP requests

// --- Wi-Fi Configuration ---
const char* ssid = "YOUR_WIFI_SSID";         // <<<<<<<<<<< REPLACE WITH YOUR WI-FI SSID
const char* password = "YOUR_WIFI_PASSWORD"; // <<<<<<<<<<< REPLACE WITH YOUR WI-FI PASSWORD

// --- Server Configuration ---
// CRITICAL NOTE: "127.0.0.1" (localhost) WILL NOT WORK from the ESP32 to reach your PC.
// You MUST replace "127.0.0.1" with the actual IP address of your PC 
// on the local network (e.g., "192.168.1.100").
const char* serverHost = "127.0.0.1"; // <<<<<<<<<<< REPLACE WITH YOUR PC's ACTUAL LAN IP ADDRESS
const int serverPort = 8000;
const char* serverLoginPath = "/api/fingerprint_login";  // API endpoint for login
const char* serverEnrollPath = "/api/fingerprint_enroll"; // API endpoint for enrollment

// Inisialisasi UART fingerprint
HardwareSerial mySerial(2); // Serial2 uses GPIO16 (RX2) and GPIO17 (TX2) by default.
Adafruit_Fingerprint finger(&mySerial);

// LED login di pin 4
const int loginLed = 4; // Make sure this pin is available and correct for your ESP32 board

// Maximum number of fingerprints the sensor can store (consult your sensor's datasheet)
const int MAX_FINGERPRINTS = 127; 


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
    if (retries > 20) { // Timeout after 10 seconds
        Serial.println("\nFailed to connect to WiFi. Please check credentials or network.");
        // You might want to implement a different behavior here, e.g. retry later or enter a config mode
        return; 
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect (needed for native USB)
  delay(1000);

  // Setup LED output
  pinMode(loginLed, OUTPUT);
  digitalWrite(loginLed, LOW); // Matikan LED saat awal

  // Setup Wi-Fi
  setupWifi(); // Ensure Wi-Fi is set up

  // Serial untuk sensor: RX=16, TX=17
  mySerial.begin(57600, SERIAL_8N1, 16, 17); // Adjust pins if necessary for your board
  finger.begin(57600);

  Serial.println("\nInisialisasi sensor sidik jari...");

  if (finger.verifyPassword()) {
    Serial.println("Sensor fingerprint terdeteksi!");
  } else {
    Serial.println("Sensor fingerprint tidak ditemukan. Periksa koneksi!");
    while (1) { delay(1); } // Halt
  }
  // Get sensor capacity after verification
  // finger.getTemplateCount(); // Prime templateCount, good practice though findNextAvailableID will do it.
  // Serial.print("Kapasitas sensor: "); Serial.println(finger.capacity);


  Serial.println("Ketik perintah:");
  Serial.println("  'a' untuk daftar otomatis (auto enroll & notify)");
  Serial.println("  'd' untuk daftar manual (enroll by ID, no notify yet)");
  Serial.println("  'l' untuk login (verifikasi & notify)");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    // Consume any extra characters like newline
    while(Serial.available()) Serial.read();

    if (cmd == 'a') {
      Serial.println("Memulai pendaftaran otomatis...");
      int newId = findNextAvailableID();
      if (newId == 0) { // 0 indicates error or full
        Serial.println("Tidak dapat mendaftar: Tidak ada ID tersedia atau database penuh.");
      } else {
        enrollFingerprintAndNotify(newId);
      }
    } else if (cmd == 'd') {
      Serial.println("Masukkan ID (1~" + String(MAX_FINGERPRINTS) + ") untuk disimpan (pendaftaran manual):");
      while (!Serial.available()); // Wait for user input
      String idStr = Serial.readStringUntil('\n');
      idStr.trim();
      int id = idStr.toInt();

      if (id < 1 || id > MAX_FINGERPRINTS) {
        Serial.println("ID tidak valid.");
        return;
      }
      // Note: This manual enrollFingerprint_original does not yet call the HTTP POST.
      // You could modify it or call enrollFingerprintAndNotify(id) if you want it to notify.
      enrollFingerprint_original(id); 
    } else if (cmd == 'l') {
      loginFingerprint();
    } else {
      Serial.print("Perintah tidak dikenal: ");
      Serial.println(cmd);
    }
  }
}

int findNextAvailableID() {
  Serial.println("Mencari ID yang tersedia...");
  uint8_t p = finger.getTemplateCount(); // This updates finger.templateCount

  if (p != FINGERPRINT_OK) {
    Serial.print("Gagal mendapatkan jumlah template. Kode error: 0x"); 
    // The actual error code for getTemplateCount might be in finger.lastPacket->data[0]
    // or it might be the return value 'p' itself if it's a direct error code.
    // For simplicity, we check 'p'. Consult library for specific error details if needed.
    Serial.println(p, HEX); 
    return 0; // Error
  }
  
  int current_template_count = finger.templateCount;
  Serial.print("Jumlah template tersimpan saat ini: "); Serial.println(current_template_count);

  if (current_template_count >= MAX_FINGERPRINTS) {
    Serial.println("Database sidik jari penuh.");
    return 0; // Database full
  }
  
  // Next ID is count + 1 (IDs are typically 1-indexed)
  int next_id = current_template_count + 1;
  Serial.print("ID berikutnya yang akan digunakan untuk pendaftaran: "); Serial.println(next_id);
  return next_id;
}


// Fungsi daftar sidik jari (versi original tanpa notifikasi HTTP)
// Kept for users who might want manual enrollment without notification, or for reference
void enrollFingerprint_original(int id) {
  int p = -1;
  Serial.print("Mendaftarkan ID #"); Serial.println(id);
  Serial.println("Letakkan jari pertama...");
  finger.LEDcontrol(FINGERPRINT_LED_ON, 255, FINGERPRINT_LED_BLUE); 

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK: Serial.println("Gambar diambil"); break;
      case FINGERPRINT_NOFINGER: delay(50); break;
      case FINGERPRINT_PACKETRECIEVEERR: Serial.println("Kesalahan komunikasi"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
      case FINGERPRINT_IMAGEFAIL: Serial.println("Gagal mengambil gambar"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
      default: Serial.println("Kesalahan tidak diketahui saat getImage"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
    }
  }

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK: Serial.println("Gambar diubah ke template 1"); break;
    case FINGERPRINT_IMAGEMESS: Serial.println("Gambar terlalu berantakan"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
    // ... (add other specific error cases from Adafruit_Fingerprint.h for image2Tz if desired)
    default: Serial.print("Gagal mengubah gambar ke template 1. Kode: 0x"); Serial.println(p, HEX); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
  }

  Serial.println("Angkat jari...");
  finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 10, FINGERPRINT_LED_PURPLE, 250);
  delay(1000); 
  p = FINGERPRINT_OK; // Assume finger is there initially
  while (p != FINGERPRINT_NOFINGER) { // Wait until finger is removed
    p = finger.getImage();
    delay(50); 
  }
  Serial.println("Jari diangkat.");


  Serial.println("Letakkan jari yang sama lagi...");
  finger.LEDcontrol(FINGERPRINT_LED_ON, 255, FINGERPRINT_LED_BLUE);
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK: Serial.println("Gambar kedua diambil"); break;
      case FINGERPRINT_NOFINGER: delay(50); break;
      // ... (similar error handling as first getImage)
      default: Serial.print("Gagal mengambil gambar kedua. Kode: 0x"); Serial.println(p, HEX); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
    }
  }

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK: Serial.println("Gambar kedua diubah ke template 2"); break;
    // ... (similar error handling as first image2Tz)
    default: Serial.print("Gagal mengubah gambar ke template 2. Kode: 0x"); Serial.println(p, HEX); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
  }

  Serial.print("Membuat model untuk ID #"); Serial.println(id);
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Sidik jari cocok! Model dibuat.");
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Sidik jari tidak cocok. Ulangi."); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
  } else {
    Serial.print("Gagal membuat model. Kode: 0x"); Serial.println(p, HEX); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
  }

  Serial.print("Menyimpan model ID #"); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Pendaftaran berhasil (manual)!");
    finger.LEDcontrol(FINGERPRINT_LED_SOLID, 10, FINGERPRINT_LED_GREEN, 1000);
  } else {
    Serial.print("Gagal menyimpan sidik jari. Kode: 0x"); Serial.println(p, HEX);
    finger.LEDcontrol(FINGERPRINT_LED_SOLID, 10, FINGERPRINT_LED_RED, 1000);
  }
  delay(1000); 
  finger.LEDcontrol(FINGERPRINT_LED_OFF);
}


// Fungsi daftar sidik jari otomatis DENGAN notifikasi HTTP POST
void enrollFingerprintAndNotify(int id) {
  int p = -1;
  Serial.print("Mendaftarkan (otomatis) ID #"); Serial.println(id);
  Serial.println("Letakkan jari pertama...");
  finger.LEDcontrol(FINGERPRINT_LED_ON, 255, FINGERPRINT_LED_BLUE); 

  // --- Get Image 1 ---
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK: Serial.println("Gambar diambil"); break;
      case FINGERPRINT_NOFINGER: delay(50); break; // Wait for finger
      case FINGERPRINT_PACKETRECIEVEERR: Serial.println("Kesalahan komunikasi"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
      case FINGERPRINT_IMAGEFAIL: Serial.println("Gagal mengambil gambar"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
      default: Serial.println("Kesalahan tidak diketahui saat getImage"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
    }
  }

  // --- Convert Image 1 to Template ---
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK: Serial.println("Gambar diubah ke template 1"); break;
    case FINGERPRINT_IMAGEMESS: Serial.println("Gambar terlalu berantakan"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
    case FINGERPRINT_PACKETRECIEVEERR: Serial.println("Kesalahan komunikasi (image2Tz 1)"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
    case FINGERPRINT_FEATUREFAIL: Serial.println("Tidak dapat menemukan fitur sidik jari (image2Tz 1)"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
    case FINGERPRINT_INVALIDIMAGE: Serial.println("Gambar tidak valid (image2Tz 1)"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
    default: Serial.print("Gagal mengubah gambar ke template 1. Kode: 0x"); Serial.println(p, HEX); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
  }

  Serial.println("Angkat jari...");
  finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 10, FINGERPRINT_LED_PURPLE, 250);
  delay(1000); 
  p = FINGERPRINT_OK; // Reset p to enter loop
  while (p != FINGERPRINT_NOFINGER) { // Wait until finger is removed
    p = finger.getImage(); // This is just to detect FINGERPRINT_NOFINGER
    delay(50); 
  }
  Serial.println("Jari diangkat.");


  Serial.println("Letakkan jari yang sama lagi...");
  finger.LEDcontrol(FINGERPRINT_LED_ON, 255, FINGERPRINT_LED_BLUE);
  p = -1; // Reset p for next getImage loop
  // --- Get Image 2 ---
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK: Serial.println("Gambar kedua diambil"); break;
      case FINGERPRINT_NOFINGER: delay(50); break;
      case FINGERPRINT_PACKETRECIEVEERR: Serial.println("Kesalahan komunikasi (getImage 2)"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
      case FINGERPRINT_IMAGEFAIL: Serial.println("Gagal mengambil gambar kedua"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
      default: Serial.print("Gagal mengambil gambar kedua. Kode: 0x"); Serial.println(p, HEX); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
    }
  }

  // --- Convert Image 2 to Template ---
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK: Serial.println("Gambar kedua diubah ke template 2"); break;
    // ... (add other specific error cases as for image2Tz(1))
    default: Serial.print("Gagal mengubah gambar ke template 2. Kode: 0x"); Serial.println(p, HEX); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
  }

  // --- Create Model ---
  Serial.print("Membuat model untuk ID #"); Serial.println(id);
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Sidik jari cocok! Model dibuat.");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Kesalahan komunikasi (createModel)"); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Sidik jari tidak cocok. Ulangi."); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
  } else {
    Serial.print("Gagal membuat model. Kode: 0x"); Serial.println(p, HEX); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
  }

  // --- Store Model ---
  Serial.print("Menyimpan model ID #"); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Pendaftaran berhasil!");
    finger.LEDcontrol(FINGERPRINT_LED_SOLID, 10, FINGERPRINT_LED_GREEN, 1000); 

    // --- Send HTTP POST Request for Enrollment ---
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String serverUrl = "http://" + String(serverHost) + ":" + String(serverPort) + String(serverEnrollPath);
      
      Serial.print("Mengirim POST request (enroll) ke: "); Serial.println(serverUrl);

      http.begin(serverUrl); 
      http.addHeader("Content-Type", "application/json");

      String jsonPayload = "{\"status\":\"enrolled\", \"fingerId\":\"" + String(id) + "\"}";
      Serial.print("Payload: "); Serial.println(jsonPayload);

      int httpResponseCode = http.POST(jsonPayload);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("HTTP Response code (enroll): "); Serial.println(httpResponseCode);
        Serial.print("Response: "); Serial.println(response);
      } else {
        Serial.print("Error on sending POST (enroll): "); Serial.println(httpResponseCode);
        Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      http.end(); 
    } else {
      Serial.println("WiFi tidak terhubung. Tidak dapat mengirim data pendaftaran.");
    }
    // --- End HTTP POST ---

  } else {
    Serial.print("Gagal menyimpan sidik jari. Kode: 0x"); Serial.println(p, HEX);
    finger.LEDcontrol(FINGERPRINT_LED_SOLID, 10, FINGERPRINT_LED_RED, 1000); // Red LED for error
  }
  delay(1000); // Keep LED on for a bit
  finger.LEDcontrol(FINGERPRINT_LED_OFF);
}


// Fungsi login fingerprint
void loginFingerprint() {
  Serial.println("Letakkan jari untuk login...");
  finger.LEDcontrol(FINGERPRINT_LED_ON, 255, FINGERPRINT_LED_BLUE); 

  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) { delay(50); continue; }
    if (p != FINGERPRINT_OK) {
      Serial.println("Gagal mengambil gambar. Coba lagi."); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
    }
  }
  Serial.println("Gambar diambil.");

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.print("Gagal mengubah gambar ke template (login). Kode: 0x"); Serial.println(p, HEX); finger.LEDcontrol(FINGERPRINT_LED_OFF); return;
  }
  Serial.println("Gambar diubah ke template (login).");

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("Login berhasil! ID: "); Serial.println(finger.fingerID);
    Serial.print("Confidence: "); Serial.println(finger.confidence);

    digitalWrite(loginLed, HIGH);
    finger.LEDcontrol(FINGERPRINT_LED_SOLID, 10, FINGERPRINT_LED_GREEN, 3000);

    // --- Send HTTP POST Request for Login ---
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String serverUrl = "http://" + String(serverHost) + ":" + String(serverPort) + String(serverLoginPath);
      
      Serial.print("Mengirim POST request (login) ke: "); Serial.println(serverUrl);

      http.begin(serverUrl); 
      http.addHeader("Content-Type", "application/json");

      String jsonPayload = "{\"fingerId\":\"" + String(finger.fingerID) + "\", \"confidence\":\"" + String(finger.confidence) + "\"}";
      Serial.print("Payload: "); Serial.println(jsonPayload);

      int httpResponseCode = http.POST(jsonPayload);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("HTTP Response code (login): "); Serial.println(httpResponseCode);
        Serial.print("Response: "); Serial.println(response);
      } else {
        Serial.print("Error on sending POST (login): "); Serial.println(httpResponseCode);
        Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      http.end(); 
    } else {
      Serial.println("WiFi tidak terhubung. Tidak dapat mengirim data login.");
    }
    // --- End HTTP POST ---

    delay(3000); 
    digitalWrite(loginLed, LOW);

  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Sidik jari tidak dikenali.");
    finger.LEDcontrol(FINGERPRINT_LED_SOLID, 10, FINGERPRINT_LED_RED, 1000); 
  } else { // Covers FINGERPRINT_PACKETRECIEVEERR and other errors
    Serial.print("Kesalahan saat mencari sidik jari. Kode: 0x"); Serial.println(p, HEX);
    finger.LEDcontrol(FINGERPRINT_LED_SOLID, 10, FINGERPRINT_LED_RED, 1000);
  }
  
  delay(1000); 
  finger.LEDcontrol(FINGERPRINT_LED_OFF);
}