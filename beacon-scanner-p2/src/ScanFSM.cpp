#include "ScanFSM.h"

ScanFSM::ScanFSM(TagScanner& scanner, LiquidCrystal_I2C& lcd)
  : _currentState{SystemState::Idle}, _scanner{scanner}, _lcd{lcd} {}

SystemState ScanFSM::update() {
  switch (_currentState) {
    case SystemState::Idle:
      _currentState = handleIdle();
      break;
    case SystemState::Scanning:
      _currentState = handleScanning();
      break;
    case SystemState::ChangeQuantity:
      _currentState = handleChangeQuantity();
      break;
    case SystemState::ConfirmedScan:
      _currentState = handleConfirmedScan();
      break;
    case SystemState::WriteTagId:
      _currentState = handleWriteTagId();
      break;
    case SystemState::ConfirmedIdWrite:
      _currentState = handleConfirmedIdWrite();
      break;
    default:
      _currentState = SystemState::Idle;
  }

  return _currentState;
}

SystemState ScanFSM::handleIdle() {
  if (!digitalRead(Pins::BtnMiddle)) {
    resetLcd();
    _lcd.print("Adjust ID:");
    _lcd.setCursor(0, 3);
    _lcd.print("  -   Confirm   +");

    _tagId = 1;

    return SystemState::WriteTagId;
  }

  auto foundId = _scanner.getCardId();
  if (foundId) {
    _tagId = foundId.value();
    _lcd.setCursor(0, 1);
    _lcd.printf("Found Tag with ID =\n%d", _tagId);
    delay(1000);

    return SystemState::Scanning;
  }

  return SystemState::Idle;
}

SystemState ScanFSM::handleScanning() {
  resetLcd();
  _lcd.print("Adjust quantity:");
  _lcd.setCursor(0, 3);
  _lcd.print("  -   Confirm   +");

  _quantity = 0;
  
  return SystemState::ChangeQuantity;
}

SystemState ScanFSM::handleChangeQuantity() {
  if (!digitalRead(Pins::BtnLeft)) {
    _quantity--;
    _lcd.setCursor(0, 2);
    _lcd.printf("Value = %d", _quantity);
    delay(50);
  } else if (!digitalRead(Pins::BtnMiddle)) {
    return SystemState::ConfirmedScan;
  } else if (!digitalRead(Pins::BtnRight)) {
    _quantity++;
    _lcd.setCursor(0, 2);
    _lcd.printf("Value = %d", _quantity);
    delay(50);
  }

  return SystemState::ChangeQuantity;
}

SystemState ScanFSM::handleConfirmedScan() {
  resetLcd();
  _lcd.printf("Updated quantity by\n%d for tag %d", _quantity, _tagId);
  delay(1000);

  char jsonBuf[512];
  memset(jsonBuf, 0, sizeof(jsonBuf));
  JSONBufferWriter writer(jsonBuf, sizeof(jsonBuf));
  writer.beginObject();
  writer.name("scannerId").value(ScannerId);
  writer.name("quantity").value(_quantity);
  writer.name("tagId").value(_tagId);
  writer.endObject();
  Particle.publish("scan_event_raw", jsonBuf);

  resetLcd();
  _lcd.print("Change quantity or\nwrite an ID");
  _lcd.setCursor(6, 3);
  _lcd.print("Write ID");

  return SystemState::Idle;
}

SystemState ScanFSM::handleWriteTagId() {
  if (!digitalRead(Pins::BtnLeft)) {
    _tagId--;
    _lcd.setCursor(0, 2);
    _lcd.printf("Value = %d", _tagId);
    delay(50);
  } else if (!digitalRead(Pins::BtnMiddle)) {
    return SystemState::ConfirmedIdWrite;
  } else if (!digitalRead(Pins::BtnRight)) {
    _tagId++;
    _lcd.setCursor(0, 2);
    _lcd.printf("Value = %d", _tagId);
    delay(50);
  }

  return SystemState::WriteTagId;
}

SystemState ScanFSM::handleConfirmedIdWrite() {
  bool foundTag = _scanner.update();

  if (foundTag) {
    auto writeSuccess = _scanner.writeCardId(_tagId);

    if (writeSuccess) {
      if (writeSuccess.value()) {
        Serial.println("Found tag and wrote ID");
      } else {
        Serial.println("Found tag but failed to write ID");
      }
    } else {
        Serial.println("Unable to find tag when writing");
    }

    resetLcd();
    _lcd.print("Change quantity or\nwrite an ID");
    _lcd.setCursor(6, 3);
    _lcd.print("Write ID");
  }

  delay(50);

  return SystemState::ConfirmedIdWrite;
}