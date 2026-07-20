# OmniBox Board 1.0 Beta - Open Vehicle Interface for J2534 / ELM327 Development

Hello everyone,

I would like to present **OmniBox**, an open/public non-commercial vehicle
interface project intended for diagnostic, research, and firmware development
work.

The current public release is:

- **Board:** 1.0 beta
- **Firmware:** 0.1 dev

The goal of this first public version is not to claim a finished commercial
product, but to provide a serious hardware base that people can review, build,
test, and improve.

## Project Scope

OmniBox 1.0 beta is public on the hardware side:

- KiCad project files
- Schematics
- PCB layout
- BOM
- Gerbers / drill files
- Enclosure files

The public software scope for firmware 0.1 dev is intentionally limited to:

- J2534 development
- ELM327-compatible mode

No proprietary diagnostic software, paid tool ecosystem, or closed commercial
target is part of the public release.

## Hardware Overview

The board is built around an **STM32H7-class MCU**, chosen for timing margin,
USB handling, protocol work, and future expansion.

Main hardware points:

- STM32H7-class main controller
- Galvanic isolation between host and vehicle domains
- Multiple vehicle-side communication interfaces
- CAN / CAN-FD-oriented hardware
- K-Line / L-Line style interfaces
- Single-wire CAN style support
- Protected vehicle power input
- SM8S24A load-dump TVS protection
- Routing matrix for adapting channels to vehicle pins
- Proper enclosure design
- KiCad source, BOM, and fabrication files available

The intent is to avoid the usual "dev board plus wires" approach and provide a
cleaner platform for real diagnostic-interface development.

## Design Choices and Limitations

Some choices were deliberate:

- The hardware is more capable than the initial public firmware.
- Isolation and input protection were prioritized because vehicle environments
  are harsh.
- The firmware keeps a multi-identity / device-profile architecture internally.
- The public release does not ship compatibility logic for proprietary tools or
  specific commercial ecosystems.

The architecture leaves room for advanced contributors to experiment with
alternative device behaviors or compatibility profiles, but the public project
stays focused on generic interfaces such as J2534 and ELM327.

This should be considered a development platform, not a guaranteed replacement
for any existing professional interface.

## Firmware Status / Work Remaining

Firmware 0.1 dev is still early. Areas needing work or validation include:

- J2534 API behavior and edge cases
- CAN message filtering and timing behavior
- ISO-TP handling
- K-Line initialization and protocol timing
- ELM327 command coverage and compatibility
- USB transport stability
- Firmware update process
- Host DLL behavior and test tools
- Logging and debug tooling
- Long-duration stability testing
- Bench ECU and real vehicle validation

The hardware has flexibility beyond the initial firmware release, but the
project needs real testing before strong compatibility claims can be made.

## Contributors Wanted

I am looking for people interested in helping with:

- Board design review
- Protection / isolation review
- Firmware development, especially J2534
- Host DLL and PC-side tools
- Test utilities
- Bench ECU and vehicle testing
- Documentation
- Manufacturing feedback
- Enclosure and mechanical improvements

Feedback from people experienced with automotive diagnostic interfaces, J2534
behavior, CAN/K-Line timing, or robust vehicle-side hardware would be especially
valuable.

## Disclaimer

OmniBox is intended for diagnostic, repair, research, education, and
non-commercial development use.

The project is public so that the hardware and basic diagnostic firmware can be
reviewed, tested, and improved by the community.
