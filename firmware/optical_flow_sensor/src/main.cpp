/*
 * Optical Flow Sensor — unified firmware (capture + analysis)
 * Board : Arduino Zero (SAMD21 Cortex-M0+)
 *
 * Two user-selectable modes from STANDBY:
 *   B1 → Capture mode    : phased-burst light intensity logger that writes
 *                          LOG_NNN.CSV files to the microSD card. Mirrors the
 *                          standalone TSL2561 logger flow.
 *   B3 → Analysis mode   : walks every LOG_*.CSV on the card, runs the droplet
 *                          characterization pipeline, prints per-file progress
 *                          and a cumulative summary, then returns to STANDBY.
 *
 * Capture and analysis are mutually exclusive in time, so the 24 KB sample
 * buffer is shared between them via SharedBuffer::g_buf.
 *
 * Pin map
 * -------
 *   D2   B1   STANDBY: enter Capture; CAPTURING: stop early; PROMPT: next burst
 *   D3   B2   PROMPT: end session → STANDBY
 *   D5   B3   STANDBY: enter Analysis
 *   D8   LED_G  ready / capturing / prompt slow-blink
 *   D9   LED_R  error / rejected press
 *   D12  LED_Y  init / offloading / prompt / processing
 *   A5   LED_B  capturing / processing
 *   D4   SD chip-select
 *   SDA/SCL    TSL2561 I2C
 *   MOSI=23, MISO=22, SCK=24  (hardware SPI, fixed on Zero)
 *
 * State machine
 * -------------
 *   INIT       (Y on)            — probe TSL2561 + SD; both required
 *   STANDBY    (G on)            — B1 → CAPTURING, B3 → PROCESSING
 *   CAPTURING  (G + B on)        — RAM buffer fills; B1 or buffer-full → OFFLOADING
 *   OFFLOADING (Y fast-blink)    — buffer is written to LOG_NNN.CSV; auto → PROMPT
 *   PROMPT     (Y on, G slow-blink)
 *                                — B1 → CAPTURING (next file); B2 → STANDBY
 *   PROCESSING (Y + B on)        — walk LOG_*.CSV, fold into Welford, print summary; auto → STANDBY
 *   ERROR_TSL  (R steady)        — TSL2561 not found at boot
 *   ERROR_SD   (R fast-blink)    — SD mount failed
 *
 * CSV format (per row)
 * --------------------
 *   "<millis_as_float_2dp>, <lux_2dp>\r\n"   — no header.
 *   Matches the OFS project's Serial output (Library.cpp:TSLReading_a) so the
 *   existing detection pipeline can read these files unchanged.
 *
 * Compile-time experiment parameter
 * ---------------------------------
 *   Config::UD_MM_PER_S — droplet velocity along the sensing path.
 *   Edit Config.h and rebuild to match your experiment.
 */

#include <Arduino.h>

#include "Config.h"
#include "Leds.h"
#include "Buttons.h"
#include "SensorTSL.h"
#include "Sampler.h"
#include "Logger.h"
#include "FileIterator.h"
#include "DropletAnalysis.h"
#include "SharedBuffer.h"

// ── Static SRAM (analysis-only working arrays, ~1.2 KB) ───────────────────────
static uint16_t g_allCrossings[Config::MAX_CROSSINGS];
static uint16_t g_fallingIdx[Config::MAX_CROSSINGS];
static uint16_t g_risingIdx[Config::MAX_CROSSINGS];
static uint16_t g_extremaIdx[Config::MAX_CROSSINGS];
static uint16_t g_fallingPts[Config::MAX_CROSSINGS];
static uint16_t g_risingPts[Config::MAX_CROSSINGS];

enum class State : uint8_t
{
    INIT,
    STANDBY,
    CAPTURING,
    OFFLOADING,
    PROMPT,
    PROCESSING,
    ERROR_TSL,
    ERROR_SD,
};
static State g_state = State::INIT;

static Button g_b1(Config::PIN_B1);
static Button g_b2(Config::PIN_B2);
static Button g_b3(Config::PIN_B3);

static unsigned long g_lastBlinkMs = 0;
static bool g_blinkOn = false;
static char g_filename[13]; // "LOG_NNN.CSV" + NUL

// ── Cumulative state for an analysis pass ─────────────────────────────────────
static DropletAnalysis::Stats g_stats;
static uint16_t g_filesProcessed = 0;
static uint16_t g_filesFailed = 0;

// ── Helpers ───────────────────────────────────────────────────────────────────

static void enterStandby()
{
    Leds::allOff();
    Leds::green(true);
    g_state = State::STANDBY;
    DBGLN(F("STANDBY — B1: Capture, B3: Analysis"));
}

// Brief red flash when the user presses a button that's valid for some mode
// but unavailable right now (e.g. B3 pressed but no LOG_*.CSV on card).
static void rejectFlash()
{
    Leds::green(false);
    Leds::red(true);
    delay(Config::REJECT_FLASH_MS);
    Leds::red(false);
    Leds::green(true);
}

// ── Analysis pipeline (one call per file) ─────────────────────────────────────

