# OmniBox firmware

Bare-metal STM32H723 firmware for the OmniBox V1 board.

Version: 0.1 dev.

Public features:

- J2534 vendor bulk transport over WinUSB.
- ELM327-compatible CDC serial interface.
- USB identity selector with three empty public vendor slots.
- Board drivers for CAN/CAN-FD, K-line, J1850/SWCAN, FEPS and the routing matrix.

The empty slots are intentionally protocol-free. They are placeholders for
non-commercial experiments without shipping any private identity implementation.

## Build

Prerequisites: `arm-none-eabi-gcc`, CMake and Ninja.

```bash
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake
cmake --build build
```

The first configure downloads CMSIS, STM32H7 HAL and TinyUSB through CMake
FetchContent. For offline builds, pre-populate `extern/` with the pinned
dependencies and configure with `-DFETCHCONTENT_FULLY_DISCONNECTED=ON`.
