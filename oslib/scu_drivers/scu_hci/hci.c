/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <string.h>
#include <hci.h>

#define UART_DEVICE_NODE DT_NODELABEL(uart4)
#define MSG_SIZE 128
#define DID_LSM6DSL
#define REPLY 0x02

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 8);
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

K_MSGQ_DEFINE(comms_to_sensor_msgq, sizeof(struct did_packet), 10, 4);
K_MSGQ_DEFINE(sensor_to_comms_msgq, sizeof(struct did_packet), 10, 4);

/*
 * Read bytes from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void receive_comms_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((rx_buf_pos == 0) && (c != 0xAA)) {
			//Ignore any junk data at front of message
			continue;
		}
		// printk("not in\r\n");
		if ((c == 0xFA) && rx_buf_pos > 0) {
			/* terminate string */
			rx_buf[rx_buf_pos] = '\0';

			for (int i = 0; i < rx_buf_pos; i++) {
				printk("%i: %x\r\n", i, rx_buf[i]);
			}

			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

/*
* Converts given parameters to the correctly formatted string and then sends through UART
*/
void send_packet(int type, uint8_t did, struct sensor_value value) {

	printk("Starting uart end\r\n");
	int len = sizeof(value.val1) * 2 + sizeof(did);
	uint16_t preamble = (0xAA << 8) | (type << 4) | (len);

	int msg_len = sizeof(preamble);

	printf("Preamble is 0x%x\n", preamble);
	

	unsigned char *chptr = (unsigned char *) &preamble;
	printf("Sending in form\n");
	for (int i = 0; i < msg_len; i++) {
		printf("%x", ((uint8_t *)chptr)[0]);
		uart_poll_out(uart_dev, *chptr++);
	}
	printf("\n");

	msg_len = sizeof(did);
	printf("DID length %i\n", msg_len);
	chptr = (unsigned char *) &did;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}

	msg_len = sizeof(value.val1);
	printf("Val1 len %i\n", msg_len);
	chptr = (unsigned char *) &value.val1;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}

	msg_len = sizeof(value.val2);
	printf("Val2 len %i\n", msg_len);
	chptr = (unsigned char *) &value.val2;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}
	uart_poll_out(uart_dev, 0xFA);
	printk("End uart end\r\n");
}

/*
* Converts given parameters to the correctly formatted string and then sends through UART
*/
void send_long_packet(int type, uint8_t did, struct sensor_value value, struct sensor_value value1, struct sensor_value value2) {

	printk("Starting uart end\r\n");
	int len = sizeof(value.val1) * 6 + sizeof(did); // 6 = Number of vals being sent, 2 in each struct.
	if (len > 0xF) {
		len = 0xF;
	}
	uint16_t preamble = (0xAA << 8) | (type << 4) | (len);

	int msg_len = sizeof(preamble);
	printf("Len being sent %i\n", len);

	unsigned char *chptr = (unsigned char *) &preamble;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}

	msg_len = sizeof(did);
	printf("DID length %i\n", msg_len);
	chptr = (unsigned char *) &did;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}

	msg_len = sizeof(value.val1);
	chptr = (unsigned char *) &value.val1;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}

	msg_len = sizeof(value.val2);
	chptr = (unsigned char *) &value.val2;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}

	msg_len = sizeof(value1.val1);
	chptr = (unsigned char *) &value1.val1;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}

	msg_len = sizeof(value1.val2);
	chptr = (unsigned char *) &value1.val2;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}

	msg_len = sizeof(value2.val1);
	chptr = (unsigned char *) &value2.val1;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}

	msg_len = sizeof(value2.val2);
	chptr = (unsigned char *) &value2.val2;
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, *chptr++);
	}
	uart_poll_out(uart_dev, 0xFA);
	printk("End uart long print\r\n");
}

/*
* Controlling thread function for HCI, set's up RX/TX and processes messages to be sent and received.
*/
void hci_main(void)
{
	// printk("testing 12\n");
	char tx_buf[MSG_SIZE];

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return;
	}

	/* configure interrupt and callback to receive data */
	uart_irq_callback_user_data_set(uart_dev, receive_comms_cb, NULL);
	// int ret = uart_irq_callback_user_data_set(uart_dev, receive_comms_cb, NULL);
	// if (ret < 0) {
	// 	if (ret == -ENOTSUP) {
	// 		printk("Interrupt-driven UART API support not enabled\n");
	// 	} else if (ret == -ENOSYS) {
	// 		printk("UART device does not support interrupt-driven API\n");
	// 	} else {
	// 		printk("Error setting UART callback: %d\n", ret);
	// 	}
	// 	return;
	// }
	uart_irq_rx_enable(uart_dev);

	/* wait 500ms for input from UART */
	while (k_msgq_get(&uart_msgq, &tx_buf, K_MSEC(500)) == 0) {
		//Check Preamble is correct
		printk("Here1\r\n");
		if (tx_buf[0] == 0xAA) {
			printk("Here2\r\n");

			//Check type is request (0x01)
			uint8_t temp = ((tx_buf[1] & 0xF0) >> 4);
			if (temp == 0x01) {
				
				//Send message to sensor.
				struct did_packet msg;
				msg.DID = (tx_buf[2]);
				msg.options = (tx_buf[3]);
				printk("Here3\r\n");
				if (k_msgq_put(&comms_to_sensor_msgq, &msg, K_NO_WAIT) != 0) {
					k_msgq_purge(&comms_to_sensor_msgq);
				}
			}
			
		}		
		
	}

	// uart_poll_out(uart_dev, '\n');

	//Read response from sensor
	struct did_packet sens_recv;
	while (k_msgq_get(&sensor_to_comms_msgq, &sens_recv, K_MSEC(500)) == 0) {
		printk("DID AND OPT: %i, %i", sens_recv.DID, sens_recv.options);
		
		//Send sensor reading to AHU
		if ((sens_recv.DID == DID_ACCELX || sens_recv.DID == DID_PRESSURE) && sens_recv.options) {
			printk("Sending long msg\n");
			printk("ValuesX(Press): %i.%i\r\n ValuesY(Data): %i.%i\r\n ValuesZ: %i.%i\r\n", sens_recv.sensor_reading.val1, sens_recv.sensor_reading.val2,
			sens_recv.opt1_sensor_reading.val1, sens_recv.opt1_sensor_reading.val2, sens_recv.opt2_sensor_reading.val1, sens_recv.opt2_sensor_reading.val2);
			send_long_packet(REPLY, sens_recv.DID, sens_recv.sensor_reading, sens_recv.opt1_sensor_reading, sens_recv.opt2_sensor_reading);
		} else {
			printk("Sending to Uart: DID: %i, Values: %i.%i\r\n", sens_recv.DID, sens_recv.sensor_reading.val1, sens_recv.sensor_reading.val2);
			send_packet(REPLY, sens_recv.DID, sens_recv.sensor_reading);
		}
		
	}
	k_sleep(K_MSEC(50));
}