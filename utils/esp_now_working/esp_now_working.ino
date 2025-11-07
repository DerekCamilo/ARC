#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"

#define WIFI_SSID     "I209-3000@ENCLAVE"
#define WIFI_PASSWORD "*6mO6p0ba2TD"
#define API_KEY       "AIzaSyDhc2-M7o61YzJj1IcyrdSW_T6YYtC4BUU"
#define DATABASE_URL  "https://arc-team-no-clue-default-rtdb.firebaseio.com/"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Syncing time...");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" done");
}

long long getUnixMillis() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return ((long long)tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
}

void setup() {
  Serial.begin(115200);
  
  // Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(300); }
  Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());

  // Sync Time
  syncTime();

  // Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase signUp OK");
  } else {
    Serial.printf("Firebase signUp error: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  long long timestamp = getUnixMillis();
  String path = "Log/" + String(timestamp); // use timestamp or UID as key
  
  if (!Firebase.RTDB.setString(&fbdo, path.c_str(), "This is working!!! test (9:32)")) {
    Serial.printf("Write failed: %s\n", fbdo.errorReason().c_str());
  } else {
    Serial.printf("Write failed: %s\n", fbdo.errorReason().c_str());
  }
}

void loop() {
  // not needed rn
}