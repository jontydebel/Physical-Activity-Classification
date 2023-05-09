/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <scu_lis3mdl.h>

/*
* Retrieve the magnetometer readings from the LIS3MDL
*/
void process_lis3mdl_sample(const struct device *dev, 
							struct sensor_value *magn_x,
                            struct sensor_value *magn_y,
                            struct sensor_value *magn_z)
{
    if (sensor_sample_fetch(dev) < 0) {
		printf("Sensor sample update error\n");
		return;
	}

    if (sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, magn_x) < 0) {
		printf("Cannot read LIS3MDL pressure channel\n");
		return;
	}

    if (sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, magn_y) < 0) {
		printf("Cannot read LIS3MDL pressure channel\n");
		return;
	}

    if (sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, magn_z) < 0) {
		printf("Cannot read LIS3MDL pressure channel\n");
		return;
	}
}

/*
* Initialise the LIS3MDL
*/
void lis3mdl_initialise(const struct device *dev)
{

	if (dev == NULL) {
		printk("Could not get LIS3MDL device\n");
		return;
	}

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return;
	}
}