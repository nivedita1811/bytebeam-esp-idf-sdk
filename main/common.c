#include "bytebeam.h"
#include "common.h"



typedef struct nwy_file_ftp_info_s
{
    int is_vaild;
    char filename[256];
    int pos;
    // int length;
    // int file_size;
} nwy_file_ftp_info_s;
typedef enum
{
    NWY_CUSTOM_IP_TYPE_OR_DNS_NONE = -1,
    NWY_CUSTOM_IP_TYPE_OR_DNS_IPV4 = 0,
    NWY_CUSTOM_IP_TYPE_OR_DNS_IPV6 = 1,
    NWY_CUSTOM_IP_TYPE_OR_DNS_DNS = 2
} nwy_ip_type_or_dns_enum;

#define NWY_NTP_BASE_TIME 0xBC191380

#define NWY_APPIMG_FOTA_BLOCK_SIZE (2 * 1024)
#define NWY_UART_RECV_SINGLE_MAX 512
#define NWY_FILE_NAME_MAX 20
#define NUM_OF_FILES 10
#define MSG_Q_BLOCK_SIZE 5

static uint8 nwy_update_flag = 0;
char nwy_ext_sio_recv_buff[NWY_EXT_SIO_RX_MAX + 1] = {0};
extern nwy_osiThread_t *network_app_thread;
extern nwy_osiThread_t *device_config_thread_handle;
static int nwy_ext_sio_len = 0;
dataRecvContext_t *g_recv = NULL;

void nwy_app_timer_cb(void)
{
    OSI_LOGI(0, "nwy_app_timer_cb");
    nwy_ext_echo("\r\n Timer test ok");
}

void nwy_ext_send_sig(nwy_osiThread_t *thread, uint16 sig)
{
    nwy_osiEvent_t event;

    memset(&event, 0, sizeof(event));
    event.id = sig;
    nwy_send_thead_event(thread, &event, 0);
}

void nwy_ext_echo(char *fmt, ...)
{
    static char echo_str[NWY_EXT_SIO_RX_MAX];
    static nwy_osiMutex_t *echo_mutex = NULL;
    va_list a;
    int i, size;

    if (NULL == echo_mutex)
        echo_mutex = nwy_create_mutex();
    if (NULL == echo_mutex)
        return;
    nwy_lock_mutex(echo_mutex, 0);
    va_start(a, fmt);
    vsnprintf(echo_str, NWY_EXT_SIO_RX_MAX, fmt, a);
    va_end(a);
    size = strlen((char *)echo_str);
    i = 0;
    while (1)
    {
        int tx_size;

        tx_size = nwy_usb_serial_send((char *)echo_str + i, size - i);
        if (tx_size <= 0)
            break;
        i += tx_size;
        if ((i < size))
            nwy_sleep(10);
        else
            break;
    }
    nwy_unlock_mutex(echo_mutex);
}

void nwy_ext_input_gets(char *msg, ...)
{
    static char echo_str[NWY_EXT_SIO_RX_MAX] = {0};
    nwy_osiEvent_t event;

    memset(&event, 0, sizeof(event));
    va_list a;
    va_start(a, msg);
    vsprintf(echo_str, msg, a);
    va_end(a);
    nwy_usb_serial_send(echo_str, strlen((char *)echo_str));
    while (1)
    {
        memset(&event, 0, sizeof(event));
        nwy_wait_thead_event(nwy_get_current_thread(), &event, 0);
        if (event.id == NWY_EXT_INPUT_RECV_MSG)
        {
            return;
        }
    }
}


