#ifndef MPU6050_H
#define MPU6050_H

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

void init_mpu6050(const struct device *mpu6050);
int process_mpu6050(const struct device *dev, struct sensor_value *accel_x, 
                    struct sensor_value *accel_y, struct sensor_value *accel_z, 
                    struct sensor_value *gyro_x, struct sensor_value *gyro_y, 
                    struct sensor_value *gyro_z);

#endif