/**
 * @file NOP100.cpp
 * @author Paul Reeve (preeve@pdjr.eu)
 * @brief Extensible firmware based on the NMEA2000 library.
 * @version 0.1
 * @date 2023-01-15
 * 
 * @copyright Copyright (c) 2023
 *
 * This firmware is targetted at hardware based on the
 * [NOP100](https://github.com/preeve9534/NOP100)
 * module design.
 * It implements a functional NMEA 2000 device that performs no
 * real-world task, but which can be easily extended or specialised
 * into a variant that can perform most things required of an NMEA 2000
 * module.
 * 
 * Support for NMEA 2000 networking is provided by Timo Lappalainen's
 * [NMEA2000](https://github.com/ttlappalainen/NMEA2000)
 * library.
 * 
 * Support for configuration management and operator interaction
 * is provided by a number of bespoke libraries that relieve derived
 * applications of much of the heavy lifting.
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <NMEA2000.h>
#include <N2kTypes.h>
#include <N2kMessages.h>
#include <NMEA2000_Teensyx.h>
#include <NMEA2000_CAN.h>
#include <Button.h>
#include <IC74HC165.h>
#include <SPI.h>
#include <LedManager.h>
#include <ModuleOperatorInterface.h>
#include <ModuleConfiguration.h>
#include <FunctionMapper.h>
#include <arraymacros.h>

#include "includes.h"

/**********************************************************************
 * @brief Configure debug output to Teensy serial port.
 */
#define DEBUG_SERIAL
#define DEBUG_SERIAL_PORT_SPEED 9600
#define DEBUG_SERIAL_START_DELAY 4000

/**********************************************************************
 * @brief GPIO pin definitions for Teensy 4.0
 */
#define GPIO_D0 0
#define GPIO_D1 1
#define GPIO_D2 2
#define GPIO_D3 3
#define GPIO_D4 4
#define GPIO_D5 5
#define GPIO_D6 6
#define GPIO_D7 7
#define GPIO_D8 8
#define GPIO_D9 9
#define GPIO_D10 10
#define GPIO_D11 11
#define GPIO_D12 12
#define GPIO_D13 13
#define GPIO_D14 14
#define GPIO_D15 15
#define GPIO_D16 16
#define GPIO_D17 17
#define GPIO_D18 18
#define GPIO_D19 19
#define GPIO_D20 20
#define GPIO_D21 21
#define GPIO_D22 22
#define GPIO_D23 23

#define GPIO_SERIAL_RX GPIO_D0
#define GPIO_SERIAL_TX GPIO_D1
#define GPIO_MIKROBUS_MODULE0_INT GPIO_D2
#define GPIO_MIKROBUS_MODULE0_PWM GPIO_D3
#define GPIO_MIKROBUS_MODULE0_EN GPIO_D4
#define GPIO_MIKROBUS_RST GPIO_D5
#define GPIO_MIKROBUS_MODULE0_CS GPIO_D6
#define GPIO_MIKROBUS_MODULE1_INT GPIO_D7
#define GPIO_MIKROBUS_MODULE1_EN GPIO_D8
#define GPIO_MIKROBUS_MODULE1_PWM GPIO_D9
#define GPIO_MIKROBUS_MODULE1_CS GPIO_D10
#define GPIO_SPI_MOSI GPIO_D11
#define GPIO_SPI_MISO GPIO_D12
#define GPIO_SPI_SCK GPIO_D13
#define GPIO_PISO_DATA GPIO_D14
#define GPIO_PISO_LATCH GPIO_D15
#define GPIO_PISO_CLOCK GPIO_D16
#define GPIO_LED_PRG GPIO_D17
#define GPIO_I2C_SDA GPIO_D18
#define GPIO_I2C_SCL GPIO_D19
#define GPIO_BUTTON_PRG GPIO_D20
#define GPIO_LED_CAN GPIO_D21
#define GPIO_CAN_TX GPIO_D22
#define GPIO_CAN_RX GPIO_D23

/**********************************************************************
 * @brief NMEA2000 device information.
 * 
 * Most specialisations of NOP100 will want to override DEVICE_CLASS,
 * DEVICE_FUNCTION and perhaps DEVICE_UNIQUE_NUMBER.
 * 
 * DEVICE_CLASS and DEVICE_FUNCTION are explained in the document
 * "NMEA 2000 Appendix B.6 Class & Function Codes".
 * 
 * DEVICE_INDUSTRY_GROUP we can be confident about (4 says maritime).
 * 
 * DEVICE_MANUFACTURER_CODE is only allocated to subscribed NMEA
 * members so we grub around and use 2046 which is currently not
 * allocated.  
 * 
 * DEVICE_UNIQUE_NUMBER is a bit of mystery.
 */
