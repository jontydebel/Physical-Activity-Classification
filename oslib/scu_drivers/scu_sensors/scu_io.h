#ifndef IO_H
#define IO_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins);
int toggle_io(const struct gpio_dt_spec dev);
int get_io_state(const struct gpio_dt_spec dev);
void io_initialise(const struct gpio_dt_spec led0, const struct gpio_dt_spec led1, const struct gpio_dt_spec button);

#endif