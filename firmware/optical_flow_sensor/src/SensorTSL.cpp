#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

#include "SensorTSL.h"

namespace {
Adafruit_TSL2561_Unified tsl(TSL2561_ADDR_FLOAT, 12345);
}  // namespace

namespace SensorTSL {

bool begin() {
    if (!tsl.begin()) return false;
    tsl.setGain(TSL2561_GAIN_1X);
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
    return true;
}

bool readLux(float &lux) {
    sensors_event_t event;
    tsl.getEvent(&event);
    lux = event.light;
    return true;
}

}  // namespace SensorTSL
