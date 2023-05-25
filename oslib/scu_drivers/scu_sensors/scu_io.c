/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <scu_io.h>

static struct gpio_callback button_cb_data;

/*
* Toggle the IO refered to by dev.
*/
int toggle_io(const struct gpio_dt_spec dev) {
	return gpio_pin_toggle_dt(&dev);
}

/*
* Get IO state of IO refered to by dev.
*/
int get_io_state(const struct gpio_dt_spec dev) {
    return gpio_pin_get_dt(&dev);
}

/*
* Initialise all IO modules.
*/
void io_initialise(const struct gpio_dt_spec led0, const struct gpio_dt_spec led1, const struct gpio_dt_spec button) {
	int ret;
	//LED0
	if (led0.port && !device_is_ready(led0.port)) {
		printk("Error %d: LED device %s is not ready; ignoring it\n",
		       ret, led0.port->name);
		// led0.port = NULL;
	}
	if (led0.port) {
		ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n",
			       ret, led0.port->name, led0.pin);
			// led0.port = NULL;
		} else {
			printk("Set up LED at %s pin %d\n", led0.port->name, led0.pin);
		}
	}
	
	//LED1
	if (led1.port && !device_is_ready(led1.port)) {
		printk("Error %d: LED device %s is not ready; ignoring it\n",
		       ret, led1.port->name);
		// led1.port = NULL;
	}
	if (led1.port) {
		ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n",
			       ret, led1.port->name, led1.pin);
			// led1.port = NULL;
		} else {
			printk("Set up LED at %s pin %d\n", led1.port->name, led1.pin);
		}
	}
	
	// if (!gpio_is_ready_dt(&led1)) {
	// 	return;
	// }
	// ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	// if (ret < 0) {
	// 	return;
	// }

	//Pushbutton
	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);
}

/*
* DEBUG FUNCTION: Prints an output upon button press.
*/
void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
    #ifdef DEBUG
        printk("DEBUG: Button is working");
    #endif
}