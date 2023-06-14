#include "bytebeam.h"
#include "nwy_gpio.h"
#include "nwy_osi_api.h"
#include "nwy_sim.h"
#include "nwy_vir_at.h"
#include "osi_log.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "send_message_queue.h"

#define true TRUE
#define false FALSE

volatile send_message_struct rx_message_queue  = {0,} ;

volatile send_message_struct tx_message_queue  ={0,} ;


void message_queue_init()
{
	rx_message_queue.empty_status = true;
	rx_message_queue.full_status = false;
	rx_message_queue.head = rx_message_queue.tail = 0;
	memset(rx_message_queue.message, 0, sizeof(rx_message_queue.message));
}

int32_t message_rx_array_queue_put(volatile UART_data_struct *queue_data_in) {

	if (rx_message_queue.full_status == TRUE) {
		return MESSAGE_QUEUE_FAILED; /* Queue Full*/
	} else {
		memcpy(&rx_message_queue.message[rx_message_queue.tail], &queue_data_in, sizeof(send_message_struct));
		rx_message_queue.empty_status = FALSE;
	}

	if (rx_message_queue.tail == ((rx_message_queue.head - 1 + MESSAGE_QUEUE_SIZE) % MESSAGE_QUEUE_SIZE)) {

		rx_message_queue.full_status = TRUE;
		return MESSAGE_QUEUE_SUCCESS;
	} else {
		rx_message_queue.full_status = FALSE;
	}

	rx_message_queue.tail = (rx_message_queue.tail + 1) % MESSAGE_QUEUE_SIZE;
	return MESSAGE_QUEUE_SUCCESS; // No errors
}

int32_t message_tx_array_queue_put(volatile UART_data_struct *queue_data_in) {

	if (tx_message_queue.full_status == TRUE) {
		return MESSAGE_QUEUE_FAILED; /* Queue Full*/
	} else {
		memcpy(&tx_message_queue.message[tx_message_queue.tail], &queue_data_in, sizeof(send_message_struct));
		tx_message_queue.empty_status = FALSE;
	}

	if (tx_message_queue.tail == ((tx_message_queue.head - 1 + MESSAGE_QUEUE_SIZE) % MESSAGE_QUEUE_SIZE)) {

		tx_message_queue.full_status = TRUE;
		return MESSAGE_QUEUE_SUCCESS;
	} else {
		tx_message_queue.full_status = FALSE;
	}

	tx_message_queue.tail = (tx_message_queue.tail + 1) % MESSAGE_QUEUE_SIZE;
	return MESSAGE_QUEUE_FAILED; // No errors
}


int32_t message_rx_array_queue_get(volatile UART_data_struct *queue_data_out) {

	if (rx_message_queue.empty_status == TRUE) {
		return MESSAGE_QUEUE_FAILED;
	} else {
		memcpy(queue_data_out, &rx_message_queue.message[rx_message_queue.head], sizeof(send_message_struct));
		rx_message_queue.full_status = FALSE;
	}
	rx_message_queue.head = (rx_message_queue.head + 1) % MESSAGE_QUEUE_SIZE;
	if (rx_message_queue.tail == rx_message_queue.head)
	{
		rx_message_queue.empty_status = TRUE;
		rx_message_queue.full_status = FALSE;
		return MESSAGE_QUEUE_SUCCESS;
	} else {
		rx_message_queue.empty_status = FALSE;
	}
	return 0; // No errors
}
int32_t can_tx_array_queue_get(volatile UART_data_struct *queue_data_out) {

	if (tx_message_queue.empty_status == TRUE) {
		return MESSAGE_QUEUE_FAILED;
	} else {
		memcpy(queue_data_out, &tx_message_queue.message[tx_message_queue.head], sizeof(send_message_struct));
		rx_message_queue.full_status = FALSE;
	}
	tx_message_queue.head = (tx_message_queue.head + 1) % MESSAGE_QUEUE_SIZE;
	if (tx_message_queue.tail == tx_message_queue.head) {
		tx_message_queue.empty_status = TRUE;
		tx_message_queue.full_status = FALSE;
		return MESSAGE_QUEUE_SUCCESS;
	} else {
		tx_message_queue.empty_status = FALSE;
	}
	return 0; // No errors
}

uint32_t message_rx_array_queue_size() {
	if (rx_message_queue.head == rx_message_queue.tail) {
		return 0;
	}

	if (rx_message_queue.full_status == 1) {
		return MESSAGE_QUEUE_SIZE;
	}

	return rx_message_queue.tail > rx_message_queue.head ?
			rx_message_queue.tail - rx_message_queue.head :
			MESSAGE_QUEUE_SIZE - (rx_message_queue.head - rx_message_queue.tail);
}

uint32_t message_tx_array_queue_size() {
	if (tx_message_queue.head == tx_message_queue.tail) {
		return 0;
	}

	if (tx_message_queue.full_status == 1) {
		return MESSAGE_QUEUE_SIZE;
	}

	return tx_message_queue.tail > tx_message_queue.head ?
			tx_message_queue.tail - tx_message_queue.head :
			MESSAGE_QUEUE_SIZE - (tx_message_queue.head - tx_message_queue.tail);
}