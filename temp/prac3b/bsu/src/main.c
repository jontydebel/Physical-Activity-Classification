/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/gpio.h>
#include "bsu_shell_cmds.h"
// #include "bsu_hci.h"
#include "bsu_bt.h"

#define BSU_SHELL_THREAD_PRIORITY 1
#define BSU_SHELL_THREAD_STACK 1024
#define BSU_HCI_THREAD_PRIORITY 2
#define BSU_HCI_THREAD_STACK 1024
#define BSU_BT_THREAD_PRIORITY 3
#define BSU_BT_THREAD_STACK 4096

//Define both threads needed for Prac 3 - Shell and Bluetooth
K_THREAD_DEFINE(bsu_shell_thread_tid, BSU_SHELL_THREAD_STACK, bsu_shell_thread, NULL, NULL, NULL, BSU_SHELL_THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(bsu_bt_thread_tid, BSU_BT_THREAD_STACK, bsu_bt_thread, NULL, NULL, NULL, BSU_BT_THREAD_PRIORITY, 0, 0);