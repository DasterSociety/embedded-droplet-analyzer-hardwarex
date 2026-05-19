# Optical Flow Sensor — Firmware

Open-source unified firmware for an Arduino Zero–based optical flow sensor that combines two functions in a single image:

1. **Capture mode** — phased-burst TSL2561 light-intensity logger; writes `LOG_NNN.CSV` files to a microSD card with deterministic 17 ms sample spacing.
2. **Analysis mode** — on-device droplet characterization; walks every `LOG_*.CSV` on the card, runs the detrend / zero-crossing / pairing pipeline, and reports a cumulative droplet-length summary (count, mean, std, CV%) over Serial.

Companion firmware to the *HardwareX* manuscript "[manuscript title TBD]". For day-to-day operation, see [`OPERATION.md`](./OPERATION.md).

## Status

Active development. Target hardware: **Arduino Zero** (Atmel SAMD21G18A, Cortex-M0+, 32 KB SRAM, 256 KB Flash).

This firmware supersedes the standalone [`TSL2561_logger`](../TSL2561_logger/) and [`droplet_characterization`](../droplet_characterization/) projects, which remain in the repository as reference implementations of the individual modes.

## Quick start

Requires [PlatformIO](https://platformio.org/).

```sh
cd firmware/optical_flow_sensor
pio run                # build
pio run -t upload      # build + flash to Arduino Zero
pio device monitor     # open Serial debug at 115200 baud
```

PlatformIO automatically pulls all library dependencies declared in `platformio.ini`.

## Modes at a glance

From STANDBY (green LED steady):

| Press | Mode | Outcome |
|---|---|---|
| **B1** | Capture | Records up to ~51 s into RAM, then offloads to `LOG_NNN.CSV`. Repeat as needed via the PROMPT loop; B2 ends the session and returns to STANDBY. |
| **B3** | Analysis | Walks every `LOG_*.CSV` in numerical order, prints one progress line per file, then a cumulative summary. Returns to STANDBY automatically. |

The TSL2561 sensor and the SD card are **both required at boot** — the firmware refuses to enter STANDBY otherwise (steady red = sensor missing; red fast-blink = SD missing).

## Architecture

**Single shared 24 KB buffer.** Capture and Analysis are mutually exclusive in time, so their two data layouts overlay the same physical SRAM via a `union` (`SharedBuffer::g_buf`):

- *Capture view* — `Sample[3000]` (uint32_t millis + float lux), 24 KB.
- *Analysis view* — parallel `uint16_t t[3000]` + `float L[3000]`, 18 KB.

This keeps RAM peak at ~25 KB (24 KB buffer + ~1.2 KB analysis index arrays + library state), well inside the SAMD21's 32 KB ceiling.

**Phased-burst capture.** Each Capture burst fills the RAM buffer with no SD activity, then offloads it to the next free `LOG_NNN.CSV`. Because the sensor and SD card never run concurrently, the timestamps within each file are uniform 17 000 µs apart with sub-millisecond jitter — any visible deviation is real sensor behaviour, not storage jitter.

**Single-shot per file analysis.** Each `LOG_NNN.CSV` is read end-to-end into the shared buffer, then processed in a single pass. Per-droplet lengths from each file are folded into a running Welford accumulator that lives across files, so the final cumulative summary is mathematically identical to merging every droplet length into one one-pass calculation. Welford's algorithm extends naturally across batches.

Pipeline (one call per file, plus a single summary at the end):

```
readCSV()        →  uint16_t t[]   (zeroed ms)  +  float L[]   (raw lux)
detrend()        →  L[] -= mean(L)              (in-place)
findCrossings()  →  all / falling / rising index arrays
findExtrema()    →  dominant peak/trough between consecutive crossings
checkSampling()  →  reports if any peak-to-trough gap is < 2 samples
pickCloserEdge() →  snap each crossing to the nearer-zero bracketing sample
pairDroplets()   →  match falling↔rising edges, drop incomplete leading/trailing
statsAdd()       →  fold this file's droplet lengths into the running Stats
                                       ┄
statsSummarize() →  final mean / std / CV%      (called once at end of pass)
```

## File layout

```
src/
├── Config.h              ← pin map, timing, buffer dimensions, Ud, LED-state table
├── Leds.{h,cpp}          ← LED helpers (allOff, green, red, yellow, blue)
├── Buttons.{h,cpp}       ← debounced active-low Button class
├── SensorTSL.{h,cpp}     ← TSL2561 wrapper (1× gain, 13 ms integration)
├── Sampler.{h,cpp}       ← fixed-grid scheduler, fills SharedBuffer in capture
├── Logger.{h,cpp}        ← SD lifecycle, OFS-compatible row writer
├── FileIterator.{h,cpp}  ← walks LOG_001.CSV … LOG_999.CSV on the SD card
├── DropletAnalysis.{h,cpp} ← detrend / crossings / pairing / Welford stats
├── SharedBuffer.{h,cpp}  ← 24 KB union shared between capture and analysis
└── main.cpp              ← unified state machine + setup() / loop()
```

## Output format

### Capture mode

Each `LOG_NNN.CSV` is a stream of rows with **no header**:

```
<millis_as_float_2dp>, <lux_2dp>\r\n
```

Example:

```
12345.00, 234.56
12362.00, 234.78
12379.00, 234.55
```

The format matches the *OFS* detection pipeline's expected input (`OFS/Software Dev/OFS/lib/MyLib/Library.cpp:TSLReading_a`), so existing analysis code can read these files unchanged. Timestamps are milliseconds since power-on, captured immediately before the TSL2561 read.

### Analysis mode

Serial output at 115 200 baud. One compact line per file as it is processed:

```
LOG_001.CSV : 17 droplets (3000 samples)
LOG_002.CSV : 16 droplets (3000 samples, 1 undersampled)
```

Then a single cumulative block:

```
=== Cumulative results ===
Files processed: 2
Total droplets : 33
Avg length     : 612.34 um
Std dev        : 11.20 um
CV             : 1.83 %
```

Files that fail to read are reported as `LOG_NNN.CSV : read failed` and counted under `Files failed:` in the cumulative block.

## Configuration

Compile-time knobs in `src/Config.h`:

| Constant | Default | Meaning |
|---|---|---|
| `UD_MM_PER_S` | 1.55089 | Droplet velocity along the optical sensing path; sets the µm scale of the reported lengths |
| `SAMPLE_PERIOD_US` | 17 000 | Inter-sample interval in capture (~58 Hz) |
| `BUFFER_SIZE_SAMPLES` | 3 000 | Per-burst capacity (24 KB → ~51 s) |
| `MAX_CROSSINGS` | 100 | Upper bound on rising+falling crossings per file |
| `DEBOUNCE_MS` | 50 | Button debounce window |
| `FLUSH_EVERY_N` | 100 | SD-flush cadence during offload (crash-resilience) |
| `OFFLOAD_BLINK_MS` | 100 | OFFLOADING yellow fast-blink half-period |
| `PROMPT_BLINK_MS` | 500 | PROMPT green slow-blink half-period |
| `ERROR_BLINK_MS` | 200 | ERROR_SD red fast-blink half-period |
| `REJECT_FLASH_MS` | 400 | Brief red flash for rejected button press (e.g. analysis with no LOG files) |
| `DEBUG_OFS` | 1 | Set to 0 to disable all Serial output |

`SAMPLE_PERIOD_US` cannot go below ~14 000 µs without changing the TSL2561 driver — the 13 ms integration window plus driver overhead is the hard floor. `BUFFER_SIZE_SAMPLES` can be raised toward ~4000 on a 32 KB SAMD21 before the stack and library state become tight.

Length is computed as `length_k = (t_rise - t_fall) [ms] × Ud [µm/ms = mm/s]`, so `Ud` is the only experiment-specific value that needs to be fixed before flashing.

## Dependencies

Declared in `platformio.ini`, pulled automatically:

- `arduino-libraries/SD ^1.2.4`
- `adafruit/Adafruit Unified Sensor ^1.1.13`
- `adafruit/Adafruit TSL2561 ^1.1.0`

## Hardware

A minimal build requires:

- Arduino Zero (or compatible SAMD21G18A board)
- Adafruit TSL2561 breakout (or equivalent) on I²C
- microSD card module on hardware SPI, chip-select on D4
- 4 LEDs (green, red, yellow, blue) with current-limiting resistors
- 3 push buttons (B1, B2, B3) wired to ground

Full bill of materials, schematic, and assembly are in the *HardwareX* manuscript.

## License

CERN Open Hardware Licence Version 2 — Strongly Reciprocal (CERN-OHL-S v2).
See [`LICENSE-hardware`](../../LICENSE-hardware) at the repository root.

## Citation

If you use this firmware, please cite:

> [manuscript citation TBD]

## Maintainer

[Author / maintainer block TBD — e.g., name, affiliation, contact.]
