#ifndef __JSON_SERVER_H
#define __JSON_SERVER_H

#define JSON_CAN_MESSAGES_BUFF_LEN 1024                //*100
#define JSON_GPS_MESSAGES_BUFF_LEN 1024
#define JSON_IMU_MESSAGES_BUFF_LEN 1024
#define JSON_SOS_MESSAGES_BUFF_LEN 256
#define JSON_IGN_MESSAGES_BUFF_LEN 256
#define DEV_SHADOW_MSG_BUFF_LEN 512

extern char can_mess_json[];
extern char gps_mess_json[];
extern char imu_mess_json[];
extern char sos_mess_json[];
extern char ignition_mess_json[];
extern char dev_shadow_mess_json[];

typedef struct __attribute__((packed)) JSMN_JSON_CAN_MESSAGE_tst_tag
{
    //uint8_t can_type;
    uint8_t can_dlc;
    uint32_t can_id;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
    uint8_t byte4;
    uint8_t byte5;
    uint8_t byte6;
    uint8_t byte7;
    uint8_t byte8;
    uint16_t sequence_id;
} JSMN_JSON_CAN_MESSAGE_tst;

extern JSMN_JSON_CAN_MESSAGE_tst JSMN_JSON_CAN_MESSAGE_st_obj;

typedef struct __attribute__((packed)) JSMN_JSON_GPS_MESSAGE_tst_tag
{
    double gps_lat;
    double gps_lng;
    
} JSMN_JSON_GPS_MESSAGE_tst;

extern JSMN_JSON_GPS_MESSAGE_tst JSMN_JSON_GPS_MESSAGE_st_obj;

typedef struct __attribute__((packed)) JSMN_JSON_IMU_MESSAGE_tst_tag
{
    uint16_t accel_x;
    uint16_t accel_y;
    uint16_t accel_z;
    uint16_t gyro_x;
    uint16_t gyro_y;
    uint16_t gyro_z;
    uint16_t temperature;
} JSMN_JSON_IMU_MESSAGE_tst;

extern JSMN_JSON_IMU_MESSAGE_tst JSMN_JSON_IMU_MESSAGE_st_obj;

extern uint8_t json_can_message_buff_gau8[JSON_CAN_MESSAGES_BUFF_LEN];
extern uint8_t json_gps_message_buff_gau8[JSON_GPS_MESSAGES_BUFF_LEN];
extern uint8_t json_imu_message_buff_gau8[JSON_IMU_MESSAGES_BUFF_LEN];
extern uint8_t json_sos_message_buff_gau8[JSON_SOS_MESSAGES_BUFF_LEN];
extern uint8_t json_ignition_message_buff_gau8[JSON_IGN_MESSAGES_BUFF_LEN];
extern uint8_t json_dev_shadow_msg_buff_gau8[DEV_SHADOW_MSG_BUFF_LEN];

extern uint8_t json_pack_can_message(JSMN_JSON_CAN_MESSAGE_tst *can_mess_pst, uint64_t timestamp);
extern uint8_t json_pack_gps_message(JSMN_JSON_GPS_MESSAGE_tst *gps_mess_pst);
extern uint8_t json_pack_imu_message(JSMN_JSON_IMU_MESSAGE_tst *imu_mess_pst);

#endif
