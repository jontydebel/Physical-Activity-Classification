#ifndef LPS22HB_H
#define LPS22HB_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

void process_lps22hb_sample(const struct device *dev, 
                            struct sensor_value *pressure, struct sensor_value *temp);
void lps22hb_initialise(const struct device *dev);
void io_initialise(const struct gpio_dt_spec led0, const struct gpio_dt_spec led1, 
                    const struct gpio_dt_spec button);

#endif