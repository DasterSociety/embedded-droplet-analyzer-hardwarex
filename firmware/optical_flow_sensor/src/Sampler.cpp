#include "Sampler.h"
#include "SharedBuffer.h"
#include "Config.h"
#include "SensorTSL.h"

namespace
{
    size_t g_count = 0;
    unsigned long g_nextSampleUs = 0;
}  // namespace

namespace Sampler
{

void begin()
{
    g_count = 0;
}

void startCapture()
{
    g_count = 0;
    g_nextSampleUs = micros();  // first sample is due immediately
}

void tick()
{
    if (g_count >= Config::BUFFER_SIZE_SAMPLES)
        return;

    unsigned long now = micros();
    // Signed compare handles micros() rollover (~71 min).
    if ((long)(now - g_nextSampleUs) < 0)
        return;

    // Advance the grid before the read so timestamps stay uniform even if
    // a single read happens to take a fraction of a ms longer than budget.
    g_nextSampleUs += Config::SAMPLE_PERIOD_US;

    // Capture the timestamp at the moment we ask the chip to integrate.
    // (Matches OFS Library.cpp:TSLReading_a — t = (float)millis() before getEvent.)
    uint32_t t_ms = millis();

    float lux;
    SensorTSL::readLux(lux);

    SharedBuffer::g_buf.capture[g_count].t_ms = t_ms;
    SharedBuffer::g_buf.capture[g_count].lux  = lux;
    g_count++;
}

bool isFull()
{
    return g_count >= Config::BUFFER_SIZE_SAMPLES;
}

size_t count()
{
    return g_count;
}

const Sample *data()
{
    return SharedBuffer::g_buf.capture;
}

size_t capacity()
{
    return Config::BUFFER_SIZE_SAMPLES;
}

}  // namespace Sampler
