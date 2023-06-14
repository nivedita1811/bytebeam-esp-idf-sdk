
#include "bytebeam.h"
#include "nwy_gpio.h"
#include "nwy_osi_api.h"
#include "nwy_sim.h"
#include "nwy_vir_at.h"
#include "osi_log.h"
#include <stdio.h>
#include <stdlib.h>
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
#include "nwy_adc.h"
#include "nwy_uart.h"
#include "nwy_network.h"
#include "nwy_file.h"
#include "nwy_socket.h"
#include "json_server.h"
#include "cJSON.h"
#include "nwy_ftp.h"
#include "nwy_http.h"
#include "bytebeam.h"
#include "untar.h"

#define NWY_APPIMG_FOTA_BLOCK_SIZE (4096+512) //(2*4096)                       //changed from 1024 to 4096
#define TEST_BLOCK_SIZE 1024

// char filename[128] = {"tork_firmware_app.bin"};
#define TAR_BLOCK_SIZE 512
char *tar_file = "nwy/test_tar.tar";


extern volatile int http_download_flag;
extern int network_thread_counter;
// char test_filename[128] = {"test_app.pack"};
char test_filename[128] = {"test_tar.tar"};
bool ota_file_write = false;

long ota_http_offset = 0;
int ota_http_size;

extern char ota_action_id[10];

static int update_completed = 0;


void ota_get(uint8_t keepalive, uint32_t offset, uint32_t size, bool https);
typedef struct nwy_file_ftp_info_s
{
    int is_vaild;
    char filename[256];
    int pos;
    // int length;
    // int file_size;
} nwy_file_ftp_info_s;
static nwy_file_ftp_info_s g_fileftp;

void http_result_cb(nwy_ftp_result_t *param);
void http_test(uint8_t *ota_url);
void ftp_test();
void ftp_result_cb(nwy_ftp_result_t *param);
int test_ftp_write_file(nwy_file_ftp_info_s *pFileFtp, unsigned char *data, unsigned int len);
static uint8 nwy_update_flag = 0;

