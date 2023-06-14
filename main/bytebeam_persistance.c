#include "bytebeam.h"
#include "nwy_gpio.h"
#include "nwy_osi_api.h"
#include "nwy_sim.h"
#include "nwy_vir_at.h"
#include "osi_log.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
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
#include "bytebeam_uart.h"

#define NUM_OF_FILES 10

#define BYTES_PER_FILE (1024*1024*1024)

int can_files[NUM_OF_FILES] = {
    0,
};

int gps_files[NUM_OF_FILES] = {
    0,
};

int fd_global = 0;
int persist_file_count = 0;

int file_read_thread_fd = 0;


void store_data_ThreadEntry(void *param)
{
    #if 1
    char file_name_string[100] = {
        0,
    };
    // int fd = 0;
    int len = 0;
    long file_size = 0;
    char file_data[MQTT_BATCH_SIZE] = {
        0,
    };
    char can_msg[] = "CAN|";
    char gps_msg[] = "GPS|";
    bool storage_task_fd_open = false;
    UART_data_struct store_data;
    UART_data_struct read_data_st;

    while (1)
    {

#if 1
        if (nwy_semaphore_acquire(file_semaphore, 0) == true)
        {
            //   nwy_ext_echo("\r\nWrite thread semaphore aquired\r\n");
            if(nwy_get_msg_que(store_queue, &store_data, 0xffffffff) == true)
            {
                nwy_ext_echo("STX:%x CMD:%x Len:%x\r\n", store_data.stx, store_data.cmd, store_data.length_u16);
                if (nwy_read_sdcart_status() == true && fd_global != 0)
                {
                    // nwy_ext_echo("\r\nfile call 1\r\n");
                    nwy_sdk_fclose(fd_global);
                }
                storage_task_fd_open = false;
                // nwy_ext_echo("In Storage Task ");
                if (store_msg_flag == true)
                {
                    if (nwy_read_sdcart_status() == true)
                    {
                        sprintf(file_name_string, "/sdcard0/persistent/data_backup_%d.txt", persist_file_count);
                        fd_global = nwy_sdk_fopen(file_name_string, NWY_CREAT | NWY_RDWR | NWY_APPEND);
                        if (fd_global < 0)
                        {
                            nwy_ext_echo("\r\nfile open fail\r\n");
                            return;
                        }
                        storage_task_fd_open = true;
                        // nwy_ext_echo("\r\nfile call 3\r\n");
                        file_size = nwy_sdk_fsize_fd(fd_global);
                        nwy_ext_echo("\r\nfile call 4\r\n");
                        nwy_sdk_fseek(fd_global, file_size, NWY_SEEK_SET);
                        memset(&file_data[0], 0, sizeof(file_data));

                        if ((uint64_t)(file_size + store_data.length_u16 + 8) < (uint64_t)BYTES_PER_FILE)
                        {
                            switch (store_data.cmd)
                            {
                            case 0x04:
                            {
                                // nwy_ext_echo("\r\nfile call 5\r\n");
                                len = 0;
                                len = nwy_sdk_fwrite(fd_global, &(store_data.stx), sizeof(store_data.stx));
                                // nwy_ext_echo("\r\ STX Count = %d\r\n", sizeof(store_data.data_struct.stx));
                                if (len < sizeof(store_data.stx))
                                {
                                    nwy_ext_echo("\r\nFile Write Error store_data.stx!! its printing from here sizeof():%d written:%d\r\n", sizeof(store_data.stx), len);
                                    continue;
                                }
                                // nwy_ext_echo("\r\nfile call 6\r\n");
                                len = nwy_sdk_fwrite(fd_global, &(store_data.cmd), sizeof(store_data.cmd));
                                nwy_ext_echo("\r\ CMD Count = %d\r\n", sizeof(store_data.cmd));
                                if (len < sizeof(store_data.cmd))
                                {
                                    nwy_ext_echo("\r\nFile Write Error store_data.cmd!!\r\n");
                                    continue;
                                }
                                // nwy_ext_echo("\r\nfile call 7\r\n");
                                len = nwy_sdk_fwrite(fd_global, &(store_data.length_u16), sizeof(store_data.length_u16));
                                //  nwy_ext_echo("\r\ length_u16 Count = %d\r\n", sizeof(store_data.data_struct.length_u16));
                                if (len < sizeof(store_data.length_u16))
                                {
                                    nwy_ext_echo("\r\nFile Write Error store_data.length_16!!\r\n");
                                    continue;
                                }
                                // nwy_ext_echo("\r\nfile call 8\r\n");
                                len = nwy_sdk_fwrite(fd_global, &(store_data.payload[0]), store_data.length_u16);
                                
                                // nwy_ext_echo("\r\ pay_load Count = %d\r\n", (store_data.data_struct.length_u16));
                                if (len < store_data.length_u16)
                                {
                                    nwy_ext_echo("\r\nFile Write Error store_data.payload!!\r\n");
                                    continue;
                                }
                                
                                // nwy_ext_echo("\r\nfile call 9\r\n");
                                len = nwy_sdk_fwrite(fd_global, &(store_data.crc_u16), sizeof(store_data.crc_u16));
                                
                                // nwy_ext_echo("\r\ crc_u16 Count = %d\r\n", sizeof(store_data.data_struct.crc_u16));
                                if (len < sizeof(store_data.crc_u16))
                                {
                                    nwy_ext_echo("\r\nFile Write Error store_data.crc!!\r\n");
                                    continue;
                                }

                                // nwy_ext_echo("Data written timestamp: %llu %d\r\n", store_data.data_struct.timestamp, sizeof(store_data.data_struct.timestamp));
                                len = nwy_sdk_fwrite(fd_global, &(store_data.timestamp), sizeof(store_data.timestamp));
                                
                                if (len < sizeof(store_data.timestamp))
                                {
                                    nwy_ext_echo("\r\nFile Write Error store_data.timestamp!!\r\n");
                                    continue;
                                }

                                len = nwy_sdk_fwrite(fd_global, &(store_data.etx), sizeof(store_data.etx));
                                if (len < sizeof(store_data.etx))
                                {
                                    nwy_ext_echo("\r\nFile Write Error store_data.etx!!\r\n");
                                    continue;
                                }
                            }
                            break;
                            case 0x05:
                            {

                                memcpy(&file_data[0], &gps_msg[0], strlen(gps_msg));
                                memcpy(&file_data[strlen(gps_msg)], &store_data.payload[0], store_data.length_u16);

                                len = nwy_sdk_fwrite(fd_global, file_data, strlen(file_data));
                                if (len < strlen(file_data))
                                {
                                    nwy_ext_echo("\r\nFile Write Error GPS!!\r\n");
                                    return;
                                }
                            }
                            break;
                            }

                            // nwy_ext_echo("\r\nFile write completed in %d\r\n", persist_file_count);
                            // nwy_ext_echo("\r\nfile call fclose 111\r\n");
                            nwy_sdk_fclose(fd_global);
                            storage_task_fd_open = false;
                        }
                        else
                        {

                            // one file write is full
                            // nwy_ext_echo("\r\nfile call 7\r\n");
                            nwy_sdk_fclose(fd_global);
                            storage_task_fd_open = false;
                            if (persist_file_count < NUM_OF_FILES)
                                persist_file_count++;
                            else
                                persist_file_count = 0;

                            // nwy_ext_echo("Starting next file to store data %d", persist_file_count);
                            sprintf(file_name_string, "/sdcard0/persistent/data_backup_%d.txt", persist_file_count);
                            // nwy_ext_echo("file name is %s", file_name_string);
                            // nwy_ext_echo("\r\nfile call 8\r\n");
                            fd_global = nwy_sdk_fopen(file_name_string, NWY_CREAT | NWY_RDWR | NWY_APPEND); // Opening Next file
                            if (fd_global < 0)
                            {
                                nwy_ext_echo("\r\nfile open fail\r\n");
                                return;
                            }
                            // nwy_ext_echo("\r\nfile call 9\r\n");
                            file_size = nwy_sdk_fsize_fd(fd_global);
                            nwy_sdk_fseek(fd_global, file_size, NWY_SEEK_SET); // Move the file pointer to the last location from the beginning
                            if (file_size + store_data.length_u16 + 8 < BYTES_PER_FILE)
                            {
                                // nwy_ext_echo("\r\nfile call 10\r\n");
                                int len = nwy_sdk_fwrite(fd_global, file_data, strlen(file_data));
                                // nwy_ext_echo("size of written text is %d", len);
                                if (len < store_data.length_u16 + 8)
                                {
                                    nwy_ext_echo("\r\nFile Write Error!!\r\n");
                                    return;
                                }
                                // nwy_ext_echo("\r\nfile call 11\r\n");
                                nwy_sdk_fclose(fd_global);
                                storage_task_fd_open = true;
                            }
                        }
                    }
                    memset(store_data.payload, "\0", sizeof(store_data.payload));
                    // nwy_ext_echo("\r\nfile call 12\r\n");
                    nwy_sdk_fclose(fd_global);
                    storage_task_fd_open = true;
                }
            }
            nwy_semahpore_release(file_semaphore);
        }

        nwy_sleep(1);
#endif
    }
#endif
}

