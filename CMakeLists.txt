# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky)

target_include_directories(app PRIVATE
    RTE
    )

target_sources(app PRIVATE 
    src/main.c
    RTE/Rte_OS.c
    )
