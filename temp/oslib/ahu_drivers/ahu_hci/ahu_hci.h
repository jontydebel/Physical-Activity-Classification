#include <zephyr/drivers/uart.h>
// #include <zephyr/usb/usb_device.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <time.h>
#include <string.h>

#ifndef AHU_HCI
#define AHU_HCI

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

void ahu_hci_thread(void);
extern struct k_msgq hci2shell;
extern struct k_msgq shell2hci;
extern struct k_msgq hci2bt;
extern struct k_msgq bt2hci;
extern struct k_mutex ahuCommsMutex; 
void send_packet(uint8_t type, uint8_t did, uint8_t option);
struct shell2hci_package {
	uint8_t device_id;
	uint8_t option;
};

struct hci2shell_package {
	uint8_t device_id;
	uint32_t value1;
    uint32_t value2;
	uint32_t value3;
    uint32_t value4;
	uint32_t value5;
    uint32_t value6;
};

#endif