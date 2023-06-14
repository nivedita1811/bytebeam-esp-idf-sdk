// #include "cJSON.h"
// #include "bytebeam_sdk.h"
// #include "common.h"


// #include "bytebeam.h"
// #include "nwy_gpio.h"
// #include "nwy_osi_api.h"
// #include "nwy_sim.h"
// #include "nwy_vir_at.h"
// #include "osi_log.h"
// #include "stdlib.h"
// #include "stdarg.h"
// #include "string.h"
// #include "nwy_sms.h"
// #include "nwy_data.h"
// #include "nwy_usb_serial.h"
// #include "nwy_adc.h"
// #include "nwy_led.h"
// #include "nwy_gpio_open.h"
// #include "nwy_fota.h"
// #include "nwy_fota_api.h"
// #include "nwy_pm.h"
// #include "stdio.h"
// #include "nwy_network.h"
// #include "nwy_file.h"
// #include "nwy_socket.h"
// #include "json_server.h"
// #include "mqtt_api.h"
// #include "MQTTClient.h"
// #include "nwy_mqtt.h"
// #include "cJSON.h"
// #include <time.h>
// #include <math.h>
// #include <sys/time.h>
// #include "nwy_ftp.h"
// #include "nwy_http.h"
// #include "nwy_file.h"

// #define TOTAL_FILE_LEN 1024

// int32_t write_to_file_path(char*file_path, char*data, uint32_t data_len);

// extern device_config bb_device_config;

// void http_result_cb(nwy_ftp_result_t *param)
// {
//     int *size;
//     int ret;
//     if (NULL == param)
//     {
//         nwy_ext_echo("event is NULL\r\n");
//     }

//     OSI_LOGI(0, "event is %d", param->event);
//     if (NWY_HTTP_DNS_ERR == param->event)
//     {
//         nwy_ext_echo("\r\nHTTP dns err");
//     }
//     else if (NWY_HTTP_OPEN_FAIL == param->event)
//     {
//         nwy_ext_echo("\r\nHTTP open fail");
//     }
//     else if (NWY_HTTP_OPENED == param->event)
//     {
//         nwy_ext_echo("\r\nHTTP setup success");
//     }
//     else if (NWY_HTTPS_SSL_CONNECTED == param->event)
//     {
//         nwy_ext_echo("\r\nHTTPS setup success");
//     }
//     else if (NWY_HTTP_CLOSED_PASV == param->event || NWY_HTTP_CLOSED == param->event)
//     {
//         nwy_ext_echo("\r\nHTTP closed");
//     }
//     else if (NWY_HTTP_DATA_RECVED == param->event)
//     {
//         nwy_ext_echo("\r\nHTTP recv data len %d.\r\n", param->data_len);
//         write_to_file_path("/nwy/ota.pac", param->data, param->data_len);
//         char *buff = (char *)malloc(NWY_EXT_SIO_RX_MAX);
//         if (NULL == buff)
//         {
//             nwy_ext_echo("%s\r\n", param->data);
//             return;
//         }
//         int offset = 0;
//         int tmeplen = 0;
//         while (param->data_len)
//         {
//             memset(buff, 0x00, NWY_EXT_SIO_RX_MAX);
//             tmeplen = param->data_len > NWY_EXT_SIO_RX_MAX ? NWY_EXT_SIO_RX_MAX : param->data_len;
//             memcpy(buff, param->data + offset, tmeplen);
//             nwy_ext_echo("%s", buff);
//             offset += tmeplen;
//             param->data_len -= tmeplen;
//             OSI_LOGI(0, "param->data_len %d", param->data_len);
//         }
//         free(buff);
//         nwy_ext_echo("\r\n");
//     }
//     else if (NWY_HTTP_DATA_SEND_ERR == param->event)
//     {
//         nwy_ext_echo("\r\ndata send error");
//     }
//     else if (NWY_HTTP_DATA_SEND_FINISHED == param->event)
//     {
//         nwy_ext_echo("\r\ndata send finished");
//     }
//     else if (NWY_HTTPS_SSL_INIT_ERROR == param->event)
//     {
//         nwy_ext_echo("\r\nHTTPS SSL init fail");
//     }
//     else if (NWY_HTTPS_SSL_HANDSHAKE_ERROR == param->event)
//     {
//         nwy_ext_echo("\r\nHTTPS SSL handshare fail");
//     }
//     else if (NWY_HTTPS_SSL_AUTH_FAIL == param->event)
//     {
//         nwy_ext_echo("\r\nHTTPS SSL Authentication fail");
//     }
//     else
//     {
//         nwy_ext_echo("\r\n unkown error");
//     }

