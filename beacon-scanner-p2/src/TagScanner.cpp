#include "TagScanner.h"

TagScanner::TagScanner(SPIClass* spi, uint8_t rstPin, uint8_t csPin)
  : _rstPin{rstPin}, _csPin{csPin} {
  _reader = std::make_unique<MFRC522>(_csPin, _rstPin);
}

void TagScanner::init() {
  _reader->setSPIConfig();
  _reader->PCD_Init();
}

bool TagScanner::update() {
  if (!_reader->PICC_IsNewCardPresent()) {
    return false;
  }

  // Select one of the cards
  if (!_reader->PICC_ReadCardSerial()) {
    return false;
  }

  return true;
}

std::optional<int16_t> TagScanner::getCardId() {
  bool hasCard = update();

  if (!hasCard) {
    return std::nullopt;
  }

  byte buf[18];
  byte size = sizeof(buf);

  MFRC522::StatusCode status = (MFRC522::StatusCode)_reader->MIFARE_Read(4, buf, &size);
  Serial.printf("Scanned card with status=%d", static_cast<int>(status));

  if (status == MFRC522::StatusCode::STATUS_OK) {
    return *(reinterpret_cast<uint16_t*>(buf));
  }

  return -1;
}

std::optional<bool> TagScanner::writeCardId(int16_t id) {
  bool hasCard = update();

  if (!hasCard) {
    return std::nullopt;
  }

  byte buf[4];
  memcpy(buf, &id, sizeof(id));

  MFRC522::StatusCode status = (MFRC522::StatusCode)_reader->MIFARE_Ultralight_Write(4, buf, sizeof(buf));

  if (status == MFRC522::StatusCode::STATUS_OK) {
    Serial.println("Successfully wrote new ID to card");
    _reader->PICC_HaltA();

    return true;
  }

  return false;
}