#ifndef UART_H
#define UART_H

// 需要 UART 类型定义
#include "driver/uart.h"

// 初始化UART（端口、TX、RX、波特率）
void uart_init(uart_port_t port, int tx_gpio, int rx_gpio, int baud_rate);

// 日志输出
void uart_log(const char *msg);

#endif
