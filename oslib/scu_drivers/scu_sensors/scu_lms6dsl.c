/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <scu_lms6dsl.h>

static int print_samples;
static int lsm6dsl_trig_cnt;

static struct sensor_value accel_x_out, accel_y_out, accel_z_out;
static struct sensor_value gyro_x_out, gyro_y_out, gyro_z_out;
#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
static struct sensor_value magn_x_out, magn_y_out, magn_z_out;
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
static struct sensor_value press_out, temp_out;
#endif

static inline float out_ev(struct sensor_value *val)
{
	return (val->val1 + (float)val->val2 / 1000000);
}

/*
* Retrieve the gyroscope and accelerometer readings from the LMS6DSL
*/
void process_lms6dsl_sample(const struct device *dev, struct sensor_value *accel_x, struct sensor_value *accel_y,
								struct sensor_value *accel_z, struct sensor_value *gyro_x, 
								struct sensor_value *gyro_y, struct sensor_value *gyro_z)
{
	print_samples = 1;
	// char out_str[64];

	// sprintf(out_str, "accel x:%f ms/2 y:%f ms/2 z:%f ms/2",
	// 						  out_ev(&accel_x_out),
	// 						  out_ev(&accel_y_out),
	// 						  out_ev(&accel_z_out));
	// 	printk("%s\n", out_str);

	// 	/* lsm6dsl gyro */
	// 	sprintf(out_str, "gyro x:%f dps y:%f dps z:%f dps",
	// 						   out_ev(&gyro_x_out),
	// 						   out_ev(&gyro_y_out),
	// 						   out_ev(&gyro_z_out));
	// 	printk("%s\n", out_str);

	/* Pass values back to main thread. */
	*accel_x = accel_x_out;
	*accel_y = accel_y_out;
	*accel_z = accel_z_out;
	*gyro_x = gyro_x_out;
	*gyro_y = gyro_y_out;
	*gyro_z = gyro_z_out;
}

/*
*	Trigger Handler for LSM6DSL, fetches sensor values to be retrieved at a later date.
*/
#ifdef CONFIG_LSM6DSL_TRIGGER
static void lsm6dsl_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	static struct sensor_value accel_x, accel_y, accel_z;
	static struct sensor_value gyro_x, gyro_y, gyro_z;
#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
	static struct sensor_value magn_x, magn_y, magn_z;
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
	static struct sensor_value press, temp;
#endif
	lsm6dsl_trig_cnt++;

	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &accel_x);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &accel_y);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &accel_z);

	/* lsm6dsl gyro */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_X, &gyro_x);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Y, &gyro_y);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Z, &gyro_z);

#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
	/* lsm6dsl external magn */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_MAGN_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &magn_x);
	sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, &magn_y);
	sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, &magn_z);
#endif

#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
	/* lsm6dsl external press/temp */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);
	sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);

	sensor_sample_fetch_chan(dev, SENSOR_CHAN_AMBIENT_TEMP);
	sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
#endif

	if (print_samples) {
		print_samples = 0;

		accel_x_out = accel_x;
		accel_y_out = accel_y;
		accel_z_out = accel_z;

		gyro_x_out = gyro_x;
		gyro_y_out = gyro_y;
		gyro_z_out = gyro_z;

#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
		magn_x_out = magn_x;
		magn_y_out = magn_y;
		magn_z_out = magn_z;
#endif

#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
		press_out = press;
		temp_out = temp;
#endif
	}

}
#endif

/*
*	Initialise LSM6DSL
*/
void lsm6dsl_initialise(const struct device *dev)
{
	struct sensor_value odr_attr;
	/* set accel/gyro sampling frequency to 104 Hz */
	odr_attr.val1 = 104;
	odr_attr.val2 = 0;

	if (dev == NULL) {
		printk("Could not get LSM6DSL device\n");
		return;
	}

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return;
	}

	if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for accelerometer.\n");
		return;
	}

	if (sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for gyro.\n");
		return;
	}

#ifdef CONFIG_LSM6DSL_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;

	if (sensor_trigger_set(dev, &trig, lsm6dsl_trigger_handler) != 0) {
		printk("Could not set sensor type and channel\n");
		return;
	}
#endif


	if (sensor_sample_fetch(dev) < 0) {
		printk("Sensor sample update error\n");
		return;
	}
}