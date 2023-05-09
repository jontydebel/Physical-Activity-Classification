# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(BOARD nrf52840dongle_nrf52840)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(p2)

FILE(GLOB app_sources src/*.c)

set(repo ../..)
set(oslib ${repo}/oslib/)
set(bsushell ${oslib}/bsu_drivers/bsu_shell/)
set(bsubt ${oslib}/bsu_drivers/bsu_bluetooth/)

target_include_directories(app PRIVATE ${p2} ${oslib} ${bsushell} ${bsubt})

include_directories(${oslib} ${bsushell} ${bsuhci} ${bsubt})

target_sources(app PRIVATE ${bsushell}/bsu_shell_cmds.c)
target_sources(app PRIVATE ${bsubt}/bsu_bt.c)
target_sources(app PRIVATE ${app_sources})
target_sources(app PRIVATE src/main.c) #necessary