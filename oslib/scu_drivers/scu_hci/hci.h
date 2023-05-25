#ifndef HCI_H
#define HCI_H

#define MSG_SIZE 32

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

#define DID_SAMPLE		0
#define DID_TEMP		1
#define DID_PRESSURE	2
#define DID_ACCELX		3
#define DID_ACCELY		4
#define DID_ACCELZ		5
#define DID_GYROX		6
#define DID_GYROY		7
#define DID_GYROZ		8
#define DID_MAGNX		9
#define DID_MAGNY		10
#define DID_MAGNZ		11
#define DID_US			12
#define DID_ORIENTATION 13
#define DID_ALTITUDE    14
#define DID_LED0		15
#define DID_LED1		16
#define DID_PB			17

struct did_packet {
	int DID;
	int options;
	struct sensor_value sensor_reading;
	struct sensor_value opt1_sensor_reading;
	struct sensor_value opt2_sensor_reading;
};

// struct uart_packet {
// 	uint16_t preamble; //Will contain 0xAA, 
// 	uint8_t type_len; // WIll contain type (0x01/02) and Data length
// 	uint8_t length;
// 	uint8_t did;
// 	uint8_t data[16];
// };

extern struct k_msgq comms_to_sensor_msgq;
extern struct k_msgq sensor_to_comms_msgq;
void serial_cb(const struct device *dev, void *user_data);
void send_packet(int type, uint8_t did, struct sensor_value value);
void hci_main(void);

#endif