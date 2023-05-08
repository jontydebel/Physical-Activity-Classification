#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "bt.h"
#include "t52_mpu6050.h"

//Priorities for all threads. BT will have highest 
#define AHU_BT_THREAD_PRIORITY      5
#define AHU_BT_THREAD_STACK         4096
// #define AHU_NODE_THREAD_PRIORITY    5
// #define AHU_NODE_THREAD_STACK       4096
#define AHU_SENSOR_THREAD_PRIORITY    5
#define AHU_SENSOR_THREAD_STACK       4096

int bt_received;

//TODO: redefine this struct for prac3
static struct BT_Message *bt_packet = &(struct BT_Message) {
	.preamble = 0xAA,
	.type = 0x02,
	.length = 0,
	.data = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	.did = 0
};

/**
 * Main function, initialises bluetooth
*/
void main(void) {
    //Initialise bt in the main() thread, at the start of device.
    bt_init();
}

/**
 * AHU's Bluetooth thread, will process all received messaged from the BSU 
*/
void ahu_bt_thread(void)
{
    bt_received = 0;                    //Global Variable if BT msg recv.

    //Structs used for sending and receiving messages
    static struct BT_Message bt_recv_packet;    //From BT
    
    int count = 0;
    int last_time = 0;

    while (1) {
        // //Get BLE connection state.
        // int ble_connected = get_ble_connected();

        // // Wait for semaphore to be given before receiving bluetooth message.
        // if (!k_sem_take(&bt_recv_sem, K_MSEC(100))) {
        //     //Receive bluetooth message
        //     if (k_msgq_get(&bt_queue, &bt_recv_packet, K_FOREVER) == 0) {
        //         bt_received = 1;
		//     }
        // }
        k_sleep(K_SECONDS(10));
    }

}

// void ahu_node_thread(void)
// {   
//     //Initial node scan - this will repeatedly read bluetooth devices and print the relevant devices and their RSSI value 
//     bt_init_node_scan();
// }

void ahu_sensor_thread(void)
{   
    const char *const label = DT_LABEL(DT_INST(0, invensense_mpu6050));
	const struct device *mpu6050 = device_get_binding(label);
    init_mpu6050(mpu6050);

    while (!IS_ENABLED(CONFIG_MPU6050_TRIGGER)) {
		printk("Processing now.\n");
		int rc = process_mpu6050(mpu6050);

		if (rc != 0) {
			break;
		}
		k_sleep(K_SECONDS(1));
	}
}



//Define both threads needed for Prac 3 - Main BT Thread and Node Thread
// K_THREAD_DEFINE(ahu_node_thread_tid, AHU_BT_THREAD_STACK, ahu_node_thread, NULL, NULL, NULL, AHU_NODE_THREAD_PRIORITY, 0, 500);
K_THREAD_DEFINE(ahu_sensor_thread_tid, AHU_SENSOR_THREAD_STACK, ahu_sensor_thread, NULL, NULL, NULL, AHU_SENSOR_THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(ahu_bt_thread_tid, AHU_BT_THREAD_STACK, ahu_bt_thread, NULL, NULL, NULL, AHU_BT_THREAD_PRIORITY, 0, 0);