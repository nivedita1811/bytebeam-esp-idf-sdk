#include "bytebeam_device_config.h"
#include "cJSON.h"
#include "bytebeam_nwy_sdk.h"
#include "common.h"

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
#include "cJSON.h"
#include <time.h>
#include <math.h>
#include <sys/time.h>

#define CERT_EFS_CHECKSUM_STR_SIZE 4

typedef enum
{
    UNKNOWN_FILE_LOC,
    CONFIG_FILE_IN_NWY,
    CONFIG_FILE_IN_SDCARD = 2,
}config_file_loc_te;

char device_id_global[10] = {0,};

char project_id_global[25] ={0};

device_config bb_device_config;

int config_data_present_in_nwy = 0;

extern dataRecvContext_t *g_recv;

extern uint8_t whole_bin_file_array_u8[81920];

#if 0
char *ca_cert = {"-----BEGIN CERTIFICATE-----\n\
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\n\
a0cIevjvYAtVysoXInV0kg==\n\
-----END CERTIFICATE-----\n"};


char *client_cert = {"-----BEGIN CERTIFICATE-----\n\
MIIEazCCAlOgAwIBAgICB+MwDQYJKoZIhvcNAQELBQAwdzEOMAwGA1UEBhMFSW5k\n\
aWExETAPBgNVBAgTCEthcm5hdGFrMRIwEAYDVQQHEwlCYW5nYWxvcmUxFzAVBgNV\n\
BAkTDlN1YmJpYWggR2FyZGVuMQ8wDQYDVQQREwY1NjAwMTExFDASBgNVBAoTC0J5\n\
dGViZWFtLmlvMB4XDTIyMTIyNzE2MTc1NVoXDTMyMTIyNzE2MTc1NVowIjEUMBIG\n\
A1UEChMLcHJhZGVlcHRlc3QxCjAIBgNVBAMTATMwggEiMA0GCSqGSIb3DQEBAQUA\n\
A4IBDwAwggEKAoIBAQC3TfbXL1jpyDWI1jzNEkLL7gUmk32FxBS7yf0soxS5bhnS\n\
yGZ+Aq80FAHkJfZb/Ycf2i1YzlDoFhhihR1uwh3hwkfXBKHfzx06U5ULXllzJRAB\n\
HCsGnq2q/QiD/ILsx9B0vnmeSjNzPUYRR1bidyrtlRPEybxzKiMsJwP6kYmIUw9v\n\
QbWnLBl/l1abtwWB6LBThwKZwCy/tt9njARACV99vKcQPS171DqPrvwRUOahds2F\n\
dZMhK94JUnyL+OzHNIYfmTw/bQInzyKzZdIXsgIo6c0uyEVy36qch16EuDQlC9Dt\n\
uBTWyDH71xvnlVlVYz3y8KPOy/oh6FxCOBIRme4tAgMBAAGjVjBUMA4GA1UdDwEB\n\
/wQEAwIFoDATBgNVHSUEDDAKBggrBgEFBQcDAjAfBgNVHSMEGDAWgBSpfzE2y62d\n\
ILq53jpgHwRzvix8/jAMBgNVHREEBTADggEzMA0GCSqGSIb3DQEBCwUAA4ICAQCt\n\
jA3AlLxvyLGwybh0y+QyTrsfGddCiJ/G0O8efzx4TwkRCIl2twTcgTV/oZf8LCsj\n\
mi3Igenf9mnSboXiM5b2FBhfGbDfo5aSqNDtbDzNr1Mr8yqDUWbPIpDbZjX9PgZF\n\
FlvjnA+YsxhuddlHJpg55OA0Pof4uAM8wuK2luoR0U7a0yHoT1yyaswytU3PccF5\n\
Adw6t2XklO+xDJn7F3mmY/r1D6lZ3B2VXr7WLqq71sw0hHNovyVYWxYaiaTrm0As\n\
64lXaaKt3jLM3sm2b3JzN95TPJDTw0gSak+oWIhHj6CbqdJeBuRxP2ooFlBX4HbS\n\
M0q2k9NMgCXp+XNq00qVi2xB28Pc+/kOrg0NwFneE4Cuq12phVDARi4UDD4HmmIc\n\
PjhmYfQm1vlI8iHeRKKZLUfYk2daNKBIwrd448t6AnsYuKOtoEoMM4CTay5y6WMK\n\
qUISwfxatP8k6of5LHleIUb8FC9UwjEXlz0su7GHgMpEynVvSK0Vf2ZXAVNAFs8c\n\
EHdqOeWog1GnlRMneLZieSulyG8nPGd5oSzg60aMdQsMgVAo0dayXTv5WtUuKWqt\n\
csgToQypQLQ/uej4Aosf++DbtL4neJ2cVr2fAKspp7bf/DQG7/TxhjFGUMvVG2SR\n\
ae/IvYddFFiat9dWEBThqzkOz3bxd6ROv6Evy/PRGg==\n\
-----END CERTIFICATE-----\n"};


