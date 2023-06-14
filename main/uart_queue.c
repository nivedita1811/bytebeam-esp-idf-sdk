#include "bytebeam.h"
#include "nwy_gpio.h"
#include "nwy_osi_api.h"
#include "nwy_sim.h"
#include "nwy_vir_at.h"
#include "osi_log.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "uart_queue.h"

static volatile uart_queue_struct rx_uart_queue = { .tail = 0, .head = 0, .full_status = 0,
		.empty_status = true, .message[UART_QUEUE_SIZE - 1] = { 0, } , } ;

int32_t uart_rx_array_queue_put(volatile uint8_t queue_data_in) {

	if (rx_uart_queue.full_status == TRUE) {
		return FAILED; /* Queue Full*/
	} else {
        rx_uart_queue.message[rx_uart_queue.tail] = queue_data_in;
		rx_uart_queue.empty_status = FALSE;
	}

	if (rx_uart_queue.tail == ((rx_uart_queue.head - 1 + UART_QUEUE_SIZE) % UART_QUEUE_SIZE)) {

		rx_uart_queue.full_status = TRUE;
		return SUCCESS;
	} else {
		rx_uart_queue.full_status = FALSE;
	}

	rx_uart_queue.tail = (rx_uart_queue.tail + 1) % UART_QUEUE_SIZE;
	return SUCCESS; // No errors
}

int32_t uart_rx_array_queue_get(volatile uint8_t *queue_data_out) {

	if (rx_uart_queue.empty_status == TRUE) {
		return FAILED;
	} else {
        *queue_data_out = rx_uart_queue.message[rx_uart_queue.head];
		rx_uart_queue.full_status = FALSE;
	}

	rx_uart_queue.head = (rx_uart_queue.head + 1) % UART_QUEUE_SIZE;
	
    if (rx_uart_queue.tail == rx_uart_queue.head) {
		rx_uart_queue.empty_status = TRUE;
		rx_uart_queue.full_status = FALSE;
		return SUCCESS;
	} else {
		rx_uart_queue.empty_status = FALSE;
    }

	return 0; // No errors
}

uint32_t uart_rx_array_queue_size() {
	if (rx_uart_queue.head == rx_uart_queue.tail) {
		return 0;
	}

	if (rx_uart_queue.full_status == 1) {
		return UART_QUEUE_SIZE;
	}

	return rx_uart_queue.tail > rx_uart_queue.head ?
			rx_uart_queue.tail - rx_uart_queue.head :
			UART_QUEUE_SIZE - (rx_uart_queue.head - rx_uart_queue.tail);
}