void nwy_ext_sio_data_cb(const char *data, uint32 length)
{
    static int len = 0;
    OSI_LOGI(0, "\r\nnwy length = %d", length);
    if (nwy_update_flag == 0)
    {
//        if (length == 2 && data[length - 1] == '\n' && data[length - 2] == '\r')
//        {
//            OSI_LOGI(0, "\r\ndrop \\r\\n and input end");
//            nwy_ext_echo("\r\n%s", nwy_ext_sio_recv_buff);
//            len = 0;
//            nwy_ext_send_sig(device_config_thread_handle, NWY_EXT_INPUT_RECV_MSG);
//            return;
//        }
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



/* added by wangchen for N58 sms api to test 20200215 begin */

void nwy_ext_app_info_proc(const uint8_t *data, int length)
{
    memcpy((g_recv->data + g_recv->pos), data, length);
    g_recv->pos = g_recv->pos + length;

    nwy_ext_echo("\r\n nwy_app_info.rev_len = %d length = %d\r\n", g_recv->pos, length);
    if (g_recv->pos == g_recv->size)
    {
        nwy_ext_send_sig(network_app_thread, NWY_EXT_FOTA_DATA_REC_END);
        nwy_update_flag = 0;
    }
    if (g_recv->pos > g_recv->size)
    {
        nwy_ext_send_sig(network_app_thread, NWY_EXT_FOTA_DATA_REC_END);
        nwy_update_flag = 0;
        // nwy_ext_echo("\r\nAPP info is error,upgrade fail");
    }
}

void nwy_pull_out_sim()
{
    nwy_ext_echo("\r\n SIM pull out");
}

static void nwy_ext_virtual_at_test()
{
    int ret = -1;
    char resp[2048] = {0};
    nwy_at_info at_cmd;
    nwy_sleep(500);

    nwy_ext_input_gets("\r\nSend AT cmd :");
    if (strlen(nwy_ext_sio_recv_buff) > 128)
    {
        return;
    }

    memset(&at_cmd, 0, sizeof(nwy_at_info));
    memcpy(at_cmd.at_command, nwy_ext_sio_recv_buff, strlen(nwy_ext_sio_recv_buff));
    at_cmd.at_command[strlen(nwy_ext_sio_recv_buff)] = '\r';
    at_cmd.at_command[strlen(nwy_ext_sio_recv_buff) + 1] = '\n';
    at_cmd.length = strlen(at_cmd.at_command);
    ret = nwy_sdk_at_cmd_send(&at_cmd, resp, NWY_AT_TIMEOUT_DEFAULT);
    if (ret == NWY_SUCESS)
    {
        nwy_ext_echo("\r\n Resp:%s", resp);
    }
    else if (ret == NWY_AT_GET_RESP_TIMEOUT)
    {
        nwy_ext_echo("\r\n AT timeout");
    }
    else
    {
        nwy_ext_echo("\r\n AT ERROR");
    }
}

void nwy_HexStrToByte(const char *source, int sourceLen, unsigned char *dest)
{
    short i;
    unsigned char highByte = 0, lowByte = 0;

    for (i = 0; i < sourceLen; i += 2)
    {
        highByte = toupper((unsigned char)source[i]);
        lowByte = toupper((unsigned char)source[i + 1]);

        if (highByte > 0x39)
            highByte -= 0x37;
        else
            highByte -= 0x30;

        if (lowByte > 0x39)
            lowByte -= 0x37;
        else
            lowByte -= 0x30;

        dest[i / 2] = (highByte << 4) | lowByte;
    }
    return;
}

void nwy_at_cmd_process_callback(void *handle, char *atcmd, int type, char *para0, char *para1, char *para2)
{
    switch (type)
    {
    case 0:
        nwy_at_forward_send(handle, "\r\n", 2);
        nwy_at_forward_send(handle, atcmd, strlen(atcmd));
        if (para0)
        {
            nwy_at_forward_send(handle, para0, strlen(para0));
        }

        if (para1)
        {
            nwy_at_forward_send(handle, para1, strlen(para1));
        }

        if (para2)
        {
            nwy_at_forward_send(handle, para2, strlen(para2));
        }

        nwy_at_forward_send(handle, "\r\nOK\r\n", 6);
        break;
    case 1:
        nwy_at_forward_send(handle, "\r\nTEST\r\n", 8);
        break;
    case 2:
        nwy_at_forward_send(handle, "\r\nREAD\r\n", 8);
        break;
    case 3:
        nwy_at_forward_send(handle, "\r\nEXE\r\n", 7);
        break;
    }
}

void nwy_ext_appimg_info_proc(const uint8_t *data, int length)
{
    memcpy((g_recv->data + g_recv->pos), data, length);
    g_recv->pos = g_recv->pos + length;

    nwy_ext_echo("\r\n nwy_app_info.rev_len = %d length = %d\r\n", g_recv->pos, length);
    if (g_recv->pos == g_recv->size)
    {
        nwy_ext_send_sig(network_app_thread, NWY_EXT_APPIMG_FOTA_DATA_REC_END);
        nwy_update_flag = 0;
    }
    if (g_recv->pos > g_recv->size)
    {
        nwy_ext_send_sig(network_app_thread, NWY_EXT_APPIMG_FOTA_DATA_REC_END);
        nwy_update_flag = 0;
        nwy_ext_echo("\r\nAPP info is error,upgrade fail");
    }
}

void nwy_appimg_fota_test()
{
    int fd;
    ota_package_t ota_pack = {0};
    int ota_size = 0;
    int tmp_len = 0;
    int read_len = 0;
    char buff[NWY_APPIMG_FOTA_BLOCK_SIZE] = {0};
    int ret = -1;
    int len = 0;
    nwy_osiEvent_t event;
    char *fileanme = "app.bin";

    // nwy_get_fota_result();

    nwy_ext_input_gets("\r\n Please input app packet size:");
    int rtn;
    rtn = atoi(nwy_ext_sio_recv_buff);
    if (rtn <= 0)
    {
        nwy_ext_echo("\r\n Fota Error : invalid packet size:%s", nwy_ext_sio_recv_buff);
        return;
    }
    nwy_ext_echo("\r\n update packet size:%d", rtn);
    uint32_t size = (uint32_t)rtn;
    g_recv = malloc(sizeof(dataRecvContext_t) + size);
    if (g_recv == NULL)
    {
        nwy_ext_echo("\r\n Fota Error : malloc packet buffer fail");
        return;
    }

    g_recv->size = size;
    g_recv->pos = 0;
    nwy_update_flag = 2;

    nwy_ext_echo("\r\n Please send app.bin... \r\n");

    // while(1)
    {
        memset(&event, 0, sizeof(event));
        nwy_wait_thead_event(network_app_thread, &event, 0);
        if (event.id == NWY_EXT_APPIMG_FOTA_DATA_REC_END)
        {
            nwy_ext_echo("\r\n recv %d bytes appimg packet success", g_recv->size);

            if (nwy_sdk_fexist(fileanme))
            {
                ret = nwy_sdk_file_unlink(fileanme);
                nwy_ext_echo("\r\n del %s \r\n", fileanme);

                nwy_sleep(500);
            }

            fd = nwy_sdk_fopen(fileanme, NWY_CREAT | NWY_RDWR | NWY_TRUNC);

            len = nwy_sdk_fwrite(fd, g_recv->data, g_recv->size);

            nwy_sdk_fclose(fd);
            if (len != g_recv->size)
                nwy_ext_echo("\r\nfile write: len=%d, return len=%d\r\n", g_recv->size, len);
            else
                nwy_ext_echo("\r\nfile write success and close\r\n");

            free(g_recv);
            g_recv = NULL;
        }
    }

    fd = nwy_sdk_fopen("app.bin", NWY_RDONLY);

    if (fd < 0)
    {
        nwy_ext_echo("\r\nopen appimg fail\r\n");
        return;
    }

    ota_pack.data = (unsigned char *)malloc(NWY_APPIMG_FOTA_BLOCK_SIZE);
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
        tmp_len = NWY_APPIMG_FOTA_BLOCK_SIZE;
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

        ret = nwy_fota_dm(&ota_pack);

        if (ret < 0)
        {
            nwy_ext_echo("write ota error:%d\r\n", ret);
            free(ota_pack.data);

            nwy_sdk_fclose(fd);
            return;
        }
        ota_pack.offset += read_len;
        memset(ota_pack.data, 0, read_len);
        ota_size -= read_len;
    }
    free(ota_pack.data);
    nwy_sdk_fclose(fd);

    nwy_ext_echo("write end\r\n");

    nwy_ext_echo("start checksum\r\n");

    ret = nwy_package_checksum();
    if (ret < 0)
    {
        nwy_ext_echo("checksum failed\r\n");
        return;
    }

    nwy_ext_echo("start update\r\n");
    ret = nwy_fota_ua();
    if (ret < 0)
    {
        nwy_ext_echo("update failed\r\n");
        return;
    }
}

void nwy_get_chip_id_test()
{
    uint8_t uid[8] = {0};
    nwy_get_chip_id(uid);
    for (int i = 0; i < 8; i++)
    {
        nwy_ext_echo("\r\n nwy_get_chip_id_test 0x%02x \r\n", uid[i]);
    }
}
void nwy_get_ver_test()
{
    char dev_ver[64] = {
        0,
    };
    char lin_ver[64] = {
        0,
    };
    nwy_dm_get_device_version(dev_ver, lin_ver);
    nwy_ext_echo("\r\n nwy_get_ver_test dev_ver:%s,lin_ver:%s\r\n", dev_ver, lin_ver);
}
#define flash_size (4 * 1024)
#define flash_addr 0x607b0000
void nwy_flash_test()
{
    int ret = -1;
    int len = 0;
    uint8_t data_str[NWY_UART_RECV_SINGLE_MAX + 1] = {0};
    nwyspiflash_t *fp = NULL;
    fp = nwy_flash_open(NAME_FLASH_1);

    if (fp == NULL)
    {
        nwy_ext_echo("flash open filed \r\n");
        return;
    }
    ret = nwy_flash_erase(fp, flash_addr, flash_size);
    if (ret == false)
    {
        nwy_ext_echo("nwy_flash_erase filed \r\n");
        return;
    }

    nwy_ext_input_gets("\r\n send data):");
    len = nwy_ext_sio_len > NWY_UART_RECV_SINGLE_MAX ? NWY_UART_RECV_SINGLE_MAX : nwy_ext_sio_len;
    strcpy(data_str, nwy_ext_sio_recv_buff);
    ret = nwy_flash_write(fp, flash_addr, data_str, sizeof(data_str));
    if (ret == false)
    {
        nwy_ext_echo("nwy_flash_write filed \r\n");
        return;
    }
    memset(data_str, 0, sizeof(data_str));

    ret = nwy_flash_read(fp, flash_addr, data_str, sizeof(data_str));
    if (ret == false)
    {
        nwy_ext_echo("nwy_flash_read filed \r\n");
        return;
    }
    for (int i = 0; i < len; i++)
        nwy_ext_echo("\r\n nwy_flash_read 0x%02x \r\n", data_str[i]);

    return;
}

void nwy_boot_causes_test(void)
{
    uint32_t causes = nwy_get_boot_causes();
    char string_buf[256];
    memset(string_buf, 0, sizeof(string_buf));
    int idx = sprintf(string_buf, "boot cause[%02x]:", causes);
    if (causes == NWY_BOOTCAUSE_UNKNOWN)
        idx += sprintf(string_buf + idx, " %s", "UNKNOWN");
    else
    {
        if (causes & NWY_BOOTCAUSE_PWRKEY)
            idx += sprintf(string_buf + idx, " %s", "PWRKEY");
        if (causes & NWY_BOOTCAUSE_PIN_RESET)
            idx += sprintf(string_buf + idx, " %s", "PIN_RESET");
        if (causes & NWY_BOOTCAUSE_ALARM)
            idx += sprintf(string_buf + idx, " %s", "ALARM");
        if (causes & NWY_BOOTCAUSE_CHARGE)
            idx += sprintf(string_buf + idx, " %s", "CHARGE");
        if (causes & NWY_BOOTCAUSE_WDG)
            idx += sprintf(string_buf + idx, " %s", "WDG");
        if (causes & NWY_BOOTCAUSE_PIN_WAKEUP)
            idx += sprintf(string_buf + idx, " %s", "PIN_WAKEUP");
        if (causes & NWY_BOOTCAUSE_PSM_WAKEUP)
            idx += sprintf(string_buf + idx, " %s", "PSM_WAKEUP");
    }
    nwy_ext_echo("%s\r\n", string_buf);
    nwy_uart_printf("%s\r\n", string_buf);
}

void power_supply_check()
{
    int value = -1;
    int channel = 4; // VBAT
    int scale = 3;   // scale of 5V
    value = nwy_adc_get(channel, scale);
    nwy_ext_echo("\r\n Module supply voltage = %fV", (value * 0.001));
}
