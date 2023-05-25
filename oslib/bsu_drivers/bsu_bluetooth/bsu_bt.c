/*
 * Largely based on example code: zephyr/samples/bluetooth/central_ht
 */
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/gpio.h>
// #include "bsu_hci.h"
#include "bsu_bt.h"

#define LED2_NODE DT_ALIAS(led2)

/* BSU UUID */
#define BLU_UUID	BT_UUID_DECLARE_16(0xDEAD)
#define BLU_CHRC_UUID	BT_UUID_DECLARE_16(0xD00D)

/* AHU UUID */
#define AHU_UUID	BT_UUID_DECLARE_16(0xB00B)
#define AHU_CHRC_UUID	BT_UUID_DECLARE_16(0x1350)

static int scan_start(void);
//Basic BT structs required for connection.
static struct bt_conn *default_conn;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static uint16_t ahu_handle; //Handle for sending/receiving from ahu

//LED to be toggled - shows device is functioning
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

/* struct to hold BT Node packet data */
struct __attribute__((__packed__)) BT_Node_Message {
	uint8_t preamble;
	uint8_t type;
	uint8_t length;
	//Vals will be in alphabetical order. A -> L
	int8_t rssiVals[12];
};

// /* Initialise the packet for receiving nodes readings over BT */
// struct BT_Node_Message *recv_node_packet = &(struct BT_Node_Message){
// 	.preamble = 0,
// 	.type = 0,
// 	.length = 0,
// 	.rssiVals = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
// };

/* Initialise the packet for receiving nodes readings over BT */
struct BT_Message *recv_node_packet = &(struct BT_Message){
	.preamble = 0,
	.type = 0,
	.length = 0,
	.accelX = 0,
	.accelY = 0,
	.accelZ = 0,
	.gyroX = 0,
	.gyroY = 0,
	.gyroZ = 0,
};

/* struct to hold HCI packet data */
struct __attribute__((__packed__)) HCI_Message
{
	uint8_t preamble;
	uint8_t type;
	uint8_t length;
	uint8_t data[16];
	uint8_t did;
};

/* holds HC packet to send */
struct HCI_Message *tx_packet = &(struct HCI_Message){
	.preamble = 0xAA,
	.type = 0x01,
	.length = 0,
	.data = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.did = 0
};

/* struct to hold data required for static node */
typedef struct static_node {
	char name[10];
	char mac_address[20];
	uint16_t major;
	uint16_t minor;
	int8_t x_pos;
	int8_t y_pos;
	uint8_t currRSSI;
	char r_neighbour[10];
	char l_neighbour[10];
	sys_dnode_t node; //Needed for a double-linked list.
} static_node;

//Initialise the LinkedList array
static_node nodes_list[12];
sys_dlist_t LinkedList;

/* Returns AHU handle for external access */
uint16_t get_ahu_handle(void)
{
	return ahu_handle;
}

/**
 * @brief Callback function for sending to AHU
 */
static void ahu_send_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_write_params *params)
{
	//Check for fail, then clear the params
	if (err != BT_ATT_ERR_SUCCESS)
	{
		printk("Write failed: 0x%02X", err);
	}

	(void)memset(params, 0, sizeof(*params));
}

/**
 * @brief Send data to ahu
 * @param handle handler pointing ot the ahu GATT.
 */
void ahu_send(uint16_t handle, struct BT_Message *bt_package) {
	static struct bt_gatt_write_params write_params;
	int err;

	//Check handle is correct and assign the write_params data
	if (handle == ahu_handle)
	{

		write_params.data = bt_package;
		write_params.length = sizeof(*bt_package);
	}

	//Give callback function and handle to write_params
	write_params.func = ahu_send_cb;
	write_params.handle = handle;

	//Start the GATT write to send the data
	err = bt_gatt_write(default_conn, &write_params);
	if (err != 0)
	{
		printk("bt_gatt_write failed: %d", err);
	}
}

/**
 * @brief Function used for discovery of AHU
*/
static uint8_t discover_func(struct bt_conn *conn,
							 const struct bt_gatt_attr *attr,
							 struct bt_gatt_discover_params *params)
{
	int err;

	//Check if attr exists - if doesnt cancel the discover.
	if (!attr)
	{
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	//Check type is correct, continue if it is.
	if (params->type == BT_GATT_DISCOVER_PRIMARY &&
		(!bt_uuid_cmp(params->uuid, AHU_UUID)))
	{
		params->uuid = NULL;
		params->start_handle = attr->handle + 1;
		params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, params);
		if (err != 0)
		{
			printk("Discover failed (err %d)", err);
		}

		return BT_GATT_ITER_STOP;
	}
	//Check the characteristics and assign the ahu handle.
	else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC)
	{
		struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, AHU_CHRC_UUID) == 0)
		{
			ahu_handle = chrc->value_handle;
		}
	}
	//All data required has been retrieved, stop.
	return BT_GATT_ITER_STOP;
}

