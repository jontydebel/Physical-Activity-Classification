/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>
#include "scu_hcsr04.h"

#define SLEEP_TIME_MS   1000

// /* The devicetree node identifier for the "led0" alias. */
#define TRIG0_NODE DT_ALIAS(trig0)
#define ECHO0_NODE DT_ALIAS(echo0)

// /*
//  * A build error on this line means your board is unsupported.
//  * See the sample documentation for information on how to fix this.
//  */
const struct gpio_dt_spec trig = GPIO_DT_SPEC_GET(TRIG0_NODE, gpios);
const struct gpio_dt_spec echo = GPIO_DT_SPEC_GET(ECHO0_NODE, gpios);

static struct gpio_callback echoCb;
uint32_t echoStart, echoEnd;
int ready = 0;
// K_SEM_DEFINE(echoSemaphore, 0, 1);

/**
 * @brief Callback function for the echo pin, triggers on both edges
 * and gets the time in ticks at each state.
*/
void echo_callback(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins)
{
  int val;
  //Callback has triggered, so check if start or end time.
  //If high, read has just started. If low, read has ended
  val = gpio_pin_get_dt(&echo);
  if (!val) {
    //Start time of echo response
    echoStart = k_cycle_get_32();
    ready = 0;
  } else if (val) {
    //End time of echo response, release semaphore
    echoEnd = k_cycle_get_32();
    ready = 1;
    //TODO: remove this?
    // k_sem_give(&echoSemaphore);
  }

  
}

/**
 * @brief Retrieve hcsr04 data. Sets the trigger pin to enable the sensor then
 * retrieves the duration of the echo response.
 * @returns Duration of the echo response, to be converted to distance in AHU
*/
uint32_t process_hcsr04_sample(void) {
  //Initialise both values to -1
  //If duration is too large, will return -1
  uint32_t distance = -1;
  uint32_t duration = -1;

    //Set the trigger pin high to start the reading. 
    //Note the pins are active_low and thus reversed logic for pin_set.
    gpio_pin_set_dt(&trig, 1);
    k_busy_wait(30);
    gpio_pin_set_dt(&trig, 0);
    k_busy_wait(10);
    gpio_pin_set_dt(&trig, 1);

    //TODO: add a mutex lock around processing - this sensor is precarious.
    // Ready will be set to 1 in the cb func upon the echo pin going back to low.
    if (ready) {
      //Get the duration in ticks
      duration = echoEnd - echoStart;
      //Convert duration to uS
      duration = k_cyc_to_us_near32(duration);
      
      //If reading is greater than 100cm, ignore it
      if ((duration/58) < 100) {
        printk("Distance in loop (uS): %d \n", distance);
        //From sensor datasheet: pulse width (uS) / 58 = distance (cm)
        //This distance is what will be caculated in AHU
        //Left here to provide understanding of hcsr04 sensor
        distance = duration / 58;
      }
      //Distance will be -1 if duration is too large
      printk("Distance (cm): %d \n", distance);
    }
		// k_msleep(SLEEP_TIME_MS);
    //Return the 
    return duration;
	// }
}

/**
 * @brief Initialise the hcsr04 sensor, set up trig and echo pins
*/
void hcsr04_initialise(void)
{
  int ret;
  //Check port is available
  if (!device_is_ready(trig.port)) {
    return;
  }

  //Configure trigger pin as output
  ret = gpio_pin_configure_dt(&trig, GPIO_OUTPUT);
	if (ret < 0) {
    printk("Error: trigger pin config failed\n");
		return;
	}

  //Configure echo pin as an input
  ret = gpio_pin_configure_dt(&echo, GPIO_INPUT);
	if (ret < 0) {
    printk("Error: echo pin config failed\n");
		return;
	}

  //Set up the echo pin to trigger a cb on both edges
  ret = gpio_pin_interrupt_configure_dt(&echo, GPIO_INT_EDGE_BOTH);
  if (ret < 0) {
    printk("Error: echo interrupt config failed\n");
		return;
	}
  // BIT() will bit-shift echo.pin to become correct mask
  gpio_init_callback(&echoCb, echo_callback, BIT(echo.pin));
  gpio_add_callback(echo.port, &echoCb);
}