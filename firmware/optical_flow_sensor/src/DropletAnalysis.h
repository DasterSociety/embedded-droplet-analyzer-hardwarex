#pragma once
#include <stdint.h>

// Droplet-length analysis for LOG_NNN.CSV files.
//
// Input format (per row, no header — produced by Logger::writeAll()):
//   "<millis_as_float_2dp>, <lux_as_float_2dp>\r\n"
//
// Pipeline (one call per file):
//   readCSV()        → t[]   (uint16_t, ms since first sample of this file)
//                      L[]   (float,    raw lux)
//   detrend()        → L[]   (float,    detrended in-place: L[i] -= mean)
//   findCrossings()  → all/falling/rising index arrays into the detrended buffer
//   findExtrema()    → dominant peak/trough between each consecutive crossing pair
//   checkSampling()  → adequacy of sample density between extrema
//   pickCloserEdge() → snap each crossing to the nearer-zero bracketing sample
//   pairDroplets()   → match falling↔rising edges into complete droplets
//   statsAdd()       → fold this file's droplet lengths into a running Stats
//
// Final mean / std / CV% across all processed files come from statsSummarize().
//
// Each LOG_NNN.CSV is at most ~51 s (3000 × 17 ms), so timestamps fit in uint16_t
// once zeroed to the first sample.
namespace DropletAnalysis {

// Read (t, L) pairs from `filename` on the SD card. Timestamps are zeroed to
// the first sample. Returns the number of samples read, or 0 on error.
uint16_t readCSV(const char *filename,
                 uint16_t *t_arr, float *L_arr,
                 uint16_t maxSamples);

// Subtract the signal mean from L_arr in-place (equivalent to MATLAB
// detrend(L, 0)). After this call L_arr holds the detrended signal as float.
void detrend(float *L_arr, uint16_t n);

// Detect zero-crossings in the detrended signal.
// Populates allCrossings (all, in order), fallingIdx (+ → −), risingIdx (− → +).
// Returns the total crossing count (capped at maxCrossings).
uint16_t findCrossings(const float *dtCs, uint16_t n,
                       uint16_t *allCrossings,
                       uint16_t *fallingIdx, uint16_t *nFalling,
                       uint16_t *risingIdx,  uint16_t *nRising,
                       uint16_t maxCrossings);

// Find the dominant extremum in each interval between consecutive crossings.
// extremaIdx must have capacity for (nAllCrossings - 1) entries.
void findExtrema(const float *dtCs,
                 const uint16_t *allCrossings, uint16_t nAllCrossings,
                 uint16_t *extremaIdx);

// Check whether sampling is adequate (all peak-to-trough gaps ≥ 2 samples).
// Sets worstGap (minimum gap) and nBad (number of under-sampled transitions).
bool checkSampling(const uint16_t *extremaIdx, uint16_t nExtrema,
                   uint16_t *worstGap, uint16_t *nBad);

// For each crossing index, snap to whichever bracketing sample is closer to 0.
// Equivalent to MATLAB: pts = idx + (|dtCs[idx+1]| < |dtCs[idx]|).
void pickCloserEdge(const float *dtCs,
                    const uint16_t *idx, uint16_t n,
                    uint16_t *pts);

// Pair falling and rising edges into complete droplets.
// Discards a leading rising edge or trailing falling edge if present.
// Sets fallStart / riseStart (offsets into the pts arrays).
// Returns the number of complete droplets.
uint16_t pairDroplets(const uint16_t *fallingIdx, uint16_t nFalling,
                      const uint16_t *risingIdx,  uint16_t nRising,
                      uint16_t *fallStart, uint16_t *riseStart);

// Cumulative droplet-length statistics across one or more files.
// Reset once with statsReset(), update per file with statsAdd(), then read out
// final mean / std / CV% via statsSummarize(). Welford's algorithm extends
// naturally across batches, so the result is identical to merging every
// droplet length into a single one-pass calculation.
struct Stats {
    uint32_t n;     // total droplet count across all calls
    float    mean;  // running mean (Welford)
    float    M2;    // running sum of squared deviations (Welford)
};

void statsReset(Stats *s);

// Fold this file's per-droplet lengths into the running state.
//   length_k = (t[risingPts[riseStart+k]] - t[fallingPts[fallStart+k]]) * Ud  [µm]
void statsAdd(Stats *s,
              const uint16_t *t,
              const uint16_t *fallingPts, const uint16_t *risingPts,
              uint16_t nDroplets,
              uint16_t fallStart, uint16_t riseStart,
              float Ud);

// Extract mean length, sample std, CV% from the accumulated state.
void statsSummarize(const Stats &s,
                    float *avgLength, float *stdLength, float *cv);

}  // namespace DropletAnalysis
