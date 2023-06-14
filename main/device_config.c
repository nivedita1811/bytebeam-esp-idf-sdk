#include "bytebeam.h"
#include "common.h"
#include "nwy_file.h"
#include "device_config.h"

static uint8 nwy_update_flag = 0;
extern nwy_osiThread_t *device_config_thread_handle;
static int nwy_ext_sio_len = 0;

extern int config_data_present_in_nwy;
char *device_config_path = "/sdcard0/device_config.json";
static bool device_config_update_flag = false;
static int device_config_crc = 0;

extern void mqtt_topics_init();

static void nwy_ext_sio_data_device_config_cb(const char *data, uint32 length);
static void sd_card_data_retrieve();
static void create_device_config();
static void delete_old_device_config();
static int get_device_config_crc();

void DeviceConfig_ThreadEntry(void *param) {
    nwy_ext_echo("\r\n****************************\r\n");
    nwy_ext_echo("\r\nDevice Config thread created\r\n");
    nwy_sdk_sdcard_mount();
    char *sptr = nwy_ext_sio_recv_buff;
    nwy_usb_serial_reg_recv_cb((nwy_sio_recv_cb_t) nwy_ext_sio_data_device_config_cb);
    int device_config_fd;

    if (nwy_sdk_fexist(device_config_path) || nwy_sdk_fexist("/nwy/device_config.json")) 
    {
        nwy_ext_echo("Using existing device config\r\n");
        sd_card_data_retrieve();
        mqtt_topics_init();
    } 
    else 
    {
        nwy_ext_echo("Did not find existing device config\r\n");
    }
    nwy_ext_input_gets("\r\nDevice Config Options:\n"
                       "u Update existing config\n"
                       "c Create new configuration\n");
    nwy_ext_echo("Got option: %c\n", *sptr);
    while(1) {
        switch (*sptr) {
            case 'u': {
                nwy_ext_echo("Updating existing configuration not supported\n");
                device_config_fd = nwy_sdk_fopen(device_config_path, NWY_RDWR);
                delete_old_device_config();
                nwy_sdk_fclose(device_config_fd);
                device_config_update_flag = true;
                break;
            }
            case 'c': {
                create_device_config();
                device_config_crc = get_device_config_crc();
                // nwy_sdk_fclose(device_config_fd);
                device_config_update_flag = true;
                break;
            }
            default:
                if (nwy_sdk_fexist(device_config_path)) {
                    device_config_update_flag = false;
                    break;
                }
        }

        if (device_config_update_flag) {
            sd_card_data_retrieve();
            mqtt_topics_init();
            nwy_ext_echo("mqtt_topics initialized\n");
            device_config_update_flag = false;
        }
        nwy_sleep(50);
    }
    nwy_exit_thread();
}

static void sd_card_data_retrieve()
{
    static int parse_retries = 0;
    do{
        parse_retries++;
        if(parse_retries > 60)
        {
            nwy_power_off(2);
        }
        nwy_sdk_sdcard_unmount();
        nwy_sleep(500);
        if (nwy_sdk_sdcard_mount() == true)
        {
            nwy_ext_echo("Preparing SD card for config data retrieval\r\n");
            nwy_sleep(500);
        }
        else
        {
            nwy_ext_echo("Config data retrieval failed...Failed to mount SD Card\r\n");
            nwy_sleep(3000);
        }
    } while(parse_device_config_file() < 0);
}

static void create_device_config()
{
    if (nwy_sdk_fexist(device_config_path)) {
        nwy_sdk_file_unlink(device_config_path);
    }
    if (nwy_sdk_fexist("/nwy/device_config.json")) {
        nwy_sdk_file_unlink("/nwy/device_config.json");
    }
    int device_config_fd = nwy_sdk_fopen(device_config_path, NWY_RDWR | NWY_CREAT);
    int written = 0;
    char *sptr = nwy_ext_sio_recv_buff;
    nwy_ext_input_gets("Enter file\n");
    nwy_ext_echo("\nGOT: %s :: SIZE: %d", sptr, strlen(sptr));
    nwy_sdk_fwrite(device_config_fd, sptr, strlen(sptr));
//    device_config_update_flag = true;
    nwy_sdk_fclose(device_config_fd);
    int fsize = nwy_sdk_fsize(device_config_path);
    char buffer[fsize];
    nwy_ext_echo("\nFREAD: %s \n", buffer);
}