uint8_t data_backup_file_count()
{
    int i = 0;
    int file_count_iterator = 0;
    char file_name_string[50] = {
        0,
    };

    for (; i < NUM_OF_FILES; i++)
    {
        sprintf(file_name_string, "/sdcard0/persistent/data_backup_%d.txt", i);
        // nwy_ext_echo("\r\nfile call 13\r\n");
        can_files[i] = nwy_sdk_fexist(file_name_string); // Checking for the Files stored
        //  nwy_ext_echo("CAN file data: %d, %d \n", i, can_files[i]);
        if (can_files[i] == 1)
        {
            store_msg_flag = true;
            // nwy_ext_echo("CAN file is available\n");
            file_count_iterator++;
        }
    }
    if (store_msg_flag == true)
        return file_count_iterator;
    else
        return 0;
}

int32_t process_file_data(uint8_t data)
{
    static uint8_t data_process_state = STX1_STATE;
    // static UART_APP_RX_HANDLE_tst uart_rx_data_gst;
    static UART_data_struct pub_msg_queue;
    uint8_t data_to_send_uart = 1;
    static uint8_t len_byte1;
    static uint8_t len_byte2;
    static uint8_t crc_byte_1;
    static uint8_t crc_byte_2;

    static uint16_t length_16;
    static uint16_t crc_16;
    static uint8_t stx_by1_u8 = 0;
    static uint8_t stx_by2_u8 = 0;

    static uint16_t len_counter = 0;
    static uint8_t cmd = 0;
    static uint64_t timestamp = 0;
    static uint8_t timestamp_array[8] = {
        0,
    };
    int read_count = 0;
    // static uint8_t pay_load[100] = {0,};
    // nwy_ext_echo("\r\nData is %x %d\r\n", data, data_process_state);
   // nwy_sleep(10);

    switch (data_process_state)
    {
    case STX1_STATE:
    {
        if (data == 0xAD)
        {
            // nwy_ext_echo("\r\nSTX_STATE\r\n");
            data_process_state = STX2_STATE;
            stx_by1_u8 = data;
        }
        break;
    }

    case STX2_STATE:
    {
        if (data == 0xDE)
        {
            // nwy_ext_echo("\r\nSTX_STATE\r\n");
            data_process_state = CMD_STATE;
            stx_by2_u8 = data;
            pub_msg_queue.stx = (uint16_t)(stx_by2_u8 << 8 | stx_by1_u8);
            // nwy_ext_echo("\r\nSTX_ ID = %x\r\n", pub_msg_queue.data_struct.stx);
        }
        break;
    }

    case CMD_STATE:
    {
        cmd = data;
        // nwy_ext_echo("\r\nCMD_STATE\r\n");
        data_process_state = LEN1_STATE;
        pub_msg_queue.cmd = data;
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
        pub_msg_queue.length_u16 = length_16;
        data_process_state = DATA_STATE;
        nwy_ext_echo("\r\nLenght is : %d, %d %d\r\n", length_16, len_byte1, len_byte2);
        if (length_16 > 4096 || length_16 <= 0)
        {
            data_process_state = STX1_STATE;
        }
    }
    break;

    case DATA_STATE:
    {
        int ret_val = nwy_sdk_fread(file_read_thread_fd, pub_msg_queue.payload, length_16);
        if (ret_val < length_16)
        {
            nwy_ext_echo("Read data failure\r\n");
        }
        else
        {
            data_process_state = CRC1_STATE;
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
        data_process_state = TIMESTAMP_STATE;
        pub_msg_queue.crc_u16 = (uint16_t)(crc_byte_2 << 8 | crc_byte_1);
        nwy_ext_echo("\r\nCRC2_STATE %x\r\n", pub_msg_queue.crc_u16);
        len_counter = 0;
        break;
    }

    case TIMESTAMP_STATE:
    {
        timestamp_array[len_counter] = data;

        if (len_counter <= 7)
        {
            data_process_state = TIMESTAMP_STATE;
            nwy_ext_echo("\r\nTime stamp count:%d  data %d \r\n", len_counter, data);
            len_counter++;
        }
        else
        {
            data_process_state = ETX_STATE;
            len_counter = 0;
            timestamp = (timestamp_array[0] << (8 * 7)) | (timestamp_array[1] << (8 * 6)) | (timestamp_array[2] << (8 * 5)) | (timestamp_array[3] << (8 * 4)) |
                        (timestamp_array[4] << (8 * 3)) | (timestamp_array[5] << (8 * 2)) | (timestamp_array[6] << (8 * 1)) | (timestamp_array[7]);
            pub_msg_queue.timestamp = timestamp;
            nwy_ext_echo("\r\Timestamp %llu\r\n", pub_msg_queue.timestamp);
        }
        break;
    }
    case ETX_STATE:
    {
        nwy_ext_echo("\r\nETX_STATE %d\r\n", data);
        if (data == 0x03)
        {
            //nwy_uart_send_data(STM_UART_flag, &data_to_send_uart, 1);
            pub_msg_queue.etx = data;
            // execute_command(uart_rx_data_gst);
            // enqueue to mqtt
            nwy_ext_echo("\r\nSuccessfully published to mqtt thread\r\n");
            nwy_put_msg_que(mqtt_publish_msg_queue, &pub_msg_queue, 0xffffffff);

            data_process_state = STX1_STATE;
        }
    }
    break;
    default:
    {
        data_process_state = STX1_STATE;
        break;
    }
    }
}

long file_read_data_count = 0;
void read_and_send_data_mqtt(void)
{
    #if 1
    int len = 0;
    static UART_data_struct pub_msg_queue;
    len = nwy_sdk_fread(fd_global, &(pub_msg_queue.stx), sizeof(pub_msg_queue.stx));
    file_read_data_count += len;
    if (len < sizeof(pub_msg_queue.stx))
    {
        nwy_ext_echo("\r\nFile read Error pub_msg_queue.stx!! its printing from here sizeof():%d written:%d\r\n", sizeof(pub_msg_queue.stx), len);
    }
    if (pub_msg_queue.stx == 0xdead)
    {
        // nwy_ext_echo("\r\nFile read STX = :%d \r\n", pub_msg_queue.data_struct.stx);
        len = nwy_sdk_fread(fd_global, &(pub_msg_queue.cmd), sizeof(pub_msg_queue.cmd));
        file_read_data_count += len;
        if (len < sizeof(pub_msg_queue.cmd))
        {
            nwy_ext_echo("\r\nFile read Error pub_msg_queue.cmd!!\r\n");
        }
        // nwy_ext_echo("\r\nfile call 7\r\n");
        len = nwy_sdk_fread(fd_global, &(pub_msg_queue.length_u16), sizeof(pub_msg_queue.length_u16));
        file_read_data_count += len;
        if (len < sizeof(pub_msg_queue.length_u16))
        {
            nwy_ext_echo("\r\nFile read Error pub_msg_queue.length_16!!\r\n");
        }
        // nwy_ext_echo("\r\nfile call 8\r\n");
        len = nwy_sdk_fread(fd_global, &(pub_msg_queue.payload[0]), pub_msg_queue.length_u16);
        // nwy_ext_echo("\r\nFile read length_u16 = :%d \r\n", pub_msg_queue.data_struct.length_u16);
        file_read_data_count += len;
        if (len < pub_msg_queue.length_u16)
        {
            nwy_ext_echo("\r\nFile read Error pub_msg_queue.payload!!\r\n");
        }
        // nwy_ext_echo("\r\nfile call 9\r\n");
        len = nwy_sdk_fread(fd_global, &(pub_msg_queue.crc_u16), sizeof(pub_msg_queue.crc_u16));
        file_read_data_count += len;
        // nwy_ext_echo("\r\nFile read CRC = :%d \r\n", pub_msg_queue.data_struct.crc_u16);
        if (len < sizeof(pub_msg_queue.crc_u16))
        {
            nwy_ext_echo("\r\nFile Read Error pub_msg_queue.crc!!\r\n");
        }

        // nwy_ext_echo("Data written timestamp: %llu %d\r\n", store_data.data_struct.timestamp, sizeof(store_data.data_struct.timestamp));
        len = nwy_sdk_fread(fd_global, &(pub_msg_queue.timestamp), sizeof(pub_msg_queue.timestamp));
        file_read_data_count += len;
        // nwy_ext_echo("\r\nFile read timestamp = :%llu \r\n", pub_msg_queue.data_struct.timestamp);
        if (len < sizeof(pub_msg_queue.timestamp))
        {
            nwy_ext_echo("\r\nFile Read Error pub_msg_queue.timestamp!!\r\n");
        }

        // nwy_ext_echo("\r\nfile call 10\r\n");4
        nwy_ext_echo("Data Read ETX: %d\r\n", pub_msg_queue.etx);
        len = nwy_sdk_fread(fd_global, &(pub_msg_queue.etx), sizeof(pub_msg_queue.etx));
        file_read_data_count += len;
        if (len < sizeof(pub_msg_queue.etx))
        {
            nwy_ext_echo("\r\nFile Read Error pub_msg_queue.etx!!\r\n");
        }

        if (pub_msg_queue.etx == 0x03)
        {
            if(nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue) != 0)
            {
                nwy_put_msg_que(mqtt_publish_msg_queue, &pub_msg_queue, 0x00);
            }
        }
    }
    #endif
}