//     return;
// }


// int32_t write_to_file_path(char*file_path, char*data, uint32_t data_len)
// {
//     int32_t ret = 0;
//     int32_t fd = 0;
//     int32_t write_len = 0;
//     int32_t file_len = 0;
//     int32_t file_offset = 0;
//     int32_t file_size = 0;
//     int32_t file_size_len = 0;

//     fd = nwy_sdk_fopen(file_path, NWY_CREAT | NWY_RDWR | NWY_APPEND);
//     if (fd < 0)
//     {
//         nwy_ext_echo("open file failed, fd = %d\r\n", fd);
//     }
//     else
//     {
//         int retvalue = nwy_sdk_fwrite(fd, data, data_len);
//         if(retvalue <0)
//         {
//             nwy_ext_echo("File read failed\r\n");
//         }
//     }
//     nwy_sdk_fclose(fd);
// }

// int bytebeam_http_download_to_file(char* file_path, char *url)
// {
//     int result;
//     uint8_t channel;
//     char buf[NWY_HTTP_POST_MAX_DATA_LEN] = {0};
//     char filename[128] = {0};
//     int port;
//     int keepalive;
//     int offset;
//     int size;
//     uint8_t type;
//     nwy_app_ssl_conf_t ssl_cfg;
//     nwy_osiEvent_t event;
//     channel = 1;
//     port = 883;
//     memset(&ssl_cfg, 0x00, sizeof(ssl_cfg));
//     ssl_cfg.ssl_version = 2;
//     ssl_cfg.authmode = 2;

//     strncpy(ssl_cfg.cacert.cert_name, bb_device_config.ca_cert_pem_name, strlen(bb_device_config.ca_cert_pem_name));
//     strncpy(ssl_cfg.clientcert.cert_name, bb_device_config.client_cert_pem_name, strlen(bb_device_config.client_cert_pem_name));
//     strncpy(ssl_cfg.clientkey.cert_name, bb_device_config.client_key_pem_name, strlen(bb_device_config.client_key_pem_name));

//     result = nwy_https_setup(channel, url, port, http_result_cb, &ssl_cfg);
//     if (0 == result)
//     {
//         nwy_ext_echo("\r\n %s success", "nwy_https_setup");

//         keepalive = 0;
//         offset = 0;
//         size = TOTAL_FILE_LEN;
//         result = nwy_http_get(keepalive, offset, size, 0);
//         if (0 == result)
//         {
//             nwy_ext_echo("\r\n nwy_http_get success");
//         }
//         else
//         {
//             nwy_ext_echo("\r\n nwy_http_get fail");
//         }
//        // nwy_sleep(5000);
//         // result = nwy_http_head(keepalive, 0);
//         // if (0 == result)
//         // {
//         //     nwy_ext_echo("\r\n nwy_http_head success");
//         // }
//         // else
//         // {
//         //     nwy_ext_echo("\r\n nwy_http_head fail");
//         // }
//     }
//     else
//     {
//         nwy_ext_echo("\r\n %s fail", "nwy_https_setup");
//     }

    
// }


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
#include "nwy_ftp.h"
#include "nwy_http.h"
#include "nwy_file.h"

#define TOTAL_FILE_LEN 1024
#if 0
int32_t write_to_file_path(char*file_path, char*data, uint32_t data_len);

extern device_config bb_device_config;

extern int ota_content_length;

int ota_content_offset = 0;

int ota_http_size = 1024;

typedef struct nwy_file_https_info_s
{
    int is_vaild;
    char filename[256];
    int pos;
    // int length;
    // int file_size;
} nwy_file_https_info_s;
static nwy_file_https_info_s g_filehttps_info;