#define DEVICE_CLASS 10                 // System Tools
#define DEVICE_FUNCTION 130             // Diagnostic
#define DEVICE_INDUSTRY_GROUP 4         // Maritime
#define DEVICE_MANUFACTURER_CODE 2046   // Currently not allocated.
#define DEVICE_UNIQUE_NUMBER 849        // Bump me?

/**********************************************************************
 * @brief NMEA2000 product information.
 * 
 * Specialisations of NOP100 will want to override most of these.
 * 
 * PRODUCT_CERTIFICATION_LEVEL is granted by NMEA when a product is
 * officially certified. We won't be.
 * 
 * PRODUCT_CODE is our own unique numerical identifier for this device.
 * 
 * PRODUCT_FIRMWARE_VERSION should probably be generated automatically
 * from semewhere.
 * 
 * PRODUCT_LEN specifies the Load Equivalence Network number for the
 * product which encodes the normal power loading placed on the host
 * NMEA bus. One LEN = 50mA and values are rounded up.
 * 
 * PRODUCT_N2K_VERSION is the version of the N2K specification witht
 * which the firmware complies. 
 */
#define PRODUCT_CERTIFICATION_LEVEL 0   // Not certified
#define PRODUCT_CODE 002                // Our own product code
#define PRODUCT_FIRMWARE_VERSION "1.1.0 (Jun 2022)"
#define PRODUCT_LEN 1                   // This device's LEN
#define PRODUCT_N2K_VERSION 2100        // N2K specification version 2.1
#define PRODUCT_SERIAL_CODE "002-849"   // PRODUCT_CODE + DEVICE_UNIQUE_NUMBER
#define PRODUCT_TYPE "PDJRSIM"           // The product name?
#define PRODUCT_VERSION "1.0 (Mar 2022)"

/**********************************************************************
 * @brief NMEA2000 transmit and receive PGNs.
 * 
 * NMEA_TRANSMITTED_PGNS is a zero terminated array initialiser that
 * lists all the PGNs we transmit.
 * 
 * NMEA_RECEIVED_PGNS is a an array initialiser consisting of pairs
 * which associate the PGN of a message we will accept to a callback
 * which will accept a received message. For example,
 * { 127501L, handlerForPgn127501 }. The list must terminate with the
 * special flag value { 0L, 0 }.
 */
#define NMEA_TRANSMITTED_PGNS { 0L }
#define NMEA_RECEIVED_PGNS  { { 0L, 0 } }

/**********************************************************************
 * @brief ModuleConfiguration library stuff.
 */
#define MODULE_CONFIGURATION_SIZE 1
#define MODULE_CONFIGURATION_EEPROM_STORAGE_ADDRESS 0

#define MODULE_CONFIGURATION_CAN_SOURCE_INDEX 0
#define MODULE_CONFIGURATION_CAN_SOURCE_DEFAULT 22

#define MODULE_CONFIGURATION_DEFAULT { \
  MODULE_CONFIGURATION_CAN_SOURCE_DEFAULT \
}

/**********************************************************************
 * @brief FunctionMapper library stuff.
 * 
 * This provides just one function that wipes configuration data from
 * EEPROM. A specialisation of NOP100 that needs to add functions to
 * the function mapper will need to increase FUNCTION_MAPPER_SIZE
 * appropriately.
 */
#define FUNCTION_MAP_ARRAY { { 255, [](unsigned char i, unsigned char v) -> bool { ModuleConfiguration.erase(); return(true); } }, { 0, 0 } };
#define FUNCTION_MAPPER_SIZE 0

/**********************************************************************
 * @brief ModuleOperatorInterface library stuff.
 */
#define MODULE_OPERATOR_INTERFACE_LONG_BUTTON_PRESS_INTERVAL 1000UL
#define MODULE_OPERATOR_INTERFACE_DIALOG_INACTIVITY_TIMEOUT 30000UL

/**********************************************************************
 * @brief LedManager library stuff.
 *
 * NOP100 supports two LEDs: one used to indicate CAN bus activity and
 * one to provide feedback in the configuration user-interface.
 */
#define CAN_LED_UPDATE_INTERVAL 100UL
#define PRG_LED_UPDATE_INTERVAL 100UL

#include "defines.h"

