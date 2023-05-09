#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#define DID_SAMPLE 		0
#define DID_TEMP 		1
#define DID_PRESSURE 	2
#define DID_ACCELX 		3
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
#define DID_ALTITUDE	14
#define DID_CONTINUOUS	30

extern struct k_msgq hci2shell;
extern struct k_msgq shell2hci;
extern struct k_msgq bt2hci;

void bsu_hci_thread(void);
void send_packet(uint8_t type, uint8_t did, uint8_t option);
struct shell2hci_package {
	uint8_t device_id;
	uint8_t option;
};

struct hci2shell_package {
	uint32_t value1;
    uint32_t value2;
};

struct bt2hci_package {
	uint8_t did;
	float val1;
	float val2;
	float val3;
	float val4;
};