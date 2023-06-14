#if 1
#include "bytebeam.h"
#include "nwy_loc.h"
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

char *field[20];
int parse_comma_delimited_str(char *string, char **fields, int max_fields);

void gpsThreadEntry(void *param)
{
    nwy_ext_echo("\r\nGPS THREAD\r\n");
    gps_configure();
    while (1)
    {
        gps_recieve();
        nwy_sleep(2000);
    }
}

void gps_configure()
{
    int ret = -1;
    int result = -1;
    nwy_loc_close_uart_nmea_data(); // to disable data on UART2
    ret = nwy_loc_start_navigation();
    if (ret)
    {
        nwy_ext_echo("\r\n open location success");
    }
    else
    {
        nwy_ext_echo("\r\n open location fail");
    }

    result = nwy_loc_set_position_mode(1); // GPS MODE
    if (result)
    {
        nwy_ext_echo("\r\n set position mode success");
    }
    else
    {
        nwy_ext_echo("\r\n set position mode fail");
    }
    nwy_ext_echo("\r\n update rate: 1000\n");
    result = nwy_loc_nmea_format_mode(2, 1000); // frequency is set to 1Hz
    if (result)
    {
        nwy_ext_echo("\r\n set location update rate success");
    }
    else
    {
        nwy_ext_echo("\r\n set location update rate fail");
    }

    result = nwy_loc_nmea_format_mode(3, 1); // GPSGGA string is enabled
    if (result)
    {
        nwy_ext_echo("\r\n set nmea statements output format success");
    }
    else
    {
        nwy_ext_echo("\r\n set nmea statements output format fail");
    }
    result = nwy_loc_set_startup_mode(2); // HOT start is set
    nwy_ext_echo("GPS initialise Completed \r\n");
}

JSMN_JSON_GPS_MESSAGE_tst JSMN_JSON_GPS_MESSAGE_st_obj;
extern uint8_t json_gps_message_buff_gau8[];
void gps_recieve()
{
    int result = -1;
    char nmea_data[2048] = {0};
    double lat_data, long_data;
    struct timeval tv;
    int i = 0;
    result = nwy_loc_get_nmea_data(nmea_data);
    if (result)
    {
        /*
        $GPGGA,152655.000,1254.24568,N,07737.45196,E,1,09,1.0,917.7,M,-87.1,M,,*7B<CR><LF>
        $GPTXT,01,01,02,MO=G*35<CR><LF>
        Parsed 18 fields<CR><LF>
        Field 00: [$GPGGA]<CR><LF>
        Field 01: [152655.000]<CR><LF>
        Field 02: [1254.24568]<CR><LF>
        Field 03: [N]<CR><LF>
        Field 04: [07737.45196]<CR><LF>
        Field 05: [E]<CR><LF>
        Field 06: [1]<CR><LF>
        Field 07: [09]<CR><LF>
        Field 08: [1.0]<CR><LF>
        Field 09: [917.7]<CR><LF>
        Field 10: [M]<CR><LF>
        Field 11: [-87.1]<CR><LF>
        Field 12: [M]<CR><LF>
        Field 13: []<CR><LF>
        Field 14: [*7B<CR><LF>

        UTC Time  :152655.000<CR><LF>
        Latitude  :1254.24568<CR><LF>
        Longitude :07737.45196<CR><LF>
        Altitude  :917.7<CR><LF>
        Satellites:09<CR><LF>*/
      //  nwy_ext_echo("\r\n %s", nmea_data);
        if ((strncmp(nmea_data, "$GPGGA", 6) == 0))
        {
            i = parse_comma_delimited_str(nmea_data, field, 20);
            if (memcmp(field[6],"1",1) == 0)
            {
                lat_data = convertToDecimalDegrees(field[2], field[3]);
                long_data = convertToDecimalDegrees(field[4], field[5]);

                gettimeofday(&tv, NULL);

                double s = tv.tv_sec;
                double ms = ((double)tv.tv_usec) / 1.0e6;

                unsigned long long timestamp1 = (unsigned long long)(s + ms) * 1000;

                static uint32_t seq_id = 0;
                seq_id++;
                memset(json_gps_message_buff_gau8, 0, JSON_GPS_MESSAGES_BUFF_LEN);
                snprintf(json_gps_message_buff_gau8, sizeof(json_gps_message_buff_gau8), gps_mess_json,
                         timestamp1,
                         lat_data,
                         long_data,
                         seq_id);
                nwy_ext_echo("%s", json_gps_message_buff_gau8); // For the verification of the JSON frame

                UART_data_struct gps_publish_msg;
                gps_publish_msg.cmd = 0x05;
                gps_publish_msg.length_u16 = strlen((char *)json_gps_message_buff_gau8);

                memcpy(gps_publish_msg.payload, json_gps_message_buff_gau8, gps_publish_msg.length_u16);

                // if (get_MQTT_COnnection_Status() && (store_msg_flag == false) && (nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue)!= 0))
                if (get_MQTT_COnnection_Status() && (store_msg_flag == false)) 
                {
                    nwy_ext_echo("publishing GPS in MQTT\r\n");
                    if(nwy_get_queue_spaceevent_cnt(mqtt_publish_msg_queue) == 0) 
                    {
                        // nwy_ext_echo("\r\nMQTT Queue full agidhe\r\n");
                    }
                    else
                    {
                        nwy_ext_echo("\r\npublishing device shadow in MQTT\r\n");
                        nwy_put_msg_que(mqtt_publish_msg_queue, &gps_publish_msg, 0);
                    }
                }
                else
                {
                    
                    if (nwy_read_sdcart_status() == true) 
                    {
                        store_msg_flag = false;           //disabled until persistence is implemented
                        nwy_put_msg_que(store_queue, &gps_publish_msg, 0);
                    }
                    else 
                    {
                        nwy_ext_echo("GPS Thread: no SD Card");
                    }
                }
                // bytebeam_gps_publish();
            }
        }
        else
        {
            nwy_ext_echo("\r\n get nmea data fail");
        }
    }
    
}

double convertToDecimalDegrees(const char *latLon, const char *direction)
{
    char deg[4] = {0};
    char *dot, *min;
    int len;
    double dec = 0;

    if ((dot = strchr(latLon, '.')))
    {                                     // decimal point was found
        min = dot - 2;                    // mark the start of minutes 2 chars back
        len = min - latLon;               // find the length of degrees
        strncpy(deg, latLon, len);        // copy the degree string to allow conversion to float
        dec = atof(deg) + atof(min) / 60; // convert to float
        if (strcmp(direction, "S") == 0 || strcmp(direction, "W") == 0)
            dec *= -1;
    }
    return dec;
}

int parse_comma_delimited_str(char *string, char **fields, int max_fields)
{
    int i = 0;
    fields[i++] = string;

    while ((i < max_fields) && NULL != (string = strchr(string, ',')))
    {
        *string = '\0';
        fields[i++] = ++string;
    }

    return --i;
}

int debug_print_fields(int numfields, char **fields)
{
    nwy_ext_echo("Parsed %d fields\r\n", numfields);

    for (int i = 0; i <= numfields; i++)
    {
        nwy_ext_echo("Field %02d: [%s]\r\n", i, fields[i]);
    }
    return 0;
}

#endif