void http_test(uint8_t *ota_url)
{
    int result;
    uint8_t channel;
    char buf[NWY_HTTP_POST_MAX_DATA_LEN] = {0};
    char filename[128] = {0};
    int port;

    int size;
    uint8_t type;
    nwy_app_ssl_conf_t ssl_cfg;
    nwy_osiEvent_t event;

    while (1)
    {
        nwy_ext_echo("\nhttps_test\n");
        channel = 1;
        port = 443;
        memset(&ssl_cfg, 0x00, sizeof(ssl_cfg));
        ssl_cfg.ssl_version = 3;
        ssl_cfg.authmode = 2;
        strncpy(ssl_cfg.cacert.cert_name, "ca_cert.pem", strlen("ca_cert.pem"));
        strncpy(ssl_cfg.clientcert.cert_name, "client_cert.pem", strlen("client_cert.pem"));
        strncpy(ssl_cfg.clientkey.cert_name, "client_key.pem", strlen("client_key.pem"));
       // nwy_sleep(10000);
       //"firmware.cloud.bytebeam.io/api/v1/firmwares/0.0.1/artifact"
        result = nwy_https_setup(channel, ota_url, port, http_result_cb, &ssl_cfg);
        if (0 == result)
        {
            nwy_ext_echo("\r\n Bytebeam https_setup in progress\n");
            memset(&g_fileftp, 0x00, sizeof(g_fileftp));
            g_fileftp.is_vaild = 1;
            strcpy(g_fileftp.filename, filename);
            http_flag = false;
        }
        else
        {
            nwy_ext_echo("\r\n %s fail", "Bytebeam https_setup");
        }
        break;
    }
}
extern int ota_content_length;
void http_result_cb(nwy_ftp_result_t *param)
{
    int result;
    int *size;
    int ret;
    int keepalive;
    int offset;
    static int32_t total_legth_received =0;
    static int32_t remaining_length_to_receive =0;
    static int update_checkpoint = 0;

    if (NULL == param)
    {
        nwy_ext_echo("event is NULL\r\n");
    }
     nwy_ext_echo("\r\nHTTP CALL BACK");
    OSI_LOGI(0, "event is %d", param->event);
    if (NWY_HTTP_DNS_ERR == param->event)
    {
        nwy_ext_echo("\r\nHTTP dns err");
    }
    else if (NWY_HTTP_OPEN_FAIL == param->event)
    {
        publish_action_status(ota_action_id,5, "Failed", "HTTP open fail");
        nwy_resume_thread(gps_app_thread);
        nwy_resume_thread(device_config_thread_handle);
        http_download_flag = 0;
        nwy_ext_echo("\r\nHTTP open fail");
        send_ack_nack(0, 0x77, NULL, 0);
    }
    else if (NWY_HTTP_OPENED == param->event)
    {
        nwy_ext_echo("\r\nHTTP setup success");
    }
    else if (NWY_HTTPS_SSL_CONNECTED == param->event)
    {
        nwy_ext_echo("\r\nHTTPS setup success");
        // nwy_sdk_file_unlink("/nwy/tork_firmware_app.bin");
        nwy_sdk_file_unlink("/nwy/test_app.pack");
        nwy_sdk_file_unlink("/nwy/test_app.bin");
        nwy_sdk_file_unlink("/nwy/tork_firmware_app.bin");
        nwy_sdk_file_unlink("/nwy/test_tar.tar");
        nwy_ext_echo("\r\n...............SSL connected..............\r\n");
        // if(nwy_read_sdcart_status() != true)
        // {
        //     publish_action_status(ota_action_id,15, "Failed", "Error in reading SD card");
        //     nwy_sleep(5000);
        //     nwy_power_off(2);
        // }
        publish_action_status(ota_action_id,15, "Progress", "");
        
        if(ota_content_length > 4096)                 //changed from 1024 to 4096
        {
            ota_http_size = 4096;
        }
        else
        {
            ota_http_size = ota_content_length;
        }
        ota_get(1, ota_http_offset, ota_http_size, 1); // getting 1024 for the first time with a offset of 0
    }
    else if (NWY_HTTP_CLOSED_PASV == param->event || NWY_HTTP_CLOSED == param->event)
    {
        if (update_completed)
        {
        // Deleting the files after the update is completed
        nwy_sdk_file_unlink("/nwy/test_app.pack");
        nwy_sdk_file_unlink("/nwy/test_app.bin");
        nwy_sdk_file_unlink("/nwy/tork_firmware_app.bin");
        nwy_sdk_file_unlink("/nwy/test_tar.tar");
        
        // Reseting the flag for future updates
        update_completed = 0;
        nwy_ext_echo("\r\nHTTP closed");
        }
    }
    else if (NWY_HTTP_DATA_RECVED == param->event)
    {
       nwy_ext_echo("\r\nHTTP Data Received\r\n");
       int32_t header_length = (param->data_len - ota_http_size);
       uint8_t header_buff[200] = {0};
       nwy_ext_echo("Data length is %d\r\n",param->data_len);
       nwy_ext_echo("http size is %d\r\n",ota_http_size);
       nwy_ext_echo("Header length is %d\r\n",header_length);
    //    memcpy(header_buff,(param->data),param->data_len);
    //    nwy_ext_echo("Header content is \r\n %.*s \r\n",(int)param->data_len,header_buff);
       test_ftp_write_file(&g_fileftp, (param->data)+header_length, param->data_len - header_length);
       total_legth_received += param->data_len - header_length; 
       remaining_length_to_receive = ota_content_length - total_legth_received;
       
       if(ota_content_length > 8192)
       {
            if((total_legth_received > (ota_content_length/4)) && (total_legth_received < (ota_content_length/2)) && update_checkpoint == 0)
            {
                    update_checkpoint = 1;
                    publish_action_status(ota_action_id,30, "Progress", "");
            }
            if((total_legth_received > (ota_content_length/2)) && (total_legth_received < (ota_content_length*3/4)) && update_checkpoint == 1)
            {
                    update_checkpoint = 2;
                    publish_action_status(ota_action_id,45, "Progress", "");
            }
            if((total_legth_received > (ota_content_length*3/4)) && (total_legth_received < (ota_content_length)) && update_checkpoint == 2)
            {
                    update_checkpoint = 3;
                    publish_action_status(ota_action_id,60, "Progress", "");
            }
       }
       
       if(remaining_length_to_receive > 4096)                       //changed from 1024 to 4096
       {
            ota_get(1, total_legth_received, 4096, 1);              //changed from 1024 to 4096
            nwy_ext_echo("\r\nHTTP remaining data to receive: %d\r\n", remaining_length_to_receive);
       }
       else if(remaining_length_to_receive > 0)
       {
            ota_http_size = remaining_length_to_receive;
            ota_get(1, total_legth_received, remaining_length_to_receive, 1);
            nwy_ext_echo("\r\nHTTP Data Received last packet: %d\r\n", remaining_length_to_receive);
       }
       else
       {
            nwy_ext_echo("\r\nOTA Download Completed\r\n");
            if(ota_content_length == total_legth_received)
            {
                publish_action_status(ota_action_id,75, "Progress", "");
                untar_files();
                // ia_fota_test();
                nwy_semahpore_release(tork_update_semaphore);
            }
            else
            {
                nwy_ext_echo("Ota content length: %d, total length received: %d", ota_content_length, total_legth_received);
            }
            ota_content_length = 0;
            total_legth_received = 0;
            remaining_length_to_receive = 0;
            update_checkpoint = 0;
            //need to trigger external ECU update    
       }

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
        http_download_flag = 0;
        nwy_resume_thread(gps_app_thread);
        nwy_resume_thread(device_config_thread_handle);
        publish_action_status(ota_action_id,5, "Failed", "HTTPS SSL init fail");
        nwy_ext_echo("\r\nHTTPS SSL init fail");
        send_ack_nack(0, 0x77, NULL, 0);
    }
    else if (NWY_HTTPS_SSL_HANDSHAKE_ERROR == param->event)
    {
        http_download_flag = 0;
        nwy_resume_thread(gps_app_thread);
        nwy_resume_thread(device_config_thread_handle);
        publish_action_status(ota_action_id,5, "Failed", "HTTPS SSL Handshake fail");
        nwy_ext_echo("\r\nHTTPS SSL handshake fail");
        send_ack_nack(0, 0x77, NULL, 0);
    }
    else if (NWY_HTTPS_SSL_AUTH_FAIL == param->event)
    {
        http_download_flag = 0;
        nwy_resume_thread(gps_app_thread);
        nwy_resume_thread(device_config_thread_handle);
        publish_action_status(ota_action_id,5, "Failed", "HTTPS SSL Authentication fail");
        nwy_ext_echo("\r\nHTTPS SSL Authentication fail");
        send_ack_nack(0, 0x77, NULL, 0);
    }
    else
    {
        // HTTP_TRIGGER;
        // http_flag = true;
        nwy_ext_echo("\r\n unkown error");
    }
  //  http_flag = false;
    return;
}
nwy_result_type result;

