#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <time.h>
#include <string.h>

#define NODE1 0
#define NODE2 1
#define NODE3 2
#define NODE4 3
#define NODE5 4
#define NODE6 5
#define NODE7 6

#define NODE_A_MAC "F5:75:FE:85:34:67"
#define NODE_B_MAC "E5:73:87:06:1E:86" 
#define NODE_C_MAC "CA:99:9E:FD:98:B1"
#define NODE_D_MAC "CB:1B:89:82:FF:FE"
#define NODE_E_MAC "D4:D2:A0:A4:5C:AC"
#define NODE_F_MAC "C1:13:27:E9:B7:7C"
#define NODE_G_MAC "F1:04:48:06:39:A0"
#define NODE_H_MAC "CA:0C:E0:DB:CE:60"
#define NODE_I_MAC "D4:7F:D4:7C:20:13"
#define NODE_J_MAC "F7:0B:21:F1:C8:E1"
#define NODE_K_MAC "FD:E0:8D:FA:3E:4A"
#define NODE_L_MAC "EE:32:F7:28:FA:AC"

#define NODE_A_MAJOR 2753
#define NODE_B_MAJOR 32975
#define NODE_C_MAJOR 26679
#define NODE_D_MAJOR 41747
#define NODE_E_MAJOR 30679
#define NODE_F_MAJOR 6195
#define NODE_G_MAJOR 30525
#define NODE_H_MAJOR 57395
#define NODE_I_MAJOR 60345
#define NODE_J_MAJOR 12249
#define NODE_K_MAJOR 36748
#define NODE_L_MAJOR 27564

#define NODE_A_MINOR 32998 
#define NODE_B_MINOR 20959
#define NODE_C_MINOR 40363
#define NODE_D_MINOR 38800
#define NODE_E_MINOR 51963
#define NODE_F_MINOR 18394 
#define NODE_G_MINOR 30544
#define NODE_H_MINOR 28931
#define NODE_I_MINOR 49995
#define NODE_J_MINOR 30916
#define NODE_K_MINOR 11457
#define NODE_L_MINOR 27589

#define NODE_A_X 0
#define NODE_B_X 2
#define NODE_C_X 4
#define NODE_D_X 4
#define NODE_E_X 4
#define NODE_F_X 2
#define NODE_G_X 0
#define NODE_H_X 0
#define NODE_UNASSIGNED_X -1

#define NODE_A_Y 0
#define NODE_B_Y 0
#define NODE_C_Y 0
#define NODE_D_Y 2
#define NODE_E_Y 4
#define NODE_F_Y 4
#define NODE_G_Y 4
#define NODE_H_Y 2
#define NODE_UNASSIGNED_Y -1

#define NODE_ENABLE 1
#define NODE_DISABLE 0

void bsu_bt_thread(void);

uint16_t get_ahu_handle(void);
/* struct to hold BT packet data */
/* struct to hold BT packet data */
struct __attribute__((__packed__)) BT_Message {
	uint8_t preamble;
	uint8_t type;
	uint8_t length;
	float accelX;
	float accelY;
	float accelZ;
	float gyroX;
	float gyroY;
	float gyroZ;
	// struct sensor_values sensorReadings;
};

void remove_node(char name[6]);
void add_node(char name[6], int newXPos, int newYPos);
void node_details(char name[6]);
void node_details_all(void);
void ahu_send(uint16_t handle, struct BT_Message *bt_package);
void print_json(int8_t rssi_unprocessed_data[12]);