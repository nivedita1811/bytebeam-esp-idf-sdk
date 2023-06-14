#ifndef _SEND_MESSAGE_QUEUE_H_
#define _SEND_MESSAGE_QUEUE_H

#include "bytebeam.h"
#include "nwy_gpio.h"
#include "nwy_osi_api.h"
#include "nwy_sim.h"
#include "nwy_vir_at.h"
#include "osi_log.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"

//#define  (4096*2)
#define SUCCESS 0
#define FAILED 1

#define MESSAGE_QUEUE_FAILED FAILED

#define MESSAGE_QUEUE_SUCCESS SUCCESS

#define MESSAGE_QUEUE_SIZE 3

typedef struct send_message_struct{
	uint32_t tail;
	uint32_t head;
	uint32_t full_status;
	uint32_t empty_status;
    UART_data_struct message[30];
}send_message_struct;

int32_t message_rx_array_queue_put(volatile UART_data_struct *queue_data_in);
int32_t message_rx_array_queue_get(volatile UART_data_struct *queue_data_out);
uint32_t message_rx_array_queue_size();

#endif