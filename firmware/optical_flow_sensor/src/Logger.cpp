#include <SPI.h>
#include <SD.h>

#include "Logger.h"
#include "Config.h"

namespace {

File g_file;

bool nextFilename(char *buf, size_t len) {
    for (int i = 1; i <= 999; i++) {
        snprintf(buf, len, "LOG_%03d.CSV", i);
        if (!SD.exists(buf)) return true;
    }
    return false;
}

// Print "<t_ms_2dp>, <lux_2dp>\r\n" — matches the OFS project's
// Serial output exactly (`Serial.print((float)millis()); Serial.print(", ");
// Serial.println(event.light);` → both default to 2 decimal places).
inline void writeRowImpl(uint32_t t_ms, float lux) {
    g_file.print((float)t_ms);
    g_file.print(F(", "));
    g_file.println(lux);
}

}  // namespace

namespace Logger {

bool begin() {
    return SD.begin(Config::PIN_SD_CS);
}

bool openNext(char *filenameOut, size_t filenameOutLen) {
    if (!nextFilename(filenameOut, filenameOutLen)) return false;
    g_file = SD.open(filenameOut, FILE_WRITE);
    if (!g_file) return false;
    return true;
}

void writeRow(uint32_t t_ms, float lux) {
    if (!g_file) return;
    writeRowImpl(t_ms, lux);
}

void writeAll(const Sampler::Sample *samples, size_t count) {
    if (!g_file) return;
    for (size_t i = 0; i < count; i++) {
        writeRowImpl(samples[i].t_ms, samples[i].lux);
        if ((i + 1) % Config::FLUSH_EVERY_N == 0) {
            g_file.flush();
        }
    }
}

void close() {
    if (!g_file) return;
    g_file.flush();
    g_file.close();
}

bool isOpen() {
    return (bool)g_file;
}

}  // namespace Logger
