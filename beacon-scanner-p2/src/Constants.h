#pragma once

// Pin constants
namespace Pins {
  constexpr uint8_t BtnLeft = D3;
  constexpr uint8_t BtnMiddle = D4;
  constexpr uint8_t BtnRight = D5;

  constexpr uint8_t RC522Rst = D2;
  constexpr uint8_t RC522Cs = A2;
}

// Scanner ID
constexpr uint8_t ScannerId = 1;
constexpr int8_t QuantityChange = 1;
constexpr int16_t TagIdToWrite = 1;

constexpr uint32_t BeaconScanDelayMs = 5000;

// LCD
namespace LCDConstants {
  constexpr uint8_t I2CAddress = 0x20;
  constexpr uint8_t ColumnCount = 20;
  constexpr uint8_t RowCount = 4;
}