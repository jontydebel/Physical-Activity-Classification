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
#include <scu_lps22hb.h>


struct did_packet {
	uint16_t preamble; //Will contain 0xAA, 0x01/02 and Data length
	char* data;
	int DID;
	double sensor_reading;
};

/*
* Retrieve the pressure and temperature readings from the LPS22HB
*/
void process_lps22hb_sample(const struct device *dev, 
							struct sensor_value *pressure, struct sensor_value *temp)
{

	if (sensor_sample_fetch(dev) < 0) {
		printf("Sensor sample update error\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_PRESS, pressure) < 0) {
		printf("Cannot read LPS22HB pressure channel\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, temp) < 0) {
		printf("Cannot read LPS22HB temperature channel\n");
		return;
	}

	// /* display pressure */
	// printf("Pressure:%.1f kPa\n", sensor_value_to_double(pressure));

	// /* display temperature */
	// printf("Temperature:%.1f C\n", sensor_value_to_double(temp));


}

/*
* Initialise the LPS22HB
*/
void lps22hb_initialise(const struct device *dev)
{

	if (dev == NULL) {
		printk("Could not get LPS22HB device\n");
		return;
	}

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return;
	}
}