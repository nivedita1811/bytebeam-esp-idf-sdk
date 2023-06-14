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
// #include "nwy_network.h"
#include "nwy_file.h"
// #include "nwy_socket.h"
#include "nwy_ble.h"
#include "json_server.h"
#include "uart_queue.h"
#include "bytebeam_uart.h"
#include <time.h>
#include "send_message_queue.h"
#include "json_server.h"

JSMN_JSON_CAN_MESSAGE_tst JSMN_JSON_CAN_MESSAGE_st_obj;

extern bool get_MQTT_COnnection_Status(void);

extern volatile int http_download_flag;

extern char ota_action_id[10];

extern const char n58_app_fw_ver[15];
extern const char n58_base_fw_ver[8];
char s32_app_fw_ver[15] = "\"NULL\"";
char tork_fw_ver[10] = "\"NULL\"";
char tork_hw_ver[10] = "\"NULL\"";
extern char ble_mac_id_g[50];
extern uint8_t nw_csq_val;
extern uint8_t ble_status;
extern uint8_t imu_status;

uint8_t ign_status = 0;

extern int network_thread_counter;
static int device_shadow_counter = 0;

static unsigned long long last_can_rx_msg_time = 0;

uint8_t whole_bin_file_array_u8[81920] = {0};

int uart_thread_entry = 0;

float batt_voltage = 0.0;
float input_voltage = 0.0;

typedef struct __attribute__((packed)) can_msg_tst_tag
{
    uint32_t can_id;
    uint8_t can_data_buff[8];
    uint8_t seq;
} can_msg_tst;

int heart_beat_received_counter = 0;
/************tork update params********/
tork_bldr_cmds_list_te tork_bldr_cmd_e = TORK_BLDR_INIT;

int tork_file_present = 0;
int all_blocks_sent = 0;
int num_of_complete_blocks = 0;
int tork_firmware_file_size = 0;
// uint8_t tork_bin_file_array_u8[81920]={0};
uint8_t can_data_buff_g[8] = {0};
uint32_t frame_counter_u32 = 0;
uint32_t tork_app_update_start = 0;
/************************************/

/***********s32 update params********/
typedef struct __attribute__((packed)) bootloader_config_tst_tag
{

    uint32_t new_firmware_size_u32;
    uint32_t new_firmware_crc_u32;
    uint8_t new_firmware_status_u8;
} bootloader_config_tst;

bootloader_config_tst bootloader_confif_msg_gst;
s32_bldr_cmd_ack_list_te bootloader_uart_tx_cmd_e = BOOTLOADER_MODE_ENABLE_CMD_SEND;

uint32_t total_bin_file_data_read_u32 = 0;
int new_firmware_file_size_u32 = 0;
int s32_update_complete = 0;
int num_of_one_kb_blocks = 0;
int last_block_byte_count = 0;
uint32_t whole_bin_crc_u32 = 0;
uint32_t s32_update_retries = 3;
/************************************/

uint32_t bootloader_app_crc32_generate_u32(uint8_t *buf, uint32_t len)
// uint32_t calculateCRC(uint8_t *buf, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < len; i++)
    {
        uint8_t ch = buf[i];
        for (size_t j = 0; j < 8; j++)
        {
            uint32_t b = (ch ^ crc) & 1;
            crc >>= 1;
            if (b)
                crc = crc ^ 0xEDB88320;
            ch >>= 1;
        }
    }

    return ~crc;
}

void uart_config(void)
{
    STM_UART_fd = nwy_uart_init(STM_UART, 1);
    nwy_uart_set_baud(STM_UART_fd, 2000000); // 2000000 2Mbps baud rate
    nwy_uart_reg_recv_cb(STM_UART_fd, uart_recv_handle);
    nwy_set_rx_frame_timeout(STM_UART_fd, 2);
    message_queue_init();
}

void nwy_uart_printf(const char *format, ...)
{
    char send_buf[512] = "\r\n";
    va_list ap;
    size_t size = 2;
    va_start(ap, format);
    size += vsnprintf(send_buf + size, sizeof(send_buf) - size, format, ap);
    va_end(ap);
    nwy_uart_send_data(STM_UART_fd, (uint8_t *)send_buf, size);
}

void uart_recv_handle(const char *str, uint32_t length)
{

    static struct timeval tv;
    static double s = 0;
    static double ms = 0;
    static unsigned long long timestamp_start = 0;
    unsigned long long timestamp_end = 0;

    gettimeofday(&tv, NULL);
    s = tv.tv_sec;
    ms = ((double)tv.tv_usec) / 1.0e3;
    if (timestamp_start == 0)
    {
        timestamp_start = (unsigned long long)(s * 1000 + ms);
    }

    timestamp_end = (unsigned long long)(s * 1000 + ms);

    if ((timestamp_end - timestamp_start) > 1000)
    {
        nwy_ext_echo("\r\n In uart callback Length Got: %d\r\n", length);
        timestamp_start = timestamp_end;
    }

    // nwy_ext_echo("value of data received\r\n");
    if (uart_thread_entry == 1)
    {
        for (int i = 0; i < length; i++)
        {
            //    nwy_ext_echo("%X ",str[i]);
            if (FAILED == uart_rx_array_queue_put(str[i]))
            {
                nwy_ext_echo("Put queue failure of data: %d", str[i]);
            }
        }
    }
    // nwy_ext_echo("\r\n");
}

void print_uart_data_structure(UART_data_struct uart_strcut)
{
    nwy_ext_echo("STX: %x CMD:%x Length:%d \r\nPayload:", uart_strcut.stx,
                 uart_strcut.cmd,
                 uart_strcut.length_u16);

    for (int i = 0; i < uart_strcut.length_u16; i++)
    {
        if (i % 10 == 0)
        {
            nwy_ext_echo("\r\n");
        }
        nwy_ext_echo("%x ", uart_strcut.payload[i]);
    }

    nwy_ext_echo("\r\n");
    nwy_ext_echo("CRC: %d timestamp:%llu etx:%x", uart_strcut.crc_u16,
                 uart_strcut.timestamp,
                 uart_strcut.etx);
}

int32_t send_can_mqtt(UART_data_struct *uart_strcut)
{
    static uint16_t package_count = 0;

    // if (nwy_read_sdcart_status() == false) // direct pass through
    // {
    if (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue) == 0)
    {
        nwy_ext_echo("\r\nMQTT Queue full agidhe\r\n");
    }
    else
    {
        nwy_put_msg_que(mqtt_publish_msg_queue, uart_strcut, 0);
    }
    // }
    // else
    // {
    //     if (get_MQTT_COnnection_Status() == true && (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue) != 0) && store_msg_flag == false)
    //     {
    //         nwy_put_msg_que(mqtt_publish_msg_queue, uart_strcut, 0);
    //     }
    //     else
    //     {
    //     store_msg_flag = true;
    //        if(nwy_get_queue_spaceevent_cnt(store_queue) != 0)
    //        {
    //             nwy_put_msg_que(store_queue, uart_strcut, 0);
    //        }
    //        else
    //        {
    //             nwy_ext_echo("\r\n Dropping it!!\r\n");
    //        }
    //     }
    // }
}

int32_t send_ack_nack(uint8_t status, uint8_t command, uint8_t *payload, uint16_t length)
{
    uint16_t len = 13;
    uint8_t can_data[100] = {
        0,
    };
    length = length;

    can_data[0] = 0xDE;
    can_data[1] = 0xAD;
    can_data[2] = command;
    // can_data[3] = (length+1) & 0xFF;		//length lo byte //status length is one more byte
    // can_data[4] = (length+1) >> 8;		//length hi byte
    can_data[3] = (len)&0xFF;
    can_data[4] = (len) >> 8;
    can_data[5] = status;
    // if(length !=0)
    // {
    //     memcpy(&can_data[6], payload, (length)); //0 to 5 header 6+ length, payload
    // }
    // memcpy(&can_data[6], payload, (len - 1));
    // can_data[length + 6] = 0x00;
    // can_data[length + 7] = 0x00;
    // can_data[length + 8] = 0x03;
    can_data[len + 5] = 0x00;
    can_data[len + 6] = 0x00;
    can_data[len + 7] = 0x03;
    // nwy_ext_echo("\r\nSending Ack Bytes:");

    if (nwy_semaphore_acquire(uart_tx_semaphore, 0xFFFF) == true)
    {
        for (int i = 0; i < 8 + len; i++)
        {
            // nwy_ext_echo("%x ", can_data[i]);
            nwy_uart_send_data(STM_UART_fd, (uint8_t *)&can_data[i], 1);
            nwy_usleep(1);
            // nwy_usleep(50);                      //changed from 1100 to 50
        }
        nwy_sleep(1);
        nwy_semahpore_release(uart_tx_semaphore);
    }

    return 0;
}

