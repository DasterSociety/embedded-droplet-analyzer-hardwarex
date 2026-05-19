#include <Arduino.h>
#include "Leds.h"
#include "Config.h"

namespace Leds {

void begin() {
    pinMode(Config::PIN_LED_G, OUTPUT);
    pinMode(Config::PIN_LED_R, OUTPUT);
    pinMode(Config::PIN_LED_Y, OUTPUT);
    pinMode(Config::PIN_LED_B, OUTPUT);
    allOff();
}

void allOff() {
    digitalWrite(Config::PIN_LED_G, LOW);
    digitalWrite(Config::PIN_LED_R, LOW);
    digitalWrite(Config::PIN_LED_Y, LOW);
    digitalWrite(Config::PIN_LED_B, LOW);
}

void green (bool on) { digitalWrite(Config::PIN_LED_G, on ? HIGH : LOW); }
void red   (bool on) { digitalWrite(Config::PIN_LED_R, on ? HIGH : LOW); }
void yellow(bool on) { digitalWrite(Config::PIN_LED_Y, on ? HIGH : LOW); }
void blue  (bool on) { digitalWrite(Config::PIN_LED_B, on ? HIGH : LOW); }

}  // namespace Leds
