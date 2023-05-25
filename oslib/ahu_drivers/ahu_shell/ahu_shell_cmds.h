#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/kernel.h>
#include <time.h>

void init_ahu_shell(void);
void ahu_shell_thread(void);
void print_response(int32_t value1, int32_t value2);