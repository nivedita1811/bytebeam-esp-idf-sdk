#include "bytebeam.h"
#include "nwy_gpio.h"
#include "nwy_osi_api.h"
#include "nwy_sim.h"
#include "nwy_vir_at.h"
#include "osi_log.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "nwy_sms.h"
#include "nwy_data.h"
#include "nwy_uart.h"
#include "nwy_usb_serial.h"
#include "nwy_adc.h"
#include "nwy_led.h"
#include "nwy_gpio_open.h"
#include "nwy_fota.h"
#include "nwy_fota_api.h"
#include "nwy_pm.h"
#include "stdio.h"
//#include "nwy_network.h"
#include "nwy_file.h"
//#include "nwy_socket.h"
#include "json_server.h"
#include "uart_queue.h"
#include "bytebeam_uart.h"
#include <time.h>
#include "cJSON.h"

uint8_t json_can_message_buff_gau8[JSON_CAN_MESSAGES_BUFF_LEN] = {
    0,
};

char can_mess_json[] = "{"
                       "\"timestamp\": %llu,"
                       "\"can_id\": %d,"
                       "\"byte1\": %d,"
                       "\"byte2\": %d,"
                       "\"byte3\": %d,"
                       "\"byte4\": %d,"
                       "\"byte5\": %d,"
                       "\"byte6\": %d,"
                       "\"byte7\": %d,"
                       "\"byte8\": %d,"
                       "\"sequence\":%d"
                       "}";

uint8_t json_pack_can_message(JSMN_JSON_CAN_MESSAGE_tst *can_mess_pst)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    double s = tv.tv_sec;
    double ms = ((double)tv.tv_usec) / 1.0e3;
    unsigned long long timestamp1 = (unsigned long long)(s*1000 + ms);
    static uint32_t seq_id = 0;
    seq_id++;
    memset(json_can_message_buff_gau8, 0, JSON_CAN_MESSAGES_BUFF_LEN);
    snprintf((char *)&json_can_message_buff_gau8, sizeof(json_can_message_buff_gau8), can_mess_json,
             timestamp1,
             can_mess_pst->can_id,
             can_mess_pst->byte1,
             can_mess_pst->byte2,
             can_mess_pst->byte3,
             can_mess_pst->byte4,
             can_mess_pst->byte5,
             can_mess_pst->byte6,
             can_mess_pst->byte7,
             can_mess_pst->byte8,
             seq_id);
    return 0;
}

int32_t json_pack_array(UART_data_struct *uart_strcut)
{
    for(int i = 0; i < uart_strcut->length_u16/14; i++) {
        static uint16_t json_package_count = 0;
        static uint8_t can_pub_array_json[MQTT_BATCH_SIZE] = {0,};
        uint32_t src_len = 0;
        uint8_t *p = uart_strcut->pay_load + i*14;
        memset((void *)&JSMN_JSON_CAN_MESSAGE_st_obj, 0, sizeof(JSMN_JSON_CAN_MESSAGE_tst));
        memcpy(&JSMN_JSON_CAN_MESSAGE_st_obj, p, 14);
    }
}

#if 0
int32_t serialize(UART_data_struct uart_strcut)
{
    static uint64_t sq_num = 0;
	cJSON *can_data_json_list = NULL;
	cJSON *can_data_json = NULL;
	cJSON *seq_id_json = NULL;
	cJSON *timestamp_json = NULL;
	cJSON *device_status_json = NULL;
	char *string_json = NULL;
	
    
	can_data_json_list = cJSON_CreateArray();
    if (can_data_json_list == NULL)
	{
		//goto end;
		nwy_ext_echo("Json Init failed.\r\n");
	} 


    for(int i = 0; i < uart_strcut.length_u16/14; i++)
    {
        sq_num++;
        can_data_json = cJSON_CreateObject();
        if (can_data_json == NULL)
        {
            nwy_ext_echo("Json add failed.\r\n");
        }

        timestamp_json = cJSON_CreateNumber(uart_strcut.timestamp);
        if (timestamp_json == NULL)
        {
            nwy_ext_echo("Json add time stamp failed.\r\n");
        }

        cJSON_AddItemToObject(can_data_json, "timestamp", timestamp_json);

        seq_id_json = cJSON_CreateNumber(sq_num);
        if (seq_id_json == NULL)
        {
            nwy_ext_echo("Json add time stamp failed.\r\n");
        }
        cJSON_AddItemToObject(can_data_json, "sequence", seq_id_json);




    }

}

#endif