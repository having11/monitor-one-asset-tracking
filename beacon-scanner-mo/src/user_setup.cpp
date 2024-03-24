/*
 * Copyright (c) 2023 Particle Industries, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Particle.h"
#include "edge.h" // For sleep API
#include "user_config.h"
#include "edge_cellular.h" // For reading cellular RSSI
#include "Adp8866GnssLed.h"
#include "DebounceSwitchRK.h"
#include "eeprom_helper.h"


//
// Constants
//
static constexpr uint8_t EEPROM_DATA_ADDRESS        {0x50};

static constexpr int32_t THRESHOLD_LOW_PERCENT      {50L};
static constexpr int32_t THRESHOLD_HIGH_PERCENT     {80L};
static constexpr uint32_t LED_DISPLAY_PERIOD_MS     {10000UL};
static constexpr uint8_t COLOR_MAX_VAL              {255};


//
// Global variables
//
static Logger monitorOneLog("MonitorOne");
static CellularSignal Sig;
static Timer *RestoreTmr;
static MonitorOneCardFunction callInit = []() {
    return 0;
};
static MonitorOneCardFunction callLoop = []() {
    return 0;
};

/**
 * @brief Restore default system LED behaviour
 *
 * @return None
 */
static void defaultLedBehaviour()
{
    // Restore the LED control back to the system
    RGB.control(false);
}


/**
 * @brief Handle short button press on Monitor One
 *
 * @details Asserts the correct LED pattern associated with cellular signal strength
 *          when the button is pressed for a short period
 *
 * @param[in] switchState state of button press (short, long, release, etc)
 * @param[in] context user data
 *
 * @return None
 */
static void buttonHandler(DebounceSwitchState* switchState, void *context)
{
    // See https://github.com/rickkas7/DebounceSwitchRK for more information
    // Some example DebouncePressState states:
    //           Single tap (< 3s):  PRESS_START -> SHORT -> RELEASED -> TAP
    //           Double tap (< 3s):  PRESS_START -> SHORT -> RELEASED -> PRESS_START -> SHORT -> RELEASED -> TAP
    //    Long press (> 3s, < 10s):  PRESS_START -> PROGRESS -> LONG -> RELEASED
    //    Very long press  (> 10s):  PRESS_START -> PROGRESS -> VERY_LONG -> RELEASED
    if( DebouncePressState::SHORT == switchState->getPressState() )
    {
        // Use EdgeCellular service to get the cellular signal strength
        if(!EdgeCellular::instance().getSignal(Sig))
        {
            auto pct = static_cast<int32_t>(Sig.getStrength());
            monitorOneLog.trace("Cell Strength = %ld", pct);

            // Take control of the system LED
            RGB.control(true);

            // Display the appropriate colour based on the signal strength
            if( (pct >= 0) && (pct < THRESHOLD_LOW_PERCENT) )
            {
                // Display RED for 10s
                RGB.color(COLOR_MAX_VAL, 0, 0);                 // [R, G, B]
                RestoreTmr->start();
            }
            else if( (pct >= THRESHOLD_LOW_PERCENT) && (pct < THRESHOLD_HIGH_PERCENT) )
            {
                // Display YELLOW for 10s
                RGB.color(COLOR_MAX_VAL, COLOR_MAX_VAL, 0);     // [R, G, B]
                RestoreTmr->start();
            }
            else if( pct >= THRESHOLD_HIGH_PERCENT )
            {
                // Display GREEN for 10s
                RGB.color(0, COLOR_MAX_VAL, 0);                 // [R, G, B]
                RestoreTmr->start();
            }
            else
            {
                // Shouldn't reach here. But, just in case, return control of the system LED
                RGB.control(false);
            }
        }
        else
        {
            monitorOneLog.trace("Cell strength not available");
        }
    }
}

/**
 * @brief Common setup function for Monitor One
 *
 * @details Configure common Monitor One IO and peripherals
 *
 */
static int commonMonitorOneSetup()
{
    // Set up a timer to restore LED behaviour after a button press
    RestoreTmr = new Timer(LED_DISPLAY_PERIOD_MS, defaultLedBehaviour, true);

    // Associate user button with debounce handler
    DebounceSwitch::getInstance()->setup();
    DebounceSwitch::getInstance()->addSwitch(MONITORONE_USER_BUTTON, DebounceSwitchStyle::PRESS_LOW_PULLUP, buttonHandler);

    // We want the user button to wake us up
    EdgeSleep::instance().wakeFor(MONITORONE_USER_BUTTON, FALLING);

    return SYSTEM_ERROR_NONE;
}


/**
 * @brief User setup function for Monitor Edge
 *
 * @details Sets up basic Monitor One functions and then detects expansion cards
 *
 * @return int Success or failure codes
 */
int user_init()
{
    CHECK(commonMonitorOneSetup());

    ExpansionEeprom eeprom {};
    auto ret = readEepromBytes(Wire, EEPROM_DATA_ADDRESS, 0, (uint8_t*)&eeprom, sizeof(eeprom));
    if (ret || !isEeepromValid(eeprom)) {
        monitorOneLog.error("This is not the card you are looking for");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    if (!strcmp(eeprom.sku, MONITOREDGE_IOEX_SKU)) {
        monitorOneLog.info("Detected a basic IO expansion card with RS-485 and CAN bus");
        callInit = expanderIoInit;
        callLoop = expanderIoLoop;
    }
    else if (!strcmp(eeprom.sku, MONITOREDGE_PROTO_SKU)) {
        monitorOneLog.info("Detected a basic prototype expansion card");
        // Do proto card stuff here or reassign callInit to your init function
    }

    return callInit();
}
/**
 * @brief User loop function for Monitor Edge
 *
 * @return int Success or failure codes
 */
int user_loop()
{
    return callLoop();
}