/**
 * @brief Declarations of local functions.
 */
void messageHandler(const tN2kMsg&);
void onN2kOpen();
bool configurationValidator(unsigned int index, unsigned char value);

/**
 * @brief Create and initialise an array of transmitted PGNs.
 * 
 * Array initialiser is specified in defined.h. Required by NMEA2000
 * library. 
 */
const unsigned long TransmitMessages[] = NMEA_TRANSMITTED_PGNS;

/**
 * @brief Create and initialise a vector of received PGNs and their
 *        handlers.
 * 
 * Array initialiser is specified in defined.h. Required by NMEA2000
 * library. 
 */
typedef struct { unsigned long PGN; void (*Handler)(const tN2kMsg &N2kMsg); } tNMEA2000Handler;
tNMEA2000Handler NMEA2000Handlers[] = NMEA_RECEIVED_PGNS;

/**
 * @brief Create a ModuleConfiguration object for managing all module
 *        configuration data.
 * 
 * ModuleConfiguration implements the ModuleOperatorInterfaceHandler interface
 * and can be managed by the user-interaction manager.
*/
unsigned char defaultConfiguration[] = MODULE_CONFIGURATION_DEFAULT;
ModuleConfiguration ModuleConfiguration(defaultConfiguration, MODULE_CONFIGURATION_SIZE, MODULE_CONFIGURATION_EEPROM_STORAGE_ADDRESS, configurationValidator);

/**
 * @brief Create a FunctionHandler object for managing all extended
 *        configuration functions.
 * 
 * FunctionHandler implements the ModuleOperatorInterfaceHandler interface
 * and can be managed by the user-interaction manager. We'all add
 * functions later in setup().
 */
FunctionMapper::FunctionMap functionMapArray[] = FUNCTION_MAP_ARRAY;
FunctionMapper FunctionMapper(functionMapArray, FUNCTION_MAPPER_SIZE);

/**
 * @brief Create a ModuleOperatorInterface supporting ModuleConfiguration and
 *        FunctionHandler objects.
 */
ModuleOperatorInterfaceClient *modeHandlers[] = { &ModuleConfiguration, &FunctionMapper, 0 };
ModuleOperatorInterface ModuleOperatorInterface(modeHandlers);

/**
 * @brief Button object for debouncing the module's PRG button.
 */
Button PRGButton(GPIO_BUTTON_PRG);

/**
 * @brief Interface to the IC74HC165 PISO IC that connects the eight 
 *        DIL switch parallel inputs.
 */
IC74HC165 CodeSwitchPISO (GPIO_PISO_CLOCK, GPIO_PISO_DATA, GPIO_PISO_LATCH);

/**
 * @brief tLedManager objects for operating the CAN and PRG LEDs.
 * 
 * Both LEDs are connected directly to a GPIO pin, so the lambda
 * callback just uses a digital write operation to drive the output.
 */
LedManager CanLed([](unsigned int status){ digitalWrite(GPIO_LED_CAN, (status & 0x01)); }, CAN_LED_UPDATE_INTERVAL);
LedManager PrgLed([](unsigned int status){ digitalWrite(GPIO_LED_PRG, (status & 0x01)); }, PRG_LED_UPDATE_INTERVAL);

#include "definitions.h"

/**********************************************************************
 * MAIN PROGRAM - setup()
 */
void setup() {
  #ifdef DEBUG_SERIAL
  Serial.begin(DEBUG_SERIAL_PORT_SPEED);
  delay(DEBUG_SERIAL_START_DELAY);
  #endif

  // Set the mode of GPIO pins which are not configured by interface
  // libraries.
  pinMode(GPIO_MIKROBUS_MODULE0_CS, OUTPUT);
  pinMode(GPIO_MIKROBUS_MODULE1_CS, OUTPUT);
  pinMode(GPIO_LED_CAN, OUTPUT);
  pinMode(GPIO_LED_PRG, OUTPUT);

  SPI.begin();

  PRGButton.begin();

  CodeSwitchPISO.begin();
  
  // Run a startup sequence in the LED display: all LEDs on to confirm
  // function.
  CanLed.setStatus(0xff); PrgLed.setStatus(0xff);
  delay(100);
  CanLed.setStatus(0x00); PrgLed.setStatus(0x00);

  #include "setup.h"

  // Initialise and start N2K services.
  NMEA2000.SetProductInformation(PRODUCT_SERIAL_CODE, PRODUCT_CODE, PRODUCT_TYPE, PRODUCT_FIRMWARE_VERSION, PRODUCT_VERSION);
  NMEA2000.SetDeviceInformation(DEVICE_UNIQUE_NUMBER, DEVICE_FUNCTION, DEVICE_CLASS, DEVICE_MANUFACTURER_CODE);
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, ModuleConfiguration.getByte(MODULE_CONFIGURATION_CAN_SOURCE_INDEX)); // Configure for sending and receiving.
  NMEA2000.EnableForward(false); // Disable all msg forwarding to USB (=Serial)
  NMEA2000.ExtendTransmitMessages(TransmitMessages); // Tell library which PGNs we transmit
  NMEA2000.SetMsgHandler(messageHandler);
  NMEA2000.SetOnOpen(onN2kOpen);
  NMEA2000.Open();

  #ifdef DEBUG_SERIAL
  Serial.println();
  Serial.println("Starting:");
  Serial.print("  N2K Source address is "); Serial.println(NMEA2000.GetN2kSource());
  #endif
}

