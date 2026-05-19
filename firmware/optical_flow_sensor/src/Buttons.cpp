#include "Buttons.h"
#include "Config.h"

Button::Button(uint8_t pin)
    : pin_(pin),
      last_(HIGH),
      stable_(HIGH),
      debounceAt_(0),
      pressed_(false) {}

void Button::begin() {
    pinMode(pin_, INPUT_PULLUP);
}

void Button::poll() {
    int reading = digitalRead(pin_);
    if (reading != last_) {
        debounceAt_ = millis();
        last_ = reading;
    }
    if ((millis() - debounceAt_) >= Config::DEBOUNCE_MS && reading != stable_) {
        stable_ = reading;
        if (stable_ == LOW) pressed_ = true;   // falling edge = pressed
    }
}

bool Button::consumePress() {
    if (!pressed_) return false;
    pressed_ = false;
    return true;
}
