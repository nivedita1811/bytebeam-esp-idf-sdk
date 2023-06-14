#include <time.h>
#include <math.h>
#include <sys/time.h>
#include "stdint.h"
#include "bytebeam.h"
#include "json_server.h"

uint8_t json_can_message_buff_gau8[JSON_CAN_MESSAGES_BUFF_LEN] = {
    0,
};
uint8_t json_gps_message_buff_gau8[JSON_GPS_MESSAGES_BUFF_LEN] = {
    0,
};
uint8_t json_imu_message_buff_gau8[JSON_IMU_MESSAGES_BUFF_LEN] = {
    0,
};
uint8_t json_sos_message_buff_gau8[JSON_SOS_MESSAGES_BUFF_LEN] = {
    0,
};
uint8_t json_ignition_message_buff_gau8[JSON_IGN_MESSAGES_BUFF_LEN] = {
    0,
};
uint8_t json_dev_shadow_msg_buff_gau8[DEV_SHADOW_MSG_BUFF_LEN] = {
    0,
};


uint8_t json_pack_can_message(JSMN_JSON_CAN_MESSAGE_tst *can_mess_pst, uint64_t timestamp);
uint8_t json_pack_gps_message(JSMN_JSON_GPS_MESSAGE_tst *gps_mess_pst);
uint8_t json_pack_imu_message(JSMN_JSON_IMU_MESSAGE_tst *imu_mess_pst);

// uint64_t timestamp = 160815325;

extern char can_mess_json[];
extern char gps_mess_json[];
extern char imu_mess_json[];
char can_mess_json[] = "{"
                       "\"timestamp\": %llu,"
                       "\"can_id\": %d,"
                       "\"byte1\": %d,"
                       "\"byte2\": %d,"
                       "\"byte3\": %d,"
                       "\"byte4\": %d,"
                       "\"byte5\": %d,"
                       "\"byte6\": %d,"
                       "\"byte7\": %d,"
                       "\"byte8\": %d,"
                      // "\"can_seq\": %d,"
                       "\"sequence\":%d"
                       "}";

char gps_mess_json[] = "[{"
                       "\"timestamp\": %llu,"
                       "\"latitude\": %f,"
                       "\"longitude\": %f,"
                       "\"sequence\":%d"
                       "}]\n";

char imu_mess_json[] = "[{"
                       "\"stream\": \"imu\","
                       "\"timestamp\": %ld,"
                       "\"accel_x\": %u,"
                       "\"accel_y\": %u,"
                       "\"accel_z\": %u,"
                       "\"gyro_x\": %u,"
                       "\"gyro_y\": %u,"
                       "\"gyro_z\": %u,"
                       "\"temperature\": %u"
                       "}]\n";

char sos_mess_json[] = "[{"
                       "\"timestamp\": %llu,"
                       "\"sos_status\": %d,"
                       "\"sequence\":%d"
                       "}]\n";

char ignition_mess_json[] = "[{"
                            "\"timestamp\": %llu,"
                            "\"ignition_status\": %d,"
                            "\"sequence\":%d"
                            "}]\n";

char dev_shadow_mess_json[] = "[{"
                              "\"timestamp\": %llu,"
                              "\"Status\": %s,"
                              "\"ble_status\": %d,"
                              "\"carrier_signal_quality\": %d,"
                              "\"ignition_status\": %d,"
                              "\"imu_comm_status\": %d,"
                              "\"BLE_MAC_ID\": %s,"
                              "\"nw_processor_base_fw_ver\": %s,"
                              "\"nw_processor_fw_ver\": %s,"
                              "\"controller_fw_ver\": %s,"
                              "\"dev_battery_voltage\": %f,"
                              "\"input_voltage\": %f,"
                              "\"vcu_fw_ver\": %s,"
                              "\"vcu_hw_ver\": %s,"
                              "\"sequence\":%d"
                              "}]\n";

volatile uint8_t old_byte1 = 0;
uint8_t json_pack_can_message(JSMN_JSON_CAN_MESSAGE_tst *can_mess_pst, uint64_t timestamp)
{

   

    struct timeval tv;
    // gettimeofday(&tv, NULL);

    // double s = tv.tv_sec;
    // double ms = ((double)tv.tv_usec) / 1.0e3;
    // unsigned long long timestamp1 = (unsigned long long)(s*1000 + ms);
    unsigned long long timestamp1 = timestamp;
    // static uint32_t seq_id = 0;
    // seq_id++;
    memset(json_can_message_buff_gau8, 0, JSON_CAN_MESSAGES_BUFF_LEN);


    //nwy_ext_echo("\r\n Recieved %d %d %d", can_mess_pst->can_id, can_mess_pst->byte1, old_byte1);
    
    // if(can_mess_pst->can_id == 0x123) {
    //     nwy_ext_echo("\r\n Recieved 0x123 %d %d %d", can_mess_pst->byte1, old_byte1, can_mess_pst->byte2);
    //     if(can_mess_pst->byte1 - old_byte1 != 1) {
    //         nwy_ext_echo("\r\nDiff != 1 %d\r\n", can_mess_pst->byte1 - old_byte1);
    //     } 

    //     old_byte1 = can_mess_pst->byte1;
    // }

    snprintf((char *)&json_can_message_buff_gau8, sizeof(json_can_message_buff_gau8), can_mess_json,
            timestamp1,
            can_mess_pst->can_id,
            can_mess_pst->byte1,
            can_mess_pst->byte2,
            can_mess_pst->byte3,
            can_mess_pst->byte4,
            can_mess_pst->byte5,
            can_mess_pst->byte6,
            can_mess_pst->byte7,
            can_mess_pst->byte8,
            //can_mess_pst->seq,
            can_mess_pst->sequence_id);
    return 0;
}

uint8_t json_pack_gps_message(JSMN_JSON_GPS_MESSAGE_tst *gps_mess_pst)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    double s = tv.tv_sec;
    double ms = ((double)tv.tv_usec) / 1.0e3;

    unsigned long long timestamp1 = (unsigned long long)(s*1000 + ms);

    static uint32_t seq_id = 0;
    seq_id++;
    memset(json_gps_message_buff_gau8, 0, JSON_GPS_MESSAGES_BUFF_LEN);
    snprintf((char *)&json_gps_message_buff_gau8, sizeof(json_gps_message_buff_gau8), gps_mess_json,
             timestamp1,
             gps_mess_pst-> gps_lat,
             gps_mess_pst-> gps_lng,
             seq_id);
    return 0;
}

#if 0
uint8_t json_pack_imu_message(JSMN_JSON_IMU_MESSAGE_tst* imu_mess_pst)
{
    

    memset(json_imu_message_buff_gau8, 0, JSON_IMU_MESSAGES_BUFF_LEN);
    snprintf(json_imu_message_buff_gau8, sizeof(json_imu_message_buff_gau8),imu_mess_json,
        timestamp,
        imu_mess_pst->accel_x,
        imu_mess_pst->accel_y,
        imu_mess_pst->accel_z,
        imu_mess_pst->gyro_x,
        imu_mess_pst->gyro_y,
        imu_mess_pst->gyro_z,
        imu_mess_pst->temperature
        );

    int json_return_val;
    jsmn_parser json_parser_init;
    jsmntok_t token[1024];
    jsmn_init(&json_parser_init);
    json_return_val = jsmn_parse(&json_parser_init, json_imu_message_buff_gau8,strlen(json_imu_message_buff_gau8), token, sizeof(token) / sizeof(token[0]));
    if(json_return_val < 0)
    {
        return (1);
    }

    return 0;
}
#endif

