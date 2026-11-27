# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/boards/frdm_mcxw71.overlay")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(blinky)

target_include_directories(app PRIVATE
    ASW
    ASW/DIAG
    RTE
    BSW/Comm
    BSW/MCAL
    BSW/MOTOR
    BSW/Direccion
    BSW/NVM
)

target_sources(app PRIVATE
    src/main.c
    ASW/controles.c
    RTE/Rte_OS.c
    RTE/Acc_Signals.c
    BSW/Comm/Can_mis.c
    BSW/DCM/Dcm.c
    BSW/MCAL/gpio_mcal.c
    BSW/MOTOR/motor.c
    BSW/MCAL/pwm_mcal.c
    BSW/Direccion/Direccion.c
    BSW/Comm/CanIf.c
    BSW/NVM/Nvm_Manager.c
)
