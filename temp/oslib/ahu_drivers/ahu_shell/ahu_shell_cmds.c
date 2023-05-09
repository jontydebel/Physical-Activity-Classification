#include "ahu_shell_cmds.h"
#include "ahu_hci.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <zephyr/drivers/sensor.h>

/** 
* Sample command handler that prints "Hello World!" to the console.
*/
int most_recent_did = 0;

/**
 * Tester code.
*/
static void cmd_test_print(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "aaaHello World!\r\n");
}

/**
 * System time CLI function.
*/
static void system_time(const struct shell *shell, size_t argc, char **argv) {
	int divider = 1000;
    char* format = "sec";
	if (!strcmp(argv[argc-1], "minute")) {
		divider *= 60;
        format = "min";
	} else if (!strcmp(argv[argc-1], "hour")) {
        divider *= 60*60;
        format = "hr";
    } else if (!strcmp(argv[argc-1], "second")) {
        divider *= 1;
        format = "sec";
    }
	

	shell_print(shell, "%d %s\r\n", k_uptime_get_32()/divider, format);
}

/**
 * LSM6DSL sensor reading CLI function.
*/
static void lsm6dsl_read(const struct shell *shell, size_t argc, char **argv) {

    struct shell2hci_package package;
    package.option = 0;

    if (!strcmp(argv[argc-2], "a")) { // accelerometer

        if (!strcmp(argv[argc-1], "x")) { //X acceleration
            package.device_id = 3;
            k_msgq_put(&shell2hci, &package, K_NO_WAIT);
        } else if (!strcmp(argv[argc-1], "y")) { //Y acceleration
            package.device_id = 4;
            k_msgq_put(&shell2hci, &package, K_NO_WAIT);
        } else if (!strcmp(argv[argc-1], "z")) { //Z acceleration
            package.device_id = 5;
            k_msgq_put(&shell2hci, &package, K_NO_WAIT);
        }
    } else if (!strcmp(argv[argc-2], "g")) { // gyroscope

        if (!strcmp(argv[argc-1], "x")) { //X gyro
            package.device_id = 6;
            k_msgq_put(&shell2hci, &package, K_NO_WAIT);
        } else if (!strcmp(argv[argc-1], "y")) { //Y gyro
            package.device_id = 7;
            k_msgq_put(&shell2hci, &package, K_NO_WAIT);
        } else if (!strcmp(argv[argc-1], "z")) { //Z gyro
            package.device_id = 8;
            k_msgq_put(&shell2hci, &package, K_NO_WAIT);
        }
    }
    most_recent_did = package.device_id;
}

/**
 * LSP22 device sensor reading CLI function.
*/
static void lps22_read(const struct shell *shell, size_t argc, char **argv) {

    struct shell2hci_package package;
    package.option = 0;

    if (!strcmp(argv[argc-1], "t")) { //temperature
        package.device_id = 1;
        k_msgq_put(&shell2hci, &package, K_NO_WAIT);
    } else if (!strcmp(argv[argc-1], "p")) {
        package.device_id = 2;
        k_msgq_put(&shell2hci, &package, K_NO_WAIT);
    }
    most_recent_did = package.device_id;
}

/**
 * SCU LED write CLI function.
*/
static void led_write(const struct shell *shell, size_t argc, char **argv) {

    struct shell2hci_package package;

    if (!strcmp(argv[argc-2], "LD0") || !strcmp(argv[argc-2], "LD1")) { //LED0 or LED1
        if (!strcmp(argv[argc-2], "LD0")) {
            package.device_id = 12;
        } else if (!strcmp(argv[argc-2], "LD1")) {
            package.device_id = 13;
        }
        if (!strcmp(argv[argc-1], "o")) { //on
            package.option = 1;
            k_msgq_put(&shell2hci, &package, K_NO_WAIT);
        } else if (!strcmp(argv[argc-1], "f")) {
            package.option = 0;
            k_msgq_put(&shell2hci, &package, K_NO_WAIT);
        }
    }
    most_recent_did = package.device_id;
}

/**
 * Checks the current state of the SCU pushbutton.
*/
static void pushbutton_read(const struct shell *shell, size_t argc, char **argv) {

    struct shell2hci_package package;
    package.device_id = 14;
    package.option = 0;
    k_msgq_put(&shell2hci, &package, K_NO_WAIT);
    most_recent_did = package.device_id;
}

/**
 * Sets the SCU sampling rate.
*/
static void sample_write(const struct shell *shell, size_t argc, char **argv) {

    struct shell2hci_package package;
    package.device_id = 0;
    package.option = 0;

    if (argc == 2) {
        package.option = atoi(argv[argc-1]);
        k_msgq_put(&shell2hci, &package, K_NO_WAIT);
    }
    most_recent_did = package.device_id;
}

/**
 * IS3MDL device sensor reading CLI function.
*/
static void is3mdl_read(const struct shell *shell, size_t argc, char **argv) {

    struct shell2hci_package package;
    package.option = 0;

    if (!strcmp(argv[argc-1], "x")) { //X acceleration
        package.device_id = 9;
        k_msgq_put(&shell2hci, &package, K_NO_WAIT);
    } else if (!strcmp(argv[argc-1], "y")) { //Y acceleration
        package.device_id = 10;
        k_msgq_put(&shell2hci, &package, K_NO_WAIT);
    } else if (!strcmp(argv[argc-1], "z")) { //Z acceleration
        package.device_id = 11;
        k_msgq_put(&shell2hci, &package, K_NO_WAIT);
    }
    most_recent_did = package.device_id;
}

