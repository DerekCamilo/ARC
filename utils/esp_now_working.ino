#include <WiFi.h>
#include <esp_now.h>

// ---- Firebase ----
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ---- I2C LCD (4-pin: VCC, GND, SDA, SCL) ----
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Common I2C address; change if needed

// ====== EDIT THESE ======
#define WIFI_SSID     "ssid"
#define WIFI_PASSWORD "wifiPassword"
#define API_KEY       "AIzaSyDhc2-M7o61YzJj1IcyrdSW_T6YYtC4BUU"
#define DATABASE_URL  "https://arc-team-no-clue-default-rtdb.firebaseio.com/"
// ========================

// Replace with the other ESP’s MAC address:
uint8_t peerMac[6] = { 0x94, 0x54, 0xC5, 0xCF, 0xDA, 0x88 };

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

static void logNFC(const String &uidHex) {
  FirebaseJson j;
  j.set("uid", uidHex);
  j.set("ts/.sv", "timestamp");
  Firebase.RTDB.pushJSON(&fbdo, "nfc/scans", &j);
}

static void logLED(bool on) {
  Firebase.RTDB.setBool(&fbdo, "LEDs/Accepted", on);
}

static void logKey(const String &key) {
  FirebaseJson j;
  j.set("key", key);
  j.set("ts/.sv", "timestamp");
  Firebase.RTDB.pushJSON(&fbdo, "keypad/presses", &j);
}

static void handleLine(const String &line) {
  if (line.startsWith("NFC:")) {
    String uid = line.substring(4); uid.trim();
    logNFC(uid);
  } else if (line.startsWith("LED:")) {
    String v = line.substring(4); v.trim();
    logLED(v == "1" || v.equalsIgnoreCase("true"));
  } else if (line.startsWith("KEY:")) {
    String k = line.substring(4); k.trim();
    logKey(k);
  }
}

void onRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  String line;
  line.reserve(len + 1);
  for (int i = 0; i < len; ++i) line += (char)data[i];
  line.trim();
  if (Firebase.ready()) handleLine(line);
}

void setup() {
  // ---- LCD ----
  Wire.begin(21, 22);  // SDA=21, SCL=22
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Access Code:");
  lcd.setCursor(0, 1);
  lcd.print("8008");

  // ---- Wi-Fi ----
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(250); }

  // ---- Firebase ----
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // ---- ESP-NOW ----
  if (esp_now_init() != ESP_OK) {
    while (true) delay(1000);  // hang if ESP-NOW fails
  }

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, peerMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&peer);
  esp_now_register_recv_cb(onRecv);
}

void loop() {
  // Device runs silently; nothing to do here.
}