void read_data_ThreadEntry(void *param)
{
   
    #if 1
    UART_data_struct pub_msg_data;
    int f_count = 0;
    int total_files = 0;
    uint8_t file_name_string[100] = {
        0,
    };
    uint8_t file_read;
    char *file_data;
    char can_msg[] = "CAN";
    char gps_msg[] = "GPS";
    long i = 0;
    bool dont_publish = false;

#if 1
    while (1)
    {
#if 1
        // nwy_ext_echo("\r\nRead thread\r\n");
        if (store_msg_flag)
        {
            if (nwy_semaphore_acquire(file_semaphore, 0) == true)
            {
#if 1
                if (total_files == 0)
                {
                    // nwy_ext_echo("\r\nfile call 14\r\n");
                    if (nwy_read_sdcart_status() == true)
                    {
                        total_files = data_backup_file_count(); // to read no of files in SD card
                    }
                    // nwy_sleep(10);
                }
                else
                {
                    //  nwy_ext_echo("\r\nTotal file count is non zero\r\n");
                    if (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue) > 0) // if space is available read and enqueue the data
                    {
                        nwy_ext_echo("\r\nSpace available in MQTT queue\r\n");
                        // mutex lock
                        // have a loop that runs for NUM_OF_FILES if all fails return
                        sprintf(file_name_string, "/sdcard0/persistent/data_backup_%d.txt", f_count);
                        // nwy_ext_echo("\r\nfile call 15\r\n");
                        file_read_thread_fd = nwy_sdk_fopen(file_name_string, NWY_RDONLY); //| NWY_TRUNC
                        // nwy_sleep(10);
                        //  nwy_ext_echo("\r\nfile call 16\r\n");
                        long file_size = nwy_sdk_fsize_fd(file_read_thread_fd);

                        if (file_read_thread_fd < 0)
                        {
                            nwy_ext_echo("\r\n[READ DATA]: nfile open fail\r\n");
                            return;
                        }

                        // nwy_ext_echo("\r\nfile call 17\r\n");
                        // while (1 == nwy_sdk_fread(file_read_thread_fd, &file_read, 1))
                        file_read_data_count = 0;
                        nwy_ext_echo("\r\n read file size  = %ld  file read = %ld condition = %d \r\n", file_size, file_read_data_count, (file_size > file_read_data_count));
                        while (file_size > file_read_data_count)
                        {

                            // nwy_ext_echo("\r\nfile Read daat mqtt() \r\n");
                            if (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue) > 0)
                            {
                                read_and_send_data_mqtt();
                            }
                            else
                            {
                                nwy_sleep(1);
                                // nwy_ext_echo("\r\nfile call 17.9\r\n");
                            }
                        }
                        if (total_files == 1)
                        {
                            if ((10 - nwy_get_queue_spaceevent_cnt(store_queue)) < nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue))
                            {
                                nwy_ext_echo("\r\n if condition  nwy_get_queue_spaceevent_cnt \r\n");
                                while ((10 - nwy_get_queue_spaceevent_cnt(store_queue)))
                                {
                                    // nwy_ext_echo("\r\n while condition  nwy_get_queue_spaceevent_cnt \r\n");
                                    UART_data_struct store_data;
                                    UART_data_struct mqtt_data;
                                    if (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue) != 0)
                                    {
                                        if (nwy_get_msg_que(store_queue, &store_data, 0xffffffff) == true)
                                        {
                                            memcpy(&mqtt_data.payload, &store_data.payload, (store_data.length_u16));
                                            mqtt_data.stx = store_data.stx;
                                            mqtt_data.cmd = store_data.cmd;
                                            mqtt_data.length_u16 = store_data.length_u16;
                                            mqtt_data.crc_u16 = store_data.crc_u16;
                                            mqtt_data.timestamp = store_data.timestamp;
                                            mqtt_data.etx = store_data.etx;

                                            nwy_put_msg_que(mqtt_publish_msg_queue, &mqtt_data, 0xffffffff);
                                        }
                                    }
                                }
                                // mutex lock
                                store_msg_flag = false;
                                // mutex unlock
                            }
                            // read storage queue and put it in mqtt queue
                        }

                        nwy_sdk_fclose(file_read_thread_fd);
                        // nwy_ext_echo("\r\nfile call 19\r\n");
                        if (nwy_sdk_file_unlink(file_name_string) != NWY_SUCESS) // to delete the file once we emptied the file
                        {
                            nwy_ext_echo("\r\nFile unlink unsuccessfull\r\n");
                        }
                        total_files--;
                        can_files[f_count] = 0;
                        f_count++;
                        if (total_files == 0)
                        {
                            f_count = 0;
                            // store_msg_flag = false;
                        }
                        i = 0;
                        file_size--;
                        // mutex unlock
                    }
                    else
                    {
                        nwy_ext_echo("\r\nMQTT queue is full!!\r\n");
                        nwy_sleep(1);
                    }
                }
#endif
                nwy_semahpore_release(file_semaphore);
            }
        }
#endif
        nwy_sleep(1);
    }
#endif
#endif
}
