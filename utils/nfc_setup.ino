#if 1
  #define NFC_INTERFACE_SPI
  #include <SPI.h>
  #include <PN532_SPI.h>
  #include <PN532_SPI.cpp>
  #include "PN532.h"

  PN532_SPI pn532spi(SPI, 5); // use GPIO 5 as CS (safer on ESP32)
  PN532 nfc(pn532spi);
#endif

void setup() {
  Serial.begin(115200);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    while (1); // halt if PN532 not found
  }

  nfc.SAMConfig();
  Serial.println("Waiting for NFC tag...");
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    uint8_t data[4];
    String text = "";
    bool textFound = false;

    // Read a few pages — text NDEF payloads usually start at page 4
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

    // Attempt to trim NDEF header (remove leading T/en)
    int tIndex = text.indexOf("T");
    if (tIndex != -1 && text.length() > tIndex + 3) {
      // Skip 'T', language code (2 chars)
      text = text.substring(tIndex + 3);
    }

    if (textFound) {
      Serial.println(text);
    } else {
      Serial.println("No text data found on tag.");
    }

    // Faster debounce: wait only until tag is removed, short delay
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      delay(50); // faster debounce
    }

    delay(100); // short cooldown before next scan
  }

  delay(50); // small idle delay
}
