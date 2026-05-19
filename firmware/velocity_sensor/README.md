# Velocity Sensor — Firmware

Arduino Zero firmware for the dual-comparator optical droplet velocity sensor.
Companion firmware to the *HardwareX* manuscript.

## What this firmware does

Measures droplet velocity in a microfluidic channel by timing the transit between
two optical comparator sensors (COMP_A and COMP_B) separated by a known distance.
A SCADE-generated state machine processes the interrupt events and outputs velocity
in mm/s over Serial at 38 400 baud.

## Hardware requirements

- Arduino Zero (Atmel SAMD21G18A, Cortex-M0+)
- Two optical comparator outputs connected to D7 (COMP_A) and D6 (COMP_B)
- Two digital potentiometers (I²C address `0x2F`) for threshold adjustment:
  - DigiPot 1 on the default I²C bus (SDA/SCL)
  - DigiPot 2 on a second I²C bus (SDA = D11, SCL = D13)
- Status LEDs: green (D8), red (D9), yellow (D12), blue (A5), velocity LED (D10), OFS LED (A4)
- Buttons: B1 (D2), B2 (D3), B3 (D5) — active-low with internal pull-ups

Full bill of materials, schematic, and assembly are in the *HardwareX* manuscript.

## Quick start

Requires [PlatformIO](https://platformio.org/).

```sh
cd firmware/velocity_sensor
pio run                # build
pio run -t upload      # build + flash to Arduino Zero
pio device monitor     # open Serial at 38400 baud
```

## Configuration

Compile-time constants in `src/main.cpp`:

| Constant | Default | Meaning |
|---|---|---|
| `WIPER1` | 76 | DigiPot 1 wiper — sets COMP_B (Sensor 2) threshold |
| `WIPER2` | 83 | DigiPot 2 wiper — sets COMP_A (Sensor 1) threshold |
| `n` | 2 | Number of sensors active in the SCADE state machine |
| `DEBOUNCE_TIME_US` | 1000 | ISR debounce window (µs) |
| `MIN_INTER_S1_US` | 3000 | Minimum inter-event gap for Sensor 1 (µs) |
| `MIN_INTER_S2_US` | 3000 | Minimum inter-event gap for Sensor 2 (µs) |

The wiper values control the comparator thresholds and must be tuned to the signal
amplitude from your specific optical sensor assembly.

## Serial output

At 38 400 baud, the firmware prints one velocity value per detected droplet:

```
>>> Optical Droplet Velocity Sensor [OFS 3.0] <<<
Digital Potentiometer 1: 76
Digital Potentiometer 2: 83
n: 2
=== Velocity Measurements with ISR Debug ===
...
12.34
13.01
12.89
```

Each floating-point number is a droplet velocity in mm/s. Only values that differ
from the previous measurement and are > 0 are printed.

## Dependencies

Declared in `platformio.ini`; no external libraries are required beyond the
Arduino framework (pulled automatically by PlatformIO).

The SCADE/KCG state-machine code in `lib/KCG/` is a local library bundled with
the project and generated from the model file `VelSensor.cgcs`.

## License

CERN Open Hardware Licence Version 2 — Strongly Reciprocal (CERN-OHL-S v2).
See [`LICENSE-hardware`](../../LICENSE-hardware) at the repository root.

## Citation

If you use this firmware, please cite:

> [manuscript citation TBD — to be added after HardwareX acceptance]

## Maintainer

[Author block TBD — to be added before publication]
