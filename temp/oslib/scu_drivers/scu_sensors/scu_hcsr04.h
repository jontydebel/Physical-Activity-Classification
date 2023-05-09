#ifndef HCSR04_H
#define HCSR04_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>

uint32_t process_hcsr04_sample(void);
void hcsr04_initialise(void);

#endif