static void delete_old_device_config()
{
    // TODO: Replace config not implemented yet
    nwy_ext_echo("%s(): Not implemented yet\n", __func__ );
}

static int get_device_config_crc()
{
    // TODO: Add crc check for received data
    nwy_ext_echo("%s(): Not implemented yet\n", __func__ );
    return 0;
}

static void nwy_ext_sio_data_device_config_cb(const char *data, uint32 length)
{
    static int len = 0;
    OSI_LOGI(0, "\r\nnwy length = %d", length);
    if (nwy_update_flag == 0)
    {
        // clear the nwy_ext_sio_recv_buff,in next time
        if (len == 0)
        {
            OSI_LOGI(0, "\r\nnwy reset buff");
            nwy_ext_sio_len = 0;
            memset(nwy_ext_sio_recv_buff, 0, NWY_EXT_SIO_RX_MAX);
        }
        if (len + length > NWY_EXT_SIO_RX_MAX)
        {
            memcpy(nwy_ext_sio_recv_buff + len, data, (NWY_EXT_SIO_RX_MAX - len));
            nwy_ext_sio_len = nwy_ext_sio_len + (NWY_EXT_SIO_RX_MAX - len);
            len = 0;
            if(nwy_ext_sio_recv_buff[nwy_ext_sio_len - 1] == '\r')
                nwy_ext_sio_len --;
            nwy_ext_sio_recv_buff[nwy_ext_sio_len] = '\0';
            nwy_ext_echo("\r\n%s", nwy_ext_sio_recv_buff);
            nwy_ext_send_sig(device_config_thread_handle, NWY_EXT_INPUT_RECV_MSG);
            return;
        }
        else
        {
            memcpy(nwy_ext_sio_recv_buff + len, data, length);

            OSI_LOGXI(OSI_LOGPAR_S, 0, "nwy:%s", nwy_ext_sio_recv_buff);
            if (length > 1)
            {
                if (data[length - 1] == '\n' && data[length - 2] == '\r')
                {
                    nwy_ext_sio_recv_buff[len + length - 2] = '\0';
                    nwy_ext_sio_len = nwy_ext_sio_len + length - 2;
                    len = 0;

                    OSI_LOGI(0, "nwy: recv over nwy_ext_sio_len = %d", nwy_ext_sio_len);
                    nwy_ext_echo("\r\n%s", nwy_ext_sio_recv_buff);
                    nwy_ext_send_sig(device_config_thread_handle, NWY_EXT_INPUT_RECV_MSG);
                    return;
                }
                else
                {
                    nwy_ext_sio_len = nwy_ext_sio_len + length;
                }
            }
            else if (length == 1)
            {
                if (data[length - 1] == '\r' || data[length - 1] == '\n')
                {
                    nwy_ext_sio_recv_buff[nwy_ext_sio_len - 1] = '\0';
                    nwy_ext_sio_len = nwy_ext_sio_len - 1;
                    len = 0;
                    nwy_ext_echo("\r\n%s", nwy_ext_sio_recv_buff);
                    nwy_ext_send_sig(device_config_thread_handle, NWY_EXT_INPUT_RECV_MSG);
                    return;
                }
                else
                {
                    memcpy(nwy_ext_sio_recv_buff + len, data, length);
                    nwy_ext_sio_len = nwy_ext_sio_len + length;
                }
            }
        }
        len = len + length;

    }
    else if (nwy_update_flag == 1)
    {
        nwy_ext_app_info_proc(data, length);
    }
    else if (nwy_update_flag == 2)
    {
        nwy_ext_appimg_info_proc(data, length);
    }
}
