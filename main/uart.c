#include "uart.h"
#include "driver/uart.h"
#include <string.h>  // 添加string.h头文件以支持strlen函数

#define UART_PORT UART_NUM_1
#define TX_GPIO 20
#define RX_GPIO 21
#define BUF_SIZE 1024

// 初始化UART1
void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, TX_GPIO, RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
}

// 输出日志
void uart_log(const char *msg) {
    uart_write_bytes(UART_PORT, msg, strlen(msg));
}