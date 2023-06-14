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
#include "nwy_usb_serial.h"
#include "nwy_adc.h"
#include "nwy_led.h"
#include "nwy_gpio_open.h"
#include "nwy_fota.h"
#include "nwy_fota_api.h"
#include "nwy_pm.h"
#include "stdio.h"
#include "nwy_network.h"
#include "nwy_file.h"
#include "nwy_socket.h"
#include "json_server.h"

// Variable Initialisation
char modip4addr[30] = {0};
static int ppp_state[10] = {0};
int dataCallFlag = 0;
bool http_flag = false;

uint8_t nw_csq_val = 0;
int network_thread_counter = 0;
extern char ota_action_id[10];
extern int http_download_flag;

typedef enum _network_queue_t
{
  SIM_CHECK = 0,
  NETWORK_CHECK,
  DATA_CALL,
  MQTT_CONNECT,
  FAULT_LONG_DELAY,
  NETWORK_SUCCESS,
} network_queue_t;
static network_queue_t network_state;

static uint8_t sim_check_retry_counter = 0;
static uint8_t network_check_retry_counter = 0;
static uint8_t data_call_retry_counter = 0;
static uint8_t mqtt_connect_retry_counter = 0;
static uint8_t fault_delay_counter = 0;
static uint8_t network_success = 0;
#define RETRY_COUNTER (5U)

bool network_disconnected = false;

void nwy_ext_gprs_disconnect_cb(char *data)
{
  network_disconnected = true;
  nwy_ext_echo("\r\nGPRS DISCONNECTION");
  nwy_ext_echo(data);
}

void nwy_ext_sms_list_resp_cb(char *data)
{
  nwy_ext_echo("\r\n+CMGL: ");
  nwy_ext_echo(data);
}

void network_timer_cb(void)
{
  // OSI_LOGI(0, "nwy_app_timer_cb");
  nwy_ext_echo("\r\n Newtork callback ");
  network_state = SIM_CHECK;

  nwy_resume_thread(network_app_thread);
}

void Network_Init(void)
{
  if (sim_check() == true)
  {

    network_check();
    nwy_sleep(1000);
    // nwy_ext_echo("Network Check Completed\r\n");
    if (nwy_ext_check_data_connect() != 1)
    {
      data_enable();
      // nwy_ext_echo("Data call Completed\r\n");
    }
    nwy_sleep(1000);
    // if (get_MQTT_COnnection_Status() != true)
    // {

    //   bytebeam_MQTT_connect();
    //   // nwy_ext_echo("MQTTComple/ted\r\n");
    // }
  }
}

