# Operation Manual

This manual covers day-to-day use of the optical flow sensor in both modes: capturing recordings and analysing them on-device. For build instructions and architectural details, see [`README.md`](./README.md). For the bill of materials and assembly, see the *HardwareX* manuscript.

## What this device does

A single press from STANDBY chooses one of two modes:

- **Capture mode (B1)** — samples ambient light intensity from the TSL2561 at ~58 Hz and stores each recording session as a self-contained CSV file on the microSD card. A single burst captures up to ~51 s of data into RAM with deterministic timing, then writes it out to the next available `LOG_NNN.CSV` file. Multiple bursts can be captured back-to-back.
- **Analysis mode (B3)** — walks every existing `LOG_*.CSV` on the card in numerical order, runs the droplet characterization pipeline on each file, and prints a cumulative summary (file count, droplet count, mean length, standard deviation, CV%) over Serial. Returns to STANDBY automatically when done.

## Hardware overview

Pin assignments are fixed in firmware (`src/Config.h`) and must match how your board is wired:

| Function | Pin | Notes |
|---|---|---|
| Capture button (B1) | D2 | Active-low, INPUT_PULLUP |
| End-session button (B2) | D3 | Active-low, INPUT_PULLUP |
| Analysis button (B3) | D5 | Active-low, INPUT_PULLUP |
| Green LED | D8 | Standby / capturing |
| Red LED | D9 | Error / rejected press |
| Yellow LED | D12 | Init / offloading / prompt / processing |
| Blue LED | A5 | Capturing / processing |
| SD chip-select | D4 | |
| TSL2561 SDA / SCL | board defaults | I²C |
| SD MOSI / MISO / SCK | board defaults | hardware SPI (fixed on Zero) |

## Preparing the SD card

1. Format the card as **FAT32**.
   - macOS: Disk Utility → Erase → MS-DOS (FAT).
   - Windows: SD Memory Card Formatter (recommended for cards larger than 32 GB).
2. Confirm the card mounts cleanly. Existing `LOG_NNN.CSV` files from previous sessions are fine — the firmware skips occupied filenames during Capture and walks every existing one during Analysis.
3. Insert the card into the SD module **before powering the device**.

The firmware refuses to leave INIT (red fast-blink) if no card is detected.

## LED reference

| LED state | Meaning |
|---|---|
| Yellow steady | Initialising — testing TSL2561 and SD card |
| Green steady | **Standby** — ready (B1 = Capture, B3 = Analysis) |
| Green steady + Blue steady | **Capturing** — RAM buffer filling, do not remove power |
| Yellow fast-blink (~10 Hz) | **Offloading** — writing buffer to SD, do not remove power or card |
| Yellow steady + Green slow-blink (~1 Hz) | **Prompt** — choose: B1 for next file, B2 to end session |
| Yellow steady + Blue steady | **Processing** — analysing LOG files |
| Brief red flash (~400 ms) | Pressed button valid for some mode but unavailable now (e.g. B3 with no LOG files on card) |
| Red steady | TSL2561 not detected at boot |
| Red fast-blink (~5 Hz) | SD card error |

## Button reference

| Button | Active state | Action |
|---|---|---|
| B1 | Standby | Enter Capture mode (start a recording burst) |
| B1 | Capturing | Stop the current burst early and offload |
| B1 | Prompt | Start the next burst (new file) |
| B2 | Prompt | End the session and return to Standby |
| B3 | Standby | Enter Analysis mode (process every `LOG_*.CSV`) |

Button presses outside the active states listed above are ignored.

## Recording an experiment — step by step

### 1. Power on

Connect the device via USB or its dedicated power input. Watch the LEDs:

- The yellow LED comes on briefly (initialisation, < 1 s).
- The green LED comes on steadily — you are now in **Standby**.

If a red LED comes on instead, see *Troubleshooting* below before proceeding.

### 2. Start a recording

