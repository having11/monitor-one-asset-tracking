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
#include "tracker_config.h"
#include "edge.h"
#include "monitor_edge_ioexpansion.h"
#include "ModbusClient.h"


//
// Constants
//
enum class ModbusBaudRates {
    BaudUnknown,
    Baud1200,
    Baud2400,
    Baud4800,
    Baud9600,
    Baud19200,
    Baud28800,
    Baud38400,
    Baud57600,
    Baud76800,
    Baud115200,
};

enum class ModbusParity {
    Unknown,
    None,
    Even,
};

static constexpr int MODBUS_CLIENT_COUNT                {3};
static constexpr ModbusBaudRates MODBUS_BAUD_DEFAULT    {ModbusBaudRates::Baud38400};
static constexpr ModbusParity MODBUS_PARITY_DEFAULT     {ModbusParity::None};
static constexpr int32_t MODBUS_IMD_DEFAULT             {0};

enum class ModbusServerPublish
{
  Always,
  OnThreshold,
};

enum class ModbusServerFunction
{
  Coil,                                       ///< Coil register
  DiscreteInput,                              ///< Discrete input register
  InputRegister,                              ///< Input register
  HoldingRegister,                            ///< Holding register
};

enum class ModbusServerType
{
    Int16,
    Uint16,
    Int32,
    Uint32,
    Float32abcd,
    Float32badc,
    Float32cdab,
    Float32dcba,
    Bits,
};


//
// Global variables
//
static Logger monitorOneLog("IoModbus");
static Thread* modbusThread;

struct ModbusSettings
{
    ModbusBaudRates baud;
    ModbusParity parity;
};

static ModbusSettings modbusRtuSettings { MODBUS_BAUD_DEFAULT, MODBUS_PARITY_DEFAULT};
static ModbusSettings modbusRtuSettingsShadow { MODBUS_BAUD_DEFAULT, MODBUS_PARITY_DEFAULT};
static int32_t modbusInterMessageDelay {MODBUS_IMD_DEFAULT};
static ModbusClient modbusRtu;

struct ModbusServerConfig {
    bool enabled                        {false};
    int32_t id                          {1};
    int32_t timeout                     {2000};
    int32_t pollInterval                {1};
    ModbusServerPublish publish         {ModbusServerPublish::Always};
    ModbusServerFunction function       {ModbusServerFunction::Coil};
    uint32_t address                    {0};
    ModbusServerType type               {ModbusServerType::Uint16};
    uint32_t mask                       {UINT16_MAX};
    uint32_t shift                      {0};
    double offset                       {0.0};
    double scale                        {1.0};

    // Other fields that descibe the data
    unsigned int readLength             {1};
    bool signedInt                      {false};
    uint32_t negativeTest               {0x8000};
    uint32_t signExtend                 {0};
    bool isFloat                        {false};
    ModbusFloatEndianess endian         {ModbusFloatEndianess::CDAB};
};

struct ModbusServerObject {
    char name[16];
    ModbusServerConfig primary;
    ModbusServerConfig shadow;
    ConfigObject* configObject;
    RecursiveMutex mutex;
    unsigned int loopTick;
    ModbusClientContext context;
};

static Vector<ModbusServerObject*> modbusServers {};

struct ModbusPublish {
    char* name {nullptr};
    //bool isFloat {false};
    //bool signedInt {false};
    double value {0.0};
    uint8_t result {0};
};

static Vector<ModbusPublish> resultsToPublish;
static unsigned int publishTick;


//
// Functions
//

/**
 * @brief Convert an integer baud rate to an enum
 *
 * @param baud One of several standard baud rate figures
 * @return ModbusBaudRates
 */
ModbusBaudRates modbusBaudToEnum(unsigned long baud)
{
    switch (baud)
    {
        case 1200:      return ModbusBaudRates::Baud1200;
        case 2400:      return ModbusBaudRates::Baud2400;
        case 4800:      return ModbusBaudRates::Baud4800;
        case 9600:      return ModbusBaudRates::Baud9600;
        case 19200:     return ModbusBaudRates::Baud19200;
        case 28800:     return ModbusBaudRates::Baud28800;
        case 38400:     return ModbusBaudRates::Baud38400;
        case 57600:     return ModbusBaudRates::Baud57600;
        case 76800:     return ModbusBaudRates::Baud76800;
        case 115200:    return ModbusBaudRates::Baud115200;
    }

    return ModbusBaudRates::BaudUnknown;
}

/**
 * @brief Convert enum to integer baud rate
 *
 * @param baud An enum for only one baud rate
 * @return unsigned long Baud rate
 */
