#pragma once

#include <Arduino.h>
#include "SharedBuffer.h"

// Phased burst sampler. Capture only touches RAM (the SharedBuffer union), so
// sample cadence is fully decoupled from SD activity — timestamps in the
// buffer are uniform 17 000 µs apart with sub-millisecond jitter regardless of
// what the SD card does later, during offload.
//
// Typical use from main.cpp:
//     Sampler::begin();              // once, in setup()
//     Sampler::startCapture();       // when entering CAPTURING state
//     while (...) {
//         Sampler::tick();           // every loop iteration; takes one sample if due
//         if (Sampler::isFull()) ... // transition to OFFLOADING
//     }
//     Logger::writeAll(Sampler::data(), Sampler::count());
namespace Sampler {

using Sample = SharedBuffer::Sample;

void          begin();                              // initialise internal state
void          startCapture();                       // reset count, arm scheduler
void          tick();                               // call every loop iteration while CAPTURING
bool          isFull();                             // buffer reached BUFFER_SIZE_SAMPLES
size_t        count();                              // number of samples currently held
const Sample* data();                               // pointer to the buffer (size = count())
size_t        capacity();                           // BUFFER_SIZE_SAMPLES

}  // namespace Sampler
