/**
 * @file ModuleConfigurator.h
 * @author Paul Reeve (preeve@pdjr.eu)
 * @brief 
 * @version 0.1
 * @date 2023-05-10
 * @copyright Copyright (c) 2023
 */

#ifndef MODULE_CONFIGURATOR_H
#define MODULE_CONFIGURATOR_H

/**
 * ModuleConfigurator implements a simple configuration dialog
 * using the hardware capabilities of the NOP100 module which
 * consists of an 8-bit rotary selector and momentary push-button.
 * 
 * The module configuration protocol is a two-stage process
 * involving the entry of an 8-bit address followed by the entry of
 * an 8-bit value which should be saved to the location specified
 * by address.  The exact procedure is:
 * 
 * 1. Set up an address on the module's rotary selector;
 * 2. Press and hold the PRG button for more than 1 second;
 * 3. Set up a value on the module's rotary selector;
 * 4. Press the PRG button momentarily.
 * 
 * ModuleConfigurator manages the configuration process by
 * controlling data entry timings and time-outs and by allowing
 * the host application to validate entered values.
 */

class ModuleConfigurator {

  public:

  /**
   * @brief Result codes for the handleButtonEvent() method.
   */
  enum Outcome { MODE_CHANGE, ADDRESS_ACCEPTED, ADDRESS_REJECTED, VALUE_ACCEPTED, VALUE_REJECTED };

  /**
   * @brief ModuleConfigurator creates a new ModuleConfigurator instance.
   * 
   * The constructor accepts two boolean callback functions.
   * 
   * processAddress will be called when the user enters an address and
   * should return true if the address is valid, otherwise false.
   * 
   * processValue will be called when the user enters a value and will
   * be called with any presently active address and the entered value
   * and should return true if the value is valid for address.
   * 
   * @param processAddress - callback function for processing (most likely validating) an entered address.
   * @param processValue - callback function for processing (most likely validating) an entered value.
   */
  ModuleConfigurator(bool (*processAddress)(unsigned int address), bool (*processValue)(unsigned int address, unsigned char value));

  /**
   * @brief Get the time in milliseconds of the last invocation of the handleButtonEvent().
   * 
   * @return unsigned long - timetsamp in milliseconds.
   */
  unsigned long getButtonPressedAt();

  /**
   * @brief Handle a user interaction event (i.e. the press or release of the interface button).
     * 
     * This method should be called with \a buttonState set to either
     * Button::PRESSED or Button::RELEASED and, optionally, with 
     * \a value set to the current value of the DIL switch.
     * 
     * On a button press event a timestamp is taken which allows
     * subsequent release event to be associated with either a short
     * or long button press.
     * 
     * On a button release event there are several processing options:
     * 
     * A **long button press** will result in \a value being
     * passed to the current mode handler's validateAddress() method
     * and if characterised valid being saved as an *address* for
     * subsequent processing.
     * The method will return one of ADDRESS_ACCEPTED or
     * ADDRESS_REJECTED.
     *
     * A **short button press** consequent on a previously accepted
     * *address* will result in a call to the current mode handler's
     * processValue() method and, dependent upon the success or
     * failure or this call, the return of one of VALUE_ACCEPTED or
     * VALUE_REJECTED.
     * 
     * A **short button press** with no previously accepted *address*
     * will result in cycling of the currently active mode handler and
     * the return of MODE_CHANGE. 
     * 
     * @param buttonState - one of Button::PRESSED or Button::RELEASED.
     * @param value  - either an address or a value.
     * @return int - a value from the EventOutcome enum.
     */
    EventOutcome handleButtonEvent(bool buttonState, unsigned char value = 0);
    
  private:
    bool (*processAddress)(unsigned int address);
    bool (*processValue)(unsigned int address, unsigned char value);
    unsigned int currentMode;
    unsigned long buttonPressedAt;
    unsigned long revertInterval;

};

#endif
