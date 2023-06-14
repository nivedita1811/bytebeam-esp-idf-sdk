#include "bytebeam.h"
#include "common.h"
#include "bytebeam_uart.h"
#include "device_config.h"
#include "nwy_spi.h"
#include "bytebeam_sdk.h"

int STM_UART_fd = 0;
bool store_msg_flag = false;
uint8_t ble_status = 0;
uint8_t imu_status = 0;

extern int config_data_present_in_nwy;

nwy_osiEvent_t event;
const char APP_VERSION[65] = "Byte_Beam_APP_V2.0.0";
const char n58_app_fw_ver[15] = "\"1.0.13\"";
const char n58_base_fw_ver[8] = "\"1\"";
char ble_mac_id_g[25] = {0,};
char ble_data[20] = {0,};

/*Add Macro for Network Test*/
#define FEATURE_NETWORK_TEST
extern int vsnprintf(char *, size_t, const char *, va_list);

nwy_osiTimer_t *network_timer = NULL;
nwy_osiThread_t *network_app_thread = NULL;

nwy_osiThread_t *mqtt_publish_thread = NULL;
nwy_osiThread_t *gps_app_thread = NULL;

nwy_osiThread_t *store_data_thread = NULL;
nwy_osiThread_t *read_data_thread = NULL;

nwy_osiThread_t *uart_app_thread = NULL;

nwy_osiThread_t *tork_update_thread = NULL;
nwy_osiThread_t *s32_update_thread = NULL;
nwy_osiThread_t *ble_app_thread_handle = NULL;

nwy_osiThread_t *device_config_thread_handle = NULL;

nwy_osiTimer_t *uart_app_timer = NULL;


// Queue Handler
nwy_osiMessageQueue_t *mqtt_publish_msg_queue = NULL;
nwy_osiMessageQueue_t *uart_tx_msg_queue = NULL;
nwy_osiMessageQueue_t *store_queue = NULL;
// Semaphore Handler
nwy_osiSemaphore_t *file_semaphore = NULL;
nwy_osiSemaphore_t *mqtt_connect_semaphore = NULL;
nwy_osiSemaphore_t *uart_tx_semaphore = NULL;
nwy_osiSemaphore_t *tork_update_semaphore = NULL;
nwy_osiSemaphore_t *s32_update_semaphore = NULL;

void read_and_send_data_mqtt(void);
uint8_t data_backup_file_count();
extern uint8_t ota_url[];
int ble_state = 0;
extern char device_id_global[10];
char dev_id_hex[10] = {0,};
int dev_id_int = 0;

void nwy_ble_recv_data_func()
{
    nwy_ext_echo("Received Some BLE Data\r\n");
    UINT8 *length = NULL;
    UINT8 *precv = NULL;
    length = nwy_ble_receive_data(0);
    precv = nwy_ble_receive_data(1);
    if ((NULL != precv) & (NULL != length))
    {
        //char ble_send_data[] = {"received data ack"};
        // nwy_ble_send_data(strlen(ble_send_data), ble_send_data);
        //recv_data_handle(precv,length);
        strncpy(ble_data, precv, length);  
    }
    else
    {
        nwy_ext_echo("nwy_ble receive data is null.");
    }
    nwy_ble_receive_data(2);

    return;
}

void nwy_ble_connection_status_cb()
{
    int conn_status = 0;
    conn_status= nwy_ble_get_conn_status();
    if(conn_status != 0)
    {
        ble_status = 1;
        nwy_ext_echo("\r\n Ble connect,status:%d",conn_status);
        if(ble_state == 0)
        {
            nwy_ble_update_conn(0,60,80,4,500);
            ble_state = 1;
        }   
    }
    else
    {
        ble_status = 0;
        nwy_ext_echo("\r\n Ble disconnect,status:%d",conn_status);
    }

    return;
}

void get_ble_mac_id()
{
    uint8_t *ble_mac = NULL;
    
    ble_mac = nwy_ble_get_mac_addr();
    if (ble_mac != NULL) 
    {
        // Copy the MAC address to the ble_mac_id_g buffer
        sprintf(ble_mac_id_g, "\"%02x:%02x:%02x:%02x:%02x:%02x\"", ble_mac[5], ble_mac[4], ble_mac[3], ble_mac[2], ble_mac[1], ble_mac[0]);
        // Add a null terminator to the end of the string
        ble_mac_id_g[sizeof(ble_mac_id_g) - 1] = '\0';
        nwy_ext_echo("\r\n Ble mac addr:%s\r\n", ble_mac_id_g);
    }
    else 
    {
        nwy_ext_echo("\r\n ble get mac addr is null.");
    }
}