// Thread Function
void Network_ThreadEntry(void *param)
{
  while(!((nwy_sdk_fexist("sdcard0/device_config.json")) || (nwy_sdk_fexist("nwy/device_config.json"))))
  {
    ;
  }
  nwy_sleep(2000);
  network_state = SIM_CHECK;
  while (1)
  {
    nwy_ext_echo("\r\nNetwork Thread : %d ", network_state);
    nwy_nw_get_signal_csq(&nw_csq_val);
    switch (network_state)
    {
    case SIM_CHECK:
    {

      if (NWY_SIM_STATUS_READY != nwy_sim_get_card_status())
      {

        if (sim_check() == false)
        {
          sim_check_retry_counter++;
          network_state = SIM_CHECK;
        }
        else // TODO: to be eliminated during optimization
        {
          sim_check_retry_counter = 0;
          network_state++;
        }
        if (sim_check_retry_counter >= RETRY_COUNTER)
        {
          sim_check_retry_counter = 0;
          network_state = FAULT_LONG_DELAY;
        }
      }
      else
      {
        /* code */
        sim_check_retry_counter = 0;
        network_state++;
      }
    }
    break;
    case NETWORK_CHECK:
    {
      // TODO : need to add a check if netowrk is already available
      if (network_disconnected == true)
      {
        network_state = NETWORK_CHECK;

        if (network_check() == false)
        {
          network_check_retry_counter++;
          network_state = NETWORK_CHECK;
        }
        else // TODO: to be eliminated during optimization
        {
          network_check_retry_counter = 0;
          network_state++;
        }
        if (network_check_retry_counter >= RETRY_COUNTER)
        {
          network_check_retry_counter = 0;
          network_state = FAULT_LONG_DELAY;
        }
      }
      else
      {
        /* code */
        network_check_retry_counter = 0;
        network_state++;
      }

      // nwy_ext_echo("Network Check Completed\r\n");
    }
    break;
    case DATA_CALL:
    {
      if (nwy_ext_check_data_connect() == false)
      {

        if (data_enable() == false)
        {
          data_call_retry_counter++;
          network_state = DATA_CALL;
        }
        else
        {
          data_call_retry_counter = 0;
          network_state++;
        }
        if (data_call_retry_counter >= RETRY_COUNTER)
        {
          data_call_retry_counter = 0;
          network_state = FAULT_LONG_DELAY;
        }
      }
      else
      {
        data_call_retry_counter = 0;
        network_state++;

      }
      // nwy_ext_echo("Data call Completed\r\n");
    }
    break;
    case MQTT_CONNECT:
    {
      network_state = MQTT_CONNECT;
      // nwy_ext_echo("\r\nMQTT Reconnecting");
      if (get_MQTT_COnnection_Status() != true)
      {

        if (bytebeam_MQTT_connect() == false)
        {
          mqtt_connect_retry_counter++;
          network_state = MQTT_CONNECT;
        }
        else
        {
          mqtt_connect_retry_counter = 0;
          network_state = NETWORK_SUCCESS;
        }
        if (mqtt_connect_retry_counter >= RETRY_COUNTER)
        {
          mqtt_connect_retry_counter = 0;
          network_state = FAULT_LONG_DELAY;
        }
        // nwy_ext_echo("MQTTCompleted\r\n");
      }
      else
      {
        mqtt_connect_retry_counter = 0;
        network_state = NETWORK_SUCCESS;
      }
    }
    break;
    case FAULT_LONG_DELAY:
    {
      nwy_ext_echo("In fault long delay \r\n");
      fault_delay_counter++;
      if (fault_delay_counter > (RETRY_COUNTER * 3))
      {
        fault_delay_counter = 0;
        network_state = SIM_CHECK;
        // restart
        nwy_power_off(2);
      }
    }
    break;
    case NETWORK_SUCCESS:
    {
      network_success++;
      if (network_success > (RETRY_COUNTER * 5))
      {
        network_success = 0;
        network_state = SIM_CHECK;
      }
    }
    break;
    }
    network_thread_counter++;
    if (network_thread_counter > 1200)
    {
      network_thread_counter = 0;
    }
    if ((network_thread_counter > 600) && (http_download_flag == 1))
    {
      nwy_sleep(5000);
      publish_action_status(ota_action_id, 0, "Failed", "Time Out");
      nwy_sleep(5000);
      publish_action_status(ota_action_id, 0, "Failed", "TimeOut");
      nwy_sleep(1000);
      nwy_power_off(2);
    }
    
    nwy_sleep(1000);
  }
}

// Register Callback for Various SIgnal Lose
void nwy_ext_init_unsol_reg()
{
  nwy_sdk_at_unsolicited_cb_reg("+EUSIM", nwy_pull_out_sim);
  /* added by wangchen for N58 sms api to test 20200215 begin */
  // nwy_sdk_at_unsolicited_cb_reg("+CMTI", sms_recv);
  // nwy_sdk_at_unsolicited_cb_reg("+CMT", sms_recv);
  nwy_sdk_at_unsolicited_cb_reg("GPRS DISCONNECTION", nwy_ext_gprs_disconnect_cb);
  nwy_sdk_at_unsolicited_cb_reg("+CMGL: ", nwy_ext_sms_list_resp_cb);
}

void nwy_data_cb_fun(int hndl, nwy_data_call_state_t ind_state)
{
  OSI_LOGI(0, "=DATA= hndl=%d,ind_state=%d", hndl, ind_state);
  if (hndl > 0 && hndl <= 8)
  {
    ppp_state[hndl - 1] = ind_state;
    nwy_ext_echo("\r\nData call status update, handle_id:%d,state:%d\r\n", hndl, ind_state);
    // bytebeam_MQTT_connect();
  }
}

