#include <WiFi.h>
#include <esp_now.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Keypad.h>
#include "esp_wifi.h"

// Row and column numbers (4x4 keypad)
#define ROW_NUM     4
#define COLUMN_NUM  4

// LED pins
int blueLedPin = 4;
int redLedPin = 21;

// Passwords and match variable to see if they're equivalent
String password = "";
String secretPassword = "";
bool match = false;

// Keypad layout for 4x4
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Pins setup for keypad
byte pin_rows[ROW_NUM]      = {13, 12, 14, 27};
byte pin_column[COLUMN_NUM] = {26, 2, 33, 32};

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// PN532 Setup (NFC)
#define PN532_CS 5
Adafruit_PN532 nfc(PN532_CS);

// MAC address of receiver for ESP NOW
uint8_t peerMac[6] = {0x04, 0x83, 0x08, 0x74, 0xEE, 0x9C};

// SSID of the WiFi / Connection
static const char* HUB_SSID = "I209-3000@ENCLAVE";

// Helper function that finds the channel of the WiFi / connection
static uint8_t findApChannel(const char* ssid) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);

  // Scans for the correct network and then grabs its channel, returning it. 
  Serial.println("Scanning for AP channel...");
  int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
  if (n <= 0) { 
    Serial.println("No networks found"); return 0; 
  }

  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == ssid) {
      int ch = WiFi.channel(i);
      Serial.printf("Found %s on channel %d\n", ssid, ch);
      WiFi.scanDelete();
      return (uint8_t)ch;
    }
  }
  WiFi.scanDelete();
  Serial.println("Target SSID not found");
  return 0;
}

// Function to receive message
void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  // Takes in message and prints out each character
  String msg;
  for (int i = 0; i < len; i++) {
    msg += (char)data[i];
  }
  msg.trim();

  Serial.print("Received: ");
  Serial.println(msg);

  // Checks for secret code by seeing if it starts with prefix CODE:, setting secret password if so
  if (msg.startsWith("CODE:")) {
    secretPassword = msg.substring(5);  // everything after "CODE:"
    Serial.print("Secret code received: ");
    Serial.println(secretPassword);
  }
}

void setup() {
  Serial.begin(115200);

  // Setup for LED pins for password matching
  pinMode(blueLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  digitalWrite(blueLedPin, LOW);
  digitalWrite(redLedPin, LOW);

  // NFC setup (initialization)
  nfc.begin();

  // Makes NFC scan return false after brief period in order to allow keypad to cycle through and register input
  // NFC reader stuck waiting for NFC tag otherwise
  nfc.setPassiveActivationRetries(1);

  // Allows NFC reader to start reading input
  nfc.SAMConfig();
  Serial.println("Ready to scan.");

  // WiFi setup
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);

  // Finds the WiFi / connection channel to use to communicate with other ESP32
  uint8_t ch = findApChannel(HUB_SSID);
  if (ch == 0) {
    // Fallback if scan fails—set this to your router's known channel to avoid mismatch
    ch = 6;
    Serial.printf("Fallback to channel %u\n", ch);
  }
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  Serial.printf("ESP-NOW will use channel %u\n", ch);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (1) {
      delay(1000);
    }
  }

  // Registers the receiver ESP32 as a peer in order to communicate via ESP NOW
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, peerMac, 6);
  peer.ifidx = WIFI_IF_STA;
  peer.channel = ch;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  esp_now_register_recv_cb(onRecv);

  /*
  Serial.print("This ESP MAC: ");
  Serial.println(WiFi.macAddress());
  */
  Serial.println("Peer registered\n");
}

void loop() {
  // Once an nfc is scanned, will change to true and allow for password input
  static bool nfcVerified = false;
  uint8_t uid[7];
  uint8_t uidLength;

  // Waits until an NFC tag is scanned before proceeding further
  if (!nfcVerified) {
    // Attempts to read in nearby nfc
    boolean success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    // If nfc successfully found, attempts to read in text data from it.
  if (success) {
    uint8_t data[4];
    String text = "";
    bool textFound = false;

    // Read text bytes (NDEF text payload often at pages 4-15)
    for (uint8_t page = 4; page < 16; page++) {
      if (nfc.mifareultralight_ReadPage(page, data)) {
        for (uint8_t i = 0; i < 4; i++) {
          char c = (char)data[i];
          if (isPrintable(c)) {
            text += c;
            textFound = true;
          }
        }
      }
    }

    // Removes header and cleans up formatting to just get the text needed
    int tIndex = text.indexOf("T");
    if (tIndex != -1 && text.length() > tIndex + 3) {
      text = text.substring(tIndex + 3);
    }

    // If text data is found, sends the text over to the receiver ESP32. 
    if (textFound && text.length() > 0) {
      Serial.print("NFC text: ");
      Serial.println(text);

      // Format NFC message with prefix
      String nfcMsg = "NFC: " + text;

      // Send the formatted NFC message to the receiver ESP32
      esp_err_t result = esp_now_send(peerMac, (uint8_t *)nfcMsg.c_str(), nfcMsg.length());

      if (result == ESP_OK) {
        Serial.println("NFC sent");
      } else {
        Serial.println("NFC not sent");
      }

      // Sets nfcVerified to true as an nfc was successfully read in. Clears password and allows keypad input.
      nfcVerified = true;
      password = "";
      Serial.println("NFC verified — Enter 6 digit password.");
    } else {
      Serial.println("No text.");
    }
    delay(100);
  }
    // If no tag detected, goes back to the beginning and runs until tag is detected
    return;
  }

  // Keypad / Password Entry Section

  // Waits until the receiver sends over the secret password
  if (secretPassword == "") {
    return;
  }

  // Sets up keypad entry
  char key = keypad.getKey();

  // If a key is pressed, appends it to current password input. Once it reaches 6, compares to secret password and sends result.
  if (key) {
    Serial.print("Key pressed: ");
    Serial.println(key);

    // Makes sure that a nnumber is entered, not letters.
    if (key >= '0' && key <= '9') {
      password += key;
      Serial.print("Current input: ");
      Serial.println(password);
    } else if (key == '*') {
      password = "";
      Serial.println("Password cleared.");
    }

    // Compares passwords upon entering 6 digits
    if (password.length() == 6) {
      password.trim();
      secretPassword.trim();

      Serial.print("Entered: ");
      Serial.println(password);
      Serial.print("Expected: ");
      Serial.println(secretPassword);

      if(password == secretPassword)
      {
        match = true;
      } else
      {
        match = false;
      }

      // Flashes blue LED if match successful, red LED if not
      if (match) {
        Serial.println("MATCH");
        digitalWrite(blueLedPin, HIGH);
        delay(2000);
        digitalWrite(blueLedPin, LOW);
      } else {
        Serial.println("NO MATCH ");
        digitalWrite(redLedPin, HIGH);
        delay(2000);
        digitalWrite(redLedPin, LOW);
      }

      /// Sends the verification result to the receiver ESP32 in required format
      String resultMsg = "VERIFIED: " + String(match ? "true" : "false");
      esp_now_send(peerMac, (uint8_t *)resultMsg.c_str(), resultMsg.length());

      // Resets password back to blank and awaits next nfc read
      password = "";
      secretPassword = "";
      nfcVerified = false;
      Serial.println("Process complete — please scan next NFC tag.");
    }
  }

  delay(1);
}
