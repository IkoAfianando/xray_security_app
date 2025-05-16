#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Fingerprint.h> // Library yang umum digunakan untuk R307
#include <SoftwareSerial.h>       // Mungkin diperlukan, tergantung koneksi

// ----- Konfigurasi Jaringan Wi-Fi -----
const char* ssid = "NamaWiFiAnda";
const char* password = "PasswordWiFiAnda";

// ----- Konfigurasi Server API -----
const char* serverName = "http://<alamat_ip_server_api>:<port_api>/audit-log";
const char* otherServerName = "http://<alamat_ip_server_api>:<port_api>/other-pointing-logic";

// ----- Konfigurasi Sensor Sidik Jari R307 -----
// *** Sesuaikan pin RX dan TX sesuai dengan koneksi Anda ke ESP32 ***
// Contoh menggunakan SoftwareSerial pada pin 2 (RX ESP) dan 3 (TX ESP)
SoftwareSerial mySerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Jika Anda menggunakan Serial1 (pin TX1, RX1 pada ESP32), nonaktifkan SoftwareSerial dan aktifkan ini:
//#define USE_SERIAL1
//#ifdef USE_SERIAL1
//  Adafruit_Fingerprint finger(&Serial1);
//#else
//  SoftwareSerial mySerial(2, 3);
//  Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
//#endif

void connectWiFi() {
  Serial.print("Menghubungkan ke WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi terhubung");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(10);

  connectWiFi();

  // Inisialisasi Sensor Sidik Jari R307
#ifdef USE_SERIAL1
  Serial1.begin(57600); // Baud rate default R307
#else
  mySerial.begin(57600); // Baud rate default R307
#endif
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Sensor sidik jari ditemukan!");
  } else {
    Serial.println("Tidak dapat menemukan sensor sidik jari :(");
    while (1);
  }

  finger.templateSize = finger.getTemplateCount();
  Serial.print("Ukuran template yang tersimpan: ");
  Serial.println(finger.templateSize);
}

uint8_t getFingerprintID() {
  Serial.println("\nSedang mencari sidik jari...");
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Gambar diambil");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("Tidak ada jari terdeteksi");
      return 0; // Mengembalikan 0 sebagai indikasi tidak ada jari
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Kesalahan saat mengambil gambar");
      return 0;
    default:
      Serial.print("Kesalahan yang tidak diketahui: 0x"); Serial.println(p, HEX);
      return 0;
  }

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Gambar dikonversi");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Gambar terlalu berantakan");
      return 0;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Tidak dapat menemukan fitur sidik jari");
      return 0;
    case FINGERPRINT_NODETECT:
      Serial.println("Tidak dapat menemukan detail sidik jari");
      return 0;
    default:
      Serial.print("Kesalahan yang tidak diketahui: 0x"); Serial.println(p, HEX);
      return 0;
  }

  Serial.println("Mencari padanan...");
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("Ditemukan padanan untuk ID #");
    Serial.println(finger.fingerID);
    return finger.fingerID;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Sidik jari tidak ditemukan");
    return 0;
  } else {
    Serial.print("Kesalahan saat mencari: 0x"); Serial.println(p, HEX);
    return 0;
  }
}

void sendAuditLog(int fingerprintId) {
  if (fingerprintId == 0) {
    Serial.println("Tidak ada sidik jari valid untuk dikirim.");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<128> jsonDocument;
    jsonDocument["fingerprint_id"] = fingerprintId;
    jsonDocument["timestamp"] = millis();

    char jsonBuffer[128];
    serializeJson(jsonDocument, jsonBuffer);

    int httpResponseCode = http.POST(jsonBuffer);

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println("Payload:");
      Serial.println(payload);
    } else {
      Serial.print("Error pada permintaan HTTP: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("WiFi tidak terhubung.");
  }
}

void loop() {
  delay(50);
  if (finger.available()) {
    int fingerprintId = getFingerprintID();
    if (fingerprintId > 0) {
      Serial.print("Sidik jari terdeteksi dengan ID: ");
      Serial.println(fingerprintId);
      sendAuditLog(fingerprintId);
    }
  }
}