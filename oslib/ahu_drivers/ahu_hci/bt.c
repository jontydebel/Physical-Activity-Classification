/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/zephyr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include "bt.h"

//BT_DATA struct containing advertising data
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(AHU_UUID_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_DIS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static uint16_t blu_handle = 0; //Handle ID for BSU
static int ble_connected;		//BT connection status, 1 for on
uint32_t timeCurr, duration; 	//Current time value (ticks) and duration (uS). Duration will be Curr - Prev
uint32_t timePrev = 0;			//Prev time value - starts at 0 ticks
// Bluetooth connection and set-up structs
static struct bt_conn *default_conn;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;

// /* packet that holds all data from bt messages */
// static struct BT_Message *bt_recv_packet = &(struct BT_Message) {
// 	.preamble = 0,
// 	.type = 0x02,
// 	.length = 0,
// 	.data = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
// 	.did = 0
// };

//Semaphores and msgq used for sending and receiving data
K_SEM_DEFINE(bt_recv_sem, 0, 1);
K_SEM_DEFINE(bt_send_sem, 1, 2);
// K_MSGQ_DEFINE(bt_queue, sizeof(struct BT_Message), 10, 4);

/**
 * @brief Return the blu_handle UUID
 */
int get_blu_handle() {
	return blu_handle;
}

/**
 * @brief Return 1 if bluetooth connect, 0 if not
 */
int get_ble_connected() {
	return ble_connected;
}

/**
 * @brief Callback function for sending to BSU
 */
static void blu_send_cb(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_write_params *params)
{
	// printk("Cb\r\n");
	//Check write success
	if (err != BT_ATT_ERR_SUCCESS) {
		printk("Write failed: 0x%02X\n", err);
	}
	printk("Cb fine\r\n");
	k_sem_give(&bt_send_sem);
	// send_ready = 1;

	(void)memset(params, 0, sizeof(*params));

}

/**
 * @brief Send given packet to BSU
 * @param handle handler pointing to the blu handle
 * @param packet pointer to struct that contains data to be sent.
 */
void blu_send(uint16_t handle, struct BT_Message *packet)
{	
	static struct bt_gatt_write_params write_params;
	int err;

	//Check the passed handle is the correct connection.
	if (handle == blu_handle) {
		//Set the write_params struct with the given packet
		write_params.data = packet;
		write_params.length = sizeof(*packet);
		
	} 
	//Set callback function and handle for the write
	write_params.func = blu_send_cb;
	write_params.handle = handle;
	printk("SEND GATT_WRITE: BLE Connected: %i\r\n", ble_connected);
	//Write the data to the BSU
	err = bt_gatt_write(default_conn, &write_params);
	if (err != 0) {
		printk("bt_gatt_write failed: %d", err);
	}
}

/**
 * @brief Default discover_func, retrieves GATT characteristics
 */
static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;
	//Check if discover is complete
	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	printk("[ATTRIBUTE] handle %u\n", attr->handle);
	//Check parameters and ensure that the device being connected is the BSU
	if (params->type == BT_GATT_DISCOVER_PRIMARY &&
	    (!bt_uuid_cmp(params->uuid, BLU_UUID))) {
		printk("Found blu service\n");

		params->uuid = NULL;
		params->start_handle = attr->handle + 1;
		params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, params);
		if (err != 0) {
			printk("Discover failed (err %d)", err);
		}

		return BT_GATT_ITER_STOP;
	} else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		//Get specific characteristics and the blu_handle
		struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;
		//Compare the uuid and ensure it is what is expected
		if (bt_uuid_cmp(chrc->uuid, BLU_CHRC_UUID) == 0) {
			printk("Found blu chrc\n");
			blu_handle = chrc->value_handle;
		} 
	}

	return BT_GATT_ITER_STOP;
}

/**
 * @brief Basic BT connected func. Calls required functions to discover the attributes and characteristics on connection.
 */
static void connected(struct bt_conn *conn, uint8_t err)
{	
	//Check if connection suceeded or failed
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
		ble_connected = 0;
	} else {
		//If connected, set ble to connected and discover the parameters
		printk("Connected\n");
		// Set connection status to 'on' (1)
		ble_connected = 1;
		//TODO: CHECK WHAT THIS FUNCTION CALL DOES (POTENTIALLY CAUSING THE CRASH?)
		struct bt_le_conn_param *param = BT_LE_CONN_PARAM(6, 6, 0, 400);
		default_conn = conn;
		
		// static struct bt_gatt_discover_params discover_params;
		// static struct bt_uuid *blu_uuid_ptr = BLU_UUID;
		// Set all parameters for the discover_param struct
		int err;
		memcpy(&uuid, BLU_UUID, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;
		//Call gatt_discover to retrieve all characteristics
		err = bt_gatt_discover(default_conn, &discover_params);
		if (err != 0)
			printk("Discover failed(err %d)\n", err);
		printk("Discover complete\n");

		//If all good, check for parameter updates
		if (bt_conn_le_param_update(conn, param) < 0) {
			while (1) {
				printk("Connection Update Error\n");
				k_msleep(10);
			}
		}
	}
}

/**
 * @brief Sets ble_connected to 0 upon disconnect
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
	ble_connected = 0;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/**
 * @brief Starts advertising to bt.
 */
static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	//Start bluetooth advertising
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

/**
 * @brief Cancels pairing if auth fails.
 */
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

/**
 * @brief Callback function for when the device receives data over bluetooth
 * 
 * @param conn handler
 * @param attr Attribute data
 * @param buf Buffer storing val
 * @param len Length of data
 * @param err err info
 * @param flags Flags
 * @return Length of data received (bytes)
 */
static ssize_t recv_from_blu(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			    uint16_t len, uint16_t err, uint8_t flags) {

	// // Copy the data received over to the receive packet.
	// memcpy(bt_recv_packet, buf, len);

	// //Send packet to the bt processing thread.
	// if (k_msgq_put(&bt_queue, bt_recv_packet, K_NO_WAIT) != 0) {
	// 	//Msg buffer is full, silently ignore
	// }
	// //Give semaphore to bt processing thread, so it can access the message
	// k_sem_give(&bt_recv_sem);
	return len;
}

/* GATT SERVICE DEFINITION */ 
BT_GATT_SERVICE_DEFINE(hts_svc, BT_GATT_PRIMARY_SERVICE(AHU_UUID),
		       BT_GATT_CHARACTERISTIC(AHU_CHRC_UUID,
		       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
		       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ, NULL, recv_from_blu, NULL)
);

/**
 * @brief Initialises and starts device's blueooth - connects to BSU.
 */
void bt_init(void) {
	int err;
	
	//Enable bluetooth
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	//Start advertising and connect to correct device.
	bt_ready();
	bt_conn_auth_cb_register(&auth_cb_display);
}