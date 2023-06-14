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
#include "nwy_usb_serial.h"
#include "nwy_adc.h"
#include "nwy_led.h"
#include "nwy_gpio_open.h"
#include "nwy_fota.h"
#include "nwy_fota_api.h"
#include "nwy_pm.h"
#include "stdio.h"
#include "nwy_network.h"
#include "nwy_file.h"
#include "nwy_socket.h"
#include "json_server.h"
#include "mqtt_api.h"
#include "MQTTClient.h"
#include "nwy_mqtt.h"
#include "miniz.h"
#include "bytebeam_actions.h"
#include "bytebeam_nwy_sdk.h"

extern char device_id_global[5];
extern char project_id_global[25];

extern int http_download_flag;

char CAN_TOPIC[80] = {0};
char GPS_TOPIC[80] = {0};
char SUB_TOPIC[80] = {0};
char ACTION_TOPIC[80] ={0};
char SOS_TOPIC[80] ={0};
char IGN_TOPIC[80] ={0};
char DEV_SHADOW_TOPIC[80] = {0};

// char CAN_TOPIC[] =    "/tenants/pradeeptest/devices/6/events/c2c_can/jsonarray/zlibdeflate";
// char GPS_TOPIC[] =    "/tenants/pradeeptest/devices/6/events/c2c_gps/jsonarray";
// char SUB_TOPIC[] =    "/tenants/pradeeptest/devices/6/actions";
// char ACTION_TOPIC[] = "/tenants/pradeeptest/devices/6/action/status";
// char SOS_TOPIC[] =    "/tenants/pradeeptest/devices/6/events/sos/jsonarray";
// char IGN_TOPIC[] = "/tenants/pradeeptest/devices/6/events/ignition/jsonarray";

// Variable Initialisation
MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
MQTTClient paho_mqtt_client;
unsigned char *g_nwy_paho_readbuf = NULL;
unsigned char *g_nwy_paho_writebuf = NULL;
Network *paho_network = NULL;
#define NWY_EXT_SIO_PER_LEN 1024
char echo_buff[NWY_EXT_SIO_PER_LEN + 1] = {0};
nwy_paho_mqtt_at_param_type paho_mqtt_at_param = {0};
nwy_osiMutex_t *ext_mutex = NULL;  //??

UART_data_struct uart_strcut;
MQTTPacket_connectData *options;
MQTTMessage pubmsg = {0};

extern device_config bb_device_config;
bool get_MQTT_COnnection_Status(void);
// Function Definitions

void mqtt_topics_init()
{
    sprintf(CAN_TOPIC, "/%s/%s/%s/%s/%s" ,"tenants",project_id_global,"devices",device_id_global,"events/c2c_can/jsonarray/zlibdeflate");
    sprintf(GPS_TOPIC, "/%s/%s/%s/%s/%s" ,"tenants",project_id_global,"devices",device_id_global, "events/c2c_gps/jsonarray");
    sprintf(SUB_TOPIC, "/%s/%s/%s/%s/%s" ,"tenants",project_id_global,"devices",device_id_global, "actions");
    sprintf(ACTION_TOPIC, "/%s/%s/%s/%s/%s" ,"tenants",project_id_global,"devices",device_id_global, "action/status");
    sprintf(SOS_TOPIC, "/%s/%s/%s/%s/%s" ,"tenants",project_id_global,"devices",device_id_global, "events/sos/jsonarray");
    sprintf(IGN_TOPIC, "/%s/%s/%s/%s/%s" ,"tenants",project_id_global,"devices",device_id_global, "events/ignition/jsonarray");
    sprintf(DEV_SHADOW_TOPIC, "/%s/%s/%s/%s/%s", "tenants",project_id_global,"devices",device_id_global, "events/device_shadow/jsonarray");
}