int32_t execute_command(UART_data_struct *uart_strcut)
{

    struct timeval tv;
    static int count = 0;
    static double s = 0;
    static double ms = 0;
    static unsigned long long timestamp_start;

    if (s == 0)
    {
        gettimeofday(&tv, NULL);
        s = tv.tv_sec;
        ms = ((double)tv.tv_usec) / 1.0e3;
        timestamp_start = (unsigned long long)(s * 1000 + ms);
    }

    switch (uart_strcut->cmd)
    {
    case 0x00:
    {
        heart_beat_received_counter = 0;
        nwy_ext_echo("Execute command %d\r\n", uart_strcut->cmd);
        snprintf(s32_app_fw_ver, 15, "\"%d.%d.%d\"", uart_strcut->payload[0], uart_strcut->payload[1], uart_strcut->payload[2]);
        input_voltage = ((float)(((uint16_t)uart_strcut->payload[6] << 8) | uart_strcut->payload[5]) * 3.3 * 6.0) / 4096;
        batt_voltage = ((float)(((uint16_t)uart_strcut->payload[4] << 8) | uart_strcut->payload[3]) * 3.3 * 3.0) / 4096;
        ign_status = uart_strcut->payload[7];
        send_ack_nack(0, uart_strcut->cmd, NULL, 0);
        break;
    }
    case 0xCE:
    {
        snprintf(tork_fw_ver, 15, "\"%c%c%c%c\"", uart_strcut->payload[0], uart_strcut->payload[1], uart_strcut->payload[2], uart_strcut->payload[3]);
        snprintf(tork_hw_ver, 15, "\"%c%c%c%c\"", uart_strcut->payload[4], uart_strcut->payload[5], uart_strcut->payload[6], uart_strcut->payload[7]);
    }
    case 0x04:
    {
        count++;
        // nwy_ext_echo("Execute command switch %d\r\n", uart_strcut.cmd);
        gettimeofday(&tv, NULL);
        double s = tv.tv_sec;
        double ms = ((double)tv.tv_usec) / 1.0e3;
        unsigned long long timestamp_end = (unsigned long long)(s * 1000 + ms);
        last_can_rx_msg_time = timestamp_end;

        if ((timestamp_end - timestamp_start) > 950)
        {
            // nwy_ext_echo("\r\nTime elapsed:%llu count:%d, \r\n", timestamp_end-timestamp_start, count*60);
            count = 0;
            timestamp_start = timestamp_end;
            // nwy_ext_echo("The queue size is: %d", 10 - nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue));
        }

        send_ack_nack(0, uart_strcut->cmd, NULL, 0);
        if (http_download_flag == 0)
        {
            send_can_mqtt(uart_strcut);
        }
        break;
    }
    case TORK_BLDR_INIT_ACK:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Tork bootloader activation successful\r\n");
            tork_bldr_cmd_e = TORK_BLDR_CODE_SIZE_CMD;
        }
        break;
    }
    case TORK_BLDR_CODE_SIZE_CMD_ACK:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Tork bootloader code size transmit successful\r\n");
            tork_bldr_cmd_e = TORK_BLDR_ERASE_CMD;
        }
        break;
    }
    case TORK_BLDR_ERASE_CMD_ACK:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Tork bootloader flash erase successful\r\n");
            tork_bldr_cmd_e = TORK_BLDR_SEND_DATA_CMD;
        }
        break;
    }
    case TORK_BLDR_WRITE_DATA_BLOCK_ACK:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            if (all_blocks_sent == 1)
            {
                nwy_ext_echo("Tork bootloader entire firmware image write successful\r\n");
                tork_bldr_cmd_e = TORK_BLDR_SEND_JMP_CMD;
            }
            else
            {
                nwy_ext_echo("Tork bootloader block write successful\r\n");
                tork_bldr_cmd_e = TORK_BLDR_SEND_DATA_CMD;
            }
        }
        break;
    }
    case TORK_BLDR_JMP_CMD_ACK:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Tork bootloader jump to main app successful\r\n");
            tork_bldr_cmd_e = TORK_BLDR_APP_UPDATE_SUCCESS;
        }
        break;
    }
    /*******************************************************************************************************************/
    case BOOTLOADER_MODE_ENABLE_CMD_SEND:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Bootloader mode activated cmd  ACK Success received\r\n");
            bootloader_uart_tx_cmd_e = BOOTLOADER_CONFIG_DATA_CMD_SEND;
        }
        else
        {
            nwy_ext_echo("Bootloader mode activated cmd  ACK  Failer received\r\n");
        }
    }
    break;
        /*******************************************************************************************************************/
    case BOOTLOADER_CONFIG_DATA_CMD_SEND:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Bootloader config data send  cmd  ACK Success received\r\n");
            bootloader_uart_tx_cmd_e = BOOTLOADER_1KB_CRC_DATA_SEND;
        }
        else
        {
            nwy_ext_echo("Bootloader config data send  cmd  ACK failed received\r\n");
        }
    }
    break;
        /*******************************************************************************************************************/
    case BOOTLOADER_1KB_CRC_DATA_SEND:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Bootloader 1kb  data send  cmd  ACK Success received\r\n");
            bootloader_uart_tx_cmd_e = BOOTLOADER_1KB_CRC_DATA_SEND;
            // if(new_firmware_file_size_u32 != 0)
            // {
            //     bootloader_uart_tx_cmd_e = BOOTLOADER_1KB_CRC_DATA_SEND;
            // }
            // else
            // {
            //     //bootloader_uart_tx_cmd_e = BOOTLOADER_END_OF_DATA_CMD_SEND;
            // }
        }
        else
        {
            nwy_ext_echo("Bootloader config data send  cmd  ACK failed received\r\n");
        }
    }
    break;
        /*******************************************************************************************************************/
    case BOOTLOADER_END_OF_DATA_CMD_SEND:
    {
        // if(uart_rx_data.payload[0]  == 0xAA)
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Bootloader BOOTLOADER_END_OF_DATA_CMD_SEND  ACK Success received\r\n");
            bootloader_uart_tx_cmd_e = BOOTLOADER_WHOLE_BIN_CRC_CHECK_CMD_SEND;
        }
        else
        {
            nwy_ext_echo("Bootloader BOOTLOADER_END_OF_DATA_CMD_SEND ACK failed received\r\n");
        }
    }
    break;
    
    /*******************************************************************************************************************/
    case BOOTLOADER_WHOLE_BIN_CRC_CHECK_CMD_SEND:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Bootloader BOOTLOADER_WHOLE_BIN_CRC_CHECK_CMD_SEND  ACK Success received\r\n");
            bootloader_uart_tx_cmd_e = BOOTLOADER_JUMPING_TO_BOOTLOADER_CMD_SEND;
        }
        else
        {
            if (s32_update_retries > 0)
            {
                s32_update_retries--;
                bootloader_uart_tx_cmd_e = BOOTLOADER_CONFIG_DATA_CMD_SEND;
            }
            else
            {
                publish_action_status(ota_action_id, 85, "Failed", "S32 CRC Mismatch");
                nwy_sleep(3000);
                publish_action_status(ota_action_id, 85, "Failed", "S32 CRC Mismatch");
                nwy_sleep(3000);
                nwy_power_off(2);
            }
            nwy_ext_echo("Bootloader BOOTLOADER_WHOLE_BIN_CRC_CHECK_CMD_SEND ACK failed received\r\n");
        }
    }
    break;
    
    /*******************************************************************************************************************/
    case BOOTLOADER_JUMPING_TO_BOOTLOADER_CMD_SEND:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Bootloader BOOTLOADER_JUMPING_TO_BOOTLOADER_CMD_SEND  ACK Success received\r\n");
            bootloader_uart_tx_cmd_e = BOOTLOADER_INT_APP_ERASE_CMD_SEND;
        }
        else
        {
            nwy_ext_echo("Bootloader BOOTLOADER_JUMPING_TO_BOOTLOADER_CMD_SEND ACK failed received\r\n");
        }
    }
    break;
        /*******************************************************************************************************************/
    case BOOTLOADER_INT_APP_ERASE_CMD_SEND:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Bootloader BOOTLOADER_INT_APP_ERASE_CMD_SEND  ACK Success received\r\n");
            bootloader_uart_tx_cmd_e = BOOTLOADER_INT_APP_WRITE_CMD_SEND;
        }
        else
        {
            nwy_ext_echo("Bootloader BOOTLOADER_INT_APP_ERASE_CMD_SEND ACK failed received\r\n");
        }
    }
    break;
        /*******************************************************************************************************************/
    case BOOTLOADER_INT_APP_WRITE_CMD_SEND:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("Bootloader BOOTLOADER_INT_APP_WRITE_CMD_SEND  ACK Success received\r\n");
            bootloader_uart_tx_cmd_e = BOOTLOADER_JUMPING_APPLICATION_CMD_SEND;
        }
        else
        {
            nwy_ext_echo("Bootloader BOOTLOADER_INT_APP_WRITE_CMD_SEND ACK failed received\r\n");
        }
    }
    break;
        /********************************************************************************************************************/
    case BOOTLOADER_JUMPING_APPLICATION_CMD_SEND:
    {
        if (uart_strcut->payload[0] == 0xAA)
        {
            nwy_ext_echo("New Firmware Running Succesfully\r\n");
            bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
        }
        else
        {
            nwy_ext_echo("Bootloader BOOTLOADER_JUMPING_APPLICATION_CMD_SEND ACK failed received\r\n");
        }
    }
    break;
        /********************************************************************************************************************/
    case BOOTLOADER_TIMEOUT_ERROR_WITH_DIAG_TOOL_CMD_SEND:
    {
    }
    break;
        /********************************************************************************************************************/
    case BOOTLOADER_APPLICATION_NOT_AVAILABLE_CMD_SEND:
    {
        bootloader_uart_tx_cmd_e = BOOTLOADER_MODE_ENABLE_CMD_SEND;
        nwy_ext_echo("BOOTLOADER_APPLICATION_NOT_AVAILABLE_CMD_ACK_RECEIVED\r\n");
    }
    break;
    case 0xBB:
    {
        // retrieve_ble_data
        uint8_t can_buff[8] = {0};
        uint32_t msg_id = ((uint32_t)(uart_strcut->payload[0])) |
                          (((uint32_t)(uart_strcut->payload[1])) << 8) |
                          (((uint32_t)(uart_strcut->payload[2])) << 16) |
                          (((uint32_t)(uart_strcut->payload[3])) << 24);
        // nwy_ext_echo("received ble can msg, ID is %X\r\n",msg_id);

        if (msg_id == 0x777)
        {
            nwy_power_off(2);
            return 0;
        }
        if (http_download_flag == 0)
            nwy_ble_send_data(12, uart_strcut->payload);
        break;
    }