/**
 * Initialise shell for AHU.
*/
void init_ahu_shell() {
    SHELL_STATIC_SUBCMD_SET_CREATE(print_ctrl,
        SHELL_CMD(f, NULL, "Test print.", cmd_test_print),
        SHELL_SUBCMD_SET_END
    );
    SHELL_CMD_REGISTER(print, &print_ctrl, "Print 'Hello World!'", NULL);


	SHELL_STATIC_SUBCMD_SET_CREATE(sys_uptime,
        SHELL_CMD(f, NULL, "Display uptime - args: [hour][minute][second]", system_time),
        SHELL_SUBCMD_SET_END
    );
	SHELL_CMD_REGISTER(time, &sys_uptime, "Print the current system time.", NULL);


    SHELL_STATIC_SUBCMD_SET_CREATE(lsm6dsl_sensor,
        SHELL_CMD(r, NULL, "Display reading - args: [t][x][y][z]", lsm6dsl_read),
        SHELL_SUBCMD_SET_END
    );
	SHELL_CMD_REGISTER(lsm6dsl, &lsm6dsl_sensor, "Print a specified lsm6dsl sensor reading.", NULL);


    SHELL_STATIC_SUBCMD_SET_CREATE(lps22_sensor,
        SHELL_CMD(r, NULL, "Display pressure reading", lps22_read),
        SHELL_SUBCMD_SET_END
    );
	SHELL_CMD_REGISTER(lps22, &lps22_sensor, "Print the current pressure reading from lps22.", NULL);

    SHELL_STATIC_SUBCMD_SET_CREATE(led_w,
        SHELL_CMD(w, NULL, "Set LED state - provide LED and state: <LD0|LD1> <o|f>", led_write),
        SHELL_SUBCMD_SET_END
    );
	SHELL_CMD_REGISTER(led, &led_w, "Write to the SCU User LEDs.", NULL);

    SHELL_STATIC_SUBCMD_SET_CREATE(pushbutton,
        SHELL_CMD(r, NULL, "State reading", pushbutton_read),
        SHELL_SUBCMD_SET_END
    );
	SHELL_CMD_REGISTER(pb, &pushbutton, "Read the current pushhbutton state.", NULL);

    SHELL_STATIC_SUBCMD_SET_CREATE(samplerate,
        SHELL_CMD(w, NULL, "Write sampling time, args: [seconds]", sample_write),
        SHELL_SUBCMD_SET_END
    );
	SHELL_CMD_REGISTER(sample, &samplerate, "Write sampling time", NULL);

    SHELL_STATIC_SUBCMD_SET_CREATE(magneto,
        SHELL_CMD(r, NULL, "Reading", is3mdl_read),
        SHELL_SUBCMD_SET_END
    );
	SHELL_CMD_REGISTER(is3mdl, &magneto, "Get current magnetometer reading.", NULL);
}

/**
 * Produce corresponding response readings in shell.
*/
void print_response(int32_t value1, int32_t value2) {
    char* device_name;
    printk("Enter response: Vals are %i and %i\r\n", value1, value2);
    switch(most_recent_did) {
        case 0:
            printk("Set sample rate. \r\n");
            return;
            break;
        case 1:
            device_name = "LPS22 Temperature";
            break;
        case 2:
            device_name = "LPS22 Pressure";
            break;
        case 3:
            device_name = "LSM6DSL X Acceleration";
            break;
        case 4:
            device_name = "LSM6DSL Y Acceleration";
            break;
        case 5:
            device_name = "LSM6DSL Z Acceleration";
            break;
        case 6:
            device_name = "LSM6DSL X Gyroscope";
            break;
        case 7:
            device_name = "LSM6DSL Y Gyroscope";
            break;
        case 8:
            device_name = "LSM6DSL Z Gyroscope";
            break;
        case 9:
            device_name = "IS3MDL X Magnetometer";
            break;
        case 10:
            device_name = "IS3MDL Y Magnetometer";
            break;
        case 11:
            device_name = "IS3MDL Z Magnetometer";
            break;
        case 12:
            printk("Wrote to LED0\r\n");
            return;
            break;
        case 13:
            printk("Wrote to LED1\r\n");
            return;
            break;
        case 14:
            printk("Button state: %i\r\n", value1);
            return;
            break;
    }

    if (value2 < 0) {
        value2 = -1*value2;
        if (value1 == 0) {
            printk("%s : -%i.%i\r\n", device_name, value1, value2);
        } else {
            printk("%s : %i.%i\r\n", device_name, value1, value2);
        }
    } else {
        printk("%s : %i.%i\r\n", device_name, value1, value2);
    }
}

/**
 * @brief Main thread for Application Host Unit's (AHU) command line interface implementation.
 */
void ahu_shell_thread(void) {
    const struct device *dev;
    uint32_t dtr = 0;
    struct hci2shell_package incoming_package;

    dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
    if (!device_is_ready(dev)) {
        return;
    }

    init_ahu_shell();
    while (1) {
        while (k_msgq_get(&hci2shell, &incoming_package, K_FOREVER) == 0) {
			print_response(incoming_package.value1, incoming_package.value2);
		}
        k_msleep(1000);
    }
}

// K_THREAD_DEFINE(ahu_shell_thread_tid, AHU_SHELL_THREAD_STACK, ahu_shell_thread, NULL, NULL, NULL, AHU_SHELL_THREAD_PRIORITY, 0, 0);