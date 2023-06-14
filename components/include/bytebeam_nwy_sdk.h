#include "cJSON.h"
#include "MQTTClient.h"
#include "nwy_mqtt.h"

typedef struct device_config
{
	char *ca_cert_pem;
	char ca_cert_pem_name[20];
	char *client_cert_pem;
	char *client_cert_pem_name[20];
	char *client_key_pem;
	char *client_key_pem_name[20];
	char broker_uri[100];
	char device_id[10];
	char project_id[100];
	uint32_t broker_port;
}device_config;

typedef nwy_paho_mqtt_at_param_type bytebeam_client_config_t;
typedef MQTTClient bytebeam_client_handle_t;

typedef struct bytebeam_client
{
	device_config device_cfg;
	bytebeam_client_handle_t client;
	bytebeam_client_config_t mqtt_cfg;
}bytebeam_client;

