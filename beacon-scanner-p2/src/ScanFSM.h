#pragma once

#include <LiquidCrystal_I2C.h>

#include "Constants.h"
#include "TagScanner.h"

enum class SystemState {
  Idle = 0,
  Scanning,
  ChangeQuantity,
  ConfirmedScan,
  WriteTagId,
  ConfirmedIdWrite,
};

class ScanFSM {
  public:
    ScanFSM(TagScanner& scanner, LiquidCrystal_I2C& lcd);
    SystemState update();

  private:
    SystemState handleIdle();
    SystemState handleScanning();
    SystemState handleChangeQuantity();
    SystemState handleConfirmedScan();
    SystemState handleWriteTagId();
    SystemState handleConfirmedIdWrite();

    inline void resetLcd() {
      _lcd.clear();
      _lcd.home();
    }

    uint16_t _tagId = 0;
    int8_t _quantity = 0;

    SystemState _currentState;
    TagScanner& _scanner;
    LiquidCrystal_I2C& _lcd;
};