static void processFile(const char *filename)
{
    DBG(F("PROCESSING → "));
    DBGLN(filename);

    uint16_t n = DropletAnalysis::readCSV(filename,
                                          SharedBuffer::g_buf.anal.t,
                                          SharedBuffer::g_buf.anal.L,
                                          (uint16_t)Config::BUFFER_SIZE_SAMPLES);
    if (n == 0u)
    {
        Serial.print(filename);
        Serial.println(F(" : read failed"));
        g_filesFailed++;
        return;
    }

    DropletAnalysis::detrend(SharedBuffer::g_buf.anal.L, n);

    uint16_t nFalling = 0, nRising = 0;
    uint16_t nAll = DropletAnalysis::findCrossings(
        SharedBuffer::g_buf.anal.L, n, g_allCrossings,
        g_fallingIdx, &nFalling,
        g_risingIdx, &nRising,
        Config::MAX_CROSSINGS);

    if (nAll > 1u)
    {
        DropletAnalysis::findExtrema(SharedBuffer::g_buf.anal.L,
                                     g_allCrossings, nAll, g_extremaIdx);
    }

    uint16_t worstGap = 0, nBad = 0;
    bool samplingOK = false;
    uint16_t nExtrema = (nAll > 1u) ? (uint16_t)(nAll - 1u) : 0u;
    if (nExtrema > 0u)
    {
        samplingOK = DropletAnalysis::checkSampling(g_extremaIdx, nExtrema,
                                                    &worstGap, &nBad);
    }

    DropletAnalysis::pickCloserEdge(SharedBuffer::g_buf.anal.L,
                                    g_fallingIdx, nFalling, g_fallingPts);
    DropletAnalysis::pickCloserEdge(SharedBuffer::g_buf.anal.L,
                                    g_risingIdx, nRising, g_risingPts);

    uint16_t fallStart = 0, riseStart = 0;
    uint16_t nDroplets = DropletAnalysis::pairDroplets(
        g_fallingIdx, nFalling, g_risingIdx, nRising,
        &fallStart, &riseStart);

    if (nDroplets > 0u)
    {
        DropletAnalysis::statsAdd(&g_stats,
                                  SharedBuffer::g_buf.anal.t,
                                  g_fallingPts, g_risingPts,
                                  nDroplets, fallStart, riseStart,
                                  Config::UD_MM_PER_S);
    }
    g_filesProcessed++;

    Serial.print(filename);
    Serial.print(F(" : "));
    Serial.print(nDroplets);
    Serial.print(F(" droplets ("));
    Serial.print(n);
    Serial.print(F(" samples"));
    if (!samplingOK && nExtrema > 0u)
    {
        Serial.print(F(", "));
        Serial.print(nBad);
        Serial.print(F(" undersampled"));
    }
    Serial.println(F(")"));
}

static void printCumulativeResults()
{
    float avgLen = 0.0f, stdLen = 0.0f, cv = 0.0f;
    DropletAnalysis::statsSummarize(g_stats, &avgLen, &stdLen, &cv);

    Serial.println();
    Serial.println(F("=== Cumulative results ==="));
    Serial.print(F("Files processed: "));
    Serial.println(g_filesProcessed);
    if (g_filesFailed > 0u)
    {
        Serial.print(F("Files failed   : "));
        Serial.println(g_filesFailed);
    }
    Serial.print(F("Total droplets : "));
    Serial.println(g_stats.n);
    if (g_stats.n > 0u)
    {
        Serial.print(F("Avg length     : "));
        Serial.print(avgLen, 2);
        Serial.println(F(" um"));
        Serial.print(F("Std dev        : "));
        Serial.print(stdLen, 2);
        Serial.println(F(" um"));
        Serial.print(F("CV             : "));
        Serial.print(cv, 2);
        Serial.println(F(" %"));
    }
}

// ── State handlers ────────────────────────────────────────────────────────────

static void handleInit()
{
    Leds::allOff();
    Leds::yellow(true);
    DBGLN(F("INIT: testing hardware..."));

    if (!SensorTSL::begin())
    {
        DBGLN(F("INIT: TSL2561 not found"));
        Leds::allOff();
        Leds::red(true);
        g_state = State::ERROR_TSL;
        return;
    }
    DBGLN(F("INIT: TSL2561 OK"));

    if (!Logger::begin())
    {
        DBGLN(F("INIT: SD card not found"));
        Leds::allOff();
        g_lastBlinkMs = millis();
        g_blinkOn = false;
        g_state = State::ERROR_SD;
        return;
    }
    DBGLN(F("INIT: SD OK"));

    Sampler::begin();
    enterStandby();
}

