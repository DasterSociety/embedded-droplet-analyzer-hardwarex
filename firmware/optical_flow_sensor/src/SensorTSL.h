#pragma once

namespace SensorTSL {

// Initialise the TSL2561 in 1× gain, 13 ms integration (fastest mode).
// Returns false if the chip does not respond on I2C.
bool begin();

// Blocking lux read. Takes ~14 ms because the Adafruit driver triggers a
// fresh integration window on every call (enable → delay → read → disable).
// Sets `lux` and returns true on success.
bool readLux(float &lux);

}  // namespace SensorTSL