Press **B1**. The blue LED comes on (the green LED stays on). The device is now capturing data into RAM at ~58 Hz. Avoid pressing other buttons during capture unless you want to stop early.

### 3. Wait, or stop early

Capture continues until either of:

- You press **B1** again (manual stop), or
- The 51-second buffer fills (automatic stop).

In either case, capture ends and offload begins automatically.

### 4. Wait for the offload

The blue and green LEDs go off; the yellow LED begins fast-blinking. The firmware is writing the captured data to the next free `LOG_NNN.CSV` file on the SD card. **Do not remove power or the SD card during this phase.** Offload typically takes 1–3 s on a healthy card.

### 5. Decide what to do next

When offload finishes you enter **Prompt**: yellow LED steady, green LED slow-blinking.

- **Press B1** to start another recording immediately. The next file is auto-numbered (e.g., `LOG_002.CSV` after `LOG_001.CSV`).
- **Press B2** to end the session. The device returns to Standby (green LED steady), at which point it is safe to remove the SD card or power-cycle.

### 6. Repeat as needed

A single session can capture as many files as the SD card has space for, up to file 999. There is no hard limit on the number of sessions per card other than the 999 file-slot ceiling.

## Running an analysis pass — step by step

### 1. From STANDBY, press B3

The green LED switches off; yellow + blue come on (Processing). The firmware walks every `LOG_*.CSV` on the card in numerical order, fold-merging the per-droplet lengths from each file into a single Welford accumulator.

If the card has no `LOG_*.CSV` files, the red LED briefly flashes (~400 ms) and the device stays in Standby — there is nothing to analyse.

### 2. Watch the Serial monitor

At 115 200 baud you'll see one compact line per file as it completes:

```
LOG_001.CSV : 17 droplets (3000 samples)
LOG_002.CSV : 16 droplets (3000 samples, 1 undersampled)
```

The `undersampled` annotation means at least one peak-to-trough transition was resolved by fewer than 2 samples in that file — flag for manual review but the droplet count is still computed.

### 3. Read the cumulative summary

After the last file, the firmware prints:

```
=== Cumulative results ===
Files processed: 2
Total droplets : 33
Avg length     : 612.34 um
Std dev        : 11.20 um
CV             : 1.83 %
```

The yellow + blue LEDs go off and the green LED returns to steady — back in **Standby**, ready for either mode again. The analysis pass does not modify the SD card.

### 4. Re-run if needed

Pressing **B3** again re-runs the same pass with a fresh Welford accumulator (the previous summary remains in the Serial scrollback). Useful after editing/replacing files on the card.

## State diagram

```
                 ┌─────┐
                 │INIT │ Y on
                 └──┬──┘
                    │ TSL2561 + SD OK
                    ▼
       ┌───────► STANDBY  ◄──────────────────────────┐
       │         G on                                │
       │       ┌──┴──┐                               │
       │      B1     B3                              │
       │       │      │                              │
       │       ▼      ▼                              │
       │   CAPTURING  PROCESSING                     │
       │   G + B on   Y + B on                       │
       │       │      │                              │
       │   B1 / full  walks LOG_*.CSV                │
       │       ▼      │ prints cumulative summary    │
       │   OFFLOADING └──────────────────────────────┘
       │   Y fast-blink
       │       │
       │       ▼
       │     PROMPT  Y on, G slow-blink
       │     ├── B1 → CAPTURING (next file)
       │     └── B2 → STANDBY
       └───────────
```

## Recovering data from the card

1. End the capture session: in Prompt state, press **B2** so the green LED goes steady.
2. Remove power.
3. Eject the microSD card and read it on a computer.
4. Copy the `LOG_NNN.CSV` files to your analysis directory.

Each file is plain CSV, no header, format `<millis_as_float>, <lux>` per row:

```
12345.00, 234.56
12362.00, 234.78
12379.00, 234.55
```

Timestamps are milliseconds since the Arduino was powered on (boot-relative). Within a single file, samples are uniformly spaced 17 ms apart with sub-millisecond jitter; the act of writing to SD did **not** influence the spacing because no SD activity occurred during capture.

