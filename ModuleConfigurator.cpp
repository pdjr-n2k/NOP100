#include <Arduino.h>
#include <Button.h>
#include <ModuleConfigurator.h>
  
ModuleConfigurator::ModuleConfigurator(void (*processValue)(unsigned char address, unsigned char value), bool (*validateAddress)(unsigned int address)=[](unsigned int address){ return(true); }, unsigned long timeout) {
  this->processValue = processValue;
  this->validateAddress = validateAddress;
  this->timeout = timeout;
  this->address = 0;
  this->addressIsValid = false;
  this->buttonPressedAt = 0UL;
}
    
ModuleConfigurator::EventOutcome ModuleConfigurator::handleButtonEvent(bool buttonState, unsigned char value) {
  unsigned long now = millis();

  if (now > (this->buttonPressedAt + this->timeout)) this->addressIsValid = false;

  if (buttonState == Button::PRESSED) { // button pressed
    this->buttonPressedAt = now;
  } else { // button released
    if ((this->buttonPressedAt) && (now > (this->buttonPressedAt + CONFIGURATOR_LONG_BUTTON_PRESS))) { // long press - its an address
      if (this->validateAdress(value)) {
        retval = ADDRESS_ACCEPTED;
        this->address = value;
        this->addressIsValid = true;
      } else {
        retval = ADDRESS_REJECTED;
        this->addressIsValid = false;
      }
    } else { // short press - its a value
      if (this->addressIsValid) {
        if (this->processValue(this->address, value)) {
          retval = VALUE_ACCEPTED
          this->addressIsValid = false;
        } else {
          retval = value_REJECTED;
        }
      }
    }
  }
  return(retval);
}
