#ifndef __COMMON_H__
#define __COMMON_H__

#include "nwy_gpio.h"
#include "nwy_osi_api.h"
#include "nwy_sim.h"
#include "nwy_vir_at.h"
#include "osi_log.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
#include "nwy_sms.h"
#include "nwy_data.h"
#include "nwy_usb_serial.h"
#include "nwy_adc.h"
#include "nwy_led.h"
#include "nwy_gpio_open.h"
#include "nwy_fota.h"
#include "nwy_fota_api.h"
#include "nwy_pm.h"
#include "nwy_adc.h"
#include "nwy_uart.h"
#include "nwy_network.h"
#include "nwy_file.h"
#include "nwy_socket.h"
#include "nwy_ble.h"
#include "nwy_ble_client.h"

typedef struct dataRecvContext
{
    unsigned size;
    unsigned pos;
    char data[0];
} dataRecvContext_t;
void nwy_ext_input_gets(char *msg, ...);
void nwy_ext_send_sig(nwy_osiThread_t *thread, uint16 sig);

#endif  // __COMMON_H__