## Specifications

| Parameter | Value |
|---|---|
| Sample rate | ~58 Hz (one sample every 17 ms) |
| TSL2561 gain | 1× |
| TSL2561 integration time | 13 ms (fastest setting) |
| Maximum file duration | ~51 s (3000 samples × 17 ms) |
| Maximum files per card | 999 (`LOG_001` … `LOG_999`) |
| File format | CSV, no header, OFS-compatible |
| Timestamp resolution | 1 ms (since power-on) |
| Timestamp format | float, 2 decimal places |
| Lux format | float, 2 decimal places |
| Per-burst SRAM use | 24 KB (shared with analysis buffer) |
| Analysis pass time | < 1 s per LOG file on a healthy card |

## Troubleshooting

**Red LED steady at boot.**
The TSL2561 is not responding on I²C. Check SDA/SCL wiring, sensor power, and confirm the address strap (default `TSL2561_ADDR_FLOAT`) matches your breakout board. Power-cycle to retry.

**Red LED fast-blink at boot.**
SD card not detected or unreadable. Confirm the card is inserted, formatted as FAT32, and that chip-select (D4) is wired correctly. Power-cycle to retry.

**Red LED fast-blink during operation.**
Could not open a new `LOG_NNN.CSV` file during offload. Either all 999 slots are taken, or the card is full. Move some old files off the card (or use a fresh card) and power-cycle.

**Brief red flash when pressing B3.**
There are no `LOG_*.CSV` files on the card to analyse. Run a Capture pass first, or insert a card that already has logs.

**Offload phase takes much longer than expected.**
Consumer SD cards occasionally pause for several seconds to perform internal garbage collection. The yellow LED continues fast-blinking throughout — this is normal. If a card consistently exceeds 10 s for a 24 KB write, replace it with a known-good card.

**File appears truncated after a power loss.**
Mid-offload power loss leaves a partial file containing every row written before the last `flush()` (every 100 rows by default). The remainder of the buffer is lost. To minimise risk, ensure the device is powered until the yellow LED returns to steady before disconnecting.

**Analysis prints `LOG_NNN.CSV : read failed`.**
The file could not be opened (corrupt entry, encrypted card, or transient SD glitch). The pass continues with subsequent files; the failed file is counted under `Files failed:` in the cumulative summary.

**Some files report `undersampled`.**
At least one peak-to-trough transition in that file was resolved by fewer than 2 samples — typically a sign that flow rate or droplet frequency was high relative to the 58 Hz sample rate. The droplet count is still computed but the per-file length estimates may be biased; treat the affected file as a soft warning.

**The Serial monitor shows nothing.**
Confirm the monitor is at 115 200 baud. Note that `DEBUG_OFS` is enabled by default; set it to `0` in `src/Config.h` for a silent release build.

**Buffer fills sooner than ~51 s in Capture.**
Confirm `BUFFER_SIZE_SAMPLES` and `SAMPLE_PERIOD_US` in `src/Config.h` have not been altered. At defaults (3000 samples × 17 000 µs), the burst lasts 51.0 s.

## Recompiling the firmware

See [`README.md`](./README.md) for the build process. The compile-time constants in `src/Config.h` can be tuned without touching any other file:

- Edit **`UD_MM_PER_S`** to match your experiment's droplet velocity (mm/s). This is the only value that has to change between experiments; lengths reported in Analysis mode scale linearly with it.
- Increase **`BUFFER_SIZE_SAMPLES`** to extend per-file duration. Up to ~4000 is safe on the 32 KB SAMD21; beyond that the stack and library state become tight.
- Decrease **`SAMPLE_PERIOD_US`** to push the sample rate. Below ~14 000 µs the TSL2561 driver cannot keep up — the 13 ms integration window is a hard floor.
- Set **`DEBUG_OFS = 0`** for a release build with no Serial output.
