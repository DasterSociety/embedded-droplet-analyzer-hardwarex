#pragma once

#include <Arduino.h>

#ifndef DEBUG_OFS
#define DEBUG_OFS 1
#endif

namespace Config {

// ── Timing ────────────────────────────────────────────────────────────────────
constexpr unsigned long DEBOUNCE_MS         = 50;
constexpr unsigned long SAMPLE_PERIOD_US    = 17000;   // ~58 Hz: 13 ms TSL2561 integration + overhead
constexpr unsigned long ERROR_BLINK_MS      = 200;     // SD error: red LED half-period
constexpr unsigned long OFFLOAD_BLINK_MS    = 100;     // OFFLOADING: yellow fast-blink half-period
constexpr unsigned long PROMPT_BLINK_MS     = 500;     // PROMPT: green slow-blink half-period
constexpr unsigned long REJECT_FLASH_MS     = 400;     // brief red flash for rejected button press
constexpr unsigned long SERIAL_WAIT_MS      = 3000;    // 0 = don't wait for USB serial
constexpr unsigned int  FLUSH_EVERY_N       = 100;     // crash-resilience during offload

// ── Buffer dimensions ─────────────────────────────────────────────────────────
// 3000 samples × 17 ms ≈ 51 s per LOG_NNN.CSV file.
// Capture view (Sampler::Sample[]):  3000 × 8 B = 24 KB
// Analysis view (uint16_t t[] + float L[]): 3000 × 6 B = 18 KB
// Both views overlay the same physical buffer (SharedBuffer::g_buf) — modes are
// mutually exclusive in time, so the union is safe.
constexpr size_t   BUFFER_SIZE_SAMPLES = 3000;
constexpr uint16_t MAX_CROSSINGS       = 100;

// ── Experiment parameter ──────────────────────────────────────────────────────
// Droplet velocity along the optical sensing path.
// length_k [µm] = (t_rise - t_fall) [ms] × Ud [µm/ms = mm/s]
// Edit and rebuild to match your experiment.
constexpr float UD_MM_PER_S = 1.55089f;

// ── Pins ──────────────────────────────────────────────────────────────────────
constexpr uint8_t PIN_B1    = 2;    // STANDBY: enter Capture; CAPTURING: stop early; PROMPT: next burst
constexpr uint8_t PIN_B2    = 3;    // PROMPT: end session → STANDBY
constexpr uint8_t PIN_B3    = 5;    // STANDBY: enter Analysis
constexpr uint8_t PIN_LED_G = 8;    // ready / capturing
constexpr uint8_t PIN_LED_R = 9;    // error / rejected press
constexpr uint8_t PIN_LED_Y = 12;   // init / offloading / prompt / processing
constexpr uint8_t PIN_LED_B = A5;   // capturing / processing
constexpr uint8_t PIN_SD_CS = 4;

// ── LED state table (informational; states drive these in main.cpp) ───────────
//   INIT        → Y on
//   STANDBY     → G on
//   CAPTURING   → G on  + B on
//   OFFLOADING  → Y fast-blink
//   PROMPT      → Y on  + G slow-blink
//   PROCESSING  → Y on  + B on
//   ERROR_TSL   → R steady
//   ERROR_SD    → R fast-blink

}  // namespace Config

#if DEBUG_OFS
  #define DBG(x)   Serial.print(x)
  #define DBGLN(x) Serial.println(x)
#else
  #define DBG(x)
  #define DBGLN(x)
#endif