#if 0
        case 0xCC:
        {
            if(uart_strcut->payload[0] == 0xAA)
            {
                nwy_ext_echo("SOS received\r\n");
                gettimeofday(&tv, NULL);

                double s = tv.tv_sec;
                double ms = ((double)tv.tv_usec) / 1.0e6;

                unsigned long long timestamp1 = (unsigned long long)(s + ms) * 1000;
                uint32_t sos_button_status = 1;
                static uint32_t sq_id = 0;
                sq_id++;
                memset(json_sos_message_buff_gau8, 0, JSON_SOS_MESSAGES_BUFF_LEN);
                snprintf(json_sos_message_buff_gau8, sizeof(json_sos_message_buff_gau8), sos_mess_json,
                         timestamp1,
                         sos_button_status,
                         sq_id);
                nwy_ext_echo("%s", json_sos_message_buff_gau8); // For the verification of the JSON frame

                UART_data_struct sos_publish_msg;
                sos_publish_msg.cmd = 0xCC;
                sos_publish_msg.length_u16 = strlen((char *)json_sos_message_buff_gau8);

                memcpy(sos_publish_msg.payload, json_sos_message_buff_gau8, sos_publish_msg.length_u16);

                // if (get_MQTT_COnnection_Status() && (store_msg_flag == false) && (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue)!= 0))
                if (get_MQTT_COnnection_Status()) 
                {
                    nwy_ext_echo("publishing SOS in MQTT\r\n");
                    nwy_put_msg_que(mqtt_publish_msg_queue, &sos_publish_msg, 0);
                }
            }
        }
        break;
#endif
#if 1
    case 0xDD:
    {
        ign_status = uart_strcut->payload[0];
        // nwy_ext_echo("Ignition message\r\n");
        // gettimeofday(&tv, NULL);

        // double s = tv.tv_sec;
        // double ms = ((double)tv.tv_usec) / 1.0e6;

        // unsigned long long timestamp1 = (unsigned long long)(s + ms) * 1000;
        // uint32_t ignition_button_status = (uint32_t)uart_strcut->payload[0];
        // static uint32_t sq_id = 0;
        // sq_id++;
        // memset(json_ignition_message_buff_gau8, 0, JSON_IGN_MESSAGES_BUFF_LEN);
        // snprintf(json_ignition_message_buff_gau8, sizeof(json_ignition_message_buff_gau8), ignition_mess_json,
        //             timestamp1,
        //             ignition_button_status,
        //             sq_id);
        // nwy_ext_echo("%s", json_ignition_message_buff_gau8); // For the verification of the JSON frame

        // UART_data_struct ign_publish_msg;
        // ign_publish_msg.cmd = 0xDD;
        // ign_publish_msg.length_u16 = strlen((char *)json_ignition_message_buff_gau8);

        // memcpy(ign_publish_msg.payload, json_ignition_message_buff_gau8, ign_publish_msg.length_u16);

        // // if (get_MQTT_COnnection_Status() && (store_msg_flag == false) && (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue)!= 0))
        // if (get_MQTT_COnnection_Status()) 
        // {
        //     nwy_ext_echo("publishing ignition in MQTT\r\n");
        //     nwy_put_msg_que(mqtt_publish_msg_queue, &ign_publish_msg, 0);
        // }
    }
    break;
#endif
    case 0xEE:
    {
        send_ack_nack(0, uart_strcut->cmd, NULL, 0);
        nwy_sleep(500);
        nwy_power_off(1);
    }
    break;
    default:
        break;
    }
    return 0;
}

