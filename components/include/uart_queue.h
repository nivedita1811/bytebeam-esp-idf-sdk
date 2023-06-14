
#ifndef _UART_QUEUE_H_
#define _UART_QUEUE_H

#include "bytebeam.h"
#include "nwy_gpio.h"
#include "nwy_osi_api.h"
#include "nwy_sim.h"
#include "nwy_vir_at.h"
#include "osi_log.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"

#define UART_QUEUE_SIZE (4096*2)
#define SUCCESS 0
#define FAILED 1

typedef struct uart_queue_struct{
	uint32_t tail;
	uint32_t head;
	uint32_t full_status;
	uint32_t empty_status;
	uint8_t message[UART_QUEUE_SIZE];
}uart_queue_struct;

int32_t uart_rx_array_queue_put(volatile uint8_t queue_data_in);
int32_t uart_rx_array_queue_get(volatile uint8_t *queue_data_out);
uint32_t uart_rx_array_queue_size();

#endif