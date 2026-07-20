# Firmware architecture

The firmware is a small bare-metal super-loop around interrupt-driven bus
drivers and TinyUSB.

## Layers

- `src/board/`: clock tree, pin mux, low-level STM32H723 setup.
- `src/drivers/`: bus and board peripherals.
- `src/j2534/`: PassThru channel model, filters, ISO-TP and wire helpers.
- `src/elm327/`: CDC ELM327 command surface.
- `src/transport/`: framed host protocol used by the DLL and tools.
- `src/usb/`: TinyUSB transport, descriptors and mode selector.

## USB identities

OmniBox exposes one USB identity per enumeration:

| id | mode | content |
|----|------|---------|
| 0 | `USB_MODE_J2534` | J2534 vendor WinUSB + ELM327 CDC |
| 1 | `USB_MODE_SLOT1` | empty vendor slot |
| 2 | `USB_MODE_SLOT2` | empty vendor slot |
| 3 | `USB_MODE_SLOT3` | empty vendor slot |

The mode selector stores the requested mode in `.noinit` RAM and performs a
delayed software reset after acknowledging the host command. A cold boot returns
to `USB_MODE_J2534`.
