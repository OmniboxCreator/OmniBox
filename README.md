# OmniBox

OmniBox is a USB-to-OBD open-hardware automotive diagnostic interface, released
free of charge for non-commercial use.

This repository contains the V1 board release: KiCad sources, schematic, PCB,
BOM, Gerbers, manufacturing files and enclosure files. The public software
exposes J2534 and ELM327. The USB stack keeps a multi-identity architecture with
three empty slots reserved for experiments.

## Release Status

| Item | Version | Status |
|---|---:|---|
| Board | 1.0 beta | Public V1 hardware release |
| Firmware | 0.1 dev | J2534 + ELM327 development firmware |

## Hardware Capabilities

| Area | V1 capability |
|---|---|
| Host | Isolated USB Full-Speed, WinUSB J2534 interface and ELM327 CDC serial |
| Controller | STM32H723 Cortex-M7 with headroom for real-time protocol handling |
| CAN / CAN-FD | Five routable CAN channels, including native FDCAN and SPI CAN-FD controllers |
| K-line | Multiple ISO 9141-2 / ISO 14230 lines routable through the matrix |
| SWCAN / J1850 | Hardware stages for Single-Wire CAN, J1850 VPW and J1850 PWM |
| FEPS | Programmable voltage generation up to 24 V with measurement feedback |
| Measurements | VBATT reading and supervision of critical lines |
| Routing | Switching matrix for adapting bus channels to OBD/J1962 pins |
| Protection | Vehicle input protection, TVS/ESD, current limiting and safe line idle states |
| Isolation | Galvanic separation between host USB and vehicle domains |

## Hardware Choices

OmniBox V1 uses standard, documented parts: STM32H7 for compute headroom,
TinyUSB in firmware, dedicated automotive transceivers, analog switching for
routing, isolated power and protection stages sized for a vehicle environment.

The design goals are:

- manufacturable PCB files for common board services;
- a KiCad project that is easy to inspect and modify;
- clear isolation between PC and vehicle domains;
- an extensible J2534/ELM327 base without proprietary software dependencies;
- production files that can be used to assemble a V1 board.

## Repository Layout

```text
hardware/omnibox-v1/    Board 1.0 beta KiCad sources, BOM, Gerbers and enclosure files
firmware/         Firmware 0.1 dev: J2534 + ELM327 + empty slots
host/           J2534 DLL, virtual device and PC tools
docs/           Additional public notes
LICENSE.md         Free non-commercial license
SUPPORT.md         Donations, sponsoring and commercial licensing
```

## License and Support

OmniBox is released under a dual-license model. The public repository is free
for non-commercial use. Commercial use requires separate written permission.

See `LICENSE.md` for license terms and `SUPPORT.md` for donations, sponsoring
and commercial licensing.

## Manufacturing

Main manufacturing files:

- `hardware/omnibox-v1/omnibox-v1.kicad_pro`
- `hardware/omnibox-v1/omnibox-v1.kicad_sch`
- `hardware/omnibox-v1/omnibox-v1.kicad_pcb`
- `hardware/omnibox-v1/production/bom-consolide.csv`
- `hardware/omnibox-v1/production/local-finish/gerber/`
- `hardware/omnibox-v1/enclosure/`

Review the schematic, routing, BOM and fabrication rules with your assembler
before ordering boards.

## Warning

OmniBox switches OBD lines and can generate programming voltages. Assembly
errors, firmware defects or misuse can damage a vehicle or ECU. Use it only on
hardware you own or are explicitly authorized to test.
