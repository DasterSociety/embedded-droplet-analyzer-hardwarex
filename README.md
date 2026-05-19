# Modular On-Chip Droplet Sensor System

Open-source hardware companion to a manuscript submitted to _HardwareX_.
The project describes a modular, on-chip sensor system for flow and
monodispersity analysis in droplet-based microfluidics.

> Zenodo DOI: _to be added once the first release is archived._
>
> Manuscript citation: _to be added after acceptance._

## Repository contents

| Folder                                     | Contents                                                        | License       |
| ------------------------------------------ | --------------------------------------------------------------- | ------------- |
| [`design/hardware/`](design/hardware/)     | PCB design files (Gerbers, drill files) for each sensor board.  | CERN-OHL-S v2 |
| [`design/mechanical/`](design/mechanical/) | 3D design exports (STEP, STL) for chip, housing, and sensor bodies. | CERN-OHL-S v2 |
| [`firmware/`](firmware/)                   | Microcontroller firmware and data-acquisition code.             | CERN-OHL-S v2 |

## Licensing

All design files and firmware in this repository are licensed under the
[CERN Open Hardware Licence Version 2 — Strongly Reciprocal
(CERN-OHL-S v2)](LICENSE-hardware).

Each source file carries an SPDX identifier:

```
SPDX-License-Identifier: CERN-OHL-S-2.0
```

## How to cite

_Citation block to be added once the Zenodo DOI has been minted and the
HardwareX article has been assigned a DOI._

## Reproducing the hardware

Full build and operation instructions are in the companion *HardwareX* manuscript,
sections 5 (Build instructions) and 6 (Operation instructions). The Design files
summary (§3) and Bill of materials (§4) in the manuscript cross-reference the exact
files in this repository.
