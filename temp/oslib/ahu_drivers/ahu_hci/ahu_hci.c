#include "ahu_hci.h"

#define UART_DEVICE_NODE DT_NODELABEL(uart1)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define MSG_SIZE_SHELL_2_HCI sizeof(struct shell2hci_package)
#define MSG_SIZE_HCI_2_SHELL sizeof(struct hci2shell_package)
#define MSG_SIZE_BT_2_HCI sizeof(struct shell2hci_package)
#define MSG_SIZE_HCI_2_BT sizeof(struct hci2shell_package)
#define MSG_SIZE 32

#define NONE 0
#define SHELL 1
#define BT 2

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(shell2hci, MSG_SIZE_SHELL_2_HCI, 100, 4);
K_MSGQ_DEFINE(hci2shell, MSG_SIZE_HCI_2_SHELL, 100, 4);

K_MSGQ_DEFINE(hci2bt, MSG_SIZE_HCI_2_BT, 100, 4);
K_MSGQ_DEFINE(bt2hci, MSG_SIZE_BT_2_HCI, 100, 4);

K_MUTEX_DEFINE(ahuCommsMutex);

// /* Mutex for request type, either bluetooth or ahu shell */
// K_MUTEX_DEFINE(ahuMutex);
/* Mode that the request type is, either BT or SHELL. */
int ahuMode; 



/* receive buffer used in UART ISR callback */
static unsigned char rx_buf[MSG_SIZE];
static int rx_buf_pos;

void serialize_send(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
void test_send(void);

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void uart_interrupt_callback(const struct device *dev, void *user_data)
{
	struct hci2shell_package outgoing_package;

	uint8_t c;

	// Check if dev interrupt update
	if (!uart_irq_update(uart_dev)) {
		return;
	}

	// Check if rx ready
	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
	printk("Size of buffer: %i\r\n", sizeof(rx_buf));
	printk("MSGSize: %i\r\n", MSG_SIZE);
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((rx_buf_pos == 0) && (c == 0x00)) {
			//Ignore any junk data at front of message
			continue;
		}

		if ((c == 0xFA) && rx_buf_pos > 0) {
			/* terminate string */
			rx_buf[rx_buf_pos] = '\0';

			for (int i = 0; i < 4; i++) {
				printk("%i: %x\r\n", i, rx_buf[i]);
			}

			//Retrieve DID and sensor readings
			outgoing_package.device_id = rx_buf[2];
			outgoing_package.value1 = (rx_buf[6] << 24) + (rx_buf[5] << 16) + (rx_buf[4] << 8) + (rx_buf[3]);
			outgoing_package.value2 = (rx_buf[10] << 24) + (rx_buf[9] << 16) + (rx_buf[8] << 8) + (rx_buf[7]);

			//Number of data bytes
			int len = ((0x0F & rx_buf[0])); 
			
			//If len indicates that more than one reading is being recieved, then process the extra readings.
			//If more than one reading, then it will always be 3 readings (X,Y,Z etc.)
			if (len > ((sizeof(outgoing_package.value1) * 2) + sizeof(outgoing_package.device_id))) {
				//Multiple sensor values to process.
				outgoing_package.value3 = (rx_buf[14] << 24) + (rx_buf[13] << 16) + (rx_buf[12] << 8) + (rx_buf[11]);
				outgoing_package.value4 = (rx_buf[18] << 24) + (rx_buf[17] << 16) + (rx_buf[16] << 8) + (rx_buf[15]);
				outgoing_package.value5 = (rx_buf[22] << 24) + (rx_buf[21] << 16) + (rx_buf[20] << 8) + (rx_buf[19]);
				outgoing_package.value6 = (rx_buf[26] << 24) + (rx_buf[25] << 16) + (rx_buf[24] << 8) + (rx_buf[23]);
				
				printk("Received ValsY are %i.%i\r\n", outgoing_package.value3, outgoing_package.value4);
				printk("Received ValsZ are %i.%i\r\n", outgoing_package.value5, outgoing_package.value6);
			}
        


			//Send response to the correct thread, either BT or Shell
			if (ahuMode == SHELL) {
				k_msgq_put(&hci2shell, &outgoing_package, K_NO_WAIT);
			} else if (ahuMode == BT) {
				k_msgq_put(&hci2bt, &outgoing_package, K_NO_WAIT);
			}
			//Reset ahuMode for next message
			ahuMode = NONE;

			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

/**
 * Thread for running the HCI UART interface.
*/
void ahu_hci_thread(void)
{
	struct shell2hci_package incoming_package;
	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return;
	}

	/* configure interrupt and callback to receive data */
	uart_irq_callback_user_data_set(uart_dev, uart_interrupt_callback, NULL);
	uart_irq_rx_enable(uart_dev);
	
	
	while(1) {

		/* wait on commands from shell for 500ms*/
		while (k_msgq_get(&shell2hci, &incoming_package, K_MSEC(100)) == 0) {
			while (ahuMode != NONE) {
				// Busy wait
			}
			ahuMode = SHELL;
			serialize_send(0xAA, 0x01, sizeof(incoming_package.device_id) * 2, incoming_package.device_id, incoming_package.option);
			//Mutex will unlock in the CB function when response is received
		}

		/* wait on commands from bluetooth for 500ms*/
		while (k_msgq_get(&bt2hci, &incoming_package, K_MSEC(100)) == 0) {
			printk("In send mode.");
			while (ahuMode != NONE) {
				//Busy wait
			}
			ahuMode = BT;
			serialize_send(0xAA, 0x01, sizeof(incoming_package.device_id) * 2, incoming_package.device_id, incoming_package.option);
			//Mutex will unlock in the CB function when response is received
		}

		k_sleep(K_MSEC(100));
	}
}

/**
 * Converts a series of arguments into a UART transmission.
*/
void serialize_send(uint8_t preamble, uint8_t type, uint8_t length, uint8_t DID, uint8_t option) {
	//Send messages in a series of bytes.
	//Send preamble byte
	uart_poll_out(uart_dev, preamble);
	//Convert 4 bit type and length to byte then send
	uint8_t tl = (type << 4 | (length & 0x0F));
	uart_poll_out(uart_dev, tl);
	//Send DID byte
	uart_poll_out(uart_dev, DID);
	//Send remaining option data, maximum of 8 bits.
	uart_poll_out(uart_dev, option);
	//Send terminating \n, signals end of transmission
	uart_poll_out(uart_dev, 0xFA);
}

/**
 * Send a test UART sample, designed to be used with TX-RX loopback.
*/
void test_send(void) {
	uart_poll_out(uart_dev, 0xAA);
	uint8_t tl = (0x02 << 4 | (8 & 0x0F));
	uart_poll_out(uart_dev, tl);
	uart_poll_out(uart_dev, 0x05);
	uart_poll_out(uart_dev, 0x0);
	uart_poll_out(uart_dev, 0x0);
	uart_poll_out(uart_dev, 0x00);
	uart_poll_out(uart_dev, 0x05);
	uart_poll_out(uart_dev, 0x0);
	uart_poll_out(uart_dev, 0x0);
	uart_poll_out(uart_dev, 0x00);
	uart_poll_out(uart_dev, '\n');
}