int32_t process_uart_data(uint8_t data)
{
    static uint8_t data_process_state = STX1_STATE;
    static UART_data_struct uart_rx_data;
    static uint8_t len_byte1;
    static uint8_t len_byte2;
    static uint8_t crc_byte_1;
    static uint8_t crc_byte_2;
    static uint16_t length_16;
    static uint16_t crc_16;

    static uint16_t len_counter = 0;
    // static uint8_t pay_load[100] = {0,};
    // nwy_ext_echo("Data: %d\r\n", data);
    switch (data_process_state)
    {
    case STX1_STATE:
    {
        if (data == 0xDE)
        {
            // nwy_ext_echo("\r\nSTX_STATE\r\n");
            data_process_state = STX2_STATE;
        }
        break;
    }

    case STX2_STATE:
    {
        if (data == 0xAD)
        {
            // nwy_ext_echo("\r\nSTX_STATE\r\n");
            data_process_state = CMD_STATE;
            uart_rx_data.stx = (uint16_t)(0xDE << 8 | 0xAD);
            // nwy_ext_echo("\r\nSTX_STATE:%x\r\n", uart_rx_data.stx);
        }
        else
        {
            data_process_state = STX1_STATE;
        }
        break;
    }

    case CMD_STATE:
    {
        // nwy_ext_echo("\r\nCMD_STATE:%x\r\n", data);
        data_process_state = LEN1_STATE;
        uart_rx_data.cmd = data;
        break;
    }

    case LEN1_STATE:
    {
        len_byte1 = data;
        // nwy_ext_echo("\r\nlen1_STATE\r\n");
        data_process_state = LEN2_STATE;
        break;
    }

    case LEN2_STATE:
    {
        len_byte2 = data;
        // nwy_ext_echo("\r\nlen2_STATE\r\n");
        length_16 = (uint16_t)(len_byte2 << 8 | len_byte1);
        uart_rx_data.length_u16 = length_16;
        data_process_state = DATA_STATE;
        // nwy_ext_echo("\r\nLenght is : %d, %d %d\r\n", length_16, len_byte1, len_byte2);
        if (length_16 > 4096 || length_16 <= 0)
        {
            data_process_state = STX1_STATE;
        }
        break;
    }

    case DATA_STATE:
    {
        len_counter++;
        // nwy_ext_echo("\r\nDATA_STATE count:%d\r\n", len_counter);
        uart_rx_data.payload[len_counter - 1] = data;
        // nwy_ext_echo("\r\nDATA_STATE count:%d\r\n", uart_rx_data.payload[len_counter-1]);
        if (length_16 > len_counter)
        {
            data_process_state = DATA_STATE;
        }
        else
        {
            data_process_state = CRC1_STATE;
            len_counter = 0;
        }
        break;
    }

    case CRC1_STATE:
    {
        crc_byte_1 = data;
        data_process_state = CRC2_STATE;
        // nwy_ext_echo("\r\nCRC1_STATE %x\r\n", crc_byte_1);
        break;
    }

    case CRC2_STATE:
    {
        // nwy_ext_echo("\r\nCRC2_STATE\r\n");
        crc_byte_2 = data;
        data_process_state = ETX_STATE;
        crc_16 = (uint16_t)(crc_byte_2 << 8 | crc_byte_1);
        uart_rx_data.crc_u16 = crc_16;

        // nwy_ext_echo("\r\nCRC2_STATE %x\r\n", crc_byte_2);
        break;
    }

    case ETX_STATE:
    {
        // nwy_ext_echo("\r\nETX_STATE %d\r\n", data);
        if (data == 0x03)
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);

            double s = tv.tv_sec;
            double ms = ((double)tv.tv_usec) / 1.0e3;
            unsigned long long timestamp1 = (unsigned long long)(s * 1000 + ms);
            uart_rx_data.timestamp = timestamp1;
            uart_rx_data.etx = data;
            execute_command(&uart_rx_data);
            data_process_state = STX1_STATE;
        }
    }

    default:
    {
        data_process_state = STX1_STATE;
        break;
    }
    }

    return 0;
}

// Until UART data is being processed - no need of the thread
void UART_rx_ThreadEntry(void *param)
{
    uint8_t data = 0;
    int32_t length_got = 0;
    nwy_ext_echo("\r\nUART Thread created\r\n");
    struct timeval tv;
    static int count = 0;
    static double seconds = 0;
    static double ms = 0;
    static unsigned long long current_time;
    UART_data_struct stored_json_uart_strcut;

    nwy_ext_echo("\r\nUART Thread created\r\n");
    uart_config();
    uart_thread_entry = 1;
    send_ack_nack(0, 0x77, NULL, 0);
    nwy_sleep(1000);

    while (1)
    {
        length_got = uart_rx_array_queue_size();
        // nwy_ext_echo("UART task is getting executed\r\n");
        for (int i = 0; i < length_got; i++)
        {
            if (uart_rx_array_queue_get(&data) == SUCCESS)
            {
                process_uart_data(data);
            }
        }
        // if(tork_app_update_start == 0)
        // {
        //     gettimeofday(&tv, NULL);
        //     seconds = tv.tv_sec;
        //     ms = ((double)tv.tv_usec) / 1.0e3;
        //     current_time = (unsigned long long)(seconds * 1000 + ms);
        //     if(last_can_rx_msg_time != 0)
        //     {
        //         if((current_time - last_can_rx_msg_time) > 5000)
        //         {
        //             stored_json_uart_strcut.cmd = 0xBB;
        //             stored_json_uart_strcut.length_u16= 0;
        //             stored_json_uart_strcut.timestamp = current_time;
        //             last_can_rx_msg_time = 0;
        //             nwy_ext_echo("\r\n Ready to publish last stored json\r\n");
        //             nwy_put_msg_que(mqtt_publish_msg_queue, &stored_json_uart_strcut, 0);
        //         }
        //     }
        // }

        if (http_download_flag == 0)
        {
            device_shadow_counter++;
            if (device_shadow_counter >= 400)
            {
                publish_device_shadow();
                device_shadow_counter = 0;
            }
            // heart_beat_received_counter++;
            // if (heart_beat_received_counter > 1600)
            // {
            //     nwy_power_off(2);
            // }
        }
        nwy_sleep(50);
    }
}
// Until UART data is being processed - no need of the thread
void UART_tx_ThreadEntry(void *param)
{
}

/**
=====================================================================================================================================================

@fn Name			:
@b Scope            :
@n@n@b Description  :
@param Input Data   :
@return Return Value:

=====================================================================================================================================================
*/
void tork_new_fw_file_read(void)
{
    int read_count_u32 = 0;
    char firmware_file_name_string[100];
    int firmware_file_handle;
    // nwy_sdk_sdcard_unmount();
    // nwy_sleep(100);
    // while(nwy_sdk_sdcard_mount() != true)
    // {
    //     ;
    // }
    // if(nwy_read_sdcart_status() == true)
    {
        sprintf(firmware_file_name_string, "/nwy/tork_firmware_app.bin");
        // sprintf(firmware_file_name_string, "/nwy/tork_firmware_app.bin");
        firmware_file_handle = nwy_sdk_fopen(firmware_file_name_string, NWY_RDWR); // NWY_CREAT | NWY_RDWR | NWY_APPEND);
        if (firmware_file_handle < 0)
        {
            tork_file_present = 0;
            nwy_ext_echo("\r\nfile open fail\r\n");
            return;
        }
        tork_file_present = 1;
        // nwy_ext_echo("\r\nfile call 3\r\n");
        tork_firmware_file_size = nwy_sdk_fsize_fd(firmware_file_handle);
        num_of_complete_blocks = (tork_firmware_file_size / 512);
        nwy_ext_echo("\r\n Tork_Fimrware_App_Size = %d\r\n", tork_firmware_file_size);
        nwy_sdk_fclose(firmware_file_handle);
        firmware_file_handle = nwy_sdk_fopen(firmware_file_name_string, NWY_RDONLY);
        nwy_ext_echo("\r\n file open status  = %d\r\n", firmware_file_handle);
        read_count_u32 = nwy_sdk_fread(firmware_file_handle, whole_bin_file_array_u8, tork_firmware_file_size);
        nwy_ext_echo("\r\n file read status  = %d\r\n", read_count_u32);
        nwy_sdk_fclose(firmware_file_handle);
        if (read_count_u32 == tork_firmware_file_size)
        {
            nwy_ext_echo("\r\n FW file successfully retrieved from SD card\r\n");
        }
        else
        {
            nwy_ext_echo("\r\n whole file read failed\r\n");
        }
    }
    // else
    // {
    //     nwy_ext_echo("\r\n SD Card mount failed\r\n");
    // }

    // nwy_sdk_fclose(new_firmware_fd_global);
    // nwy_ext_echo("\r\nfile call 4\r\n");
    // nwy_sdk_fseek(fd_global, new_firmware_file_size_u32, NWY_SEEK_SET);
    // memset(&file_data[0], 0, sizeof(file_data));
}