void ota_get(uint8_t keepalive, uint8_t offset, uint8_t size, bool https)
{
    int result = 0;
    result = nwy_http_get(keepalive, offset, size, https);
    if (0 == result)
    {
        nwy_ext_echo("\r\n nwy_https_get success");
    }
    else
    {
        nwy_ext_echo("\r\n nwy_http_get fail");
    }
}

int test_https_write_file(nwy_file_https_info_s *pFileFtp, unsigned char *data, unsigned int len)
{
    char file_name[128 + 1] = "/nwy/";
    strcpy(file_name,  "/sdcard0/");
    strcat(file_name, pFileFtp->filename);
    int fs = nwy_sdk_fopen(file_name, NWY_CREAT | NWY_RDWR);
    if (fs < 0)
    {
        OSI_LOGE(0, "nwy file open fail");
        return -1;
    }

    nwy_sdk_fseek(fs, pFileFtp->pos, 0);
    nwy_sdk_fwrite(fs, data, len);
    nwy_sdk_fclose(fs);
    pFileFtp->pos += len;
    OSI_LOGE(0, "nwy file write %d size success.", len);
    return 0;
}


void http_result_cb(nwy_ftp_result_t *param)
{
    int *size;
    int ret;
    
    nwy_ext_echo("\r\n In http_result_cb\r\n");
    
    if (NULL == param)
    {
        nwy_ext_echo("event is NULL\r\n");
    }

    OSI_LOGI(0, "event is %d", param->event);
    if (NWY_HTTP_DNS_ERR == param->event)
    {
        nwy_ext_echo("\r\nHTTP dns err");
    }
    else if (NWY_HTTP_OPEN_FAIL == param->event)
    {
        nwy_ext_echo("\r\nHTTP open fail");
    }
    else if (NWY_HTTP_OPENED == param->event)
    {
        nwy_ext_echo("\r\nHTTP setup success");
    }
    else if (NWY_HTTPS_SSL_CONNECTED == param->event)
    {
        nwy_ext_echo("\r\nHTTPS setup success line 121\r\n");
        nwy_ext_echo("\r\nHTTPS setup success line 122\r\n");
        nwy_sleep(10);
        int ota_http_size = 1024;
        ota_get(1, ota_content_offset, ota_http_size, 1); 
    }
    else if (NWY_HTTP_CLOSED_PASV == param->event || NWY_HTTP_CLOSED == param->event)
    {
        nwy_ext_echo("\r\nHTTP closed");
    }
    else if (NWY_HTTP_DATA_RECVED == param->event)
    {
         nwy_ext_echo("\r\nHTTP data recv");

        nwy_ext_echo("\r\nHTTP recv data len %d.\r\n", param->data_len);
        char *buff = (char *)malloc(NWY_EXT_SIO_RX_MAX);
        if (NULL == buff)
        {
            nwy_ext_echo("%s\r\n", param->data);
            return;
        }
        int offset = 0;
        int tmeplen = 0;
        while (param->data_len)
        {
            memset(buff, 0x00, NWY_EXT_SIO_RX_MAX);
            tmeplen = param->data_len > NWY_EXT_SIO_RX_MAX ? NWY_EXT_SIO_RX_MAX : param->data_len;
            memcpy(buff, param->data + offset, tmeplen);
            nwy_ext_echo("%s", buff);
            offset += tmeplen;
            param->data_len -= tmeplen;
            OSI_LOGI(0, "param->data_len %d", param->data_len);

            ret = test_https_write_file(&g_filehttps_info, param->data, (param->data_len - ota_http_size));
            if (0 != ret)
            {
                nwy_ext_echo("\r\nwrite failed");
            }
            else
            {
                nwy_ext_echo("\r\n write success");
                //ota_file_read();
                ota_content_offset = ota_content_offset + ota_http_size - 1; // to make 1023 than 1024
                long ota_diff_size = ota_content_length - ota_content_offset;
                if (ota_diff_size >= 1024)
                {
                    ota_http_size = 1024;
                    ota_get(1, ota_content_offset, ota_http_size, 1); // getting 1024 for the first time with a offset of 0
                }
                else if ((ota_diff_size > 0) && (ota_diff_size <= 1023))
                {
                    ota_http_size = ota_diff_size;
                    ota_get(1, ota_content_offset, ota_http_size, 1); // getting 1024 for the first time with a offset of 0
                }
                else
                {
                   // ia_fota_test();
                   nwy_ext_echo("The file has been downloaded successfully\r\n");
                }
            }
        }
        free(buff);
        nwy_ext_echo("\r\n");
    }
    else if (NWY_HTTP_DATA_SEND_ERR == param->event)
    {
        nwy_ext_echo("\r\ndata send error");
    }
    else if (NWY_HTTP_DATA_SEND_FINISHED == param->event)
    {
        nwy_ext_echo("\r\ndata send finished");
    }
    else if (NWY_HTTPS_SSL_INIT_ERROR == param->event)
    {
        nwy_ext_echo("\r\nHTTPS SSL init fail");
    }
    else if (NWY_HTTPS_SSL_HANDSHAKE_ERROR == param->event)
    {
        nwy_ext_echo("\r\nHTTPS SSL handshare fail");
    }
    else if (NWY_HTTPS_SSL_AUTH_FAIL == param->event)
    {
        nwy_ext_echo("\r\nHTTPS SSL Authentication fail");
    }
    else
    {
        nwy_ext_echo("\r\n unkown error");
    }

    return;
}