unsigned long modbusEnumToBaud(ModbusBaudRates baud)
{
    switch (baud)
    {
        case ModbusBaudRates::Baud1200:     return 1200;
        case ModbusBaudRates::Baud2400:     return 2400;
        case ModbusBaudRates::Baud4800:     return 4800;
        case ModbusBaudRates::Baud9600:     return 9600;
        case ModbusBaudRates::Baud19200:    return 19200;
        case ModbusBaudRates::Baud28800:    return 28800;
        case ModbusBaudRates::Baud38400:    return 38400;
        case ModbusBaudRates::Baud57600:    return 57600;
        case ModbusBaudRates::Baud76800:    return 76800;
        case ModbusBaudRates::Baud115200:   return 115200;
    }

    return 0;
}

/**
 * @brief Convert a device OS parity flag to enum
 *
 * @param parity Bitmapped parity flag
 * @return ModbusParity
 */
ModbusParity modbusParityToEnum(uint32_t parity)
{
    // Mask the parity bits for serial configuration
    parity &= 0b00001100;

    switch (parity)
    {
        case SERIAL_PARITY_NO:      return ModbusParity::None;
        case SERIAL_PARITY_EVEN:    return ModbusParity::Even;
        //case SERIAL_PARITY_ODD:   // Not supported
    }

    return ModbusParity::Unknown;
}

/**
 * @brief Cnvert a enum to device OS parity flag
 *
 * @param parity Enum to convert
 * @return uint32_t Partity flag
 */
uint32_t modbusEnumToParity(ModbusParity parity)
{
    switch (parity)
    {
        case ModbusParity::None: return SERIAL_PARITY_NO;
        case ModbusParity::Even: return SERIAL_PARITY_EVEN;
    }

    return UINT32_MAX;
}

/**
 * @brief Change the serial port baud and parity settings
 *
 * @param settings Structure to contain desired baud and parity
 * @retval SYSTEM_ERROR_NONE Success
 * @retval SYSTEM_ERROR_INVALID_ARGUMENT Something wasn't good
 */