/**
=====================================================================================================================================================

@fn Name			:
@b Scope            :
@n@n@b Description  :
@param Input Data   :
@return Return Value:

=====================================================================================================================================================
*/
int tork_send_can_data(uint32_t can_msg_id, uint8_t *can_data_arr, uint8_t seq)
{
    uint8_t command = 0xAB;
    uint16_t length = 13;
    uint8_t can_data[21] = {
        0,
    };

    can_msg_tst can_msg_st_obj;
    memset((void *)&can_msg_st_obj, 0, sizeof(can_msg_tst));
    can_msg_st_obj.can_id = can_msg_id;
    can_msg_st_obj.seq = seq;

    int loop_var = 0;

    memcpy((uint8_t *)can_msg_st_obj.can_data_buff, can_data_arr, 8);

    can_data[0] = 0xDE;
    can_data[1] = 0xAD;
    can_data[2] = command;

    can_data[3] = (length)&0xFF; // length lo byte
    can_data[4] = (length) >> 8; // length hi byte

    memcpy(&can_data[5], (uint8_t *)&can_msg_st_obj, 13);

    can_data[18] = 0x00;
    can_data[19] = 0x00;
    can_data[20] = 0x03;
    // nwy_ext_echo("Entering UART transmission\r\n");
    if (nwy_semaphore_acquire(uart_tx_semaphore, 0xFFFF) == true)
    {
        for (int i = 0; i < 21; i++)
        {
            // nwy_ext_echo("Entering UART transmission\r\n");
            nwy_uart_send_data(STM_UART_fd, (uint8_t *)&can_data[i], 1);
            nwy_usleep(50);
        }
        nwy_semahpore_release(uart_tx_semaphore);
    }
    // nwy_ext_echo("Exiting UART transmission\r\n");
    return 0;
}

/**
=====================================================================================================================================================

@fn Name			:
@b Scope            :
@n@n@b Description  :
@param Input Data   :
@return Return Value:

=====================================================================================================================================================
*/
void tork_transmit_512_bytes()
{
    uint8_t can_buff[8] = {0};
    uint32_t can_msg_id = 0;
    static int current_block = 0;
    nwy_ext_echo("Current block:- %d\r\n", current_block);
    if (current_block < num_of_complete_blocks)
    {
        for (int loop_var_1 = 0; loop_var_1 < 64; loop_var_1++)
        {
            for (int loop_var_2 = 0; loop_var_2 < 8; loop_var_2++)
            {
                can_buff[loop_var_2] = whole_bin_file_array_u8[(512 * current_block) + (8 * loop_var_1) + loop_var_2];
            }
            // can_msg_id = ((((uint32_t)(loop_var_1+1)) << 16) & 0xFFFF0000) | 0xF08C;
            can_msg_id = ((frame_counter_u32 << 16) & 0xFFFF0000) | 0xF08C;
            frame_counter_u32++;
            if (frame_counter_u32 > 0xFF)
            {
                frame_counter_u32 = 0;
            }
            tork_send_can_data(can_msg_id, can_buff, loop_var_1);
            nwy_usleep(2000);
        }
        for (int loop_var = 0; loop_var < 8; loop_var++)
        {
            can_buff[loop_var] = 0;
        }
        can_msg_id = ((frame_counter_u32 << 16) & 0xFFFF0000) | 0xF08D;
        frame_counter_u32++;
        if (frame_counter_u32 > 0xFF)
        {
            frame_counter_u32 = 0;
        }
        tork_send_can_data(can_msg_id, can_buff, 0);
        // nwy_ext_echo("Data command:- %X\r\n",can_msg_id);
        current_block++;
        if (current_block == num_of_complete_blocks)
        {
            all_blocks_sent = 1;
            current_block = 0;
        }
    }
    // else
    // {
    //     all_blocks_sent = 1;
    //     current_block = 0;
    // }
}

/**
=====================================================================================================================================================

@fn Name			:
@b Scope            :
@n@n@b Description  :
@param Input Data   :
@return Return Value:

=====================================================================================================================================================
*/
void tork_app_update(void *param)
{
    static int tork_app_update_complete = 0;
    uint32_t can_msg_id = 0;
    uint32_t crc_val_u32 = 0;
    static int idle_state_counter = 0;
    nwy_sleep(100);
    while (1)
    {
        if (nwy_semaphore_acquire(tork_update_semaphore, 0) == true)
        {
            tork_new_fw_file_read();
            while (1)
            {
                // should be blocked on semaphore
                // nwy_ext_echo("Entering task\r\n");
                if (tork_file_present == 0)
                {
                    nwy_semahpore_release(s32_update_semaphore);
                    publish_action_status(ota_action_id, 85, "Progress", "");
                    break;
                }
                switch (tork_bldr_cmd_e)
                {
                case TORK_BLDR_INIT:
                {
                    idle_state_counter = 0;
                    tork_app_update_start = 1;
                    nwy_suspend_thread(ble_app_thread_handle);
                    nwy_suspend_thread(network_app_thread);
                    // nwy_suspend_thread(mqtt_publish_thread);
                    // send_ack_nack(0, 0x55, NULL, 0);
                    // nwy_sleep(22000);
                    nwy_ext_echo("Transmitting bootloader activation command\r\n");
                    for (int loop_var = 0; loop_var < 8; loop_var++)
                    {
                        can_data_buff_g[loop_var] = 0;
                    }
                    can_data_buff_g[0] = 0x8A;
                    can_data_buff_g[1] = 0xF0;
                    can_data_buff_g[4] = 0x8A;
                    can_data_buff_g[5] = 0xF0;
                    nwy_usleep(3000);
                    can_msg_id = ((frame_counter_u32 << 16) & 0xFFFF0000) | 0xF08A;
                    frame_counter_u32++;
                    tork_send_can_data(can_msg_id, can_data_buff_g, 0x00);
                    tork_bldr_cmd_e = TORK_BLDR_IDLE_STATE;
                }
                break;
                case TORK_BLDR_CODE_SIZE_CMD:
                {
                    idle_state_counter = 0;
                    nwy_ext_echo("Transmitting code size\r\n");
                    // tork_new_fw_file_read();
                    can_data_buff_g[7] = (uint8_t)((tork_firmware_file_size >> 24) & 0xFF);
                    can_data_buff_g[6] = (uint8_t)((tork_firmware_file_size >> 16) & 0xFF);
                    can_data_buff_g[5] = (uint8_t)((tork_firmware_file_size >> 8) & 0xFF);
                    can_data_buff_g[4] = (uint8_t)(tork_firmware_file_size & 0xFF);
                    for (int loop_var = 0; loop_var < 4; loop_var++)
                    {
                        can_data_buff_g[loop_var] = 0;
                    }
                    nwy_usleep(1000);
                    can_msg_id = ((frame_counter_u32 << 16) & 0xFFFF0000) | 0xF08A;
                    frame_counter_u32++;
                    tork_send_can_data(can_msg_id, can_data_buff_g, 0x00);
                    tork_bldr_cmd_e = TORK_BLDR_IDLE_STATE;
                    nwy_ext_echo("code size transmitted\r\n");
                }
                break;
                case TORK_BLDR_ERASE_CMD:
                {
                    idle_state_counter = 0;
                    nwy_ext_echo("Transmitting flash erase command\r\n");
                    for (int loop_var = 0; loop_var < 8; loop_var++)
                    {
                        can_data_buff_g[loop_var] = 0;
                    }
                    nwy_usleep(1000);
                    can_msg_id = ((frame_counter_u32 << 16) & 0xFFFF0000) | 0xF08E;
                    frame_counter_u32++;
                    tork_send_can_data(can_msg_id, can_data_buff_g, 0x00);
                    tork_bldr_cmd_e = TORK_BLDR_IDLE_STATE;
                }
                break;
                case TORK_BLDR_SEND_DATA_CMD:
                {
                    idle_state_counter = 0;
                    tork_transmit_512_bytes();
                    nwy_ext_echo("Wrote block\r\n");
                    tork_bldr_cmd_e = TORK_BLDR_IDLE_STATE;
                }
                break;
                case TORK_BLDR_SEND_JMP_CMD:
                {
                    idle_state_counter = 0;
                    nwy_ext_echo("Transmitting Jump to main app command\r\n");
                    can_data_buff_g[3] = (uint8_t)((tork_firmware_file_size >> 24) & 0xFF);
                    can_data_buff_g[2] = (uint8_t)((tork_firmware_file_size >> 16) & 0xFF);
                    can_data_buff_g[1] = (uint8_t)((tork_firmware_file_size >> 8) & 0xFF);
                    can_data_buff_g[0] = (uint8_t)(tork_firmware_file_size & 0xFF);

                    for (int loop_var = 4; loop_var < 8; loop_var++)
                    {
                        can_data_buff_g[loop_var] = 0;
                    }
                    nwy_usleep(1000);
                    can_msg_id = ((frame_counter_u32 << 16) & 0xFFFF0000) | 0xF08F;
                    tork_send_can_data(can_msg_id, can_data_buff_g, 0x00);
                    tork_bldr_cmd_e = TORK_BLDR_IDLE_STATE;
                }
                break;
                case TORK_BLDR_APP_UPDATE_SUCCESS:
                {
                    nwy_ext_echo("Tork App update competed\r\n");
                    all_blocks_sent = 0;
                    frame_counter_u32 = 0;
                    tork_bldr_cmd_e = TORK_BLDR_INIT;
                    tork_app_update_complete = 1;
                    // nwy_power_off(2);
                    // tork_bldr_cmd_e = TORK_BLDR_IDLE_STATE;
                }
                break;
                case TORK_BLDR_IDLE_STATE:
                {
                    // do nothing
                    idle_state_counter++;
                    if (idle_state_counter == 2500)
                    {
                        publish_action_status(ota_action_id, 75, "Failed", "");
                        nwy_sleep(3000);
                        publish_action_status(ota_action_id, 75, "Failed", "");
                        nwy_sleep(3000);
                        nwy_power_off(2);
                    }
                    int i = 2;
                }
                break;
                default:
                    break;
                }
                if (tork_app_update_complete == 1)
                {
                    idle_state_counter = 0;
                    tork_file_present = 0;
                    nwy_semahpore_release(s32_update_semaphore);
                    publish_action_status(ota_action_id, 85, "Progress", "");
                    // http_download_flag = 0;
                    // nwy_resume_thread(gps_app_thread);
                    // nwy_resume_thread(ble_app_thread_handle);
                    // nwy_resume_thread(network_app_thread);
                    // // nwy_resume_thread(mqtt_publish_thread);
                    // send_ack_nack(0, 0x55, NULL, 0);
                    // tork_app_update_complete = 0;
                    // tork_app_update_start = 0;
                    break;
                }
                nwy_sleep(20);
            }
        }
        nwy_sleep(50);
    }
}

