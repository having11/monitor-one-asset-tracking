/* 
 * Project Beacon Scanner
 * Author: Evan Rust
 * Date: 
 */

// #define TEST_MODE

// Include Particle Device OS APIs
#include "Particle.h"

#include <SPI.h>
#include <BeaconScanner.h>
#include <LiquidCrystal_I2C.h>

#include "Constants.h"
#include "ScanFSM.h"
#include "TagScanner.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

#ifndef TEST_MODE

TagScanner tagScanner{&SPI1, Pins::RC522Rst, Pins::RC522Cs};
LiquidCrystal_I2C lcd{LCDConstants::I2CAddress};
ScanFSM fsm{tagScanner, lcd};

void scanCb(Beacon& beacon, callback_type type);
double rssiToDistance(int8_t rssi, int txPower);
void getBeacons(void);

static char jsonBuf[512];
static uint32_t lastBeaconScan = 0;

// Changes for each listener
constexpr int ListenerId = 1;

void setup() {
  delay(4000);
  SPI.begin();

  pinMode(Pins::BtnLeft, INPUT_PULLUP);
  pinMode(Pins::BtnMiddle, INPUT_PULLUP);
  pinMode(Pins::BtnRight, INPUT_PULLUP);

  // BLE.on();
  // Scanner.setCallback(scanCb);
  // Scanner.startContinuous(SCAN_IBEACON);

  tagScanner.init();
  // lcd.begin(LCDConstants::ColumnCount, LCDConstants::RowCount);
}

void loop() {
  // Scanner.loop();

  if (!digitalRead(Pins::BtnLeft)) {
    auto writeResult = tagScanner.writeCardId(TagIdToWrite);
    if (writeResult) {
      if (writeResult.value()) {
        Log.info("Wrote new ID to tag!");
      } else {
        Log.info("Failed to write new ID to tag!");
      }
    } else {
      Log.info("Unable to find tag for writing");
    }

    delay(1000);
  }

  auto scannedId = tagScanner.getCardId();
  if (scannedId) {
    Log.info("Found tag with ID=%d", scannedId.value());
    time_t time = Time.now();
  
    memset(jsonBuf, 0, sizeof(jsonBuf));
    JSONBufferWriter writer(jsonBuf, sizeof(jsonBuf));
    writer.beginObject();
    writer.name("scannerId").value(ScannerId);
    writer.name("quantityChange").value(QuantityChange);
    writer.name("itemId").value(scannedId.value());
    writer.name("id").value(0);
    writer.name("timestamp").value(Time.format(time, TIME_FORMAT_ISO8601_FULL));
    writer.endObject();
    Particle.publish("INVENTORY-SCAN", jsonBuf);

    delay(1000);
  }

  // fsm.update();
  
  if (millis() - lastBeaconScan >= BeaconScanDelayMs) {
    getBeacons();
    lastBeaconScan = millis();
  }
}

void getBeacons() {
  for (auto& ibeacon : Scanner.getiBeacons()) {
    Log.info("Found iBeacon with UUID=%s, RSSI=%d, power=%d, major=%d, minor=%d",
        ibeacon.getUuid(), ibeacon.getRssi(), ibeacon.getPower(), ibeacon.getMajor(), ibeacon.getMinor());
    double estDistance = rssiToDistance(ibeacon.getRssi(), ibeacon.getPower());
    Log.info("Estimated distance=%f", estDistance);

    memset(jsonBuf, 0, sizeof(jsonBuf));
    JSONBufferWriter writer(jsonBuf, sizeof(jsonBuf));
    writer.beginObject();
    writer.name("source").value(ListenerId);
    writer.name("beacon_minor").value(ibeacon.getMinor());
    writer.name("distance_m").value(estDistance);
    writer.endObject();
    Particle.publish("BEACON-DIST", jsonBuf);
  }
}

void scanCb(Beacon& beacon, callback_type type) {
  if (beacon.type == SCAN_IBEACON) {
    iBeaconScan& ibeacon = static_cast<iBeaconScan&>(beacon);
    Log.info("Found iBeacon with UUID=%s, RSSI=%d, power=%d, major=%d, minor=%d",
      ibeacon.getUuid(), ibeacon.getRssi(), ibeacon.getPower(), ibeacon.getMajor(), ibeacon.getMinor());
  }
}

// From https://stackoverflow.com/questions/61982078/calculate-approximated-distance-using-rssi
double rssiToDistance(int8_t rssi, int txPower) {
  /* 
  * RSSI in dBm
  * txPower is a transmitter parameter that calculated according to its physic layer and antenna in dBm
  * Return value in meter
  *
  * You should calculate "PL0" in calibration stage:
  * PL0 = txPower - RSSI; // When distance is distance0 (distance0 = 1m or more)
  * 
  * SO, RSSI will be calculated by below formula:
  * RSSI = txPower - PL0 - 10 * n * log(distance/distance0) - G(t)
  * G(t) ~= 0 //This parameter is the main challenge in achiving to more accuracy.
  * n = 2 (Path Loss Exponent, in the free space is 2)
  * distance0 = 1 (m)
  * distance = 10 ^ ((txPower - RSSI - PL0 ) / (10 * n))
  *
  * Read more details:
  *   https://en.wikipedia.org/wiki/Log-distance_path_loss_model
  */
  return pow(10, ((double) (txPower - rssi - (-6))) / (10 * 2));
}
#else

#include <SPI.h>
#include <MFRC522.h>

constexpr uint8_t RST_PIN = D2;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = A2;         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
  delay(4000);
	SPI.begin();			// Init SPI bus
  mfrc522.setSPIConfig();
	mfrc522.PCD_Init();		// Init MFRC522
}

void loop() {
	// Look for new cards
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}

	// Dump debug info about the card; PICC_HaltA() is automatically called
	mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}

#endif