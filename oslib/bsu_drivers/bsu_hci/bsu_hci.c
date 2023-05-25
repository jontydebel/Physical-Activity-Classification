#include "bsu_hci.h"
#include "bsu_bt.h"
// #include <zephyr/sys/dlist.h>

#define MSG_SIZE_SHELL_2_HCI sizeof(struct shell2hci_package)
#define MSG_SIZE_HCI_2_SHELL sizeof(struct hci2shell_package)
#define MSG_SIZE_BT_2_HCI sizeof(struct bt2hci_package)



/* queues to store up to 100 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(shell2hci, MSG_SIZE_SHELL_2_HCI, 10, 4);
K_MSGQ_DEFINE(hci2shell, MSG_SIZE_HCI_2_SHELL, 10, 4);
K_MSGQ_DEFINE(bt2hci, MSG_SIZE_BT_2_HCI, 10, 4);

// /* Bluetooth package struct for transmitting */
// static struct BT_Message *outgoing_package = &(struct BT_Message) {
// 	.preamble = 0xAA,
// 	.type = 0x01,
// 	.length = 0,
// 	.data = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
// 	.did = 0
// };

volatile int last_send_time = 0;



/**
 * Thread for running the HCI Bluetooth interface.
*/
void bsu_hci_thread(void)
{
	// struct static_node nodeA, nodeB, nodeC;
	// sys_dlist_init(&LinkedList);
	// static_node_init(&nodes_list[0], "4011-A", NODE_A_MAC, NODE_A_MAJOR, NODE_A_MINOR, NODE_A_X, NODE_A_Y, "4011-H", "4011-B");
	// static_node_init(&nodes_list[1], "4011-B", NODE_B_MAC, NODE_B_MAJOR, NODE_B_MINOR, NODE_B_X, NODE_B_Y, "4011-A", "4011-C");
	// static_node_init(&nodes_list[2], "4011-C", NODE_C_MAC, NODE_C_MAJOR, NODE_C_MINOR, NODE_C_X, NODE_C_Y, "4011-B", "4011-D");
	// static_node_init(&nodes_list[3], "4011-D", NODE_D_MAC, NODE_D_MAJOR, NODE_D_MINOR, NODE_D_X, NODE_D_Y, "4011-C", "4011-E");
	// static_node_init(&nodes_list[4], "4011-E", NODE_E_MAC, NODE_E_MAJOR, NODE_E_MINOR, NODE_E_X, NODE_E_Y, "4011-D", "4011-F");
	// static_node_init(&nodes_list[5], "4011-F", NODE_F_MAC, NODE_F_MAJOR, NODE_F_MINOR, NODE_F_X, NODE_F_Y, "4011-E", "4011-G");
	// static_node_init(&nodes_list[6], "4011-G", NODE_G_MAC, NODE_G_MAJOR, NODE_G_MINOR, NODE_G_X, NODE_G_Y, "4011-F", "4011-H");
	// static_node_init(&nodes_list[7], "4011-H", NODE_H_MAC, NODE_H_MAJOR, NODE_H_MINOR, NODE_H_X, NODE_H_Y, "4011-G", "4011-I");

	struct shell2hci_package incoming_shell_package;
	struct bt2hci_package incoming_bt_package;
	
	/* indefinitely wait on commands from shell*/
	while(1) {
		if (k_msgq_get(&shell2hci, &incoming_shell_package, K_NO_WAIT) == 0) {
			
			//TODO: edit for new shell commands
			// outgoing_package->did = incoming_shell_package.device_id;
			// outgoing_package->data[0] = incoming_shell_package.option;
			// outgoing_package->length = sizeof(outgoing_package->data);
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
					printk("Ultrasonic (raw): %f\r\n", val1);
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

