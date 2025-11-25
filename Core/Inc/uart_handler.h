#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include "main.h"
#include "set_functions.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;
extern DMA_HandleTypeDef hdma_usart3_rx;

extern char uart_buffer[512];
extern volatile bool uart_data_ready;
extern char uart_rx_buffer[512];
extern volatile uint16_t uart_rx_length;
extern volatile bool uart_receiving;

void send_telemetry(void);
void send_status(void);
void process_received_data(void);
void uart_receive_init(void);
void uart_receive_start(void);
void uart_receive_stop(void);
void uart_idle_handler(UART_HandleTypeDef *huart);

// Callback redefinition
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);

#endif