void messageArrived(MessageData *md)
{
    char topic_name[PAHO_TOPIC_LEN_MAX] = {0};
    int i = 0;
    unsigned int remain_len = 0;
    // MQTTUnsubscribe(&paho_mqtt_client, SUB_TOPIC);                     //added and commented, temp changes at tork
    strncpy(topic_name, md->topicName->lenstring.data, md->topicName->lenstring.len);
    // nwy_lock_mutex(ext_mutex, 0);
    nwy_ext_echo("\r\n===messageArrived======");
    nwy_ext_echo("\r\npayloade len is %d", md->message->payloadlen);
    nwy_ext_echo("\r\n%s:\r\n", topic_name);
    remain_len = md->message->payloadlen;
    if (md->message->payloadlen > NWY_EXT_SIO_PER_LEN)
    {
        nwy_ext_echo("len is greater than what we can process\r\n");
        // for (i = 0; i < ((md->message->payloadlen / NWY_EXT_SIO_PER_LEN) + 1); i++)
        // {
        //     memset(echo_buff, 0, sizeof(echo_buff));
        //     strncpy(echo_buff, md->message->payload + i * NWY_EXT_SIO_PER_LEN,
        //             remain_len > NWY_EXT_SIO_PER_LEN ? NWY_EXT_SIO_PER_LEN : remain_len);
        //     remain_len = md->message->payloadlen - (i + 1) * NWY_EXT_SIO_PER_LEN;
        //     if(strlen(echo_buff) > 0 && strlen(echo_buff) < NWY_EXT_SIO_PER_LEN)
        //     {
        //         nwy_ext_echo("%s", echo_buff);
        //          handle_actions(echo_buff);
        //     }
            
        //    // nwy_ext_echo(echo_buff);
        //     // parse_object(json_url);
           
        // }
    }
    else
    {
        nwy_ext_echo("else \r\n");
        //if(strlen(echo_buff) > 0 && strlen(echo_buff) < NWY_EXT_SIO_PER_LEN)
        //{
            //nwy_ext_echo("%s", echo_buff);
            memset(echo_buff, 0, sizeof(echo_buff));
            strncpy(echo_buff, md->message->payload, md->message->payloadlen);
            nwy_ext_echo(echo_buff);
            if(http_download_flag == 0)
                handle_actions(echo_buff);
        //}

       
    }
    // nwy_unlock_mutex(ext_mutex);
}

void byteBeam_subscribe()
{
    int sub_rc = 0;
    if (MQTTIsConnected(&paho_mqtt_client))
    {
        memset(paho_mqtt_at_param.topic, 0, sizeof(paho_mqtt_at_param.topic));
        strncpy(paho_mqtt_at_param.topic, SUB_TOPIC, sizeof(SUB_TOPIC));
        paho_mqtt_at_param.qos = 1;
        sub_rc = MQTTSubscribe(&paho_mqtt_client,
                               (char *)paho_mqtt_at_param.topic,
                               paho_mqtt_at_param.qos,
                               messageArrived);
        if (sub_rc == SUCCESS)
        {
            nwy_ext_echo("\r\nMQTT Sub ok");
        }
        else
            nwy_ext_echo("\r\nMQTT Sub error:%d", sub_rc);
    }
    else
        nwy_ext_echo("\r\nMQTT no connect");
}

