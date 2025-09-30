#include "uart.h"
#include "driver/uart.h"
#include <string.h>

static uart_port_t S_UART_PORT = UART_NUM_1;
static int S_TX_GPIO = -1;
static int S_RX_GPIO = -1;
static int S_BAUD = 115200;
static const int BUF_SIZE = 1024;

// 初始化UART
void uart_init(uart_port_t port, int tx_gpio, int rx_gpio, int baud_rate) {
    S_UART_PORT = port;
    S_TX_GPIO = tx_gpio;
    S_RX_GPIO = rx_gpio;
    S_BAUD = baud_rate;

    uart_config_t uart_config = {
        .baud_rate = S_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(S_UART_PORT, &uart_config);
    uart_set_pin(S_UART_PORT, S_TX_GPIO, S_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(S_UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
}

// 输出日志
void uart_log(const char *msg) {
    uart_write_bytes(S_UART_PORT, msg, strlen(msg));
}
