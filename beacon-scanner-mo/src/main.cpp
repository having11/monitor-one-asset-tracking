// Uncomment for detailed BME680 debug info
// #define BME680_DEBUG

#include "Particle.h"
#include "edge.h"

#include <BeaconScanner.h>

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

#if EDGE_PRODUCT_NEEDED
PRODUCT_ID(EDGE_PRODUCT_ID);
#endif // EDGE_PRODUCT_NEEDED
PRODUCT_VERSION(EDGE_PRODUCT_VERSION);

STARTUP(
    Edge::startup();
);

SerialLogHandler logHandler(115200, LOG_LEVEL_TRACE, {
    { "app.gps.nmea", LOG_LEVEL_INFO },
    { "app.gps.ubx",  LOG_LEVEL_INFO },
    { "ncp.at", LOG_LEVEL_INFO },
    { "net.ppp.client", LOG_LEVEL_INFO },
});

void scanCb(Beacon& beacon, callback_type type);
double rssiToDistance(int8_t rssi, int txPower);

static char jsonBuf[512];
static uint32_t lastTime = 0;

constexpr int ListenerId = 2;
constexpr uint32_t intervalMs = 5000;

void setup()
{
    delay(1000);
    Serial.begin(115200);
    Wire.begin();
    Edge::instance().init();

    BLE.on();
    Scanner.setCallback(scanCb);
    Scanner.startContinuous(SCAN_IBEACON);
}

void loop()
{
    Edge::instance().loop();

    Scanner.loop();

    if (millis() - lastTime >= intervalMs) {
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

        lastTime = millis();
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
   return pow(10, ((double) (txPower - rssi - 13)) / (10 * 2));
}