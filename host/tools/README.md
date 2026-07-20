# j2534mode

`j2534mode` reads or changes the OmniBox USB identity.

OmniBox exposes one USB identity per enumeration. The public firmware includes
the J2534+ELM327 mode and three empty vendor slots reserved for downstream
experiments.

## Modes

| id | name | role |
|----|------|------|
| 0 | `j2534` | J2534 WinUSB transport + ELM327 CDC |
| 1 | `slot1` | empty vendor slot |
| 2 | `slot2` | empty vendor slot |
| 3 | `slot3` | empty vendor slot |

## Build

```bash
make
make windows
```

## Usage

```bash
j2534mode get
j2534mode set slot1
j2534mode set 0
```

Transport is selected like the DLL:

- `J2534_TCP=host:port` for the virtual device.
- WinUSB on Windows when `J2534_TCP` is not set.