void ble_set_service_and_characteristic()
{
    nwy_ble_service_info my_service = 
    {
        .ser_index = 0,
        .ser_uuid = {0x10, 0xFE},
        .char_num = 2,
        .ser_ble_char = 
        {
            {
                .char_uuid = {0x11, 0xFE},
                .permisssion = 0x02,
                .char_cp = 0x01
            },
            {
                .char_uuid = {0x12, 0xFE},
                .permisssion = 0x02,
                .char_cp = 0x01
            }
        }
    };

    // Add the service 
    nwy_ble_add_service(&my_service);
}


void bytebeam_ble()
{
    int port;
    int valu;
    int lab;
    int enable;
    int ble;
    int ble_len;
    int conn_status;
    char *dev_ver = NULL;
    char dev_name[20] = {0};//"BYTEBEAM_BLE";
    
    dev_id_int = atoi(device_id_global);
    sprintf(dev_id_hex, "%x", dev_id_int);
    
    if(strlen(dev_id_hex) == 1)
    {
        sprintf(dev_id_hex, "0000%x", dev_id_int);
    }
    if(strlen(dev_id_hex) == 2)
    {
        sprintf(dev_id_hex, "000%x", dev_id_int);
    }
    if(strlen(dev_id_hex) == 3)
    {
        sprintf(dev_id_hex, "00%x", dev_id_int);
    }
    if(strlen(dev_id_hex) == 4)
    {
        sprintf(dev_id_hex, "0%x", dev_id_int);
    }

    sprintf(dev_name, "%s%s" ,"TRQ_BLE_",dev_id_hex);

    nwy_ext_echo("BLE name is %s\r\n",dev_name);
    UINT8 uuid[16] = {0xb9, 0x40, 0x7f, 0x30, 0xf5, 0xf8, 0x46, 0x6e, 0xaf, 0xf9, 0x25, 0x55, 0x6b, 0x57, 0xfe, 0x6d};
    UINT8 major[2] = {0x00, 0x01};
    UINT8 minor[2] = {0x00, 0x02};
    UINT8 srv_uuid[2] = {0xfe, 0x10};
    UINT8 char_uuid_1[2] = {0xfe, 0x11};
    UINT8 char_uuid_2[2] = {0xfe, 0x12};
    UINT8 Manufacture_Data[8] = {0x13, 0xff, 0};
    UINT8 char_prop;

    nwy_ext_echo("start\r\n");

    nwy_ble_set_device_name(dev_name); 
    
    // nwy_ble_set_device_name(dev_name);
    //nwy_ble_set_service(srv_uuid);
    ble_set_service_and_characteristic();

    enable = nwy_ble_enable();
    nwy_ext_echo("nwy_ble_enable(%d) \r\n", enable);

    while(enable == 0)
    {   
        enable = nwy_ble_enable();
        nwy_ext_echo("nwy_ble_enable(%d) \r\n", enable);
        nwy_sleep(100);
    }
    
    nwy_sleep(100);
    
    nwy_ble_conn_status_cb(nwy_ble_connection_status_cb);
    nwy_ble_set_adv(1);
    nwy_ble_start_service(0xFE10);
    
    nwy_ble_register_callback(nwy_ble_recv_data_func);
}

void ble_app()
{
    nwy_sleep(10000);
    bytebeam_ble();
    get_ble_mac_id();
    while(1)
    {
        if (nwy_ext_check_data_connect() == true)
        {
            if (http_flag == true)
            {             
                http_test(ota_url);
            }
        }
        nwy_wait_thead_event(ble_app_thread_handle, &event, 0); //Wait for Bluetooth to receive
        nwy_sleep(4000);
    }
}

void test_imu()
{
    int spi_hd =  nwy_spi_init(NAME_SPI_BUS_1, SPI_MODE_0, 1000000, 8);
    static int test_spi = 0;
    static int l_var = 0;
    while(test_spi == 0)
    {
        if(l_var < 2)
        {
            uint8_t dummy[2] = {0};
            uint8_t recv = 0;
            uint8_t reg[2] = {0x75, 0x00};
            uint8_t reg1[2] = {0x3B, 0x00};

            reg[0] = reg[0] | 0x80;
            nwy_spi_transfer(spi_hd, SPI_CS_0, (uint8_t*) &reg[0], (uint8_t*) &dummy[0], 2);

            if(dummy[1] == 0x98)
            {
                imu_status = 1;
            }
            else
            {
                imu_status = 0;
            }
            nwy_ext_echo("Value of who am I reg is %X\r\n", dummy[1]);

            nwy_sleep(500);
            l_var++;
        }
        else
        {
            test_spi = 1;
        }
    }
}