int modbusChangeInterfaceSettings(ModbusSettings& settings)
{
    auto baud = modbusEnumToBaud(settings.baud);
    auto parity = modbusEnumToParity(settings.parity);

    if ((0 == baud) || (UINT32_MAX == parity))
    {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    Serial1.begin(baud, SERIAL_DATA_BITS_8 | SERIAL_STOP_BITS_1 | parity);

    return SYSTEM_ERROR_NONE;
}

/**
 * @brief Config service object setting upon new JSON configuration
 *
 * @param write Indicates whether the current operation is to write
 * @param context Unused
 * @return int Zero (success) always
 */
int config_modbus_enter(bool write, const void *context)
{
    if (write)
    {
        memcpy(&modbusRtuSettingsShadow, &modbusRtuSettings, sizeof(modbusRtuSettingsShadow));
    }
    return 0;
}

/**
 * @brief Config service object settings to commit new JSON configuration
 *
 * @param write Indicates whether the current operation is to write
 * @param status Status of the command return value
 * @param context Unused
 * @return int The command return value
 */
int config_modbus_exit(bool write, int status, const void *context)
{
    if (write && (0 == status))
    {
        memcpy(&modbusRtuSettings, &modbusRtuSettingsShadow, sizeof(modbusRtuSettings));
        modbusChangeInterfaceSettings(modbusRtuSettings);
    }
    return status;
}

/**
 * @brief Create the Modbus RTU configuration settings
 *
 * @return int Zero (success) always
 */
int buildModbusRtuSettings() {
    static ConfigObject modbusRtuConfiguration("modbus_rs485",
        {
            ConfigStringEnum("baud", {
                    {"1200", (int32_t) ModbusBaudRates::Baud1200},
                    {"2400", (int32_t) ModbusBaudRates::Baud2400},
                    {"4800", (int32_t) ModbusBaudRates::Baud4800},
                    {"9600", (int32_t) ModbusBaudRates::Baud9600},
                    {"19200", (int32_t) ModbusBaudRates::Baud19200},
                    {"28800", (int32_t) ModbusBaudRates::Baud28800},
                    {"38400", (int32_t) ModbusBaudRates::Baud38400},
                    {"57600", (int32_t) ModbusBaudRates::Baud57600},
                    {"76800", (int32_t) ModbusBaudRates::Baud76800},
                    {"115200", (int32_t) ModbusBaudRates::Baud115200},
                },
                config_get_int32_cb, config_set_int32_cb,
                &modbusRtuSettings.baud, &modbusRtuSettingsShadow.baud),

            ConfigStringEnum("parity", {
                    {"none", (int32_t) ModbusParity::None},
                    {"even", (int32_t) ModbusParity::Even},
                },
                config_get_int32_cb, config_set_int32_cb,
                &modbusRtuSettings.parity, &modbusRtuSettingsShadow.parity),
            ConfigInt("imd", &modbusInterMessageDelay)
        },
        config_modbus_enter,
        config_modbus_exit
    );
    ConfigService::instance().registerModule(modbusRtuConfiguration);

    // Modbus RTU settings
    modbusChangeInterfaceSettings(modbusRtuSettings);
    modbusRtu.begin(Serial1);
    // Callbacks allow us to configure the RS485 transceiver correctly
    modbusRtu.preTransmission([]() {digitalWrite(MONITOREDGE_IOEX_RS485_DE_PIN, 1);});
    modbusRtu.postTransmission([]() {digitalWrite(MONITOREDGE_IOEX_RS485_DE_PIN, 0);});
    modbusRtu.idle([]() {os_thread_yield();});

    return 0;
}

/**
 * @brief Config service object setting upon new JSON configuration for polling
 *
 * @param write Indicates whether the current operation is to write
 * @param context Unused
 * @return int Zero (success) always
 */
int modbusConfigEnter(bool write, const void *context)
{
    auto modbusContext = (ModbusServerObject*)context;
    assert(nullptr != modbusContext);

    if (write)
    {
        const std::lock_guard<RecursiveMutex> lock(modbusContext->mutex);
        memcpy(&modbusContext->shadow, &modbusContext->primary, sizeof(modbusContext->shadow));
    }
    return 0;
}

/**
 * @brief Config service object settings to commit new JSON configuration for polling
 *
 * @param write Indicates whether the current operation is to write
 * @param status Status of the command return value
 * @param context Unused
 * @return int The command return value
 */
int modbusConfigExit(bool write, int status, const void *context)
{
    auto modbusContext = (ModbusServerObject*)context;
    assert(nullptr != modbusContext);

    if (write && (0 == status))
    {
        const std::lock_guard<RecursiveMutex> lock(modbusContext->mutex);
        memcpy(&modbusContext->primary, &modbusContext->shadow, sizeof(modbusContext->primary));

        // Collect properties of the data type
        modbusContext->primary.signedInt = false;
        modbusContext->primary.isFloat = false;
        switch (modbusContext->primary.type)
        {
            case ModbusServerType::Int16:
                modbusContext->primary.signedInt = true;
                // Fall through
            case ModbusServerType::Uint16:
            // Fall through
            case ModbusServerType::Bits:
                modbusContext->primary.readLength = 1;
                break;

            case ModbusServerType::Int32:
                modbusContext->primary.signedInt = true;
                // Fall through
            case ModbusServerType::Uint32:
                modbusContext->primary.readLength = 2;
                break;

            case ModbusServerType::Float32abcd:
                // Fall through
            case ModbusServerType::Float32badc:
                // Fall through
            case ModbusServerType::Float32cdab:
                // Fall through
            case ModbusServerType::Float32dcba:
                modbusContext->primary.isFloat = true;
                modbusContext->primary.readLength = 2;
                break;
        }

        switch (modbusContext->primary.type)
        {
            case ModbusServerType::Float32abcd:
                modbusContext->primary.endian = ModbusFloatEndianess::ABCD;
                break;

            case ModbusServerType::Float32badc:
                modbusContext->primary.endian = ModbusFloatEndianess::BADC;
                break;

            case ModbusServerType::Float32cdab:
                modbusContext->primary.endian = ModbusFloatEndianess::CDAB;
                break;

            case ModbusServerType::Float32dcba:
                modbusContext->primary.endian = ModbusFloatEndianess::DCBA;
                break;
        }

        // Fixed for paranoia
        modbusContext->primary.shift = min(modbusContext->primary.shift, 15UL);
        modbusContext->primary.mask &= 0xffff;
        modbusContext->primary.address &= 0xffff;

        if (modbusContext->primary.signedInt)
        {
            // Figure out sign extension for signed types
            auto leadingZeros = __builtin_clz((unsigned int)(modbusContext->primary.mask >> modbusContext->primary.shift));
            leadingZeros -= 16; // We only care about the least significant 16 bits

            if (15 > leadingZeros)
            {
                modbusContext->primary.negativeTest = 1UL << (15 - leadingZeros);  // This is a mask to test the sign bit
                modbusContext->primary.signExtend = UINT32_MAX << (15 - leadingZeros); // This is the pattern to OR the result to extend the sign bit
            }
            else
            {
                // The number doesn't have enough bits to represent a negative number
                modbusContext->primary.negativeTest = 0;
                modbusContext->primary.signExtend = 0;
            }
        }

        // Serial.printlnf(
        //     "ID=%ld, TO=%ld, PI=%ld, FN=%d, ADD=0x%04lx, MASK=0x%04lx, SHIFT=%ld, NT=0x%04lx, SE=0x%04lx, LOW=%0.3lf, HIGH=%0.3lf",
        //     modbusContext->primary.id,
        //     modbusContext->primary.timeout,
        //     modbusContext->primary.pollInterval,
        //     (int)modbusContext->primary.function,
        //     modbusContext->primary.address,
        //     modbusContext->primary.mask, modbusContext->primary.shift,
        //     modbusContext->primary.negativeTest, modbusContext->primary.signExtend,
        //     modbusContext->primary.offset, modbusContext->primary.scale);
    }
    return status;
}

/**
 * @brief Create the Modbus polling configuration settings
 *
 * @return int Zero (success) always
 */
int buildModbusSettings(int n) {
    auto serverConfig = new ModbusServerObject();

    snprintf(serverConfig->name, sizeof(serverConfig->name), "modbus%d", n);

    serverConfig->configObject = new ConfigObject(serverConfig->name,
        {
            ConfigBool("enable",
                config_get_bool_cb, config_set_bool_cb,
                &serverConfig->primary.enabled, &serverConfig->shadow.enabled),
            ConfigInt("id",
                config_get_int32_cb, config_set_int32_cb,
                &serverConfig->primary.id, &serverConfig->shadow.id),
            ConfigInt("timeout",
                config_get_int32_cb, config_set_int32_cb,
                &serverConfig->primary.timeout, &serverConfig->shadow.timeout),
            ConfigInt("poll",
                config_get_int32_cb, config_set_int32_cb,
                &serverConfig->primary.pollInterval, &serverConfig->shadow.pollInterval),
            ConfigStringEnum("publish", {
                    {"always", (int32_t) ModbusServerPublish::Always},
                },
                config_get_int32_cb, config_set_int32_cb,
                &serverConfig->primary.publish, &serverConfig->shadow.publish),
            ConfigStringEnum("function", {
                    {"coil", (int32_t) ModbusServerFunction::Coil},
                    {"discrete_input", (int32_t) ModbusServerFunction::DiscreteInput},
                    {"input_register", (int32_t) ModbusServerFunction::InputRegister},
                    {"holding_register", (int32_t) ModbusServerFunction::HoldingRegister},
                },
                config_get_int32_cb, config_set_int32_cb,
                &serverConfig->primary.function, &serverConfig->shadow.function),
            ConfigInt("address",
                config_get_int32_cb, config_set_int32_cb,
                &serverConfig->primary.address, &serverConfig->shadow.address),
            ConfigStringEnum("type", {
                    {"int16", (int32_t) ModbusServerType::Int16},
                    {"uint16", (int32_t) ModbusServerType::Uint16},
                    {"int32", (int32_t) ModbusServerType::Int32},
                    {"uint32", (int32_t) ModbusServerType::Uint32},
                    {"float32_abcd", (int32_t) ModbusServerType::Float32abcd},
                    {"float32_badc", (int32_t) ModbusServerType::Float32badc},
                    {"float32_cdab", (int32_t) ModbusServerType::Float32cdab},
                    {"float32_dcba", (int32_t) ModbusServerType::Float32dcba},
                    {"bits", (int32_t) ModbusServerType::Bits},
                },
                config_get_int32_cb, config_set_int32_cb,
                &serverConfig->primary.type, &serverConfig->shadow.type),
            ConfigInt("mask",
                config_get_int32_cb, config_set_int32_cb,
                &serverConfig->primary.mask, &serverConfig->shadow.mask),
            ConfigInt("shift",
                config_get_int32_cb, config_set_int32_cb,
                &serverConfig->primary.shift, &serverConfig->shadow.shift),
            ConfigFloat("offset",
                config_get_float_cb, config_set_float_cb,
                &serverConfig->primary.offset, &serverConfig->shadow.offset),
            ConfigFloat("scale",
                config_get_float_cb, config_set_float_cb,
                &serverConfig->primary.scale, &serverConfig->shadow.scale),
        },
        modbusConfigEnter,
        modbusConfigExit,
        serverConfig, serverConfig
    );
    ConfigService::instance().registerModule(*serverConfig->configObject);
    modbusServers.append(serverConfig);

    return 0;
}


/**
 * @brief The thread that service all poll requests
 *
 * @param param Unused
 */
void modbusThreadLoop(void* param)
{
    while (true)
    {
        for (auto server: modbusServers) {
            ModbusServerConfig safeConfig {};
            {
                const std::lock_guard<RecursiveMutex> lock(server->mutex);
                memcpy(&safeConfig, &server->primary, sizeof(safeConfig));
            }
            if (safeConfig.enabled && ((System.uptime() - server->loopTick) >= (unsigned int)safeConfig.pollInterval)) {
                server->loopTick = System.uptime();

                uint8_t result {};
                switch (safeConfig.function)
                {
                    case ModbusServerFunction::Coil:
                        result = modbusRtu.readCoils(safeConfig.id, safeConfig.address, safeConfig.readLength, server->context);
                        break;
                    case ModbusServerFunction::DiscreteInput:
                        result = modbusRtu.readDiscreteInputs(safeConfig.id, safeConfig.address, safeConfig.readLength, server->context);
                        break;
                    case ModbusServerFunction::InputRegister:
                        result = modbusRtu.readInputRegisters(safeConfig.id, safeConfig.address, safeConfig.readLength, server->context);
                        break;
                    case ModbusServerFunction::HoldingRegister:
                        result = modbusRtu.readHoldingRegisters(safeConfig.id, safeConfig.address, safeConfig.readLength, server->context);
                        break;
                }

                uint32_t uint32_Value {};

                if (ModbusClient::ku8MBSuccess == result)
                {
                    switch (safeConfig.type)
                    {
                        case ModbusServerType::Int16:
                            // Fall through
                        case ModbusServerType::Bits:
                            // Fall through
                        case ModbusServerType::Uint16:
                            uint32_Value = (uint32_t)server->context.readBuffer[0];
                            uint32_Value &= safeConfig.mask;
                            uint32_Value >>= safeConfig.shift;
                            if (safeConfig.signedInt && (uint32_Value & safeConfig.negativeTest))
                            {
                                uint32_Value |= safeConfig.signExtend;
                            }
                            break;

                        case ModbusServerType::Int32:
                            // Fall through
                        case ModbusServerType::Uint32:
                            uint32_Value = ModbusClient::wordsToDword(server->context.readBuffer[0], server->context.readBuffer[1]);
                            break;
                    }
                }

                double sensorValue {};
                if (safeConfig.isFloat)
                {
                    sensorValue = (double)ModbusClient::wordsToFloat(server->context.readBuffer[0], server->context.readBuffer[1], safeConfig.endian);
                }
                else if (safeConfig.signedInt)
                {
                    sensorValue = (double)((int32_t)uint32_Value);
                }
                else
                {
                    sensorValue = (double)uint32_Value;
                }

                sensorValue = sensorValue * safeConfig.scale + safeConfig.offset;

                ModbusPublish publishMe {};
                publishMe.name = server->name;
                publishMe.value = sensorValue;
                publishMe.result = result;
                resultsToPublish.append(publishMe);
            }
        }

        if (!resultsToPublish.isEmpty() && ((System.uptime() != publishTick)))
        {
            publishTick = System.uptime();
            static char publish1[1024] = {};
            memset(publish1, 0, sizeof(publish1));
            JSONBufferWriter toPublish(publish1, sizeof(publish1));
            toPublish.beginObject();
            toPublish.name("modbus").beginArray();

            for (auto client: resultsToPublish)
            {
                toPublish.beginObject().name("name").value(client.name);
                toPublish.name("result").value((unsigned long)client.result);
                if (modbusRtu.ku8MBSuccess == client.result)
                {
                    toPublish.name("value").value(client.value);
                }
                toPublish.endObject();
            }
            toPublish.endArray().endObject();
            resultsToPublish.clear();
            if (Particle.connected())
                Particle.publish("modbus", publish1);
        }
        // Play fair and let other threads execute
        os_thread_yield();
    }

    // It is safe to exit here with Thread::run properly handling OS thread exit
}

/**
 * @brief Initializes the Modbus settings and polling thread
 *
 * @return int Zero (success) always
 */
int modbusInit()
{
    buildModbusRtuSettings();

    //for loop for instances
    for (int i = 1; i <= MODBUS_CLIENT_COUNT;i++)
    {
        buildModbusSettings(i);
    }

    if (nullptr == modbusThread)
    {
        modbusThread = new Thread("modbus", modbusThreadLoop, nullptr, OS_THREAD_PRIORITY_DEFAULT, 2*1024);
    }
    return 0;
}
