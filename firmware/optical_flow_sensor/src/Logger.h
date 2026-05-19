#pragma once

#include <Arduino.h>
#include "Sampler.h"

namespace Logger {

// Mount the SD card. Returns false if SD.begin() fails.
bool begin();

// Find the next free LOG_NNN.CSV (1..999) and open it for writing.
// No header row is written — the file is a stream of "<t>, <lux>\r\n" lines
// matching the OFS project's Serial output (Library.cpp:TSLReading_a).
// On success, the chosen filename is copied to `filenameOut` (≥13 bytes).
bool openNext(char *filenameOut, size_t filenameOutLen);

// Append one CSV row in OFS format: "<t_ms_2dp>, <lux_2dp>\r\n".
// `t_ms` is the acquisition timestamp captured at sample time.
void writeRow(uint32_t t_ms, float lux);

// Bulk write a block of samples (the offload phase). Periodically calls
// flush() every Config::FLUSH_EVERY_N rows so a power loss mid-offload
// still leaves a salvageable partial file.
void writeAll(const Sampler::Sample *samples, size_t count);

void close();
bool isOpen();

}  // namespace Logger