int test_ftp_write_file(nwy_file_ftp_info_s *pFileFtp, unsigned char *data, unsigned int len)
{
    uint8_t test_buff[283] = {0};
    char file_name[128 + 1] = "/nwy/";
    uint32_t file_size = 0;
    strcpy(pFileFtp->filename, test_filename);
    strcat(file_name, pFileFtp->filename);
    int fs = nwy_sdk_fopen(file_name, NWY_CREAT | NWY_RDWR | NWY_APPEND);
    if (fs < 0)
    {
        OSI_LOGE(0, "nwy file open fail");
        nwy_ext_echo("\r\n file open failed:%s\r\n", file_name);
        return -1;
    }

    //nwy_sdk_fseek(fs, pFileFtp->pos, 0);
    file_size = nwy_sdk_fsize_fd(fs);
    nwy_ext_echo("\r\n file_size is %d, writing:%d\r\n", file_size, len);
    nwy_sdk_fseek(fs, file_size, NWY_SEEK_SET);
    nwy_sdk_fwrite(fs, data, len);
    
    pFileFtp->pos += len;
    OSI_LOGE(0, "nwy file write %d size success.", len);
    nwy_ext_echo("\r\nnwy file write %d size success.\r\n", len);
    file_size = nwy_sdk_fsize_fd(fs);
    nwy_ext_echo("\r\n file_size is %d, after writting \r\n", file_size);
    nwy_sdk_fclose(fs);

    // fs = nwy_sdk_fopen(file_name,NWY_RDONLY);
    // int read_stat =  nwy_sdk_fread(fs, test_buff, file_size);
    // nwy_sdk_fclose(fs);
    
    // int loop_var = 0;
    // for(loop_var = 0;loop_var < 283;loop_var++)
    // {
    //     nwy_ext_echo("%d byte val:- %X\r\n", loop_var, test_buff[loop_var]);
    // }
    return 0;
}

