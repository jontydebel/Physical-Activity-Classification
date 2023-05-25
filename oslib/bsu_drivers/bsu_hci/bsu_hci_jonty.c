#include "bsu_hci.h"
#include "bsu_bt.h"

#define MSG_SIZE_SHELL_2_HCI sizeof(struct shell2hci_package)
#define MSG_SIZE_HCI_2_SHELL sizeof(struct hci2shell_package)
#define MSG_SIZE_BT_2_HCI sizeof(struct bt2hci_package)

#define NODE1 0
#define NODE2 1
#define NODE3 2
#define NODE4 3
#define NODE5 4
#define NODE6 5
#define NODE7 6

#define NODE_ENABLE 1
#define NODE_DISABLE 0

static int node_status[8] = {1, 1, 1, 1, 1, 1, 1, 1};

/* queues to store up to 100 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(shell2hci, MSG_SIZE_SHELL_2_HCI, 10, 4);
K_MSGQ_DEFINE(hci2shell, MSG_SIZE_HCI_2_SHELL, 10, 4);
K_MSGQ_DEFINE(bt2hci, MSG_SIZE_BT_2_HCI, 10, 4);

/* Bluetooth package struct for transmitting */
static struct BT_Message *outgoing_package = &(struct BT_Message) {
	.preamble = 0xAA,
	.type = 0x01,
	.length = 0,
	.data = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	.did = 0
};

static int continuous = 0;
static int rate_ms = 2000;
volatile int last_send_time = 0;

/**
 * Thread for running the HCI Bluetooth interface.
*/
void bsu_hci_thread(void)
{
	struct shell2hci_package incoming_shell_package;
	struct bt2hci_package incoming_bt_package;

	// char
	
	/* indefinitely wait on commands from shell*/
	while(1) {

		if (k_msgq_get(&shell2hci, &incoming_shell_package, K_NO_WAIT) == 0) {
			
			//TODO: edit for new shell commands
			outgoing_package->did = incoming_shell_package.device_id;
			outgoing_package->data[0] = incoming_shell_package.option;
			outgoing_package->length = sizeof(outgoing_package->data);
			ahu_send(get_ahu_handle(), outgoing_package);
		}

		if (k_msgq_get(&bt2hci, &incoming_bt_package, K_NO_WAIT) == 0) {
			
			//TODO: edit for new data schema
			uint8_t did = incoming_bt_package.did;
			float val1 = incoming_bt_package.val1;
			float val2 = incoming_bt_package.val2;
			float val3 = incoming_bt_package.val3;
			float val4 = incoming_bt_package.val4;

			switch (did) { //TODO: remove old case, create new ones

				case DID_TEMP:
					printk("Temperature: %f C\r\n", val1);
					break;

				case DID_PRESSURE:
					printk("Pressure: %f kPa\r\n", val1);
					break;

				case DID_ACCELX:
					printk("Acceleration X: %f\r\n", val1);
					break;

				case DID_ACCELY:
					printk("Acceleration Y: %f\r\n", val1);
					break;

				case DID_ACCELZ:
					printk("Acceleration Z: %f\r\n", val1);
					break;

				case DID_GYROX:
					printk("Gyro X: %f\r\n", val1);
					break;

				case DID_GYROY:
					printk("Gyro Y: %f\r\n", val1);
					break;

				case DID_GYROZ:
					printk("Gyro Z: %f\r\n", val1);
					break;

				case DID_MAGNX:
					printk("Magnetometer X: %f\r\n", val1);
					break;

				case DID_MAGNY:
					printk("Magnetometer Y: %f\r\n", val1);
					break;

				case DID_MAGNZ:
					printk("Magnetometer Z: %f\r\n", val1);
					break;

				case DID_US:
					printk("Ultrasonic (raw): %f\r\n"), val1;
					break;

				case DID_ORIENTATION:
					printk("Orientation \r\n\tPitch: %f\r\n\tRoll: %f\r\n", val1, val2);
					break;

				case DID_ALTITUDE:
					printk("Altitude \r\n\tCoarse (Barometer): %f\r\n\tFine (Ultrasonic): %f\r\n", val1, val2);
					break;

				case DID_CONTINUOUS:
					printk("{ \"timestamp\" : %d, \"%d\" : [%f, %f], \"%d\" : [%f, %f]}\r\n", 
							k_uptime_get_32(), DID_ORIENTATION, val1, val2, DID_ALTITUDE, val3, val4);
					break;
			}
		}

		k_sleep(K_MSEC(100));
	}
}