void s32_update_app(void *param)
{
    uint8_t crc_buff[6];
    static int unknown_state_counter = 0;
    static int update_seq_id = 0;
    int stop_s32_update = 0;
    while (1)
    {
        if (nwy_semaphore_acquire(s32_update_semaphore, 0) == true)
        {
            // bootloader_new_firmware_file_read_v();
            int temp_fd = nwy_sdk_fopen("/sdcard0/test_app.bin", NWY_RDWR);
            if (temp_fd < 0)
            {
                stop_s32_update = 1;
            }
            while (1)
            {
                if (stop_s32_update == 1)
                {
                    publish_action_status(ota_action_id, 95, "Progress", "");
                    nwy_sleep(400);
                    if (nwy_sdk_fexist("/nwy/test_app.pack") == true)
                    {
                        ia_fota_test();
                        // nwy_sleep(2000);
                        // nwy_power_off(2);
                    }
                    else
                    {
                        publish_action_status(ota_action_id, 100, "Completed", "");
                        nwy_sleep(1000);
                    }
                    nwy_sleep(2000);
                    nwy_power_off(2);
                    // uart_thread_counter = 0;
                    // http_download_flag = 0;
                    // stop_s32_update = 0;
                    // nwy_suspend_thread(mqtt_publish_thread);

                    http_download_flag = 0;
                    device_shadow_counter = 0;
                    network_thread_counter = 0;
                    bootloader_uart_tx_cmd_e = BOOTLOADER_MODE_ENABLE_CMD_SEND;
                    tork_bldr_cmd_e = TORK_BLDR_INIT;
                    nwy_resume_thread(gps_app_thread);
                    nwy_resume_thread(ble_app_thread_handle);
                    nwy_resume_thread(network_app_thread);
                    nwy_resume_thread(device_config_thread_handle);
                    send_ack_nack(0, 0x77, NULL, 0);
                    break;
                }

                switch (bootloader_uart_tx_cmd_e)
                {
                /*******************************************************************************************************************/
                case BOOTLOADER_MODE_ENABLE_CMD_SEND:
                {
                    unknown_state_counter = 0;
                    // send_ack_nack(0xAA, BOOTLOADER_MODE_ENABLE_CMD_SEND, NULL, 0);
                    // // bootloader_app_uart_cmd_ack_send_v(BOOTLOADER_MODE_ENABLE_CMD_SEND,0xAA);
                    // nwy_ext_echo("BOOTLOADER_MODE_ENABLE_CMD_SEND \r\n");
                    // total_bin_file_data_read_u32 = 0;
                    // new_firmware_file_size_u32 = 0;
                    update_seq_id = 1;
                    bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
                }
                break;
                /*******************************************************************************************************************/
                case BOOTLOADER_CONFIG_DATA_CMD_SEND:
                {
                    unknown_state_counter = 0;
                    update_seq_id = 2;
                    // bootloader_new_firmware_file_read_v();
                    // bootloader_confif_msg_gst.new_firmware_status_u8 = 0xAA;
                    // bootloader_confif_msg_gst.new_firmware_size_u32 = new_firmware_file_size_u32;
                    // if (new_firmware_file_size_u32 != 0)
                    // {
                    //     bootloader_app_uart_data_send_v(BOOTLOADER_CONFIG_DATA_CMD_SEND,
                    //                                     0xAA, (unsigned char *)&bootloader_confif_msg_gst,
                    //                                     sizeof(bootloader_confif_msg_gst));
                    // }
                    // nwy_ext_echo("BOOTLOADER_CONFIG_DATA_CMD_SEND \r\n");
                    bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
                }
                break;
                /*******************************************************************************************************************/
                case BOOTLOADER_1KB_CRC_DATA_SEND:
                {
                    unknown_state_counter = 0;
                    nwy_ext_echo("BOOTLOADER_1KB_CRC_DATA_SEND \r\n");
                    bootloader_new_firmware_file_read_and_uart_send_handling_v();
                    bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
                }
                break;
                /*******************************************************************************************************************/
                case BOOTLOADER_END_OF_DATA_CMD_SEND:
                {
                    unknown_state_counter = 0;
                    nwy_ext_echo("BOOTLOADER_END_OF_DATA_CMD_SEND \r\n");
                    bootloader_app_uart_cmd_ack_send_v(BOOTLOADER_END_OF_DATA_CMD_SEND, 0xAA);
                    bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
                }
                break;
                /*******************************************************************************************************************/
                case BOOTLOADER_WHOLE_BIN_CRC_CHECK_CMD_SEND:
                {
                    unknown_state_counter = 0;
                    nwy_ext_echo("BOOTLOADER_WHOLE_BIN_CRC_CHECK_CMD_SENDING \r\n");
                    // bootloader_app_uart_cmd_ack_send_v(BOOTLOADER_WHOLE_BIN_CRC_CHECK_CMD_SEND,0xAA);
                    crc_buff[3] = (uint8_t)((whole_bin_crc_u32 >> 24) & 0xFF);
                    crc_buff[2] = (uint8_t)((whole_bin_crc_u32 >> 16) & 0xFF);
                    crc_buff[1] = (uint8_t)((whole_bin_crc_u32 >> 8) & 0xFF);
                    crc_buff[0] = (uint8_t)(whole_bin_crc_u32 & 0xFF);
                    crc_buff[4] = (uint8_t)(last_block_byte_count & 0xFF);
                    crc_buff[5] = (uint8_t)((last_block_byte_count >> 8) & 0xFF);
                    bootloader_app_uart_data_send_v(BOOTLOADER_WHOLE_BIN_CRC_CHECK_CMD_SEND,
                                                    0xAA, crc_buff, 6);
                    bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
                }
                break;
                /*******************************************************************************************************************/
                case BOOTLOADER_JUMPING_TO_BOOTLOADER_CMD_SEND:
                {
                    unknown_state_counter = 0;
                    nwy_ext_echo("BOOTLOADER_JUMPING_TO_BOOTLOADER_CMD_SEND \r\n");
                    bootloader_app_uart_cmd_ack_send_v(BOOTLOADER_JUMPING_TO_BOOTLOADER_CMD_SEND, 0xAA);
                    bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
                }
                break;
                /*******************************************************************************************************************/
                case BOOTLOADER_INT_APP_ERASE_CMD_SEND:
                {
                    unknown_state_counter = 0;
                    nwy_ext_echo("BOOTLOADER_INT_APP_ERASE_CMD_SEND \r\n");
                    // bootloader_app_uart_cmd_ack_send_v(BOOTLOADER_INT_APP_ERASE_CMD_SEND,0xAA);
                    bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
                }
                break;
                /*******************************************************************************************************************/
                case BOOTLOADER_INT_APP_WRITE_CMD_SEND:
                {
                    unknown_state_counter = 0;
                    nwy_ext_echo("BOOTLOADER_INT_APP_WRITE_CMD_SEND \r\n");
                    // bootloader_app_uart_cmd_ack_send_v(BOOTLOADER_INT_APP_WRITE_CMD_SEND,0xAA);
                    bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
                }
                break;
                /********************************************************************************************************************/
                case BOOTLOADER_JUMPING_APPLICATION_CMD_SEND:
                {
                    unknown_state_counter = 0;
                    nwy_ext_echo("BOOTLOADER_JUMPING_APPLICATION_CMD_SEND \r\n");
                    s32_update_complete = 1;
                    unknown_state_counter = 0;
                    // bootloader_app_uart_cmd_ack_send_v(BOOTLOADER_JUMPING_APPLICATION_CMD_SEND,0xAA);
                    publish_action_status(ota_action_id, 95, "Progress", "");
                    nwy_sleep(400);
                    // ia_fota_test();
                    // nwy_power_off(2);
                    // uart_thread_counter = 0;
                    // http_download_flag = 0;
                    if (nwy_sdk_fexist("/sdcard0/test_app.pack") == true)
                    {
                        ia_fota_test();
                        // nwy_sleep(5000);
                        // nwy_power_off(2);
                    }
                    else
                    {
                        publish_action_status(ota_action_id, 100, "Completed", "");
                        nwy_sleep(1000);
                    }
                    nwy_sleep(5000);
                    nwy_power_off(2);
                    network_thread_counter = 0;
                    http_download_flag = 0;
                    device_shadow_counter = 0;
                    bootloader_uart_tx_cmd_e = BOOTLOADER_MODE_ENABLE_CMD_SEND;
                    tork_bldr_cmd_e = TORK_BLDR_INIT;
                    nwy_resume_thread(gps_app_thread);
                    nwy_resume_thread(ble_app_thread_handle);
                    nwy_resume_thread(network_app_thread);
                    nwy_resume_thread(device_config_thread_handle);
                    // bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
                }
                break;
                /********************************************************************************************************************/
                case BOOTLOADER_TIMEOUT_ERROR_WITH_DIAG_TOOL_CMD_SEND:
                {
                }
                break;
                /********************************************************************************************************************/
                case BOOTLOADER_APPLICATION_NOT_AVAILABLE_CMD_SEND:
                {
                }
                break;
                /*********************************************************************************************************************/
                case BOOTLOADER_STATE_UNKNOWN:
                {
                    // do nothing
                    unknown_state_counter++;
                    if(update_seq_id == 1)
                    {
                        update_seq_id = 0;
                        send_ack_nack(0xAA, BOOTLOADER_MODE_ENABLE_CMD_SEND, NULL, 0);
                        // bootloader_app_uart_cmd_ack_send_v(BOOTLOADER_MODE_ENABLE_CMD_SEND,0xAA);
                        nwy_ext_echo("BOOTLOADER_MODE_ENABLE_CMD_SEND \r\n");
                        total_bin_file_data_read_u32 = 0;
                        new_firmware_file_size_u32 = 0;
                    }
                    if(update_seq_id == 2)
                    {
                        update_seq_id = 0;
                        bootloader_new_firmware_file_read_v();
                        bootloader_confif_msg_gst.new_firmware_status_u8 = 0xAA;
                        bootloader_confif_msg_gst.new_firmware_size_u32 = new_firmware_file_size_u32;
                        if (new_firmware_file_size_u32 != 0)
                        {
                            bootloader_app_uart_data_send_v(BOOTLOADER_CONFIG_DATA_CMD_SEND,
                                                            0xAA, (unsigned char *)&bootloader_confif_msg_gst,
                                                            sizeof(bootloader_confif_msg_gst));
                        }
                        nwy_ext_echo("BOOTLOADER_CONFIG_DATA_CMD_SEND \r\n");
                    }
                    if (unknown_state_counter == 3000)
                    {
                        publish_action_status(ota_action_id, 85, "Failed", "S32 update timeout");
                        nwy_sleep(3000);
                        publish_action_status(ota_action_id, 85, "Failed", "S32 update timeout");
                        nwy_sleep(3000);
                        nwy_power_off(2);
                    }

                }
                break;
                default:
                {
                    nwy_ext_echo("UART tx running %d\r\n", bootloader_uart_tx_cmd_e);
                }
                break;
                    /*********************************************************************************************************************/
                }
                if (s32_update_complete == 1)
                {
                    s32_update_complete = 0;
                    break;
                }
                // nwy_sleep(20);
                nwy_sleep(20);
            }
        }
        nwy_sleep(50);
    }
}

