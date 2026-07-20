# OmniBox firmware for STM32H723ZGTx.
# Build: cmake -B build -G Ninja
#  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake && cmake --build build
#
# Dependencies are fetched by CMake FetchContent on the first configure:
# CMSIS core, STM32H7 CMSIS device, STM32H7 HAL and TinyUSB.
cmake_minimum_required(VERSION 3.20)
project(j2534_fw C ASM)

set(CMAKE_C_STANDARD 11)
set(MCU_DEFINES STM32H723xx USE_HAL_DRIVER)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
# Pinned dependency versions. Existing extern/<dep> directories are reused.
FetchContent_Declare(cmsis_core  GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis-core.git  GIT_TAG v5.4.0_cm7 SOURCE_DIR ${CMAKE_SOURCE_DIR}/extern/cmsis_core)
FetchContent_Declare(cmsis_device GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_h7.git GIT_TAG v1.10.4  SOURCE_DIR ${CMAKE_SOURCE_DIR}/extern/cmsis_device_h7)
FetchContent_Declare(stm32h7_hal  GIT_REPOSITORY https://github.com/STMicroelectronics/stm32h7xx_hal_driver.git GIT_TAG v1.11.4 SOURCE_DIR ${CMAKE_SOURCE_DIR}/extern/stm32h7xx_hal_driver)
FetchContent_Declare(tinyusb    GIT_REPOSITORY https://github.com/hathach/tinyusb.git GIT_TAG 0.16.0 SOURCE_DIR ${CMAKE_SOURCE_DIR}/extern/tinyusb)
FetchContent_MakeAvailable(cmsis_core cmsis_device stm32h7_hal tinyusb)

# HAL sources used by the firmware.
set(HAL ${stm32h7_hal_SOURCE_DIR}/Src/stm32h7xx_hal)
set(HAL_SRC
  ${HAL}.c ${HAL}_rcc.c ${HAL}_rcc_ex.c ${HAL}_gpio.c ${HAL}_cortex.c
  ${HAL}_flash.c ${HAL}_flash_ex.c
  ${HAL}_pwr.c ${HAL}_pwr_ex.c ${HAL}_fdcan.c ${HAL}_spi.c ${HAL}_i2c.c
  ${HAL}_uart.c ${HAL}_uart_ex.c ${HAL}_tim.c ${HAL}_tim_ex.c
  ${HAL}_dma.c ${HAL}_adc.c ${HAL}_adc_ex.c ${HAL}_pcd.c ${HAL}_pcd_ex.c
  ${stm32h7_hal_SOURCE_DIR}/Src/stm32h7xx_ll_usb.c)

# CMSIS device startup and system files.
set(STARTUP ${cmsis_device_SOURCE_DIR}/Source/Templates/gcc/startup_stm32h723xx.s)
set(SYSTEM ${cmsis_device_SOURCE_DIR}/Source/Templates/system_stm32h7xx.c)

# TinyUSB STM32 Synopsys DWC2 port.
set(TUSB ${tinyusb_SOURCE_DIR}/src)
file(GLOB TUSB_SRC
  ${TUSB}/tusb.c ${TUSB}/common/*.c ${TUSB}/device/*.c
  ${TUSB}/class/vendor/*.c ${TUSB}/class/cdc/*.c ${TUSB}/class/hid/*.c
  ${TUSB}/portable/synopsys/dwc2/*.c)

# Application sources.
file(GLOB_RECURSE APP_SRC ${CMAKE_SOURCE_DIR}/src/*.c)

add_executable(firmware.elf ${APP_SRC} ${HAL_SRC} ${STARTUP} ${SYSTEM} ${TUSB_SRC})

target_compile_definitions(firmware.elf PRIVATE ${MCU_DEFINES} CFG_TUSB_MCU=OPT_MCU_STM32H7)
target_include_directories(firmware.elf PRIVATE
  ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/board ${CMAKE_SOURCE_DIR}/src/usb
  ${cmsis_core_SOURCE_DIR}/Include
  ${cmsis_device_SOURCE_DIR}/Include
  ${stm32h7_hal_SOURCE_DIR}/Inc
  ${TUSB})

target_link_options(firmware.elf PRIVATE
  -T${CMAKE_SOURCE_DIR}/linker/STM32H723ZGTx.ld
  -Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map,--cref -Wl,--print-memory-usage)

# Generate .bin, .hex and size output after linking.
add_custom_command(TARGET firmware.elf POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary firmware.elf firmware.bin
  COMMAND ${CMAKE_OBJCOPY} -O ihex  firmware.elf firmware.hex
  COMMAND ${CMAKE_SIZE} firmware.elf)