char *key_cert = {"-----BEGIN RSA PRIVATE KEY-----\n\
MIIEpQIBAAKCAQEAt0321y9Y6cg1iNY8zRJCy+4FJpN9hcQUu8n9LKMUuW4Z0shm\n\
fgKvNBQB5CX2W/2HH9otWM5Q6BYYYoUdbsId4cJH1wSh388dOlOVC15ZcyUQARwr\n\
Bp6tqv0Ig/yC7MfQdL55nkozcz1GEUdW4ncq7ZUTxMm8cyojLCcD+pGJiFMPb0G1\n\
pywZf5dWm7cFgeiwU4cCmcAsv7bfZ4wEQAlffbynED0te9Q6j678EVDmoXbNhXWT\n\
ISveCVJ8i/jsxzSGH5k8P20CJ88is2XSF7ICKOnNLshFct+qnIdehLg0JQvQ7bgU\n\
1sgx+9cb55VZVWM98vCjzsv6IehcQjgSEZnuLQIDAQABAoIBAQCeUQT/x5H4oIW0\n\
QKzUxY/RiCsErWA9VyQ9KPC0S8nLec9t4M28yd6+kQUtIqiLvQWmug5uZAs22FaJ\n\
SMUp8xhR8zWO/R4+KijpRmU1I3EIZRtUihqTJhuC0fY34LDHhrCBFD4AO8ACKQrK\n\
Q55EDH6UIiyny0ErClrEeJIUKPzrMrOxFRbSi2jHZLOi/zAI6fesYm7bJsOyAdBO\n\
S3fLrm2qn5lDjH1Ewoa9dpePcACcejaVDOZXxaFHuUo4yheHzX49ckoJe3aUCG3d\n\
g0rmIGYNnhx0Bv8SNHyHTR0k8Swx6Sbh9wX/r/EdutLlJOoZuLFihnZ+aHkl64Am\n\
wSf9NCNhAoGBANgDto0ccUIbYS37ZcEu6DT7GcrDEHr9nqhZ03mNooiA8hMLnxR7\n\
OY/faRoxJ3ewr0JYRpL/Gqvg7Weu6vs9jewwWxkPRv5aKqb/GKi4Lk80DR3kf90c\n\
111rKUKCcIvFy1y+bD4FzcoSdVxdFQFnoZKIq/i4ujDBjrkeDHCBN7wJAoGBANk8\n\
OSg+vOgj14eOHmieYK6fXXpaC9Bh8rhgQdVUjLBeHoZq4vmhr8R9o7hEMpGtbYOS\n\
leQMZPYNvD0/H7dk27h2lmadK87mELHJj+IxSCJHYzVcfmwMXkQoSsgieKSLpApt\n\
5CfelAjq8UPfJkt6tCoLxor63Yvtxd7QIdl/3bIFAoGADsKCMLYmzvuL8lZhJ4gK\n\
+cPGiWI2uBCoTJyGpKGW33jgEYkTv5SD5F7BkbaWGcmK0B+0AstHiUL/Nbxv9ebI\n\
0cCkGNxAX5TNoUeA6dZb47GMy+QXEIat+IYf9ZlwEzzanmWsg7hlzU7hkgQSMVxh\n\
X9/S0FXe8wHmvKNe+jj6E/ECgYEAnWDi68Qf6XHl+D++yf5LftbXywcnXe1IAclC\n\
iIG7dx6IsMsI6t73TrvS+3D0zdM97r8otlFAZ33l7N0pZhy8jbPkHC0rirXkTFkB\n\
U1Zust7TAdqjgDWpqWkjXECU9GhyDFg2nafDyX4kEg69WJtpoLhtO0ypanF1E4nn\n\
rAHVo90CgYEAg/tf0XB4wvmAg+w6Z+n9XVfRDq0frjPaaU9wpEAyujWP3AW8UGbd\n\
YjI+jgCn8mtUcYqpNhXDZdl0nRyOpTyMu7BH89kLTKJYO20W1Jep9ilncMshbJ3b\n\
EDmTG3rGEs1K48UBG6H/wfnhxlqMWe8CiY9Mp5S8iwYE1NaPqVf3WYg=\n\
-----END RSA PRIVATE KEY-----\n"};


