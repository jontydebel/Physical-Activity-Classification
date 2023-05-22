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
uint32_t currTime, durationTime; 	//Current time value (ticks) and duration (uS). Duration will be Curr - Prev
uint32_t prevTime = 0;			//Prev time value - starts at 0 ticks

//TODO: redefine this struct for prac3
// static struct BT_Message *bt_packet = &(struct BT_Message) {
// 	.preamble = 0xAA,
// 	.type = 0x02,
// 	.length = 0,
// 	.data = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
// 	.did = 0
// };

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
/* struct to hold BT packet data */
// struct __attribute__((__packed__)) BT_Message {
// 	uint8_t preamble;
// 	uint8_t type;
// 	uint8_t length;
// 	float accelX;
// 	float accelY;
// 	float accelZ;
// 	float gyroX;
// 	float gyroY;
// 	float gyroZ;
// 	// struct sensor_values sensorReadings;
// };

void ahu_sensor_thread(void)
{   
    prevTime = k_cycle_get_32();
    const char *const label = DT_LABEL(DT_INST(0, invensense_mpu6050));
	const struct device *mpu6050 = device_get_binding(label);
    init_mpu6050(mpu6050);

    //Initialise packet with basic info
	struct BT_Message bt_sensor_packet;
	bt_sensor_packet.preamble = 0xAA;
	bt_sensor_packet.type = 0x02;
    struct sensor_value accel_x, accel_y, accel_z;
	struct sensor_value gyro_x, gyro_y, gyro_z;

    while (!IS_ENABLED(CONFIG_MPU6050_TRIGGER)) {
        while (get_ble_connected() == 0) {
        }
        currTime = k_cycle_get_32();
      	durationTime = currTime - prevTime;
      	//Convert duration to uS
     	durationTime = k_cyc_to_us_near32(durationTime);
		//Check if more than 200ms have passed (in uS)
		if (durationTime > 20000) {
            // printk("Processing now.\n");
            int rc = process_mpu6050(mpu6050, &accel_x, &accel_y, 
                                        &accel_z, &gyro_x, &gyro_y, &gyro_z);
            
            // printf("Vals are:\n"
            //        "  accel %f %f %f m/s/s\n"
            //        "  gyro  %f %f %f rad/s\n",
            //        sensor_value_to_double(&accel_x),
            //        sensor_value_to_double(&accel_y),
            //        sensor_value_to_double(&accel_z),
            //        sensor_value_to_double(&gyro_x),
            //        sensor_value_to_double(&gyro_y),
            //        sensor_value_to_double(&gyro_z));

            if (rc != 0) {
                //This should never occur.
                break;
            }

            // printk("In Send now.\n");
			//Update timePrev (ticks)
			prevTime = k_cycle_get_32();
			// Send to BSU
			// Wait for semaphore to be given before sending bluetooth message.
			if (!k_sem_take(&bt_send_sem, K_MSEC(100))) {
				printk("IN SEMAPHORE\n");
				//Assign sensor readings to BT packet
				bt_sensor_packet.accelX = sensor_value_to_double(&accel_x);
                bt_sensor_packet.accelY = sensor_value_to_double(&accel_y);
                bt_sensor_packet.accelZ = sensor_value_to_double(&accel_z);
                bt_sensor_packet.gyroX = sensor_value_to_double(&gyro_x);
                bt_sensor_packet.gyroY = sensor_value_to_double(&gyro_y);
                bt_sensor_packet.gyroZ = sensor_value_to_double(&gyro_z);
                // memcpy(bt_sensor_packet.accelY, sensor_value_to_double(&accel_y), sizeof(float));
                // memcpy(bt_sensor_packet.accelZ, sensor_value_to_double(&accel_z), sizeof(float));
                // memcpy(bt_sensor_packet.gyroX, sensor_value_to_double(&gyro_x), sizeof(float));
                // memcpy(bt_sensor_packet.gyroY, sensor_value_to_double(&gyro_y), sizeof(float));
                // memcpy(bt_sensor_packet.gyroZ, sensor_value_to_double(&gyro_z), sizeof(float));
                // printf("Vals are:\n"
                //         "  accel %f %f %f m/s/s\n"
                //         "  gyro  %f %f %f rad/s\n",
                //         bt_sensor_packet.accelX,
                //         bt_sensor_packet.accelY,
                //         bt_sensor_packet.accelY,
                //         bt_sensor_packet.gyroX,
                //         bt_sensor_packet.gyroY,
                //         bt_sensor_packet.gyroZ);
				// bt_node_packet.length = sizeof(bt_node_packet.rssiVals);
                bt_sensor_packet.length = sizeof(bt_sensor_packet.accelX) * 6;

				//Send packet over bluetooth
				blu_send(get_blu_handle(), &bt_sensor_packet);
			}
		}


		// k_sleep(K_SECONDS(1));
	}

}



//Define both threads needed for Prac 3 - Main BT Thread and Node Thread
// K_THREAD_DEFINE(ahu_node_thread_tid, AHU_BT_THREAD_STACK, ahu_node_thread, NULL, NULL, NULL, AHU_NODE_THREAD_PRIORITY, 0, 500);
K_THREAD_DEFINE(ahu_sensor_thread_tid, AHU_SENSOR_THREAD_STACK, ahu_sensor_thread, NULL, NULL, NULL, AHU_SENSOR_THREAD_PRIORITY, 0, 1000);
K_THREAD_DEFINE(ahu_bt_thread_tid, AHU_BT_THREAD_STACK, ahu_bt_thread, NULL, NULL, NULL, AHU_BT_THREAD_PRIORITY, 0, 0);