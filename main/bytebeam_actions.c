
#include <stdio.h>
#include "bytebeam_actions.h"
#include "cJSON.h"
#include "bytebeam.h"
#include "common.h"
#include "bytebeam_uart.h"

volatile int http_download_flag = 0;
extern int network_thread_counter;

char action_mess_json[] = "[{"
                       "\"timestamp\": %llu,"
                       "\"sequence\": %d,"
                       "\"state\":\"%s\","
                       "\"errors\":[\"%s\"],"
                       "\"id\":\"%s\","
                       "\"progress\": %d"
                       "}]\n";

struct action_functions_map action_funcs[] = { {"update_firmware", handle_ota}, {NULL, NULL} };

char ota_action_id[10] = {0};


void bytebeam_action_status_publish(char *json_action_message_buff)
{
    UART_data_struct uart_struct;
    memset(&uart_struct, 0 , sizeof(uart_struct));

    uart_struct.cmd = 0xff;
    uart_struct.length_u16 = strlen(json_action_message_buff);
    strcpy(uart_struct.payload, json_action_message_buff);
    if (nwy_read_sdcart_status() == false) // direct pass through
    {
        if(nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue) == 0) {
            // nwy_ext_echo("\r\nMQTT Queue full agidhe\r\n");
        }
        else
        {
            // nwy_ext_echo("\r\nMQTT Queue full agiilla\r\n");
            // nwy_ext_echo("\r\nThe data lenght is: %d\r\n", uart_mqtt_publish.data_length);
            nwy_put_msg_que(mqtt_publish_msg_queue, &uart_struct, 0);
        }
    }
    else // if connection exists and space exists send message
    {
        if (get_MQTT_COnnection_Status() == true && (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue) != 0) && store_msg_flag == false)
        {
            //nwy_ext_echo("\r\n Direct publish!!\r\n");
            nwy_put_msg_que(mqtt_publish_msg_queue, &uart_struct, 0);
        }
        else
        {
            store_msg_flag = false;             //disabled until persistance is implemented
            if(store_queue != NULL)
            {
                if(nwy_get_queue_spaceevent_cnt(store_queue) != 0)
                {
                        nwy_put_msg_que(store_queue, &uart_struct, 0);
                }
                else
                {
                    nwy_ext_echo("\r\n Dropping it due to unavailability of space in storage queue!!\r\n");
                }
            }
            else
            {
                nwy_ext_echo("\r\n No storage queue present so dropping it!!\r\n");
            }   
        }
    }
    
}

void publish_action_status(char *action_id, int progress, char *state, char *errors)
{

    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint8_t json_action_message_buff[200] = {0,};

    double s = tv.tv_sec;
    double ms = ((double)tv.tv_usec) / 1.0e6;
    unsigned long long timestamp = (unsigned long long)(s + ms) * 1000;
    static uint32_t sequence = 0;
    sequence++;
    memset(json_action_message_buff, 0, 200);
    snprintf(json_action_message_buff, sizeof(json_action_message_buff), action_mess_json,
             timestamp,
             sequence,
             state,
             errors,
             action_id,
             progress);

    nwy_ext_echo("\nTrying to print:\n%s\n", json_action_message_buff);
    bytebeam_action_status_publish(json_action_message_buff);   
}

int ota_content_length = 0;

int parse_ota_json(char *payload_string, char *url_string_return)
{
	const cJSON *url = NULL;
	const cJSON *version = NULL;
    const cJSON *content_length = NULL;

	cJSON *pl_json = cJSON_Parse(payload_string);
	url = cJSON_GetObjectItem(pl_json, "url");

	if (cJSON_IsString(url) && (url->valuestring != NULL))
	{
		nwy_ext_echo("Checking url \"%s\"\n", url->valuestring);
	}

	version = cJSON_GetObjectItem(pl_json, "version");

	if (cJSON_IsString(version) && (version->valuestring != NULL))
	{
		nwy_ext_echo("Checking version \"%s\"\n", version->valuestring);
	}

    content_length = cJSON_GetObjectItem(pl_json, "content-length");

    if(cJSON_IsNumber(content_length))
    {
        nwy_ext_echo("Checking content-length \"%d\"\n", content_length->valueint);
        ota_content_length = content_length->valueint;
    }



	sprintf(url_string_return, "%s", url->valuestring);
	nwy_ext_echo( "The constructed URL is: %s", url_string_return);

	return 0;
}
uint8_t ota_url[512] = {0,};
extern int ota_http_size;
int handle_ota(char *payload_string, char *action_id)
{
    char constrcuted_url[200] = {
		0,
	};
    nwy_ext_echo("\r\nGetting to parse OTA json\r\n");
    parse_ota_json(payload_string, constrcuted_url);
    //http download
   // bytebeam_http_download_to_file("",  constrcuted_url);
     memcpy(ota_url, &constrcuted_url[8], (strlen(constrcuted_url) - 8));
     nwy_ext_echo("\r\nNandish URL is: %s\r\n", ota_url);
     ota_http_size = ota_content_length;
     http_flag = true;
     HTTP_TRIGGER;
    publish_action_status(action_id,5, "Progress", "");
    //publish_action_status(action_id, 100, "Completed", "");
    snprintf(ota_action_id, 10, action_id);
    http_download_flag = 1;
    network_thread_counter = 0;
    send_ack_nack(0, 0x66, NULL, 0);
    if(gps_app_thread != NULL)
    {
        nwy_suspend_thread(gps_app_thread);
    }    
    if(device_config_thread_handle != NULL)
    {
        nwy_suspend_thread(device_config_thread_handle);
    }
    return 0;
}
int handle_actions(char* action_received)
{
	 cJSON *root = NULL;
	 cJSON *name = NULL;
	 cJSON *payload = NULL;
	 cJSON *action_id_obj = NULL;
	char action_id[20] = {0,};
	int action_iterator = 0;

	root = cJSON_Parse(action_received);
	name = cJSON_GetObjectItem(root,"name");
	
	if (!(cJSON_IsString(name) && (name->valuestring != NULL)))
	{
		nwy_ext_echo("Error parsing action name\n");
        return -1;
	}
	
	nwy_ext_echo("Checking name \"%s\"\n", name->valuestring);
	
	action_id_obj = cJSON_GetObjectItem(root, "id");
	strcpy(action_id, action_id_obj->valuestring);
	
	if (cJSON_IsString(action_id_obj) && (action_id_obj->valuestring != NULL))
	{
		nwy_ext_echo("Checking version \"%s\"\n", action_id_obj->valuestring);
	}
	
	payload = cJSON_GetObjectItem(root, "payload");
		
	if (cJSON_IsString(payload) && (payload->valuestring != NULL))
	{
		nwy_ext_echo("Checking payload \"%s\"\n", payload->valuestring);
		while(action_funcs[action_iterator].name)
		{
			if(!strcmp(action_funcs[action_iterator].name, name->valuestring))
			{
				action_funcs[action_iterator].func(payload->valuestring, action_id);
				break;
			}
			action_iterator++;
		}
		if(action_funcs[action_iterator].name == NULL)
		{
			nwy_ext_echo("Invalid action:%s\n", name->valuestring);
		}
	}

	free(payload);
	free(name);
	free(root);

	return 0;
}
