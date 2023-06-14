#ifndef __NWY_LOC_H__
#define __NWY_LOC_H__

#include "nwy_wifi.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum
{
  NWY_LOC_POS_MODE_STANDALONE = 0,
  NWY_LOC_POS_MODE_MS_BASED = 1,
  NWY_LOC_POS_MODE_MS_ASSISTED = 2,
  NWY_LOC_POS_MODE_MS_AUTO = 3,
  NWY_LOC_POS_MODE_MS_WWAN = 4,
  NWY_LOC_POS_MODE_MS_CELL_ID = 5,
}nwy_loc_position_mode_type_t;

typedef enum
{
	NWY_LOC_AUX_GNSS_GPS = 1,
    NWY_LOC_AUX_GNSS_BD = 2,
    NWY_LOC_AUX_GNSS_GPS_BD = 3,
    NWY_LOC_AUX_GNSS_GLONASS = 4,
    NWY_LOC_AUX_GNSS_GPS_GLONASS = 5,
    NWY_LOC_AUX_GNSS_BD_GLONASS = 6,
    NWY_LOC_AUX_GNSS_ALL = 7,

    NWY_LOC_AUX_GNSS_NONE = 100,
} nwy_loc_position_mode_t;

typedef enum
{
	NWY_LOC_HOT_START = 0,
    NWY_LOC_WORM_START = 1,
    NWY_LOC_COLD_START = 2,
    NWY_LOC_FACTORY_START = 3,
} nwy_loc_startup_mode;

typedef struct {
  nwy_loc_position_mode_type_t mode;
  nwy_loc_position_mode_t gnss;
}nwy_loc_position_mode_info_type_t;

typedef enum
{
    GNSS_OUTPUT_GGA  = 0,
    GNSS_OUTPUT_GSA  = 1,
    GNSS_OUTPUT_GSV  = 2,
    GNSS_OUTPUT_RMC  = 3,
    GNSS_OUTPUT_VTG  = 4,
    GNSS_OUTPUT_GLL  = 5,
    GNSS_OUTPUT_ALL  = 6,
}nwy_loc_output_type;

typedef struct
{
    double lat;
    double lng;
    double alt;
}nwy_cipgsmloc_info_t;

typedef struct{
    char result;    //0 == nwy_cipgsmloc_info_t 1 == errmsg
    union
    {
        nwy_cipgsmloc_info_t data;
        char errmsg[48];
    }info;
}nwy_log_cipgsmloc_result_t;


typedef struct
{
  unsigned int          mcc;
  unsigned int          mnc;
  unsigned int          lac;
  unsigned int          cell_id;
  char      imei_str[20];
  int       rssi_csq ;

} nwy_lbs_info_t;

typedef struct
{
  unsigned int          mcc;
  unsigned int          mnc;
  unsigned int          lac;
  unsigned int          cell_id;
  unsigned int   pci;
  unsigned int   arcfn;
  unsigned int    bsic;
  int   rsrp;
  int   rsrq;
  int   rssi;
} nwy_ncell_lbs_info;

typedef struct
{
  int  num;
  nwy_ncell_lbs_info  ncell_lbs_info[6];
} nwy_ncell_lbs_info_t;

typedef struct
{
    uint8_t lteReqType;
    uint8_t replyErrCode;
} nwy_RspContext_t;
struct nwyLbsCmd_t;
typedef void (*nwyLbsAsyncDestroy_t)(struct nwyLbsCmd_t *cmd);
typedef struct nwyLbsCmd_t{
    int  option;
    int  data_len;
    void   *data;
    int     uti;
    void    *async_ctx;
    nwyLbsAsyncDestroy_t async_ctx_destroy;
}nwy_lbs_cmd_s;

typedef enum nwy_pdp_status_type
{
  NWY_PDP_DISCONNECTED,
  NWY_PDP_CONNECTED,
  NWY_PDP_CONNECTTING,
  NWY_PDP_DISCONNECTTING,
}nwy_pdp_status_type;

extern void nwy_log( unsigned int flag, char* fmt, ... );

typedef void (*nwy_loc_cipgsmloc_callback)(nwy_log_cipgsmloc_result_t *text);

int nwy_loc_start_navigation();
int nwy_loc_stop_navigation();
int nwy_loc_set_position_mode(nwy_loc_position_mode_t pos_mode);
int nwy_loc_set_startup_mode(nwy_loc_startup_mode startup);
int nwy_loc_nmea_format_mode(uint16 cmd,int16 sel_parameter);
int nwy_loc_set_server(char *str_url,int port,char *user,char *password);
bool nwy_loc_cipgsmloc_open(bool value, nwy_loc_cipgsmloc_callback cb);
bool nwy_loc_agps_open(bool value);
int  nwy_loc_get_nmea_data(char *data);
int nwy_loc_get_lbs_info(nwy_lbs_info_t* databuf);
void nwy_loc_close_uart_nmea_data();
int nwy_loc_get_ncell_lbs_info(nwy_ncell_lbs_info_t* data_info);
bool nwy_loc_external_wifigsmloc_open(bool value, nwy_wifi_scan_list_t *wifi_info, nwy_loc_cipgsmloc_callback cb);
int nwy_loc_send_agps_data(char *tmp_buf,int len);
int nwy_loc_get_specific_data(nwy_loc_output_type type,char *nmea_data);

#ifdef __cplusplus
   }
#endif

#endif
