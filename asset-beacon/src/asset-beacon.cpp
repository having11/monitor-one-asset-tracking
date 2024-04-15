/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

const char* BeaconUuid = "19bc147d-857c-4b5c-a628-635f1b40c472";
constexpr uint16_t BeaconMajor = 1;
// Changes per-device
constexpr uint16_t BeaconMinor = 0;
constexpr int8_t TxPower = -55;

iBeacon beacon(BeaconMajor, BeaconMinor, BeaconUuid, TxPower);

void setup() {
    delay(2000);
    Log.info("Starting advertisement");
    BLE.advertise(beacon);
}

void loop() {
}