/**
=====================================================================================================================================================

@fn Name            : bootloader_app_uart_cmd_ack_send_v
@b Scope            : Private
@n@n@b Description  : Sends Bootloader Cmd through CAN0
@param Input Data   : command_aru8, command_status_aru8
@return Return Value: NULL

=====================================================================================================================================================
*/
void bootloader_app_uart_cmd_ack_send_v(uint8_t cmd, uint8_t cmd_ack_sts)
{
    uint8_t can_data[100] = {0};
    uint16_t length = 12;
    can_data[0] = 0xDE;
    can_data[1] = 0xAD;
    can_data[2] = cmd;
    can_data[3] = (length + 1) & 0xFF; // length lo byte //status length is one more byte
    can_data[4] = (length + 1) >> 8;   // length hi byte
    can_data[5] = cmd_ack_sts;
    can_data[length + 6] = 0xDE;
    can_data[length + 7] = 0xAD; // TODO::implement CRC check
    can_data[length + 8] = 0x03;
    for (int i = 0; i < 9 + length; i++)
    {
        // nwy_ext_echo("%x ", can_data[i]);
        nwy_uart_send_data(STM_UART_fd, (uint8_t *)&can_data[i], 1);
        nwy_usleep(50);
    }
}

/**
=====================================================================================================================================================

@fn Name            : bootloader_app_uart_cmd_ack_send_v
@b Scope            : Private
@n@n@b Description  : Sends Bootloader Cmd through CAN0
@param Input Data   : command_aru8, command_status_aru8
@return Return Value: NULL

=====================================================================================================================================================
*/