bool bytebeam_MQTT_connect()
{
    char *sptr = nwy_ext_sio_recv_buff;
    int ssl_mode = 0, auth_mode, qos, retained;
    int rc,i, ret;
    if (MQTTIsConnected(&paho_mqtt_client) == 1)
    {
        nwy_ext_echo("\r\npaho mqtt already connect");
        return true;
    }
    memset(&paho_mqtt_at_param, 0, sizeof(nwy_paho_mqtt_at_param_type));
   // strncpy(paho_mqtt_at_param.host_name, "cloud.bytebeam.io", strlen("cloud.bytebeam.io"));
   strncpy(paho_mqtt_at_param.host_name, bb_device_config.broker_uri, strlen(bb_device_config.broker_uri));
   nwy_ext_echo("\r\nhost_name is %s\r\n", paho_mqtt_at_param.host_name);
    //paho_mqtt_at_param.port = 1883; // insecure
    paho_mqtt_at_param.port = bb_device_config.broker_port; // secure
    nwy_ext_echo("\r\nport is %d\r\n", paho_mqtt_at_param.port);

    paho_mqtt_at_param.port = 1883; // insecure for now for testing
    
    strncpy(paho_mqtt_at_param.clientID, bb_device_config.device_id, sizeof(bb_device_config.device_id));
    nwy_ext_echo("\r\nclientID is %s\r\n", paho_mqtt_at_param.clientID);
    paho_mqtt_at_param.paho_ssl_tcp_conf.sslmode = 0;
    if (g_nwy_paho_writebuf != NULL)
    {
        free(g_nwy_paho_writebuf);
        g_nwy_paho_writebuf = NULL;
    }
    if (NULL == (g_nwy_paho_writebuf = (unsigned char *)malloc(PAHO_PLAYOAD_LEN_MAX)))
    {
        nwy_ext_echo("\r\nmalloc buffer g_nwy_paho_writebuf error");
        return false;
    }
    if (g_nwy_paho_readbuf != NULL)
    {
        free(g_nwy_paho_readbuf);
        g_nwy_paho_readbuf = NULL;
    }
    if (NULL == (g_nwy_paho_readbuf = (unsigned char *)malloc(PAHO_PLAYOAD_LEN_MAX)))
    {
        nwy_ext_echo("\r\nmalloc buffer g_nwy_paho_readbuf error");
        return false;
    }
    memset(g_nwy_paho_readbuf, 0, sizeof(g_nwy_paho_readbuf));
    memset(g_nwy_paho_writebuf, 0, sizeof(g_nwy_paho_writebuf));
    if (paho_network != NULL)
    {
        free(paho_network);
        paho_network = NULL;
    }
    if (NULL == (paho_network = (unsigned char *)malloc(sizeof(Network))))
    {
        nwy_ext_echo("\r\nmalloc buffer paho_network error");
        return false;
    }
    memset(paho_network, 0, sizeof(Network));
    NetworkInit(paho_network);
    //  nwy_ext_echo("\r\nis no-SSL NetworkConnect");
    paho_mqtt_at_param.cleanflag = 1;
    paho_mqtt_at_param.keepalive = 60;
    nwy_ext_echo("\r\nip:%s, port :%d", paho_mqtt_at_param.host_name, paho_mqtt_at_param.port);
    rc = NetworkConnect(paho_network, paho_mqtt_at_param.host_name, paho_mqtt_at_param.port);
    if (rc < 0)
    {
        nwy_ext_echo("\r\nNetworkConnect err rc=%d", rc);
        return false;
    }
    nwy_ext_echo("\r\nNetworkConnect ok rc: %d", rc);

    MQTTClientInit(&paho_mqtt_client, paho_network, 10000, g_nwy_paho_writebuf, PAHO_PLAYOAD_LEN_MAX,
                    g_nwy_paho_readbuf, PAHO_PLAYOAD_LEN_MAX);
    MQTTClientInit_defaultMessage(&paho_mqtt_client, messageArrived);
    data.clientID.cstring = paho_mqtt_at_param.clientID;
    if (0 != strlen((char *)paho_mqtt_at_param.username) && 0 != strlen((char *)paho_mqtt_at_param.password))
    {
        data.username.cstring = paho_mqtt_at_param.username;
        data.password.cstring = paho_mqtt_at_param.password;
    }
    data.keepAliveInterval = paho_mqtt_at_param.keepalive;
    data.cleansession = paho_mqtt_at_param.cleanflag;
    if (0 != strlen((char *)paho_mqtt_at_param.willtopic))
    {
        memset(&data.will, 0x0, sizeof(data.will));
        data.willFlag = 1;
        data.will.retained = paho_mqtt_at_param.willretained;
        data.will.qos = paho_mqtt_at_param.willqos;
        if (paho_mqtt_at_param.willmessage_len != 0)
        {
            data.will.topicName.lenstring.data = paho_mqtt_at_param.willtopic;
            data.will.topicName.lenstring.len = strlen((char *)paho_mqtt_at_param.willtopic);
            data.will.message.lenstring.data = paho_mqtt_at_param.willmessage;
            data.will.message.lenstring.len = paho_mqtt_at_param.willmessage_len;
        }
        else
        {
            data.will.topicName.cstring = paho_mqtt_at_param.willtopic;
            data.will.message.cstring = paho_mqtt_at_param.willmessage;
        }
        nwy_ext_echo("\r\nMQTT will ready");
    }
    nwy_ext_echo("\r\nConnecting MQTT");
    rc = nwy_MQTTConnect(&paho_mqtt_client, &data);
    if (rc != 0)  // Check if 
    {
        nwy_ext_echo("\r\nFailed to create client, return code %d", rc);
        return false;
    }
    else
    {
        byteBeam_subscribe();
        nwy_ext_echo("\r\nMQTT connect ok");
        return true;
    }
}