bool sim_check(void)
{
  OSI_LOGI(0, "=UIM= nwy_sim_ready_handle coming in");
  nwy_sim_result_type sim = {"", "", "", "", NWY_SIM_AUTH_NULL};
  nwy_sim_status sim_status = NWY_SIM_STATUS_NOT_INSERT;
  nwy_sim_result_type imei = {0};
  nwy_set_sim_detect_trigger_mode(1);
  sim_status = nwy_sim_get_card_status();
  switch (sim_status)
  {
  case NWY_SIM_STATUS_READY:
  {
    memset(&sim, 0, sizeof(nwy_sim_result_type));
    nwy_sim_get_iccid(&sim);
    nwy_sim_get_imsi(&sim);
    nwy_sim_get_imei(&imei);
    nwy_ext_echo("\r\n iccid:%s, imsi:%s)", sim.iccid, sim.imsi);
    nwy_ext_echo("\r\n SIM card ready, Proactive verification");
    nwy_ext_echo("\r\n imei:%s \r\n", imei.nImei);
    // network_check();
    return 1;
  }
  break;
  case NWY_SIM_STATUS_NOT_INSERT:
    nwy_ext_echo("\r\n SIM card not insert, please insert and restart the sim\n");
    break;
  case NWY_SIM_STATUS_PIN1:
    nwy_ext_echo("\r\n SIM card lock pin");
    break;

  case NWY_SIM_STATUS_PUK1:
    nwy_ext_echo("\r\n SIM card lock puk");
    break;

  case NWY_SIM_STATUS_BUSY:
    nwy_ext_echo("\r\n SIM card busy");
    break;
  default:
    nwy_ext_echo("\r\n SIM Unknown state\r\n");
    break;
  }
  return 0;
}

bool data_enable()
{
  int ret = -1;
  nwy_data_profile_info_t profile_info;
  nwy_sleep(500);
  int len = 0;
  nwy_data_addr_t_info addr_info;
  while (1)
  {
    int hndl = nwy_data_get_srv_handle(nwy_data_cb_fun);
    OSI_LOGI(0, "=DATA=  hndl= %d", hndl);
    // nwy_ext_echo("\r\nCreate a Resource Handle id: %d success\r\n",hndl);

    memset(&profile_info, 0, sizeof(nwy_data_profile_info_t));
    int profile_id = 1;
    profile_info.auth_proto = 0; // NONE
    profile_info.pdp_type = 1;   // IPV4
    memcpy(profile_info.apn, "airtelgprs.com", strlen("airtelgprs.com"));
    // memcpy(profile_info.apn, "JIOCIOT", strlen("JIOCIOT"));
    // memcpy(profile_info.apn,"move.dataxs.mobi",strlen("move.dataxs.mobi"));
    memcpy(profile_info.user_name, "", 0);
    memcpy(profile_info.pwd, "", 0);

    ret = nwy_data_set_profile(profile_id, NWY_DATA_PROFILE_3GPP, &profile_info);
    OSI_LOGI(0, "=DATA=  nwy_data_set_profile ret= %d", ret);
    if (ret != NWY_RES_OK)
      nwy_ext_echo("\r\nSet profile %d info fail, result<%d>\r\n", profile_id, ret);
    else
      nwy_ext_echo("\r\nSet profile %d info success\r\n", profile_id);
    ret = nwy_data_get_profile(profile_id, NWY_DATA_PROFILE_3GPP, &profile_info);
    OSI_LOGI(0, "=DATA=  nwy_data_get_profile ret= %d", ret);
    OSI_LOGI(0, "=DATA=  profile= %d|%d", profile_info.pdp_type, profile_info.auth_proto);
    OSI_PRINTFI("=DATA=   profile= %s|%s|%s", profile_info.apn, profile_info.user_name, profile_info.pwd);
    if (ret != NWY_RES_OK)
    {
      nwy_ext_echo("\r\nRead profile %d info fail, result%d\r\n", profile_id, ret);
    }
    else
    {
      nwy_ext_echo("\r\nProfile %d info: <pdp_type>,<auth_proto>,<apn>,<user_name>,<password>\r\n%d,%d,%s,%s,%s\r\n", profile_id, profile_info.pdp_type,
                   profile_info.auth_proto, profile_info.apn, profile_info.user_name, profile_info.pwd);
    }

    nwy_data_start_call_v02_t param_t;
    memset(&param_t, 0, sizeof(nwy_data_start_call_v02_t));
    // hndl = 1;
    param_t.profile_idx = 1;
    param_t.is_auto_recon = 1;
    param_t.auto_re_max = 0;
    param_t.auto_interval_ms = 100;
    ret = nwy_data_start_call(hndl, &param_t);
    OSI_LOGI(0, "=DATA=  nwy_data_start_call ret= %d", ret);
    if (ret != NWY_RES_OK)
      nwy_ext_echo("\r\nStart data call fail, result<%d>\r\n", ret);
    else
      nwy_ext_echo("\r\ndata call activated...\r\n");
    // nwy_sleep(2000);  // TODO: sleep is commented for testing

    memset(&addr_info, 0, sizeof(nwy_data_addr_t_info));
    OSI_LOGI(0, "=DATA=  addr_info size= %d", sizeof(nwy_data_addr_t_info));
    ret = nwy_data_get_ip_addr(hndl, &addr_info, &len);
    OSI_LOGI(0, "=DATA=  nwy_data_get_ip_addr = %d|len%d", ret, len);
    nwy_ext_echo("=DATA=  nwy_data_get_ip_addr = %d|len%d \r\n", ret, len);
    if (ret == NWY_RES_OK)
    {

      nwy_ext_echo("\r\nIP address: %s\r\n", nwy_ip4addr_ntoa(&addr_info.iface_addr_s.ip_addr.addr));
    }
    else
    {
      nwy_ext_echo("\r\nGet data info fail, result<%d>\r\n", ret);
    }

    if (ret != NWY_RES_OK)
    {
      return false;
    }

    return true;
  }
}

