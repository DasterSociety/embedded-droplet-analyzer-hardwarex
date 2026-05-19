#pragma once

#include <stdint.h>
#include <stddef.h>
#include "Config.h"

// Single 24 KB buffer in SRAM, reinterpreted by whichever mode is currently
// active. Capture mode and analysis mode are mutually exclusive in time, so
// overlaying their two data layouts in one union is safe and halves the RAM
// footprint of the unified firmware.
//
//   capture[] — Sampler fills this during CAPTURING and OFFLOADING reads it.
//               Layout: { uint32_t t_ms; float lux; } × BUFFER_SIZE_SAMPLES.
//
//   anal.t[]  — uint16_t timestamps (zeroed to first sample), filled by
//   anal.L[]    DropletAnalysis::readCSV during PROCESSING. Same physical
//               memory; the analyser only reads the file currently being
//               processed, so it cannot collide with anything else.
namespace SharedBuffer {

struct Sample {
    uint32_t t_ms;   // millis() captured immediately before the TSL2561 read
    float    lux;
};

union BufferUnion {
    Sample capture[Config::BUFFER_SIZE_SAMPLES];           // 24 KB
    struct {
        uint16_t t[Config::BUFFER_SIZE_SAMPLES];
        float    L[Config::BUFFER_SIZE_SAMPLES];
    } anal;                                                // 18 KB
};

extern BufferUnion g_buf;

}  // namespace SharedBuffer
