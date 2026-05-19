#pragma once

#include <Arduino.h>

// Debounced active-LOW push button on an INPUT_PULLUP pin.
// Usage:
//     Button b1(PIN_B1);
//     b1.begin();                 // in setup()
//     b1.poll();                  // every loop()
//     if (b1.consumePress()) ...  // true once per confirmed press
class Button {
public:
    explicit Button(uint8_t pin);
    void begin();
    void poll();
    bool consumePress();

private:
    uint8_t       pin_;
    int           last_;
    int           stable_;
    unsigned long debounceAt_;
    bool          pressed_;
};