void ia_fota_test()
{
    int fd;
    ota_package_t ota_pack = {0};
    int ota_size = 0;
    int tmp_len = 0;
    int read_len = 0;
    char buff[TEST_BLOCK_SIZE] = {0};
    int ret = -1;
    int len = 0;

    // nwy_sdk_file_unlink("/nwy/fota.pack");
    // char *fota_file = "/nwy/V006_FM.pack";
    char *fota_file = "/nwy/test_app.pack";
    fd = nwy_sdk_fopen(fota_file, NWY_RDONLY);

    if (fd < 0)
    {
        nwy_ext_echo("\r\nopen appimg fail\r\n");
        return;
    }
    else
    {
        nwy_ext_echo("\r\nopen appimg sucess\r\n");
    }
    ota_pack.data = (unsigned char *)malloc(TEST_BLOCK_SIZE);
    ota_pack.len = 0;
    ota_pack.offset = 0;

    if (ota_pack.data == NULL)
    {
        nwy_ext_echo("\r\nmalloc fail\r\n");
        return;
    }

    ota_size = nwy_sdk_fsize_fd(fd);

    nwy_ext_echo("ota_size:%d\r\n", ota_size);

    nwy_sdk_fseek(fd, 0, NWY_SEEK_SET);

    while (ota_size > 0)
    {
        tmp_len = TEST_BLOCK_SIZE;
        if (ota_size < tmp_len)
        {
            tmp_len = ota_size;
        }
        read_len = nwy_sdk_fread(fd, buff, tmp_len);
        if (read_len <= 0)
        {
            nwy_ext_echo("read file error:%d\r\n", read_len);
            free(ota_pack.data);

            nwy_sdk_fclose(fd);
            return;
        }
        
        nwy_ext_echo("read len:%d\r\n", read_len);
        memcpy(ota_pack.data, buff, read_len);
        ota_pack.len = read_len;
        // int loop_var = 0;
        // for(loop_var = 0;loop_var < ota_pack.len;loop_var++)
        // {
        //     nwy_ext_echo(" %d pack data:-%X\r\n",loop_var,*(ota_pack.data + loop_var));
        // }
        unsigned wsize = nwy_fota_download_core(&ota_pack);
        //     unsigned int wsize= nwy_fota_dm(&ota_pack);
        if (wsize != 0)
        {
            publish_action_status(ota_action_id, 95, "Failed", "Fota download core fail");
            nwy_ext_echo("\r\n Fota : write firmware fail");
            free(ota_pack.data);
            nwy_sdk_fclose(fd);
            return;
        }
        else
        {
            nwy_ext_echo("\r\n Fota : write firmware success");
            // publish_action_status(ota_action_id, 100, "Completed", "");
        }
        ota_pack.offset += read_len;
        memset(ota_pack.data, 0, read_len);
        ota_size -= read_len;
    }
    free(ota_pack.data);
    unsigned long res = nwy_sdk_vfs_ls();
    nwy_ext_echo("res %ld", res);
    nwy_sdk_fclose(fd);

    nwy_ext_echo("\r\n write end.");
    // ret = nwy_sdk_file_unlink(fota_file);
    nwy_ext_echo("\r\n del %s \r\n", fota_file);
    nwy_ext_echo("\r\n Fota start : start firmware updata");

    if (nwy_version_update(false))
    {
        publish_action_status(ota_action_id, 95, "Failed", "nwy version update fail");
        nwy_sleep(3000);
        publish_action_status(ota_action_id, 95, "Failed", "nwy version update fail");
        nwy_ext_echo("\r\n nwy_version_update :  updata fail");
    }
    else
    {
        publish_action_status(ota_action_id, 100, "Completed", "");
        nwy_ext_echo("\r\n nwy_version_update :  updata suceess!");
        // After the update is completed, setting the flag
        update_completed = 1;

    }

    return;
}

void ota_file_read()
{
    int fd;
    ota_package_t ota_pack = {0};
    int ota_size = 0;
    int tmp_len = 0;
    int read_len = 0;
    char buff[NWY_APPIMG_FOTA_BLOCK_SIZE] = {0};
    int ret = -1;
    int len = 0;

   // nwy_sdk_file_unlink("/nwy/fota.pack");
    char *fota_file = "/nwy/test.bin";
    fd = nwy_sdk_fopen(fota_file, NWY_RDONLY);

    if (fd < 0)
    {
        nwy_ext_echo("\r\nopen appimg fail\r\n");
        return;
    }
    else
    {
        nwy_ext_echo("\r\nopen appimg sucess\r\n");
    }

    nwy_sdk_fseek(fd, 0, NWY_SEEK_SET);

    read_len = nwy_sdk_fread(fd, buff, 1024);
    if (read_len <= 0)
    {
        nwy_ext_echo("read file error:%d\r\n", read_len);
        free(ota_pack.data);

        nwy_sdk_fclose(fd);
        return;
    }
    nwy_ext_echo("read len:%d\r\n", read_len);
    nwy_ext_echo("\nOTA FILE READ IS %s", buff);
}

