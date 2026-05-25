#include <Arduino.h>
#include <Button.h>
#include <ModuleUserInterface.h>
  
ModuleUserInterface::ModuleUserInterface(bool (*processAddress)(unsigned int address), bool (*processValue)(unsigned int address, unsigned char value) {
  this->processAddress = processAddress;
  this->processValue = processValue;
  this->currentMode = 0;
  this->currentAddress = -1;
  this->buttonPressedAt = 0UL;
}

unsigned long ModuleUserInterface::getButtonPressedAt() {
  return(this->buttonPressedAt);
}

void ModuleUserInterface::revertModeMaybe() {
  if (this->revertInterval != 0UL) {
    if (millis() > (this->buttonPressedAt + this->revertInterval)) {
      this->currentMode = 0;
      this->currentAddress = -1;
    }
  }
}
    
ModuleUserInterface::EventOutcome ModuleUserInterface::handleButtonEvent(bool buttonState, unsigned char value) {
  EventOutcome retval = MODE_CHANGE;
  unsigned long now = millis();

  if (buttonState == Button::PRESSED) {
    this->buttonPressedAt = now;
  } else {
    if ((this->buttonPressedAt) && (now < (this->buttonPressedAt + 1000))) {
      if (this->currentAddress != -1) {
        retval = (this->processValue((unsigned int) this->currentAddress, value))?VALUE_ACCEPTED:VALUE_REJECTED;
        this->currentAddress = -1;
      }
    } else {
      if (this->currentAddress == -1) {
        modeHandlers[this->currentMode]->validateAddress((unsigned int) value)) {
        this->currentAddress = (int) value;
        retval = ADDRESS_ACCEPTED;
      } else {
        this->currentAddress = -1;
        retval = ADDRESS_REJECTED;
      }
    }
  }
  return(retval);
}