int nwy_ext_check_data_connect()
{
  int i = 0;
  for (i = 0; i < NWY_DATA_CALL_MAX_NUM; i++)
  {
    if (ppp_state[i] == NWY_DATA_CALL_CONNECTED)
    {
      return 1;
    }
  }
  return 0;
}

bool network_check()
{
  nwy_nw_regs_info_type_t reg_info = {0};
  nwy_nw_operator_name_t opt_name = {0};
  uint8_t csq_val = 99;
  uint16_t nfreq[9] = {0};
  int lac = 0;
  int cid = 0;
  ;

  memset(&reg_info, 0x00, sizeof(reg_info));
  if (NWY_RES_OK == nwy_nw_get_register_info(&reg_info))
  {
    nwy_ext_echo("\r\n==============APP_1==========================\r\n");
    if (reg_info.data_regs_valid == 1)
    {
      nwy_ext_echo("Network Data Reg state: %d\r\n"
                   "Network Data Roam state: %d\r\n",
                   reg_info.data_regs.regs_state,
                   reg_info.data_regs.roam_state);
    }
    if (reg_info.voice_regs_valid == 1)
    {
      nwy_ext_echo("Network Voice Reg state: %d\r\n"
                   "Network Voice Roam state: %d\r\n",
                   reg_info.voice_regs.regs_state,
                   reg_info.voice_regs.roam_state);
    }

    if (reg_info.voice_regs.regs_state != NWY_NW_SERVICE_NONE)
    {
      memset(&opt_name, 0x00, sizeof(opt_name));
      if (NWY_SUCCESS == nwy_nw_get_operator_name(&opt_name))
      {
        nwy_ext_echo("Long EONS: %s\r\n"
                     "Short EONS: %s\r\n"
                     "MCC and MNC: %s %s\r\n"
                     "SPN: %s\r\n",
                     (char *)opt_name.long_eons,
                     (char *)opt_name.short_eons,
                     opt_name.mcc,
                     opt_name.mnc,
                     opt_name.spn);
      }
    }

    while ((csq_val <= 12) || (csq_val == 99))
    {
      nwy_nw_get_signal_csq(&csq_val);
      nwy_ext_echo("\r\nCSQ is %d \r\n", csq_val);
      nwy_sleep(1000);
    }
    // Get Lac and CELL ID
    nwy_sim_get_lacid(&lac, &cid);
    nwy_ext_echo("\r\nLAC: %X, CELL_ID: %X \r\n", lac, cid);
    nwy_ext_echo("\r\n========================================\r\n");
    // data_enable();
    
    network_disconnected = false;
    return true;
  }
  return false;
}
