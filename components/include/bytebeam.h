/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#ifndef _BYTEBEAM_H_
#define _BYTEBEAM_H_

#include <stdint.h>
#include <nwy_data.h>
#include "nwy_mqtt.h"
#include "mqtt_api.h"
#include "MQTTClient.h"
#include "json_server.h"
#include "nwy_osi_api.h"

#define ON  1
#define OFF 0

#define PUSH_IGNITION 0

#define STM_UART        NWY_NAME_UART2   // pin 69/70 please refer to pin definition guide



#define STATUSLED     0   //GIPO0
#define STATUSON      nwy_gpio_set_value(STATUSLED,ON)
#define STATUSOFF     nwy_gpio_set_value(STATUSLED,OFF)

#define NWY_EXT_SIO_RX_MAX          (12 * 1024)
#define NWY_EXT_INPUT_RECV_MSG       (NWY_APP_EVENT_ID_BASE + 1)
#define NWY_EXT_FOTA_DATA_REC_END      (NWY_APP_EVENT_ID_BASE + 2)
#define NWY_EXT_APPIMG_FOTA_DATA_REC_END	(NWY_APP_EVENT_ID_BASE + 3)

#define QUEUE_MSG_MQTT_PUBLISH_SIZE   (16U)
#define QUEUE_MSG_MQTT_PUBLISH_COUNT   (5U)
#define QUEUE_MSG_UART_TX_SIZE   (16U)
#define QUEUE_MSG_UART_TX_COUNT   (10U)
#define QUEUE_STORE_DATA_SIZE	(10U)

#define NWY_HTTP_SERVER_TRIG (NWY_APP_EVENT_ID_BASE + 5)
#define HTTP_TRIGGER nwy_ext_send_sig(ble_app_thread_handle, NWY_HTTP_SERVER_TRIG)

typedef struct __attribute__((packed)) UART_data_struct
{
    uint16_t stx;
    uint8_t  cmd;
    uint16_t length_u16;
	uint8_t  payload[(300*15)+10];
	uint16_t crc_u16;
    uint64_t timestamp;
    uint8_t  etx;
}UART_data_struct;


extern MQTTPacket_connectData data;
extern MQTTClient paho_mqtt_client;
extern unsigned char *g_nwy_paho_readbuf;
extern unsigned char *g_nwy_paho_writebuf;
extern nwy_paho_mqtt_at_param_type paho_mqtt_at_param;

extern char nwy_ext_sio_recv_buff[NWY_EXT_SIO_RX_MAX + 1];
extern bool http_flag;
extern int STM_UART_fd;
extern void power_supply_check();
extern bool sim_check(void);
extern bool data_enable();
extern void device_check();
extern bool network_check();
extern void uart_recv_handle (const char *str,uint32_t length);
extern void  nwy_pull_out_sim();

void Network_Init(void);
void nwy_ext_init_unsol_reg();  
void nwy_ext_echo(char *fmt, ...);
extern void nwy_uart_printf(const char *format, ...);
extern void uart_config(void);
extern int test_get_simid();
extern int test_set_simid();
extern int test_get_msisdn();
extern int test_get_imei();
extern void nwy_sim_ready_handle();
extern int nwy_ext_check_data_connect();
extern void sms_recv();
extern int sms_del();
extern int sms_storage();
extern int sms_report_mode();
extern void sms_config();
extern void sms_read(int index);
void network_timer_cb(void);
bool bytebeam_MQTT_connect();   // TODO: SHould be included in MQTT.h
void nwy_ext_app_info_proc(const uint8_t *data, int length);
void nwy_ext_appimg_info_proc(const uint8_t *data, int length);
void nwy_ext_gprs_disconnect_cb(char *data);
void nwy_ext_sms_list_resp_cb(char *data);
// extern void bytebeam_gps_publish();
extern void gps_recieve();
extern void gps_configure();
extern double convertToDecimalDegrees(const char *latLon, const char *direction);


// Thread Functions
void MQTT_publish_ThreadEntry(void *param); // MQTT Publish Thread - TODO: Should be included in MQTT.h
void MQTT_subscribe_ThreadEntry(void *param); // MQTT Subscribe Thread - TODO: Should be included in MQTT.h
void gpsThreadEntry(void *param);
void UART_tx_ThreadEntry(void *param);
void UART_rx_ThreadEntry(void *param);
void Network_ThreadEntry(void *param);
void store_data_ThreadEntry(void *param);
void read_data_ThreadEntry(void *param);
void store_data_ThreadEntry(void *param);
void tork_app_update(void *param);
void s32_update_app(void *param);

extern nwy_osiTimer_t *network_timer;
extern nwy_osiMessageQueue_t *mqtt_publish_msg_queue;
extern nwy_osiMessageQueue_t *uart_tx_msg_queue;
extern nwy_osiMessageQueue_t *store_queue;
extern nwy_osiSemaphore_t *file_semaphore;
extern nwy_osiSemaphore_t *mqtt_connect_semaphore;
extern nwy_osiSemaphore_t *uart_tx_semaphore;
extern nwy_osiSemaphore_t *tork_update_semaphore;
extern nwy_osiSemaphore_t *s32_update_semaphore;

extern nwy_osiThread_t *network_app_thread;
extern nwy_osiThread_t *mqtt_publish_thread;
extern nwy_osiThread_t *gps_app_thread;
extern nwy_osiThread_t *ble_app_thread_handle;
extern nwy_osiThread_t *s32_update_thread;
extern nwy_osiThread_t *read_data_thread;
extern nwy_osiThread_t *device_config_thread_handle;

extern bool store_msg_flag;

// #define NUM_MESSAGES_IN_MQTT_BATCH 300
#define NUM_MESSAGES_IN_MQTT_BATCH  125     //100               //125             //150
#define MQTT_BATCH_ELEMENT_SIZE 200
#define MQTT_BATCH_SIZE (MQTT_BATCH_ELEMENT_SIZE*NUM_MESSAGES_IN_MQTT_BATCH)






#endif