int32_t write_to_file_path(char*file_path, char*data, uint32_t data_len)
{
    int32_t ret = 0;
    int32_t fd = 0;
    int32_t write_len = 0;
    int32_t file_len = 0;
    int32_t file_offset = 0;
    int32_t file_size = 0;
    int32_t file_size_len = 0;

    fd = nwy_sdk_fopen(file_path, NWY_CREAT | NWY_RDWR | NWY_APPEND);
    if (fd < 0)
    {
        nwy_ext_echo("open file failed, fd = %d\r\n", fd);
    }
    else
    {
        int retvalue = nwy_sdk_fwrite(fd, data, data_len);
        if(retvalue <0)
        {
            nwy_ext_echo("File read failed\r\n");
        }
    }
    nwy_sdk_fclose(fd);
}

int bytebeam_http_download_to_file(char* file_path, char *url, int content_length)
{
    int result;
    uint8_t channel;
    char buf[NWY_HTTP_POST_MAX_DATA_LEN] = {0};
    char filename[128] = {0};
    int port;
    int keepalive;
    int offset;
    int size;
    uint8_t type;
    nwy_app_ssl_conf_t ssl_cfg;
    nwy_osiEvent_t event;
    channel = 1;
    port = 883;
    memset(&ssl_cfg, 0x00, sizeof(ssl_cfg));
    ssl_cfg.ssl_version = 3;
    ssl_cfg.authmode = 2;

    strncpy(ssl_cfg.cacert.cert_name, bb_device_config.ca_cert_pem_name, strlen(bb_device_config.ca_cert_pem_name));
    strncpy(ssl_cfg.clientcert.cert_name, bb_device_config.client_cert_pem_name, strlen(bb_device_config.client_cert_pem_name));
    strncpy(ssl_cfg.clientkey.cert_name, bb_device_config.client_key_pem_name, strlen(bb_device_config.client_key_pem_name));

    nwy_ext_echo("\r\n In HTTP download\r\n");
    nwy_sleep(1000);
    result = nwy_https_setup(channel, url, port, http_result_cb, &ssl_cfg);
    
    if (0 == result)
    {
        nwy_ext_echo("\r\nHTTP setup success\r\n");
       
        nwy_ext_echo("\r\n %s success", "nwy_https_setup");
        memset(&g_filehttps_info, 0x00, sizeof(g_filehttps_info));
        g_filehttps_info.is_vaild = 1;
        strcpy(g_filehttps_info.filename, "ota.bin");
    }
    else
    {
        nwy_ext_echo("\r\nHTTP setup fail\r\n");
        return -1;
    }
}

#endif