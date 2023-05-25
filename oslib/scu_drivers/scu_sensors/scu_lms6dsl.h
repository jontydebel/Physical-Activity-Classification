#ifndef LMS6DSL_H
#define LMS6DSL_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
void get_lms6dsl_samples(void);
void process_lms6dsl_sample(const struct device *dev, 
								struct sensor_value *accel_x, struct sensor_value *accel_y,
								struct sensor_value *accel_z, struct sensor_value *gyro_x, 
								struct sensor_value *gyro_y, struct sensor_value *gyro_z);
void lsm6dsl_initialise(const struct device *dev);
#endif