/**
 * @brief bluetooth device connected callback
*/
static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	//Retrieve address for print statements.
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	//Check for a connection error.
	if (conn_err)
	{
		printk("Failed to connect to %s (%u)\n", addr, conn_err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		scan_start();
		return;
	}

	printk("Connected: %s\n", addr);

	//If no error, set up discover parameters and pass in discover_func function.
	if (conn == default_conn)
	{
		memcpy(&uuid, AHU_UUID, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

		// When done, calls the discover_func function
		err = bt_gatt_discover(default_conn, &discover_params);
		if (err)
		{
			printk("Discover failed(err %d)\n", err);
			return;
		}
	}
}

/**
 * @brief Extended Inquiry Response - checks the UUID is correct.
 *		  If so, stops scanning and creates connection
*/
static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int i;

	switch (data->type)
	{
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
	//Data type is correct, check the full UUID is correct one wanted
		//Check length is correct size
		if (data->data_len % sizeof(uint16_t) != 0U)
		{
			printk("AD malformed\n");
			return true;
		}
		//Loop through data and assign UUID
		for (i = 0; i < data->data_len; i += sizeof(uint16_t))
		{
			struct bt_uuid *uuid;
			uint16_t u16;
			int err;

			memcpy(&u16, &data->data[i], sizeof(u16));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
			//Check UUID is correct one wanted
			if (bt_uuid_cmp(uuid, AHU_UUID))
			{
				continue;
			}

			//Stop scan - UUID correct
			err = bt_le_scan_stop();
			if (err)
			{
				printk("Stop LE scan failed (err %d)\n", err);
				continue;
			}

			//Create connection once UUID is found to be correct
			err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
									BT_LE_CONN_PARAM_DEFAULT,
									&default_conn);
			if (err)
			{
				printk("Create connection failed (err %d)\n",
					   err);
				scan_start();
			}

			return false;
		}
	}

	return true;
}

/**
 * Device found callback
*/
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
						 struct net_buf_simple *ad)
{	
	char dev[BT_ADDR_LE_STR_LEN];
	//Retrieve address string
	bt_addr_le_to_str(addr, dev, sizeof(dev));