int appimg_enter(void *param)
{
    // Basic Initialisation
    // warning:app build time length <= 64
    const char APP_BUILD_TIME[65] = {0};
    char version[70] = {0};
    
    // nwy_sleep(3000); // delayed to see response on USB

    sprintf(version, "\"%s\"", APP_VERSION);
    sprintf(APP_BUILD_TIME, "\"%s,%s\"", __DATE__, __TIME__);
    if (false == nwy_app_version(version, APP_BUILD_TIME))
    {
        OSI_LOGI(0, "app set version fail");
        return 0;
    }

    //nwy_usb_serial_reg_recv_cb(nwy_ext_sio_data_cb);
    nwy_sdk_at_parameter_init();

    // nwy_sleep(8000); // delayed to see response on USB
    nwy_ext_echo("\r\nByte Beam N58 V1.0\r\n");
    // nwy_ext_echo("\r\n============================UPDATED APP=====================\r\n");
    nwy_boot_causes_test();
    // nwy_powerkey_poweroff_ctrl(1);
    // nwy_dtr_set_state(2);   
    mqtt_publish_msg_queue = nwy_create_msg_Que(6, sizeof(UART_data_struct));
    if(mqtt_publish_msg_queue == NULL)
    {
        nwy_ext_echo("\r\nUnable to create mqtt queue\r\n");
    }
    
    nwy_ext_echo(" Starting Application\r\n ");
    // uart_config();
    device_config_thread_handle = nwy_create_thread("device_config_thread", DeviceConfig_ThreadEntry, NULL,
                                                    NWY_OSI_PRIORITY_NORMAL, 1024*18, 16);

    mqtt_topics_init();
    test_imu();
    
    // uart_config();
    nwy_uart_printf("UART SUCCESS\r\n");

    uart_tx_semaphore = nwy_semaphore_create(1,1);
    tork_update_semaphore = nwy_semaphore_create(1, 0);
    s32_update_semaphore = nwy_semaphore_create(1, 0);

    uart_app_thread = nwy_create_thread("uart_thread", UART_rx_ThreadEntry, NULL, NWY_OSI_PRIORITY_NORMAL, 1024 * 40, 16);

    //thread for updating Tork ECU
    tork_update_thread = nwy_create_thread("uart_thread", tork_app_update, NULL, NWY_OSI_PRIORITY_NORMAL, 1024 * 15, 16);
    s32_update_thread = nwy_create_thread("s32_app_thread", s32_update_app, NULL, NWY_OSI_PRIORITY_NORMAL, 1024 * 15, 16);

    Network_Init();
    network_app_thread = nwy_create_thread("mythread", Network_ThreadEntry, NULL, NWY_OSI_PRIORITY_NORMAL, 1024 * 20, 16);

    mqtt_publish_thread = nwy_create_thread("MQTT Publish Thread", MQTT_publish_ThreadEntry, NULL, NWY_OSI_PRIORITY_NORMAL, 1024 * 100, 16);

    gps_app_thread = nwy_create_thread("GPS Thread", gpsThreadEntry, NULL, NWY_OSI_PRIORITY_NORMAL, 1024 * 25, 16);
    ble_app_thread_handle = nwy_create_thread("ble_thread", ble_app, NULL, NWY_OSI_PRIORITY_NORMAL, 1024 * 20, 16);
    // nwy_sdk_sdcard_unmount();
    // nwy_sleep(100);
    // if (nwy_sdk_sdcard_mount() == true)
    // {
    //     nwy_ext_echo("\r\nsdcard mount successfull\n");
    //     file_semaphore = nwy_semaphore_create(1, 1);
    //     // sd_card_mounted_flag = 1;
    // }
    // else
    // {
    //     nwy_ext_echo("\r\nsdcard mount fail\n");
    // }
    
    // if (nwy_read_sdcart_status() == true)
    // {
    //     nwy_sdk_vfs_mkdir("/sdcard0/persistent/");
    // }
    // else
    // {
    //     nwy_ext_echo("unable to create the directory\r\n");
    // }

    
    
    // if (nwy_read_sdcart_status() == true)
    // {
    //     store_queue = nwy_create_msg_Que(3, sizeof(UART_data_struct));
    //     if(store_queue == NULL)
    //     {
    //         nwy_ext_echo("\r\nUnable to create storage queue\r\n");
    //     }
    //     store_data_thread = nwy_create_thread("store date", store_data_ThreadEntry, NULL, NWY_OSI_PRIORITY_NORMAL, 1024 * 20, 16);
    //     if (store_data_thread == NULL)
    //     {
    //         nwy_ext_echo("Failed to create Storage task!!\r\n");
    //     }
    //     read_data_thread = nwy_create_thread("read data", read_data_ThreadEntry, NULL, NWY_OSI_PRIORITY_NORMAL, 1024 * 20, 16);
    //     if (read_data_thread == NULL)
    //     {
    //         nwy_ext_echo("Failed to create Storage task!!\r\n");
    //     }
    // }
    
    nwy_ext_echo("Thread created successfully\r\n");

    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
    nwy_ext_echo("Application exit");
}