void ota_get(uint8_t keepalive, uint32_t offset, uint32_t size, bool https)
{
    nwy_ext_echo("\r\ngetting:%d of %d, %d\r\n", offset, ota_content_length,size);
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

void untar_files()
{
    /*************UNTAR************/
    uint8_t* n58_filename = "/nwy/test_app.pack";
    uint8_t* s32_filename = "/nwy/test_app.bin";
    uint8_t* tork_filename = "/nwy/tork_firmware_app.bin";
    int test_fd = nwy_sdk_fopen(tar_file,NWY_RDONLY);
    if(test_fd == NULL)
    {
        nwy_ext_echo("Error in opening tar file\r\n");
    }

    uint8_t test_buffer[TAR_BLOCK_SIZE] = {0};
    uint8_t blocks_count = 0;
    int padding = 0;
    tar_header_t header;

    while((nwy_sdk_fread(test_fd, &header, TAR_BLOCK_SIZE)) == TAR_BLOCK_SIZE)
    // while (fread(&header, BLOCK_SIZE, 1, fp) == 1) 
    {
        if(header.name[0] == NULL && header.name[1] == NULL)
        {
            break;
        }
        // Print the name and size of each file in the tar archive
        nwy_ext_echo("Name: %s\r\n", header.name);
        nwy_ext_echo("Size: %d\r\n", strtol(header.size, NULL, 8));

        // Calculate the size of the file in bytes
        int size = strtol(header.size, NULL, 8);
        if ((strcmp(header.name, "test_tar/test_app.pack") == 0) ||
            (strcmp(header.name, "test_tar/test_app.bin") == 0) ||
            (strcmp(header.name, "test_tar/tork_firmware_app.bin") == 0))
        {
            int update_file_fd;
            if(strcmp(header.name, "test_tar/test_app.pack") == 0)
            {
                update_file_fd = nwy_sdk_fopen(n58_filename, NWY_CREAT | NWY_RDWR | NWY_APPEND);
                if(update_file_fd == NULL)
                {
                    nwy_ext_echo("Error in creating n58 update file\r\n");
                }
            }

            if(strcmp(header.name, "test_tar/test_app.bin") == 0)
            {
                update_file_fd = nwy_sdk_fopen(s32_filename, NWY_CREAT | NWY_RDWR | NWY_APPEND);
                if(update_file_fd == NULL)
                {
                    nwy_ext_echo("Error in creating S32 update file\r\n");
                }
            }

            if(strcmp(header.name, "test_tar/tork_firmware_app.bin") == 0)
            {
                update_file_fd = nwy_sdk_fopen(tork_filename, NWY_CREAT | NWY_RDWR | NWY_APPEND);
                if(update_file_fd == NULL)
                {
                    nwy_ext_echo("Error in creating tork update file\r\n");
                }
            }
            
            if(size > TAR_BLOCK_SIZE)
            {
                blocks_count = (size/TAR_BLOCK_SIZE);
                for(int i = 0;i < blocks_count;i++)
                {
                    nwy_sdk_fread(test_fd, test_buffer, TAR_BLOCK_SIZE);
                    nwy_sdk_fwrite(update_file_fd, test_buffer, TAR_BLOCK_SIZE);
                }
                if((size % TAR_BLOCK_SIZE) != 0)
                {
                    int remaining_bytes = size % TAR_BLOCK_SIZE;
                    padding = TAR_BLOCK_SIZE - remaining_bytes;
                    nwy_sdk_fread(test_fd, test_buffer, remaining_bytes);
                    nwy_sdk_fwrite(update_file_fd, test_buffer, remaining_bytes);
                    nwy_sdk_fseek(test_fd, padding, SEEK_CUR);
                }
                nwy_sdk_fclose(update_file_fd);
            }
            if(size <= TAR_BLOCK_SIZE)
            {
                nwy_sdk_fread(test_fd, test_buffer, size);
                nwy_sdk_fwrite(update_file_fd, test_buffer, size);
                nwy_sdk_fclose(update_file_fd);
                if((size % TAR_BLOCK_SIZE) != 0)
                {
                    padding = TAR_BLOCK_SIZE - size;
                    nwy_sdk_fseek(test_fd, padding, SEEK_CUR);
                }
            }
        }
        else
        {
            nwy_sdk_fseek(test_fd, size, SEEK_CUR);
            
            // Check for end of tar archive
            if (size % TAR_BLOCK_SIZE != 0) 
            {
                int padding = TAR_BLOCK_SIZE - (size % TAR_BLOCK_SIZE);
                nwy_sdk_fseek(test_fd, padding, SEEK_CUR);
            }
        }
    }

    nwy_sdk_fclose(test_fd);

    /*********************************************/
}