void bootloader_new_firmware_file_read_v(void)
{
    int read_count_u32 = 0;
    char new_firmware_file_name_string[100] = {0};
    int new_firmware_file_fd = 0;
    int temp_fw_size = 0;
    // if (sd_card_mounted_flag == true)
    if (nwy_read_sdcart_status() == true)
    {
        sprintf(new_firmware_file_name_string, "/nwy/test_app.bin");

        new_firmware_file_fd = nwy_sdk_fopen(new_firmware_file_name_string, NWY_RDWR);
        if (new_firmware_file_fd < 0)
        {
            nwy_ext_echo("\r\nfile open fail\r\n");
            return;
        }
        // nwy_ext_echo("\r\nfile call 3\r\n");
        new_firmware_file_size_u32 = nwy_sdk_fsize_fd(new_firmware_file_fd);
        temp_fw_size = new_firmware_file_size_u32;
        num_of_one_kb_blocks = new_firmware_file_size_u32 / 1024;

        if (num_of_one_kb_blocks * 1024 < new_firmware_file_size_u32)
        {
            last_block_byte_count = new_firmware_file_size_u32 % 1024;
            int t_var = 0;
            for (t_var = new_firmware_file_size_u32; t_var < ((num_of_one_kb_blocks + 1) * 1024); t_var++)
            {
                whole_bin_file_array_u8[t_var] = 0xFF;
            }
            new_firmware_file_size_u32 = (num_of_one_kb_blocks + 1) * 1024;
            num_of_one_kb_blocks = num_of_one_kb_blocks + 1;
        }
        else if (num_of_one_kb_blocks * 1024 == new_firmware_file_size_u32)
        {
            last_block_byte_count = 0;
        }

        nwy_ext_echo("\r\n New_Fimrware_App_Size = %d\r\n", new_firmware_file_size_u32);
        nwy_sdk_fclose(new_firmware_file_fd);

        new_firmware_file_fd = nwy_sdk_fopen(new_firmware_file_name_string, NWY_RDONLY);
        nwy_ext_echo("\r\n file open status  = %d\r\n", new_firmware_file_fd);
        read_count_u32 = nwy_sdk_fread(new_firmware_file_fd, whole_bin_file_array_u8, new_firmware_file_size_u32);
        nwy_ext_echo("\r\n file read status  = %d\r\n", read_count_u32);
        nwy_sdk_fclose(new_firmware_file_fd);
        if (read_count_u32 == temp_fw_size)
        {
            whole_bin_crc_u32 = bootloader_app_crc32_generate_u32(whole_bin_file_array_u8, (uint32_t)(temp_fw_size));
            nwy_ext_echo("\r\n Whole bin crc = %d\r\n", whole_bin_crc_u32);
        }
        else
        {
            nwy_ext_echo("\r\n whole file read failed\r\n");
            bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
        }
    }
    else
    {
        nwy_ext_echo("\r\n SD Card mount failed\r\n");
        bootloader_uart_tx_cmd_e = BOOTLOADER_STATE_UNKNOWN;
    }

    // nwy_sdk_fclose(new_firmware_fd_global);
    // nwy_ext_echo("\r\nfile call 4\r\n");
    // nwy_sdk_fseek(fd_global, new_firmware_file_size_u32, NWY_SEEK_SET);
    // memset(&file_data[0], 0, sizeof(file_data));
}

/**
=====================================================================================================================================================

@fn Name            : bootloader_app_uart_cmd_ack_send_v
@b Scope            : Private
@n@n@b Description  : Sends Bootloader Cmd through CAN0
@param Input Data   : command_aru8, command_status_aru8
@return Return Value: NULL

=====================================================================================================================================================
*/
void bootloader_app_uart_data_send_v(uint8_t cmd, uint8_t cmd_sts, uint8_t *data_au8, uint16_t length_u16)
{
    uint16_t test_len = 13;
    uint8_t can_data[75] = {
        0,
    };
    can_data[0] = 0xDE;
    can_data[1] = 0xAD;
    can_data[2] = cmd;
    can_data[3] = (test_len)&0xFF; // length lo byte //status length is one more byte
    can_data[4] = (test_len) >> 8; // length hi byte
    can_data[5] = cmd_sts;
    if (length_u16 != 0)
    {
        // memory_copy_u8_array_v(&can_data[6], data_au8, length_u16);
        memcpy(&can_data[6], data_au8, length_u16);
    }
    can_data[18] = 0x00;
    can_data[19] = 0x00;
    can_data[20] = 0x03;
    if (nwy_semaphore_acquire(uart_tx_semaphore, 0xFFFF) == true)
    {
        for (int i = 0; i < 21; i++)
        {
            // nwy_ext_echo("%x ", can_data[i]);
            nwy_uart_send_data(STM_UART_fd, (uint8_t *)&can_data[i], 1);
            // nwy_sleep(1);
            nwy_usleep(50);
        }
        nwy_semahpore_release(uart_tx_semaphore);
    }
}

/**
=====================================================================================================================================================

@fn Name            : bootloader_new_firmware_file_read_and_uart_send_handling_v
@b Scope            : public
@n@n@b Description  :
@param Input Data   :
@return Return Value: NULL

=====================================================================================================================================================
*/
void bootloader_new_firmware_file_read_and_uart_send_handling_v(void)
{
    static int loop_var = 1;
    if (num_of_one_kb_blocks > 0)
    {
        nwy_ext_echo("1024 Block id = %d \r\n", loop_var);
        for (int temp_var = 0; temp_var < 128; temp_var++)
        {
            bootloader_app_uart_data_send_v(BOOTLOADER_1KB_CRC_DATA_SEND, 0xAA,
                                            &whole_bin_file_array_u8[total_bin_file_data_read_u32], 8);
            total_bin_file_data_read_u32 += 8;
            nwy_usleep(2000);
        }
        nwy_ext_echo("1024 bytes sent\r\n");

        num_of_one_kb_blocks--;
        loop_var++;
    }
    else
    {
        nwy_ext_echo("All Fimrware Sent Succesfully and now sending end of data\r\n");
        bootloader_app_uart_cmd_ack_send_v(BOOTLOADER_END_OF_DATA_CMD_SEND, 0xAA);
    }
}

/**
=====================================================================================================================================================

@fn Name			:
@b Scope            :
@n@n@b Description  :
@param Input Data   :
@return Return Value:

=====================================================================================================================================================
*/
int publish_device_shadow()
{
    struct timeval tval;
    // static int count = 0;
    double sec = 0;
    double msec = 0;

    nwy_ext_echo("Device shadow message\r\n");
    gettimeofday(&tval, NULL);

    sec = tval.tv_sec;
    msec = ((double)tval.tv_usec) / 1.0e6;

    unsigned long long timestamp1 = (unsigned long long)(sec + msec) * 1000;
    char *device_status = "\"Device is active\"";
    // uint32_t ignition_button_status = (uint32_t)uart_strcut->payload[0];
    static uint32_t sq_id = 0;
    memset(json_dev_shadow_msg_buff_gau8, 0, DEV_SHADOW_MSG_BUFF_LEN);
    snprintf(json_dev_shadow_msg_buff_gau8, sizeof(json_dev_shadow_msg_buff_gau8), dev_shadow_mess_json,
             timestamp1,
             device_status,
             ble_status,
             nw_csq_val,
             ign_status,
             imu_status,
             ble_mac_id_g,
             n58_base_fw_ver,
             n58_app_fw_ver,
             s32_app_fw_ver,
             batt_voltage,
             input_voltage,
             tork_fw_ver,
             tork_hw_ver,
             sq_id);
    // nwy_ext_echo("\r\n%s\r\n", json_dev_shadow_msg_buff_gau8); // For the verification of the JSON frame

    UART_data_struct dev_shadow_publish_msg;
    dev_shadow_publish_msg.cmd = 0xDE;
    dev_shadow_publish_msg.length_u16 = strlen((char *)json_dev_shadow_msg_buff_gau8);

    memcpy(dev_shadow_publish_msg.payload, json_dev_shadow_msg_buff_gau8, dev_shadow_publish_msg.length_u16);

    // if (get_MQTT_COnnection_Status() && (store_msg_flag == false) && (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue)!= 0))
    if (get_MQTT_COnnection_Status())
    {
        sq_id++;
        if (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue) == 0)
        {
            // nwy_ext_echo("\r\nMQTT Queue full agidhe\r\n");
        }
        else
        {
            nwy_ext_echo("\r\npublishing device shadow in MQTT\r\n");
            nwy_put_msg_que(mqtt_publish_msg_queue, &dev_shadow_publish_msg, 0);
        }
    }
    return 0;
}