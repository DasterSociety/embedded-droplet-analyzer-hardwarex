#include <Arduino.h>
#include <SD.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>

#include "DropletAnalysis.h"

namespace {

int8_t signOf(float x) {
    if (x > 0.0f) return 1;
    if (x < 0.0f) return -1;
    return 0;
}

// Read the next decimal number from a File: skip leading non-numeric chars,
// then accept '-', digits, '.'. Returns 0.0f on EOF or empty token.
// Format produced by Logger::writeAll is "<float>, <float>\r\n", so we need
// to handle commas, spaces, and CR/LF as separators.
float parseNextFloat(File &f) {
    char buf[24];
    uint8_t len = 0;
    bool started = false;

    while (f.available() && !started) {
        char c = (char)f.read();
        if (c == '-' || isdigit((unsigned char)c)) {
            buf[len++] = c;
            started = true;
        }
    }
    while (f.available() && len < (sizeof(buf) - 1)) {
        char c = (char)f.peek();
        if (isdigit((unsigned char)c) || c == '.') {
            buf[len++] = (char)f.read();
        } else {
            break;
        }
    }
    buf[len] = '\0';
    return (len > 0) ? (float)atof(buf) : 0.0f;
}

}  // namespace

namespace DropletAnalysis {

uint16_t readCSV(const char *filename,
                 uint16_t *t_arr, float *L_arr,
                 uint16_t maxSamples) {
    File f = SD.open(filename);
    if (!f) return 0;

    uint16_t n = 0;
    uint32_t t0 = 0;
    bool first = true;

    while (f.available() && n < maxSamples) {
        float t_val = parseNextFloat(f);
        if (!f.available()) break;
        float L_val = parseNextFloat(f);

        if (first) {
            t0 = (uint32_t)t_val;
            first = false;
        }
        t_arr[n] = (uint16_t)((uint32_t)t_val - t0);
        L_arr[n] = L_val;
        n++;
    }
    f.close();
    return n;
}

void detrend(float *L_arr, uint16_t n) {
    if (n == 0) return;

    // double accumulator avoids loss of precision when summing 3000 floats.
    double sum = 0.0;
    for (uint16_t i = 0; i < n; i++) sum += (double)L_arr[i];
    float mean = (float)(sum / (double)n);

    for (uint16_t i = 0; i < n; i++) L_arr[i] -= mean;
}

uint16_t findCrossings(const float *dtCs, uint16_t n,
                       uint16_t *allCrossings,
                       uint16_t *fallingIdx, uint16_t *nFalling,
                       uint16_t *risingIdx,  uint16_t *nRising,
                       uint16_t maxCrossings) {
    uint16_t total = 0;
    *nFalling = 0;
    *nRising  = 0;
    if (n < 2) return 0;

    for (uint16_t i = 0; i < n - 1u && total < maxCrossings; i++) {
        int8_t change = signOf(dtCs[i + 1]) - signOf(dtCs[i]);
        if (change == 0) continue;

        allCrossings[total++] = i;
        if (change < 0) {
            // Positive-to-negative crossing (falling in the detrended signal).
            fallingIdx[(*nFalling)++] = i;
        } else {
            // Negative-to-positive crossing (rising in the detrended signal).
            risingIdx[(*nRising)++] = i;
        }
    }
    return total;
}

void findExtrema(const float *dtCs,
                 const uint16_t *allCrossings, uint16_t nAllCrossings,
                 uint16_t *extremaIdx) {
    for (uint16_t k = 0; k + 1u < nAllCrossings; k++) {
        uint16_t start = allCrossings[k];
        uint16_t end   = allCrossings[k + 1u];
        bool lookMax  = dtCs[start + 1u] > 0.0f;

        uint16_t bestPos = start;
        float    bestVal = dtCs[start];

        for (uint16_t j = start + 1u; j <= end; j++) {
            if (lookMax  && dtCs[j] > bestVal) { bestVal = dtCs[j]; bestPos = j; }
            if (!lookMax && dtCs[j] < bestVal) { bestVal = dtCs[j]; bestPos = j; }
        }
        extremaIdx[k] = bestPos;
    }
}

bool checkSampling(const uint16_t *extremaIdx, uint16_t nExtrema,
                   uint16_t *worstGap, uint16_t *nBad) {
    *worstGap = 0xFFFFu;
    *nBad     = 0;

    for (uint16_t k = 0; k + 1u < nExtrema; k++) {
        uint16_t gap = extremaIdx[k + 1u] - extremaIdx[k];
        if (gap < *worstGap) *worstGap = gap;
        if (gap < 2u)        (*nBad)++;
    }
    return (*worstGap >= 2u);
}

void pickCloserEdge(const float *dtCs,
                    const uint16_t *idx, uint16_t n,
                    uint16_t *pts) {
    for (uint16_t i = 0; i < n; i++) {
        uint16_t x = idx[i];
        // Pick whichever bracketing sample is closer to zero.
        pts[i] = x + (fabsf(dtCs[x + 1u]) < fabsf(dtCs[x]) ? 1u : 0u);
    }
}

uint16_t pairDroplets(const uint16_t *fallingIdx, uint16_t nFalling,
                      const uint16_t *risingIdx,  uint16_t nRising,
                      uint16_t *fallStart, uint16_t *riseStart) {
    *fallStart = 0;
    *riseStart = 0;
    if (nFalling == 0 || nRising == 0) return 0;

    // Discard leading rising edge when recording started mid-droplet
    // (signal was already in the negative/droplet phase at t=0).
    if (risingIdx[0] < fallingIdx[0]) (*riseStart)++;

    uint16_t nFallUsed = nFalling - *fallStart;
    uint16_t nRiseUsed = nRising  - *riseStart;

    // Discard trailing falling edge if the droplet never fully exited.
    if (nFallUsed > nRiseUsed) nFallUsed = nRiseUsed;
    if (nRiseUsed > nFallUsed) nRiseUsed = nFallUsed;

    return nFallUsed; // == nRiseUsed at this point
}

void statsReset(Stats *s) {
    s->n    = 0u;
    s->mean = 0.0f;
    s->M2   = 0.0f;
}

void statsAdd(Stats *s,
              const uint16_t *t,
              const uint16_t *fallingPts, const uint16_t *risingPts,
              uint16_t nDroplets,
              uint16_t fallStart, uint16_t riseStart,
              float Ud) {
    for (uint16_t k = 0; k < nDroplets; k++) {
        float dt_ms  = (float)t[risingPts [riseStart + k]]
                     - (float)t[fallingPts[fallStart + k]];
        float length = dt_ms * Ud;

        s->n++;
        float delta = length - s->mean;
        s->mean += delta / (float)s->n;
        s->M2   += delta * (length - s->mean);
    }
}

void statsSummarize(const Stats &s,
                    float *avgLength, float *stdLength, float *cv) {
    *avgLength = s.mean;
    *stdLength = (s.n > 1u) ? sqrtf(s.M2 / (float)(s.n - 1u)) : 0.0f;
    *cv        = (s.n > 0u && s.mean > 0.0f) ? (*stdLength / s.mean * 100.0f) : 0.0f;
}

}  // namespace DropletAnalysis
