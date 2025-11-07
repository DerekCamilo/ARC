#include <WiFi.h>
#include <esp_now.h>
#include "esp_wifi.h"
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//LCD CONFIG
#define I2C_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2
#define I2C_SDA 21
#define I2C_SCL 22
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

//CONFIG
#define WIFI_SSID     "ssid"
#define WIFI_PASSWORD "password"
#define API_KEY       "api_key"
#define DATABASE_URL  "database_url"

//FIREBASE OBJECTS
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//ESPNOW CONFIG
uint8_t peerMac[6] = {0x94, 0x54, 0xC5, 0xCF, 0xDA, 0x88}; // other device's MAC
//const int ESPNOW_CHANNEL = 1;

//SEND STRING HELPER
bool sendString(const String &msg) {
  esp_err_t res = esp_now_send(peerMac, (const uint8_t *)msg.c_str(), msg.length());
  return (res == ESP_OK);
}

//SYNC TIME HELPER
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

//UNIX MILLIS
long long getUnixMillis() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return ((long long)tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
}

//LCD HELPERS
void lcdShowBoot() {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("ESP-NOW Ready");
  lcd.setCursor(0,1); lcd.print("Waiting...");
}

void lcdShowCode(const char* label, const char* digits) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(label);
  lcd.setCursor(0,1);
  lcd.print("CODE: ");
  lcd.print(digits);
}

void lcdShowText(const char* a, const char* b="") {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(a);
  lcd.setCursor(0,1); lcd.print(b);
}

//FIREBASE WRITE HELPERS
bool writeLogAtClientTs(const String& text) {
  long long ts = getUnixMillis();
  String path = "Log/" + String(ts);
  if (Firebase.RTDB.setString(&fbdo, path.c_str(), text)) {
    Serial.printf("Write Log OK -> %s\n", path.c_str());
    return true;
  } else {
    Serial.printf("Write Log failed [%s]: %s\n", path.c_str(), fbdo.errorReason().c_str());
    return false;
  }
}

bool writeVerifiedAtClientTs(bool value) {
  long long ts = getUnixMillis();
  String path = "Verified/" + String(ts);
  if (Firebase.RTDB.setBool(&fbdo, path.c_str(), value)) {
    Serial.printf("Write Verified OK -> %s = %s\n", path.c_str(), value ? "true" : "false");
    return true;
  } else {
    Serial.printf("Write Verified failed [%s]: %s\n", path.c_str(), fbdo.errorReason().c_str());
    return false;
  }
}

// QUEUE FOR ESP-NOW CALLBACK
typedef struct {
  bool hasNFC;
  char nfc[192];
  bool hasVerified;
  bool verified;
} Event;
QueueHandle_t evtQ = nullptr;

//ESPNOW CALLBACKS
//SENT CALLBACK
void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print(" Send -> ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
  if (status != ESP_NOW_SEND_SUCCESS) {
    lcdShowText("Send failed", "Check peer MAC");
  }
}

//RECIEVE CALLBACK
void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  Serial.print("MSG From ");
  for (int i = 0; i < 6; i++) {
    if (i) Serial.print(":");
    Serial.printf("%02X", info->src_addr[i]);
  }
  String msg;
  msg.reserve(len);
  for (int i = 0; i < len; i++) msg += (char)data[i];
  Serial.print(" -> ");
  Serial.println(msg);


  Event ev{}; // zeroed
  if (msg.startsWith("NFC:")) {
    String n = msg.substring(4);
    n.trim();
    n.replace("\r"," "); n.replace("\n"," ");
    strncpy(ev.nfc, n.c_str(), sizeof(ev.nfc)-1);
    ev.hasNFC = true;
  }
  int vpos = msg.indexOf("VERIFIED:");
  if (vpos >= 0) {
    String t = msg.substring(vpos + 9);
    t.trim(); t.toLowerCase();
    if (t.startsWith("true"))  { ev.hasVerified = true; ev.verified = true;  }
    if (t.startsWith("false")) { ev.hasVerified = true; ev.verified = false; }
  }

  //push to queue
  if ((ev.hasNFC || ev.hasVerified) && evtQ) {
    xQueueSend(evtQ, &ev, 0);
  }

  // CODE handling unchanged
  if (msg.startsWith("CODE:")) {
    int idx = msg.indexOf(':');
    String digits = msg.substring(idx + 1);
    digits.trim();
    char six[7] = {0};
    snprintf(six, sizeof(six), "%06d", digits.toInt());
    lcdShowCode("RX CODE", six);
    return;
  }

  //CODE SEND TO S-ESP
  uint32_t r = esp_random() % 1000000;
  char digits[7];
  snprintf(digits, sizeof(digits), "%06u", (unsigned)r);
  char reply[16];
  snprintf(reply, sizeof(reply), "CODE: %s", digits);
  bool ok = sendString(String(reply));
  Serial.print("Sent reply: ");
  Serial.println(reply);
  lcdShowCode("TX CODE", digits);
  if (!ok) {
    Serial.println("[ERR] esp_now_send failed");
    lcdShowText("TX failed", "esp_now_send()");
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

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

  // if (!Firebase.RTDB.setString(&fbdo, path.c_str(), "This is working!!! test (9:32)")) {
  //   Serial.printf("Write failed: %s\n", fbdo.errorReason().c_str());
  // } else {
  //   Serial.println("Write OK");
  // }

  // LCD init
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcdShowText("Booting...", "");

  // WiFi / ESPNOW init
  WiFi.mode(WIFI_STA);
  uint8_t primaryChannel;
  wifi_second_chan_t secondChan;
  esp_wifi_get_channel(&primaryChannel, &secondChan);
  esp_wifi_set_channel(primaryChannel, WIFI_SECOND_CHAN_NONE);

  Serial.print("This ESP MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    lcdShowText("ESP-NOW", "init failed!");
    while (true) delay(1000);
  }

  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onRecv);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, peerMac, 6);
  peer.ifidx   = WIFI_IF_STA;
  peer.channel = primaryChannel;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Failed to add peer!");
    lcdShowText("Peer add", "FAILED!");
  } else {
    lcdShowBoot();
  }

  // queue
  evtQ = xQueueCreate(10, sizeof(Event));

  // Seed randomness once
  randomSeed((uint32_t)esp_random());
}

void loop() {
  // QUEUE FOR CALLBACK
  Event ev;
  while (evtQ && xQueueReceive(evtQ, &ev, 0) == pdTRUE) {
    if (!Firebase.ready()) {
      Serial.println("[WARN] Firebase not ready; skipping queued write");
      continue;
    }
    if (ev.hasNFC) {
      writeLogAtClientTs(String(ev.nfc));
    }
    if (ev.hasVerified) {
      writeVerifiedAtClientTs(ev.verified);
    }
  }
}