int cmp_status;
uint32_t cmp_len;
//uint8_t *pCmp;
uint32_t successfull_publish = 0;
uint32_t failed_publish = 0;
unsigned long long timestamp_start= 0;

int32_t send_can_mqtt_pack(uint8_t *payload, int length, uint64_t timestamp)
{
  
    static uint16_t json_package_count = 0;
    static uint8_t can_pub_array_json[MQTT_BATCH_SIZE+1000] = {0,};
    uint32_t src_len = 0;
    struct timeval tv;
  //  nwy_ext_echo("\r\nbatching\r\n");
    memset((void *)&JSMN_JSON_CAN_MESSAGE_st_obj, 0, sizeof(JSMN_JSON_CAN_MESSAGE_tst));
    memcpy(&JSMN_JSON_CAN_MESSAGE_st_obj, payload, length);

	JSMN_JSON_CAN_MESSAGE_st_obj.sequence_id = (uint16_t)((payload[13] << 8) | payload[14]);
    JSMN_JSON_CAN_MESSAGE_st_obj.can_id = (uint32_t)((payload[1] << 24) | (payload[2] << 16) |
                                                                     (payload[3] << 8) | payload[4]);

    json_pack_can_message(&JSMN_JSON_CAN_MESSAGE_st_obj, timestamp);
    gettimeofday(&tv, NULL);
    double s = tv.tv_sec;
    double ms = ((double)tv.tv_usec) / 1.0e3;
    unsigned long long timestamp_end = (unsigned long long)(s*1000 + ms);
    //nwy_ext_echo("\r\nTime elapsed:%llu Success_count:%d, failed count:%d\r\n", timestamp_end-timestamp_start, successfull_publish*NUM_MESSAGES_IN_MQTT_BATCH, failed_publish);
    if((timestamp_end-timestamp_start) > 950)
    {
        nwy_ext_echo("\r\nTime elapsed:%llu Success_count:%d, failed count:%d\r\n", timestamp_end-timestamp_start, successfull_publish*NUM_MESSAGES_IN_MQTT_BATCH, failed_publish);
        successfull_publish = failed_publish = 0;
        timestamp_start = timestamp_end;
        nwy_ext_echo("The queue size is: %d", 30 - nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue));
    }

    if (json_package_count < NUM_MESSAGES_IN_MQTT_BATCH)
    {
     
        if (json_package_count == 0)
        {
            memset(can_pub_array_json,0,sizeof(can_pub_array_json));
            // nwy_ext_echo("\r\njsonarray is 0\r\n");
            can_pub_array_json[0] = '[';
        }
        else
        {
            strcat(can_pub_array_json, ",");
        }
        json_package_count++;
        strcat(can_pub_array_json, json_can_message_buff_gau8);
        src_len=strlen(can_pub_array_json);

       //nwy_ext_echo("\r\nJson size now is:%d\r\n", json_package_count);
    }
    else
    {
        json_package_count = 0;
        int iterator = 0 ;
        strcat(can_pub_array_json, ",");
        strcat(can_pub_array_json, json_can_message_buff_gau8);
        strcat(can_pub_array_json, "]\n");
        //nwy_ext_echo("\r\nJson size now is:%d\r\n", json_package_count);
      //  memcpy(uart_mqtt_publish.payload, &can_pub_array_json, uart_mqtt_publish.data_length+1);
//do compress
        // nwy_ext_echo("\r\nThe json before compression:\r\n\r\n");
        // while(can_pub_array_json[iterator] != NULL)
        // {
        //     nwy_ext_echo("%c", can_pub_array_json[iterator]);
        //     iterator++;
        // }
        // nwy_ext_echo("\r\n");
        uint64_t src_len = strlen(can_pub_array_json);
        uint64_t cmp_len = sizeof(paho_mqtt_at_param.message);
        uint64_t uncomp_len = src_len;
        cmp_status = compress(paho_mqtt_at_param.message, &cmp_len, (const unsigned char *)can_pub_array_json, src_len);
        
        if (cmp_status != Z_OK)
        {
            nwy_ext_echo("compress() after! %d %u %llu %llu\n", cmp_status, uart_strcut.length_u16, cmp_len, src_len);
            //nwy_ext_echo("\r\n%s\r\n", &pub_msg_queue.payload[pub_msg_queue.data_length - 1]);  
            return 1; 
        }
        else
        {
            if(cmp_len > sizeof(paho_mqtt_at_param.message))
            {
                nwy_ext_echo("Its tooo bigggg!!\r\n");
                return 2;
            }
        }      
       //do publish

       // nwy_ext_echo("%d, the data received is %s",pub_msg_queue.data_length, pub_msg_queue.payload);
        memset(paho_mqtt_at_param.topic, 0, sizeof(paho_mqtt_at_param.topic));
        strncpy(paho_mqtt_at_param.topic, CAN_TOPIC, strlen(CAN_TOPIC));
        paho_mqtt_at_param.qos = 1;
        paho_mqtt_at_param.retained = 0;
        
        memset(&pubmsg, 0, sizeof(pubmsg));
        pubmsg.payload = (void *)paho_mqtt_at_param.message;
        pubmsg.payloadlen = cmp_len;
        pubmsg.qos = paho_mqtt_at_param.qos;
        pubmsg.retained = paho_mqtt_at_param.retained;
        pubmsg.dup = 0;

        int rc = 0;
        do {
            nwy_ext_echo("trying to publish\r\n");
            rc = nwy_MQTTPublish(&paho_mqtt_client, paho_mqtt_at_param.topic, &pubmsg);
            //rc = 0;
            if(rc != 0) 
            {
                nwy_sleep(10*failed_publish);
                // nwy_ext_echo("\r\nMQTT  publish loop\r\n");
                failed_publish++;
            }
            else
            {
                successfull_publish++;
            }
        }
        while (rc != 0);
        memset(can_pub_array_json, 0, sizeof(can_pub_array_json));
    }
}