/**********************************************************************
 * MAIN PROGRAM - loop()
 * 
 * With the exception of NMEA2000.parseMessages() all of the functions
 * called from loop() implement interval timers which ensure that they
 * will mostly return immediately, only performing their substantive
 * tasks at intervals defined by program constants.
 */ 
void loop() {
  
  // Before we transmit anything, let's do the NMEA housekeeping and
  // process any received messages. This call may result in acquisition
  // of a new CAN source address, so we check if there has been any
  // change and if so save the new address to EEPROM for future re-use.
  NMEA2000.ParseMessages();
  if (NMEA2000.ReadResetAddressChanged()) {
    ModuleConfiguration.setByte(MODULE_CONFIGURATION_CAN_SOURCE_INDEX, NMEA2000.GetN2kSource());
  }

  #include "loop.h"

  // If the PRG button has been operated, then call the button handler.
  if (PRGButton.toggled()) {
    switch (ModuleOperatorInterface.handleButtonEvent(PRGButton.read(), (unsigned char) (CodeSwitchPISO.read() & 0xff))) {
      case ModuleOperatorInterface::MODE_CHANGE:
        PrgLed.setLedState(0, LedManager::ONCE);
        break;
      case ModuleOperatorInterface::ADDRESS_ACCEPTED:
        PrgLed.setLedState(0, LedManager::ONCE);
        break;
      case ModuleOperatorInterface::ADDRESS_REJECTED:
        PrgLed.setLedState(0, LedManager::THRICE);
        break;
      case ModuleOperatorInterface::VALUE_ACCEPTED:
        PrgLed.setLedState(0, LedManager::ONCE);
        break;
      case ModuleOperatorInterface::VALUE_REJECTED:
        PrgLed.setLedState(0, LedManager::THRICE);
        break;
      default:
        break;
    }
  }

  // Update LED outputs.
  CanLed.update(); PrgLed.update();
  
  // Make sure that we always eventually revert to normal operation.
  ModuleOperatorInterface.revertModeMaybe();
}

void messageHandler(const tN2kMsg &N2kMsg) {
  int iHandler;
  for (iHandler=0; NMEA2000Handlers[iHandler].PGN!=0 && !(N2kMsg.PGN==NMEA2000Handlers[iHandler].PGN); iHandler++);
  if (NMEA2000Handlers[iHandler].PGN != 0) {
    NMEA2000Handlers[iHandler].Handler(N2kMsg); 
  }
}

#ifndef CONFIGURATION_VALIDATOR
/**
 * @brief ModuleConfiguration validation callback.
 * 
 * ModuleConfiguration uses this callback to validate update values
 * before they are written into the configuration.
 * 
 * @attention Specialisations will probably need to override this
 * function and therefore must define CONFIGURATION_VALIDATOR.
 * 
 * @param index - the configuration address where value will be stored
 * if validation is successful.
 * @param value - the proposed configuration value.
 * @return true - the proposed value is acceptable.
 * @return false - the proposed value is not acceptable.
 */
bool configurationValidator(unsigned int index, unsigned char value) {
  switch (index) {
    case MODULE_CONFIGURATION_CAN_SOURCE_INDEX:
      return(true);
    default:
      return(false);
  }
}
#endif

#ifndef ON_N2K_OPEN
/**
 * @brief Function called by the NMEA2000 library once the CAN bus is
 * active.
 * 
 * @attention Specialisations will probably need to override this
 * function and therefore must define ON_N2K_OPEN.
 */
void onN2kOpen() {
}
#endif

