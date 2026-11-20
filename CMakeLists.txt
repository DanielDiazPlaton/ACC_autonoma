# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/boards/frdm_mcxw71.overlay")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(blinky)

target_include_directories(app PRIVATE
    RTE
    BSW/MCAL
    BSW/MOTOR
)

target_sources(app PRIVATE
    src/main.c
    RTE/Rte_OS.c
    BSW/MCAL/gpio_mcal.c
    BSW/MOTOR/motor.c
)