void MQTT_publish_ThreadEntry(void *param)
{
    uint8_t rc = 0 ;
    char buffer[512];
    uint8_t send_buf = 1;
    struct timeval tv;
   

    gettimeofday(&tv, NULL);
    double s = tv.tv_sec;
    double ms = ((double)tv.tv_usec) / 1.0e3;
    timestamp_start = (unsigned long long)(s*1000 + ms);
   
       
    while (1)
    {
       if (nwy_get_msg_que(mqtt_publish_msg_queue, &uart_strcut, 0) == true)
        {
            // nwy_ext_echo("\r\nPopped\r\n");
            if (MQTTIsConnected(&paho_mqtt_client))
            {
                switch (uart_strcut.cmd)
                {
                    case 0x04:
                    {  
                        //memset(paho_mqtt_at_param.message, 0, sizeof(paho_mqtt_at_param.message));
                        //construct_json_here;
                       //nwy_ext_echo("\r\nPayload size is ::%d\r\n",pub_msg_queue.data_struct.length_u16/14);
                        for(int i = 0; i < uart_strcut.length_u16/15; i++) 
                        { 
                          //  nwy_ext_echo("\r\nInside for loop\r\n");
                            uint8_t *p = uart_strcut.payload + i*15;
                            send_can_mqtt_pack(p, 15,  uart_strcut.timestamp);
                        }
                        //nwy_ext_echo("\r\nmqtt CAN publish rc:%d", rc);
                    }
                    break;
                    case 0x05:
                    {
                        #if 1
                        // nwy_ext_echo("%d, the data received is %s",pub_msg_queue.data_length, pub_msg_queue.payload);
                        memset(paho_mqtt_at_param.topic, 0, sizeof(paho_mqtt_at_param.topic));
                        strncpy(paho_mqtt_at_param.topic, GPS_TOPIC, strlen(GPS_TOPIC));
                        paho_mqtt_at_param.qos = 1;
                        paho_mqtt_at_param.retained = 0;
                        memset(paho_mqtt_at_param.message, 0, sizeof(paho_mqtt_at_param.message));
                        strncpy(paho_mqtt_at_param.message, uart_strcut.payload , uart_strcut.length_u16);
                        nwy_ext_echo("\r\nmqttpub param retained = %d, qos = %d, topic = %s, msg = %s", paho_mqtt_at_param.retained, paho_mqtt_at_param.qos, paho_mqtt_at_param.topic, paho_mqtt_at_param.message);
                        memset(&pubmsg, 0, sizeof(pubmsg));
                        pubmsg.payload = (void *)paho_mqtt_at_param.message;
                        pubmsg.payloadlen = strlen(paho_mqtt_at_param.message);
                        pubmsg.qos = paho_mqtt_at_param.qos;
                        pubmsg.retained = paho_mqtt_at_param.retained;
                        pubmsg.dup = 0;
                        do {
                            rc = nwy_MQTTPublish(&paho_mqtt_client, paho_mqtt_at_param.topic, &pubmsg);
                            nwy_ext_echo("\r\nmqtt GPS publish rc:%d", rc);
                            
                            if (rc != 0 )
                            {
                                nwy_sleep(1);
                            }
                        }while (rc != 0);
                        #endif
                    }
                   
                    break;
                    case 0xff:
                    {
                        memset(paho_mqtt_at_param.topic, 0, sizeof(paho_mqtt_at_param.topic));
                        strncpy(paho_mqtt_at_param.topic, ACTION_TOPIC, strlen(ACTION_TOPIC));
                        paho_mqtt_at_param.qos = 1;
                        paho_mqtt_at_param.retained = 0;
                        memset(paho_mqtt_at_param.message, 0, sizeof(paho_mqtt_at_param.message));
                        strncpy(paho_mqtt_at_param.message, uart_strcut.payload , uart_strcut.length_u16);
                        nwy_ext_echo("\r\nmqttpub param retained = %d, qos = %d, topic = %s, msg = %s", paho_mqtt_at_param.retained, paho_mqtt_at_param.qos, paho_mqtt_at_param.topic, paho_mqtt_at_param.message);
                        memset(&pubmsg, 0, sizeof(pubmsg));
                        pubmsg.payload = (void *)paho_mqtt_at_param.message;
                        pubmsg.payloadlen = strlen(paho_mqtt_at_param.message);
                        pubmsg.qos = paho_mqtt_at_param.qos;
                        pubmsg.retained = paho_mqtt_at_param.retained;
                        pubmsg.dup = 0;
                        int loop_var = 0;
                        do {
                            
                            rc = nwy_MQTTPublish(&paho_mqtt_client, paho_mqtt_at_param.topic, &pubmsg);
                            nwy_ext_echo("\r\naction resp publish rc:%d", rc);
                            
                            if (rc != 0 )
                            {
                                nwy_sleep(1);
                            }
                            loop_var++;
                        }while (rc != 0);

                        // byteBeam_subscribe();             //temp changes at tork HQ
                    }
                    break;
                    case 0xCC:
                    {
                        memset(paho_mqtt_at_param.topic, 0, sizeof(paho_mqtt_at_param.topic));
                        strncpy(paho_mqtt_at_param.topic, SOS_TOPIC, strlen(SOS_TOPIC));
                        paho_mqtt_at_param.qos = 1;
                        paho_mqtt_at_param.retained = 0;
                        memset(paho_mqtt_at_param.message, 0, sizeof(paho_mqtt_at_param.message));
                        strncpy(paho_mqtt_at_param.message, uart_strcut.payload , uart_strcut.length_u16);
                        nwy_ext_echo("\r\nmqttpub param retained = %d, qos = %d, topic = %s, msg = %s", paho_mqtt_at_param.retained, paho_mqtt_at_param.qos, paho_mqtt_at_param.topic, paho_mqtt_at_param.message);
                        memset(&pubmsg, 0, sizeof(pubmsg));
                        pubmsg.payload = (void *)paho_mqtt_at_param.message;
                        pubmsg.payloadlen = strlen(paho_mqtt_at_param.message);
                        pubmsg.qos = paho_mqtt_at_param.qos;
                        pubmsg.retained = paho_mqtt_at_param.retained;
                        pubmsg.dup = 0;

                        do 
                        {
                            rc = nwy_MQTTPublish(&paho_mqtt_client, paho_mqtt_at_param.topic, &pubmsg);
                            nwy_ext_echo("\r\nsos publish rc:-%d\r\n", rc);
                            
                            if (rc != 0 )
                            {
                                nwy_sleep(1);
                            }
                        } while (rc != 0);
                    }
                    break;
                    case 0xDD:
                    {
                        memset(paho_mqtt_at_param.topic, 0, sizeof(paho_mqtt_at_param.topic));
                        strncpy(paho_mqtt_at_param.topic, IGN_TOPIC, strlen(IGN_TOPIC));
                        paho_mqtt_at_param.qos = 1;
                        paho_mqtt_at_param.retained = 0;
                        memset(paho_mqtt_at_param.message, 0, sizeof(paho_mqtt_at_param.message));
                        strncpy(paho_mqtt_at_param.message, uart_strcut.payload , uart_strcut.length_u16);
                        nwy_ext_echo("\r\nmqttpub param retained = %d, qos = %d, topic = %s, msg = %s", paho_mqtt_at_param.retained, paho_mqtt_at_param.qos, paho_mqtt_at_param.topic, paho_mqtt_at_param.message);
                        memset(&pubmsg, 0, sizeof(pubmsg));
                        pubmsg.payload = (void *)paho_mqtt_at_param.message;
                        pubmsg.payloadlen = strlen(paho_mqtt_at_param.message);
                        pubmsg.qos = paho_mqtt_at_param.qos;
                        pubmsg.retained = paho_mqtt_at_param.retained;
                        pubmsg.dup = 0;

                        do 
                        {
                            rc = nwy_MQTTPublish(&paho_mqtt_client, paho_mqtt_at_param.topic, &pubmsg);
                            nwy_ext_echo("\r\nignition publish rc:-%d\r\n", rc);
                            
                            if (rc != 0 )
                            {
                                nwy_sleep(1);
                            }
                        } while (rc != 0);
                    }
                    case 0xDE:
                    {
                        memset(paho_mqtt_at_param.topic, 0, sizeof(paho_mqtt_at_param.topic));
                        strncpy(paho_mqtt_at_param.topic, DEV_SHADOW_TOPIC, strlen(DEV_SHADOW_TOPIC));
                        paho_mqtt_at_param.qos = 1;
                        paho_mqtt_at_param.retained = 0;
                        memset(paho_mqtt_at_param.message, 0, sizeof(paho_mqtt_at_param.message));
                        strncpy(paho_mqtt_at_param.message, uart_strcut.payload , uart_strcut.length_u16);
                        nwy_ext_echo("\r\nmqttpub param retained = %d, qos = %d, topic = %s, msg = %s", paho_mqtt_at_param.retained, paho_mqtt_at_param.qos, paho_mqtt_at_param.topic, paho_mqtt_at_param.message);
                        memset(&pubmsg, 0, sizeof(pubmsg));
                        pubmsg.payload = (void *)paho_mqtt_at_param.message;
                        pubmsg.payloadlen = strlen(paho_mqtt_at_param.message);
                        pubmsg.qos = paho_mqtt_at_param.qos;
                        pubmsg.retained = paho_mqtt_at_param.retained;
                        pubmsg.dup = 0;

                        do 
                        {
                            rc = nwy_MQTTPublish(&paho_mqtt_client, paho_mqtt_at_param.topic, &pubmsg);
                            nwy_ext_echo("\r\nDevice shadow publish rc:-%d\r\n", rc);
                            
                            if (rc != 0 )
                            {
                                nwy_sleep(1);
                            }
                        } while (rc != 0);
                    }
                    break;
                    default:
                    break;
                }
            }
            else
            {
                nwy_ext_echo("\r\nPublishMQTT not connected");
                // Send Semaphore from here 
                // bytebeam_MQTT_connect();
                //  nwy_put_msg_que(store_queue, &uart_strcut, 0);
                nwy_sleep(1000);
            }
        }
        else
        {
            nwy_ext_echo("MQTT Queue Empty\r\n");
            nwy_sleep(20);
        }
    }
     
}

void MQTT_subscribe_ThreadEntry(void *param)
{
    while (1)
    {
        while (MQTTIsConnected(&paho_mqtt_client))
        {
            MQTTYield(&paho_mqtt_client, 500);
            nwy_sleep(100);
        }
        nwy_sleep(1000);
    }
}


bool get_MQTT_COnnection_Status(void)
{
    if (MQTTIsConnected(&paho_mqtt_client) == true)
    {
        // nwy_ext_echo("\r\nMQTT connected\r\n");
        return true;
    }
    nwy_ext_echo("\r\nNo MQTT");
    return false;
}