void mqtts_cert_add()
{
    int result;
    char filename[128] = {0};

    strcpy(filename, ca_filename);
    int length = strlen(ca_cert);
    if (length <= 0)
    {
        nwy_ext_echo("\r\n invalid cacert size:%d", length);
        return;
    }
    nwy_ext_echo("file name is : %s, size is %d\r\n", filename, length);
    g_recv = malloc(sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    if (g_recv == NULL)
    {
        nwy_ext_echo("\r\n malloc buffer fail");
        return;
    }
    memset(g_recv, 0, sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    g_recv->size = length;
    g_recv->pos = 0;

    memcpy(g_recv->data, ca_cert, length);
    unsigned wsize = nwy_cert_add(filename, g_recv->data, g_recv->size);
    nwy_ext_echo("file name is : %s, write size is %d attempted to write:%d\r\n", filename, wsize, g_recv->size);

    if (wsize != g_recv->size)
    {
        nwy_ext_echo("\r\nnwy_cert_add fail\r\n");
    }
    else
    {
        nwy_ext_echo("\r\nnwy_cert_add success\r\n");
    }
    free(g_recv);
    g_recv = NULL;

    strcpy(filename, cl_filename);
    length = strlen(client_cert);
    if (length <= 0)
    {
        nwy_ext_echo("\r\n invalid cacert size:%d", length);
        return;
    }
    nwy_ext_echo("file name is : %s, size is %d\n", filename, length);
    g_recv = malloc(sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    if (g_recv == NULL)
    {
        nwy_ext_echo("\r\n malloc buffer fail");
        return;
    }
    memset(g_recv, 0, sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    g_recv->size = length;
    g_recv->pos = 0;

    memcpy(g_recv->data, client_cert, length);
    wsize = nwy_cert_add(filename, g_recv->data, g_recv->size);
    nwy_ext_echo("file name is : %s, write size is %d\n", filename, wsize);

    if (wsize != g_recv->size)
    {
        nwy_ext_echo("\r\nnwy_cert_add fail");
    }
    else
    {
        nwy_ext_echo("\r\nnwy_cert_add success");
    }
    free(g_recv);
    g_recv = NULL;

    strcpy(filename, clk_filename);
    length = strlen(key_cert);
    if (length <= 0)
    {
        nwy_ext_echo("\r\n invalid cacert size:%d", length);
        return;
    }
    nwy_ext_echo("file name is : %s, size is %d\n", filename, length);
    g_recv = malloc(sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    if (g_recv == NULL)
    {
        nwy_ext_echo("\r\n malloc buffer fail");
        return;
    }
    memset(g_recv, 0, sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    g_recv->size = length;
    g_recv->pos = 0;

    memcpy(g_recv->data, key_cert, length);
    wsize = nwy_cert_add(filename, g_recv->data, g_recv->size);
    nwy_ext_echo("file name is : %s, write size is %d\n", filename, wsize);

    if (wsize != g_recv->size)
    {
        nwy_ext_echo("\r\nnwy_cert_add fail");
    }
    else
    {
        nwy_ext_echo("\r\nnwy_cert_add success");
    }
    free(g_recv);
    g_recv = NULL;

    result = nwy_cert_check(ca_filename);
    if (0 == result)
    {
        nwy_ext_echo("\r\n nwy_ca_cert_check success");
    }
    else
    {
        nwy_ext_echo("\r\n nwy_cert_check fail");
    }
    result = nwy_cert_check(cl_filename);
    if (0 == result)
    {
        nwy_ext_echo("\r\n nwy_cl cert_check success");
    }
    else
    {
        nwy_ext_echo("\r\n nwy_cert_check fail");
    }
    result = nwy_cert_check(clk_filename);
    if (0 == result)
    {
        nwy_ext_echo("\r\n nwy_clk_cert_check success");
    }
    else
    {
        nwy_ext_echo("\r\n nwy_cert_check fail");
    }
}
#endif

#if 1
void cert_verify()
{
    device_config *device_cfg = &bb_device_config;

    int result = 0;

    result = nwy_cert_check(device_cfg->ca_cert_pem_name); 
    if (0 == result)
    {
        nwy_ext_echo("\r\n nwy_ca_cert_check success\r\n");
    }
    else
    {
        nwy_ext_echo("\r\n nwy_cert_check fail\r\n");
    }
    
    result = nwy_cert_check(device_cfg->client_cert_pem_name);
    if (0 == result)
    {
        nwy_ext_echo("\r\n nwy_cl cert_check success\r\n");
    }
    else
    {
        nwy_ext_echo("\r\n nwy_cert_check fail\r\n");
    }
    result = nwy_cert_check( device_cfg->client_key_pem_name);
    
    if (0 == result)
    {
        nwy_ext_echo("\r\n nwy_clk_cert_check success\r\n");
    }
    else
    {
        nwy_ext_echo("\r\n nwy_cert_check fail\r\n");
    }
}


int parse_device_config_file()
{
    int config_file_location;
    int config_json_nwy_file_fd = 0;
    int config_json_sd_file_fd = 0;
    int config_json_size = 0;
    config_file_loc_te config_file_loc_e =UNKNOWN_FILE_LOC;
    if(nwy_sdk_fexist("/nwy/device_config.json") == true)
    {
        config_data_present_in_nwy = 1;
        config_file_loc_e = CONFIG_FILE_IN_NWY;
    }
    else
    {
        config_file_loc_e = CONFIG_FILE_IN_SDCARD;
    }

    // config_file_loc_e = CONFIG_FILE_IN_SDCARD;

    switch(config_file_loc_e)
    {
        case CONFIG_FILE_IN_NWY:
        {
            int ret = parse_device_config_from_nwy_mem();
            if(ret < 0)
            {
                nwy_sdk_file_unlink("/nwy/device_config.json");
                ret = parse_device_config_from_sdcard();
                if(ret < 0)
                {
                    return -1;
                }
            }
        }
        break;
        case CONFIG_FILE_IN_SDCARD:
        {   
            int ret = parse_device_config_from_sdcard();
            if(ret < 0)
            {
                return -1;
            }
        }
        break;
        default:
        break;
    }
    return 0;	
}


int parse_device_config_from_nwy_mem()
{
    int config_json_fd = 0;
    int config_json_size = 0;

    config_json_fd = nwy_sdk_fopen("/nwy/device_config.json", NWY_RDWR ); //NWY_CREAT | NWY_RDWR | NWY_APPEND);
    if (config_json_fd < 0)
    {
        nwy_ext_echo("\r\nConfig JSON file opening fail\r\n");
        return -1;
    }
    config_json_size = nwy_sdk_fsize_fd(config_json_fd);
    nwy_sdk_fclose(config_json_fd);

    config_json_fd = nwy_sdk_fopen("/nwy/device_config.json",NWY_RDONLY);
    int read_count = nwy_sdk_fread(config_json_fd, whole_bin_file_array_u8, config_json_size);
    nwy_sdk_fclose(config_json_fd);

	// const char *json_file = (const char *)bytebeam_device_config_json;
    const char *json_file = (const char *)whole_bin_file_array_u8;
    device_config *device_cfg = &bb_device_config;
    char ca_filename[12] = {"ca_cert.pem"};
    char cl_filename[16] = {"client_cert.pem"};
    char clk_filename[15] = {"client_key.pem"};

	cJSON *cert_json = cJSON_Parse(json_file); 

	if(cert_json == NULL)
	{
		nwy_ext_echo("ERROR in parsing the JSON\n");
		return -1;
	}

	cJSON *prj_id_obj = cJSON_GetObjectItem(cert_json, "project_id");
	if(!(cJSON_IsString(prj_id_obj) && (prj_id_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR in getting the prject id\n");
		return -1;
	}
	strcpy(device_cfg->project_id, prj_id_obj->valuestring);
    snprintf(project_id_global, 25, "%s", prj_id_obj->valuestring);


	cJSON *broker_name_obj = cJSON_GetObjectItem(cert_json, "broker");
	if(!(cJSON_IsString(broker_name_obj) && (broker_name_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR parsing broker name");
		return -1;
	}

	cJSON *port_num_obj = cJSON_GetObjectItem(cert_json, "port");
	if(!(cJSON_IsNumber(port_num_obj)))
	{
		nwy_ext_echo("ERROR parsing port number.");
		return -1;
	}

	int port_int = port_num_obj->valuedouble;
	sprintf(device_cfg->broker_uri,"%s", broker_name_obj->valuestring);
    device_cfg->broker_port = port_int;
	

	cJSON *device_id_obj = cJSON_GetObjectItem(cert_json, "device_id");
	if(!(cJSON_IsString(device_id_obj) && (device_id_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR parsing device id\n");
		return -1;
	}
    snprintf(device_id_global, 10, "%s", device_id_obj->valuestring);

	strcpy(device_cfg->device_id, device_id_obj->valuestring);
	cJSON *auth_obj = cJSON_GetObjectItem(cert_json, "authentication");
	if(cert_json == NULL || !(cJSON_IsObject(auth_obj)))
	{
		nwy_ext_echo("ERROR in parsing the auth JSON\n");
		return -1;
	}

	cJSON *ca_cert_obj = cJSON_GetObjectItem(auth_obj, "ca_certificate");
	if(!(cJSON_IsString(ca_cert_obj) && (ca_cert_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR parsing ca certificate\n");
		return -1;
	}

	device_cfg->ca_cert_pem = (char*)ca_cert_obj->valuestring; 
    strcpy(device_cfg->ca_cert_pem_name, ca_filename);
    int length = strlen(device_cfg->ca_cert_pem);
    if (length <= 0)
    {
        nwy_ext_echo("\r\n invalid cacert size:%d", length);
        return -1;
    }
  //  nwy_ext_echo("file name is : %s, size is %d\n", device_cfg->ca_cert_pem_name, length);
    
    g_recv = malloc(sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    if (g_recv == NULL)
    {
        nwy_ext_echo("\r\n malloc buffer fail");
        return -1;
    }
    memset(g_recv, 0, sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    g_recv->size = length;
    g_recv->pos = 0;
    
    memcpy(g_recv->data, device_cfg->ca_cert_pem, length);
    unsigned wsize = nwy_cert_add(device_cfg->ca_cert_pem_name, g_recv->data, g_recv->size);
    nwy_ext_echo("file name is : %s, write size is %d\n", device_cfg->ca_cert_pem_name, wsize);
        
    if (wsize != g_recv->size)
    {
        nwy_ext_echo("\r\nnwy_cert_add fail");
    }
    else
    {
        nwy_ext_echo("\r\nnwy_cert_add success");
    }
    free(g_recv);


	cJSON *device_cert_obj = cJSON_GetObjectItem(auth_obj, "device_certificate");
	if(!(cJSON_IsString(device_cert_obj) && (device_cert_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR parsing device certifate\n");
		return -1;
	}

	device_cfg->client_cert_pem = (char *)device_cert_obj-> valuestring;

    g_recv = NULL;

    strcpy(device_cfg->client_cert_pem_name, cl_filename);
    length = strlen(device_cfg->client_cert_pem);
    if (length <= 0)
    {
        nwy_ext_echo("\r\n invalid cacert size:%d", length);
        return -1;
    }
    //nwy_ext_echo("file name is : %s, size is %d\n", device_cfg->client_cert_pem_name, length);
    g_recv = malloc(sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    if (g_recv == NULL)
    {
        nwy_ext_echo("\r\n malloc buffer fail");
        return -1;
    }
    memset(g_recv, 0, sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    g_recv->size = length;
    g_recv->pos = 0;

    memcpy(g_recv->data, device_cfg->client_cert_pem, length);
    wsize = nwy_cert_add( device_cfg->client_cert_pem_name, g_recv->data, g_recv->size);
    nwy_ext_echo("file name is : %s, write size is %d\n",  device_cfg->client_cert_pem_name, wsize);

    if (wsize != g_recv->size)
    {
        nwy_ext_echo("\r\nnwy_cert_add fail");
    }
    else
    {
        nwy_ext_echo("\r\nnwy_cert_add success");
    }
    free(g_recv);

	cJSON *device_private_key_obj = cJSON_GetObjectItem(auth_obj, "device_private_key");
	if(!(cJSON_IsString(device_private_key_obj) && (device_private_key_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR device private key\n");
		return -1;
	}

	device_cfg->client_key_pem = (char *)device_private_key_obj->valuestring;
	
    g_recv = NULL;

    strcpy(device_cfg->client_key_pem_name, clk_filename);
    length = strlen(device_cfg->client_key_pem);
    if (length <= 0)
    {
        nwy_ext_echo("\r\n invalid cacert size:%d", length);
        return -1;
    }
    //nwy_ext_echo("file name is : %s, size is %d\n", device_cfg->client_key_pem_name, length);
    g_recv = malloc(sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    if (g_recv == NULL)
    {
        nwy_ext_echo("\r\n malloc buffer fail");
        return -1;
    }
    memset(g_recv, 0, sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    g_recv->size = length;
    g_recv->pos = 0;

    memcpy(g_recv->data, device_cfg->client_key_pem, length);
    wsize = nwy_cert_add( device_cfg->client_key_pem_name, g_recv->data, g_recv->size);
    nwy_ext_echo("file name is : %s, write size is %d\n",  device_cfg->client_key_pem_name, wsize);

    if (wsize != g_recv->size)
    {
        nwy_ext_echo("\r\nnwy_cert_add fail");
    }
    else
    {
        nwy_ext_echo("\r\nnwy_cert_add success");
    }
    free(g_recv);
    cert_verify();
	return 0;	
}


int parse_device_config_from_sdcard()
{
    int config_json_fd = 0;
    int config_json_size = 0;

    config_json_fd = nwy_sdk_fopen("/sdcard0/device_config.json", NWY_RDWR ); //NWY_CREAT | NWY_RDWR | NWY_APPEND);
    if (config_json_fd < 0)
    {
        nwy_ext_echo("\r\nConfig JSON file opening fail\r\n");
        nwy_sdk_fclose(config_json_fd);
        return -1;
    }
    config_json_size = nwy_sdk_fsize_fd(config_json_fd);
    nwy_sdk_fclose(config_json_fd);

    config_json_fd = nwy_sdk_fopen("/sdcard0/device_config.json",NWY_RDONLY);
    int read_count = nwy_sdk_fread(config_json_fd, whole_bin_file_array_u8, config_json_size);
    nwy_sdk_fclose(config_json_fd);
    nwy_sdk_file_unlink("/nwy/device_config.json");
     config_json_fd = nwy_sdk_fopen("/nwy/device_config.json",NWY_CREAT | NWY_RDWR);
     if (config_json_fd < 0)
     {
         nwy_ext_echo("Copying Config JSON to nwy failed\r\n");
         nwy_sdk_file_unlink("/nwy/device_config.json");
     }
     else
     {
         int ret = nwy_sdk_fwrite(config_json_fd, whole_bin_file_array_u8, config_json_size);
         if(ret != config_json_size)
         {
             nwy_ext_echo("Copying Config JSON data to nwy failed\r\n");
             nwy_sdk_file_unlink("/nwy/device_config.json");
         }
     }
     nwy_sdk_fclose(config_json_fd);

	// const char *json_file = (const char *)bytebeam_device_config_json;
    const char *json_file = (const char *)whole_bin_file_array_u8;
    device_config *device_cfg = &bb_device_config;
    char ca_filename[12] = {"ca_cert.pem"};
    char cl_filename[16] = {"client_cert.pem"};
    char clk_filename[15] = {"client_key.pem"};

	cJSON *cert_json = cJSON_Parse(json_file); 

	if(cert_json == NULL)
	{
		nwy_ext_echo("ERROR in parsing the JSON\n");
		return -1;
	}

	cJSON *prj_id_obj = cJSON_GetObjectItem(cert_json, "project_id");
	if(!(cJSON_IsString(prj_id_obj) && (prj_id_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR in getting the prject id\n");
		return -1;
	}
	strcpy(device_cfg->project_id, prj_id_obj->valuestring);
    snprintf(project_id_global, 25, "%s", prj_id_obj->valuestring);


	cJSON *broker_name_obj = cJSON_GetObjectItem(cert_json, "broker");
	if(!(cJSON_IsString(broker_name_obj) && (broker_name_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR parsing broker name");
		return -1;
	}

	cJSON *port_num_obj = cJSON_GetObjectItem(cert_json, "port");
	if(!(cJSON_IsNumber(port_num_obj)))
	{
		nwy_ext_echo("ERROR parsing port number.");
		return -1;
	}

	int port_int = port_num_obj->valuedouble;
	sprintf(device_cfg->broker_uri,"%s", broker_name_obj->valuestring);
    device_cfg->broker_port = port_int;
	

	cJSON *device_id_obj = cJSON_GetObjectItem(cert_json, "device_id");
	if(!(cJSON_IsString(device_id_obj) && (device_id_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR parsing device id\n");
		return -1;
	}
    snprintf(device_id_global, 10, "%s", device_id_obj->valuestring);

	strcpy(device_cfg->device_id, device_id_obj->valuestring);
	cJSON *auth_obj = cJSON_GetObjectItem(cert_json, "authentication");
	if(cert_json == NULL || !(cJSON_IsObject(auth_obj)))
	{
		nwy_ext_echo("ERROR in parsing the auth JSON\n");
		return -1;
	}

	cJSON *ca_cert_obj = cJSON_GetObjectItem(auth_obj, "ca_certificate");
	if(!(cJSON_IsString(ca_cert_obj) && (ca_cert_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR parsing ca certificate\n");
		return -1;
	}

	device_cfg->ca_cert_pem = (char*)ca_cert_obj->valuestring; 
    strcpy(device_cfg->ca_cert_pem_name, ca_filename);
    int length = strlen(device_cfg->ca_cert_pem);
    if (length <= 0)
    {
        nwy_ext_echo("\r\n invalid cacert size:%d", length);
        return -1;
    }
  //  nwy_ext_echo("file name is : %s, size is %d\n", device_cfg->ca_cert_pem_name, length);
    
    g_recv = malloc(sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    if (g_recv == NULL)
    {
        nwy_ext_echo("\r\n malloc buffer fail");
        return -1;
    }
    memset(g_recv, 0, sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    g_recv->size = length;
    g_recv->pos = 0;
    
    memcpy(g_recv->data, device_cfg->ca_cert_pem, length);
    unsigned wsize = nwy_cert_add(device_cfg->ca_cert_pem_name, g_recv->data, g_recv->size);
    nwy_ext_echo("file name is : %s, write size is %d\n", device_cfg->ca_cert_pem_name, wsize);
        
    if (wsize != g_recv->size)
    {
        nwy_ext_echo("\r\nnwy_cert_add fail");
    }
    else
    {
        nwy_ext_echo("\r\nnwy_cert_add success");
    }
    free(g_recv);


	cJSON *device_cert_obj = cJSON_GetObjectItem(auth_obj, "device_certificate");
	if(!(cJSON_IsString(device_cert_obj) && (device_cert_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR parsing device certifate\n");
		return -1;
	}

	device_cfg->client_cert_pem = (char *)device_cert_obj-> valuestring;

    g_recv = NULL;

    strcpy(device_cfg->client_cert_pem_name, cl_filename);
    length = strlen(device_cfg->client_cert_pem);
    if (length <= 0)
    {
        nwy_ext_echo("\r\n invalid cacert size:%d", length);
        return -1;
    }
    //nwy_ext_echo("file name is : %s, size is %d\n", device_cfg->client_cert_pem_name, length);
    g_recv = malloc(sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    if (g_recv == NULL)
    {
        nwy_ext_echo("\r\n malloc buffer fail");
        return -1;
    }
    memset(g_recv, 0, sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    g_recv->size = length;
    g_recv->pos = 0;

    memcpy(g_recv->data, device_cfg->client_cert_pem, length);
    wsize = nwy_cert_add( device_cfg->client_cert_pem_name, g_recv->data, g_recv->size);
    nwy_ext_echo("file name is : %s, write size is %d\n",  device_cfg->client_cert_pem_name, wsize);

    if (wsize != g_recv->size)
    {
        nwy_ext_echo("\r\nnwy_cert_add fail");
    }
    else
    {
        nwy_ext_echo("\r\nnwy_cert_add success");
    }
    free(g_recv);

	cJSON *device_private_key_obj = cJSON_GetObjectItem(auth_obj, "device_private_key");
	if(!(cJSON_IsString(device_private_key_obj) && (device_private_key_obj->valuestring != NULL)))
	{
		nwy_ext_echo("ERROR device private key\n");
		return -1;
	}

	device_cfg->client_key_pem = (char *)device_private_key_obj->valuestring;
	
    g_recv = NULL;

    strcpy(device_cfg->client_key_pem_name, clk_filename);
    length = strlen(device_cfg->client_key_pem);
    if (length <= 0)
    {
        nwy_ext_echo("\r\n invalid cacert size:%d", length);
        return -1;
    }
    //nwy_ext_echo("file name is : %s, size is %d\n", device_cfg->client_key_pem_name, length);
    g_recv = malloc(sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    if (g_recv == NULL)
    {
        nwy_ext_echo("\r\n malloc buffer fail");
        return -1;
    }
    memset(g_recv, 0, sizeof(dataRecvContext_t) + length + CERT_EFS_CHECKSUM_STR_SIZE + 1);
    g_recv->size = length;
    g_recv->pos = 0;

    memcpy(g_recv->data, device_cfg->client_key_pem, length);
    wsize = nwy_cert_add( device_cfg->client_key_pem_name, g_recv->data, g_recv->size);
    nwy_ext_echo("file name is : %s, write size is %d\n",  device_cfg->client_key_pem_name, wsize);

    if (wsize != g_recv->size)
    {
        nwy_ext_echo("\r\nnwy_cert_add fail");
    }
    else
    {
        nwy_ext_echo("\r\nnwy_cert_add success");
    }
    free(g_recv);
    cert_verify();
	return 0;	
}


#endif