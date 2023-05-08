#ifndef MPU6050_H
#define MPU6050_H

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

void init_mpu6050(const struct device *mpu6050);
int process_mpu6050(const struct device *dev);

#endif