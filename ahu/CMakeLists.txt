# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# set(DTC_OVERLAY_FILE usb.overlay)
set(BOARD thingy52_nrf52832)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(prac1)

FILE(GLOB app_sources src/*.c)

set(repo ${CMAKE_CURRENT_SOURCE_DIR}/../)
set(oslib ${repo}/oslib/)
set(ahuhci ${oslib}/ahu_drivers/ahu_hci/)
set(sensor ${oslib}/scu_drivers/scu_sensors/)


target_include_directories(app PRIVATE ${prac1} ${oslib} ${ahuhci} ${sensor})

include_directories(${oslib} ${ahushell} ${ahuhci})

target_sources(app PRIVATE ${sensor}/t52_mpu6050.c)
target_sources(app PRIVATE ${ahuhci}/bt.c)
target_sources(app PRIVATE ${app_sources})
target_sources(app PRIVATE src/main.c) #necessary