static void handleStandby()
{
    // B1 → enter Capture mode
    if (g_b1.consumePress())
    {
        Leds::allOff();
        Leds::green(true);
        Leds::blue(true);
        Sampler::startCapture();
        g_state = State::CAPTURING;
        DBGLN(F("STANDBY → CAPTURING"));
        return;
    }

    // B3 → enter Analysis mode
    if (g_b3.consumePress())
    {
        FileIterator::reset();
        if (!FileIterator::findFirst(g_filename, sizeof(g_filename)))
        {
            DBGLN(F("STANDBY: no LOG_*.CSV on card"));
            rejectFlash();
            return;
        }
        DropletAnalysis::statsReset(&g_stats);
        g_filesProcessed = 0;
        g_filesFailed = 0;

        Leds::allOff();
        Leds::yellow(true);
        Leds::blue(true);
        g_state = State::PROCESSING;

        Serial.println();
        DBGLN(F("=== Analysis pass ==="));
        DBG(F("Ud = "));
        DBG(Config::UD_MM_PER_S);
        DBGLN(F(" mm/s"));
        return;
    }

    // B2 in STANDBY — ignored (only meaningful in PROMPT)
    g_b2.consumePress();
}

static void handleCapturing()
{
    Sampler::tick();

    const bool userStop = g_b1.consumePress();
    const bool bufferFull = Sampler::isFull();

    if (userStop || bufferFull)
    {
        Leds::blue(false);
        Leds::green(false);
        g_lastBlinkMs = millis();
        g_blinkOn = false;
        g_state = State::OFFLOADING;
        DBG(F("CAPTURING → OFFLOADING ("));
        DBG(Sampler::count());
        DBG(F(" samples, "));
        DBGLN(bufferFull ? F("buffer full)") : F("user stop)"));
    }
}

static void handleOffloading()
{
    // Yellow fast-blink while we work, so the user can see the firmware is
    // alive even when this takes a couple of seconds on a slow card.
    unsigned long now = millis();
    if ((now - g_lastBlinkMs) >= Config::OFFLOAD_BLINK_MS)
    {
        g_lastBlinkMs = now;
        g_blinkOn = !g_blinkOn;
        Leds::yellow(g_blinkOn);
    }

    char filename[13];
    if (!Logger::openNext(filename, sizeof(filename)))
    {
        DBGLN(F("OFFLOADING: cannot open log file"));
        Leds::allOff();
        g_state = State::ERROR_SD;
        return;
    }

    DBG(F("OFFLOADING → "));
    DBGLN(filename);

    Logger::writeAll(Sampler::data(), Sampler::count());
    Logger::close();

    DBGLN(F("OFFLOADING: done"));

    Leds::allOff();
    Leds::yellow(true); // steady yellow = "decision needed"
    g_lastBlinkMs = millis();
    g_blinkOn = false;
    g_state = State::PROMPT;
}

static void handlePrompt()
{
    // Slow-blink the green LED to signal "press a button".
    unsigned long now = millis();
    if ((now - g_lastBlinkMs) >= Config::PROMPT_BLINK_MS)
    {
        g_lastBlinkMs = now;
        g_blinkOn = !g_blinkOn;
        Leds::green(g_blinkOn);
    }

    if (g_b1.consumePress())
    {
        Leds::yellow(false);
        Leds::green(true);
        Leds::blue(true);
        Sampler::startCapture();
        g_state = State::CAPTURING;
        DBGLN(F("PROMPT → CAPTURING (next file)"));
        return;
    }

    if (g_b2.consumePress())
    {
        Leds::yellow(false);
        DBGLN(F("PROMPT → STANDBY (session ended)"));
        enterStandby();
    }
}

static void handleProcessing()
{
    // Process one file per loop() iteration; chain through all of them, then
    // print the cumulative summary and return to STANDBY automatically.
    processFile(g_filename);
    if (FileIterator::findNext(g_filename, sizeof(g_filename)))
    {
        return; // stay in PROCESSING for the next file
    }
    printCumulativeResults();
    enterStandby();
}

static void handleErrorSd()
{
    unsigned long now = millis();
    if ((now - g_lastBlinkMs) >= Config::ERROR_BLINK_MS)
    {
        g_lastBlinkMs = now;
        g_blinkOn = !g_blinkOn;
        Leds::red(g_blinkOn);
    }
}

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup()
{
    Leds::begin();
    g_b1.begin();
    g_b2.begin();
    g_b3.begin();

    Serial.begin(115200);
#if DEBUG_OFS
    {
        unsigned long t0 = millis();
        while (!Serial && (millis() - t0) < Config::SERIAL_WAIT_MS)
        {
        }
    }
    DBGLN(F("=== Optical Flow Sensor (capture + analysis) ==="));
    DBG(F("Ud = "));
    DBG(Config::UD_MM_PER_S);
    DBGLN(F(" mm/s"));
#endif

    DropletAnalysis::statsReset(&g_stats);
    handleInit();
}

void loop()
{
    g_b1.poll();
    g_b2.poll();
    g_b3.poll();

    switch (g_state)
    {
    case State::INIT:
        break; // never re-entered
    case State::STANDBY:
        handleStandby();
        break;
    case State::CAPTURING:
        handleCapturing();
        break;
    case State::OFFLOADING:
        handleOffloading();
        break;
    case State::PROMPT:
        handlePrompt();
        break;
    case State::PROCESSING:
        handleProcessing();
        break;
    case State::ERROR_TSL:
        break; // R steady, nothing to do
    case State::ERROR_SD:
        handleErrorSd();
        break;
    }
}
