#include "bsu_shell_cmds.h"
#include "bsu_bt.h"
// #include "bsu_hci.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LED0_NODE DT_ALIAS(led0)

/**
 * Remove specified node from network, shell command
*/
static void remove_cmd(const struct shell *shell, size_t argc, char **argv) {
    remove_node(argv[argc - 1]);
}

/**
 * Add specified node to network at a specified X,Y position, shell command
*/
static void add_cmd(const struct shell *shell, size_t argc, char **argv) {
    add_node(argv[argc - 3], atoi(argv[argc - 2]), atoi(argv[argc - 1]));
}

/**
 * Display details for a specific node or all nodes with '-a'
*/
static void details_node(const struct shell *shell, size_t argc, char **argv) {
    if (strcmp(argv[argc-1], "-a") == 0) {
        node_details_all();
    } else {
        node_details(argv[argc - 1]);
    }
}

/**
 * Tester command
*/
static void cmd_test_print(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "aaaHello World!\r\n");
}

/**
 * Used for individual sensor requests
*/
static void cmd_ble_g(const struct shell *shell, size_t argc, char **argv) {

    int did = atoi(argv[argc - 1]);

//     if (did < 40 && did > 0) {
        
//         struct shell2hci_package outgoing;
//         outgoing.device_id = did;
//         outgoing.option = 0;
//         k_msgq_put(&shell2hci, &outgoing, K_NO_WAIT);
//     }
}

/**
 * Used for controlling continuous sampling
*/
static void cmd_ble_c(const struct shell *shell, size_t argc, char **argv) {

    // struct shell2hci_package outgoing;

    // if (!strcmp(argv[1], "s")) { //start continuous sampling
        
    //     outgoing.device_id = 30;
    //     outgoing.option = atoi(argv[2]);
    //     k_msgq_put(&shell2hci, &outgoing, K_NO_WAIT);

    // } else if (!strcmp(argv[1], "p")) { //stop continuous sampling

    //     outgoing.device_id = 31;
    //     outgoing.option = 0;
    //     k_msgq_put(&shell2hci, &outgoing, K_NO_WAIT);
    // }

}

/**
 * Initialise shell commands for BSU.
*/
void init_bsu_shell() {

    SHELL_STATIC_SUBCMD_SET_CREATE(print_ctrl,
        SHELL_CMD(f, NULL, "Test print.", cmd_test_print),
        SHELL_SUBCMD_SET_END
    );
    SHELL_CMD_REGISTER(print, &print_ctrl, "Print 'Hello World!'", NULL);

	SHELL_STATIC_SUBCMD_SET_CREATE(cmd_bluetooth,
        SHELL_CMD(g, NULL, "Usage: ble g <DID>", cmd_ble_g),
        SHELL_CMD(c, NULL, "Usage: ble c [s/p] <sample time>", cmd_ble_c),
        SHELL_SUBCMD_SET_END
    );
	SHELL_CMD_REGISTER(ble, &cmd_bluetooth, "Get a sensor reading via bluetooth.", NULL);

    SHELL_STATIC_SUBCMD_SET_CREATE(node_cmd,
        SHELL_CMD(remove, NULL, "Usage: node remove <name>", remove_cmd),
        SHELL_CMD(add, NULL, "Usage: node add <name> <X> <Y>", add_cmd),
        SHELL_CMD(details, NULL, "Usage: node details <name | -a>", details_node),
        SHELL_SUBCMD_SET_END
    );
	SHELL_CMD_REGISTER(node, &node_cmd, "Edit node configuration", NULL);
}

/**
 * Main thread for Base Station Unit's (BSU) command line interface implementation.
 */
void bsu_shell_thread(void) {

    static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

    init_bsu_shell();

    if (usb_enable(NULL)) {

		return;
	}

    while (1) {

		k_sleep(K_SECONDS(1));
		gpio_pin_toggle_dt(&led);
        k_msleep(100);
    }
}

