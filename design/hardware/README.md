# hardware

PCB design files for the sensor electronics. Licensed under CERN-OHL-S v2.

Each subdirectory corresponds to one board and contains:

- `GerberFiles/` — Gerber layers (copper, silkscreen, soldermask, solderpaste, profile) and a `.gbrjob` job file.
- `DrillFiles/` — drill file (`.xln`).

## Boards

| Folder | Description |
| --- | --- |
| [`arduino_shield/`](arduino_shield/) | Arduino shield that interfaces the sensor boards with the microcontroller. |
| [`leds_board/`](leds_board/) | LED driver board for optical excitation. |
| [`velocity_sensor/`](velocity_sensor/) | PCB for the droplet velocity sensor. |
