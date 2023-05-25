#ifndef LIS3MDL_H
#define LIS3MDL_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
void process_lis3mdl_sample(const struct device *dev, 
							struct sensor_value *magn_x,
                            struct sensor_value *magn_y,
                            struct sensor_value *magn_z);
void lis3mdl_initialise(const struct device *dev);
#endif