	/* We're only interested in connectable events */
	if (type == BT_HCI_ADV_IND || type == BT_HCI_ADV_DIRECT_IND)
	{	
		//If correct type, parse the data for the UUID
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

/**
 * Active scanning, disable duplicate filtering
*/
static int scan_start(void)
{
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	return bt_le_scan_start(&scan_param, device_found);
}

/**
 * Bluetooth device disconnected callback
*/
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;
	//Retrieve address string
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	if (default_conn != conn)
	{
		return;
	}
	//Unreference the default connection.
	bt_conn_unref(default_conn);
	default_conn = NULL;

	//Restart scanning
	err = scan_start();
	if (err)
	{
		printk("Scanning failed to start (err %d)\n", err);
	}
}

/* Assigning callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/**
 * @brief Callback function after GATT has been written to by BLU
 *
 * @param conn handler
 * @param attr Attribute data
 * @param buf Buffer storing val
 * @param len Length of data
 * @param flags Flags
 * @return Number of bytes written, or negative values if error.
 */
static ssize_t recv_from_ahu(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
							 uint16_t len, uint16_t err, uint8_t flags)
{
	//Copy the buffer to the receiving packet
	memcpy(recv_node_packet, buf, len);
	//Pass values to be converted to JSON in required format - and then printed to serial
	printk("Data is %f:%f:%f:%f:%f:%f\n", recv_node_packet->accelX, recv_node_packet->accelY, recv_node_packet->accelZ, 
	recv_node_packet->gyroX, recv_node_packet->gyroY, recv_node_packet->gyroZ);
	// print_json(recv_node_packet->rssiVals);
	//Return number of bytes written
	return len;
}

/**
 * @brief Construct a new bt gatt service define object.
 * Defines AHU's GATT for SCU to write to.
 */
BT_GATT_SERVICE_DEFINE(blu_svc, BT_GATT_PRIMARY_SERVICE(BLU_UUID),
					   BT_GATT_CHARACTERISTIC(BLU_CHRC_UUID, BT_GATT_CHRC_WRITE,
											  BT_GATT_PERM_WRITE, NULL, 
											  recv_from_ahu, NULL), );

/**
 * @brief Initialise the static nodes with required data and then add them to a doubly-linked list
 *
 * @param static_node struct to assign all data of given node to.
 * @param name Name of node
 * @param mac_address MAC Address of node
 * @param major Major number of node
 * @param minor Minor number of node
 * @param x_pos X position of node
 * @param y_pos Y position of node
 * @param r_neighbour Name of right neighbour
 * @param l_neighbour Name of left neighbour
 * @return Number of bytes written, or negative values if error.
 */
void static_node_init(struct static_node* static_node, char* name, char mac_address[20], uint16_t major, 
		uint16_t minor, int8_t x_pos, int8_t y_pos, char* r_neighbour, char* l_neighbour) {
	//Initialise all base variables of node.
	strcpy(static_node->name, name);
	strcpy(static_node->mac_address, mac_address);
	static_node->major = major;
	static_node->minor = minor;
	static_node->x_pos = x_pos;
	static_node->y_pos = y_pos;
	strcpy(static_node->l_neighbour, l_neighbour);
	strcpy(static_node->r_neighbour, r_neighbour);
	//Initialise struct as a double node - required for adding to a linkedl ist.
	sys_dnode_init(&static_node->node);
	//Append the struct to the end of the linked list - this is done sequentially.
	sys_dlist_append(&LinkedList, &static_node->node);
}

/**
 * @brief Remove a node from grid readings.
 * 		  Will need to be replaced with another node in same position for accurate readings.
 * @param name Name of node to be removed
 */
void remove_node(char name[6]) {
	//Loop through list to find node with given name
	for (int i = 0; i < 12; i++) {
		if (strcmp(name, nodes_list[i].name) == 0) {
			//Remove node from an active state by setting its x and y to -1
			nodes_list[i].x_pos = -1;
			nodes_list[i].y_pos = -1;
			//Break from for loop if correct node has been found.
			break;
		}
	}
}

/**
 * @brief Add a node to be used for readings.
 * @param name Name of node to be removed
 * @param newXPos new X position to be updated to - must be one of the set grid locations provided.
 * @param newYPos new Y position to be updated to - must be one of the set grid locations provided.
 */
void add_node(char name[6], int newXPos, int newYPos) {
	//Loop through list to find node with given name
	for (int i = 0; i < 12; i++) {
		if (strcmp(name, nodes_list[i].name) == 0) {
			//Update nodes x and y position
			nodes_list[i].x_pos = newXPos;
			nodes_list[i].y_pos = newYPos;
			//Break from for loop if correct node has been found.
			break;
		}
	}
}

/**
 * @brief Print node details of a specific node
 * @param name Name of node to display details of.
 */
void node_details(char name[6]) {
	//Loop through list to find node with given name
	for (int i = 0; i < 12; i++) {
		if (strcmp(name, nodes_list[i].name) == 0) {
			//Print relevant details.
			printk("Name: %s\r\nMAC Address: %s\r\n", nodes_list[i].name, nodes_list[i].mac_address);
			if (nodes_list[i].x_pos < 0) {
				printk("Status: Disconnected\r\n");
			} else {
				printk("Status: Assigned to position %i,%i\r\n", nodes_list[i].x_pos, nodes_list[i].y_pos);
			}
			return;
		}
	}
	//If node does not exist, print an error
	printk("ERROR: That node does not exist.\r\n");
}

/**
 * @brief Print node details of all nodes
 */
void node_details_all() {
	//Loop through all nodes and print the relevant details of each.
	for (int i = 0; i < 12; i++) {
		printk("%s, %s, ", nodes_list[i].name, nodes_list[i].mac_address);
		if (nodes_list[i].x_pos < 0) {
			printk("Status: Disconnected\r\n");
		} else {
			printk("Status: Assigned to position %i,%i\r\n", nodes_list[i].x_pos, nodes_list[i].y_pos);
		}
		k_msleep(10);
	}
}

/**
 * @brief Thread for running all bluetooth related processes
*/
void bsu_bt_thread(void)
{	
	int err;
	//Initialise LED
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	//Enable BT
	err = bt_enable(NULL);
	if (err) {

		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");
	int ret;
	//Check LED port is working.
	if (!device_is_ready(led.port)) {

		return;
	}
	//Configure led and toggle it on
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	ret = gpio_pin_toggle_dt(&led);

	if (ret < 0) {

		return;
	}
	
	//Start scanning for a BT connection
	err = scan_start();
	if (err) {

		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	//Initialise the linked list and add all nodes to said list.
	sys_dlist_init(&LinkedList);
	//Base Nodes
	static_node_init(&nodes_list[0], "4011-A", NODE_A_MAC, NODE_A_MAJOR, NODE_A_MINOR, NODE_A_X, NODE_A_Y, "4011-L", "4011-B");
	static_node_init(&nodes_list[1], "4011-B", NODE_B_MAC, NODE_B_MAJOR, NODE_B_MINOR, NODE_B_X, NODE_B_Y, "4011-A", "4011-C");
	static_node_init(&nodes_list[2], "4011-C", NODE_C_MAC, NODE_C_MAJOR, NODE_C_MINOR, NODE_C_X, NODE_C_Y, "4011-B", "4011-D");
	static_node_init(&nodes_list[3], "4011-D", NODE_D_MAC, NODE_D_MAJOR, NODE_D_MINOR, NODE_D_X, NODE_D_Y, "4011-C", "4011-E");
	static_node_init(&nodes_list[4], "4011-E", NODE_E_MAC, NODE_E_MAJOR, NODE_E_MINOR, NODE_E_X, NODE_E_Y, "4011-D", "4011-F");
	static_node_init(&nodes_list[5], "4011-F", NODE_F_MAC, NODE_F_MAJOR, NODE_F_MINOR, NODE_F_X, NODE_F_Y, "4011-E", "4011-G");
	static_node_init(&nodes_list[6], "4011-G", NODE_G_MAC, NODE_G_MAJOR, NODE_G_MINOR, NODE_G_X, NODE_G_Y, "4011-F", "4011-H");
	static_node_init(&nodes_list[7], "4011-H", NODE_H_MAC, NODE_H_MAJOR, NODE_H_MINOR, NODE_H_X, NODE_H_Y, "4011-G", "4011-I");
	//Additional Nodes
	static_node_init(&nodes_list[8], "4011-I", NODE_I_MAC, NODE_I_MAJOR, NODE_I_MINOR, NODE_UNASSIGNED_X, NODE_UNASSIGNED_Y, "4011-H", "4011-J");
	static_node_init(&nodes_list[9], "4011-J", NODE_J_MAC, NODE_J_MAJOR, NODE_J_MINOR, NODE_UNASSIGNED_X, NODE_UNASSIGNED_Y, "4011-I", "4011-K");
	static_node_init(&nodes_list[10], "4011-K", NODE_K_MAC, NODE_K_MAJOR, NODE_K_MINOR, NODE_UNASSIGNED_X, NODE_UNASSIGNED_Y, "4011-J", "4011-L");
	static_node_init(&nodes_list[11], "4011-L", NODE_L_MAC, NODE_L_MAJOR, NODE_L_MINOR, NODE_UNASSIGNED_X, NODE_UNASSIGNED_Y, "4011-K", "4011-A");
	while (1) {
		k_msleep(2000);
	}
}

/**
 * @brief Function for ordering the RSSI values into correct order based on their X and Y positions.
 * @param rssi_unprocessed_data RSSI values that have not yet been ordered correctly - currently alphabetical
*/
void print_json(int8_t rssi_unprocessed_data[12]) {
	//Initialise rssi data as array of 0's - if no node for a given point will stay as 0.
	int8_t rssi_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	//Define the order of nodes that the web is expecting
	//Will go through the arrays sequentially at same time.
	// I.E. (x,y) = (0,0) (2,0) (4,0) (4,2) (4,4) etc.
	int xOrder[8] = {0, 2, 4, 4, 4, 2, 0, 0};
	int yOrder[8] = {0, 0, 0, 2, 4, 4, 4, 2};
	//Loop through the amount of active nodes
	for (int j = 0; j < 8; j++) {
		//Loop through number of total nodes
		for (int i = 0; i < 12; i++) {
			//Check if current node is at wanted x,y position
			if ((nodes_list[i].x_pos == xOrder[j]) && (nodes_list[i].y_pos == yOrder[j])) {
				//If x,y position is correct, assign the rssi value of node to the output array
				rssi_data[j] = rssi_unprocessed_data[i];
				break;
			}
		}
	}
	//Print output rssi data in correct order, in the form of a JSON message. Will be picked up through serial and sent to web dashboard.
	printk("{\"node1\": %i, \"node2\": %i, \"node3\": %i, \"node4\": %i, \"node5\": %i, \"node6\": %i, \"node7\": %i, \"node8\": %i}\r\n", 
			rssi_data[0], rssi_data[1], rssi_data[2], rssi_data[3], rssi_data[4], rssi_data[5], rssi_data[6], rssi_data[7]);
}