/*
 * yutu_fw_defines.h
 *
 * Created: 2/19/2019 4:18:24 PM
 *  Author: Mohan
 */

#ifndef YUTU_FW_DEFINES_H_
#define YUTU_FW_DEFINES_H_
// to be moved to a header file.
#define APN_ID "airtelgprs.com"
#define HOST_NAME "Yukee_123456"
typedef enum
{
  STATE_UNKNOWN,
  ECHO_OFF_SENT,
  ECHO_OFF_DONE,
  ECHO_OFF_ERR,
  MODEM_RDY_SENT,
  MODEM_RDY_DONE,
  MODEM_RDY_ERR,
  BT_SET_HOST_SENT,
  BT_SET_HOST_DONE,
  BT_SET_HOST_ERR,
  BT_POWER_OFF_SENT,
  BT_POWER_OFF_DONE,
  BT_POWER_OFF_ERR,
  BT_POWER_ON_SENT,
  BT_POWER_ON_DONE,
  BT_POWER_ON_ERR,
  BT_STATUS_SENT,
  BT_STATUS_DONE,
  BT_STATUS_ERR,
  BT_HFP_DISCONN_SENT,
  BT_SPPDATA_SENT,
  BT_SPPDATA_DONE,
  BT_SPPDATA_ERR,
  BT_CMD_PROCESSED,
  BT_URC_START,
  BT_URC_END,
  BT_URC_ERR,
  GSM_SIM_PRESENT_SENT,
  GSM_SIM_PRESENT,
  GSM_SIM_ABSENT,
  GSM_SIM_ERR,
  GSM_ROAMING_ENABLED_SENT,
  GSM_ROAMING_READY,
  GPRS_SET_SINGLE_CONN_SENT,
  GPRS_SET_SINGLE_CONN_DONE,
  GPRS_SET_SINGLE_CONN_ERR,
  GPRS_SET_APN_SENT,
  GPRS_APN_SET,
  GPRS_APN_ERR,
  GPRS_SET_WIRELESS_SENT,
  GPRS_WIRELESS_SET,
  GPRS_WIRELESS_ERR,
  GPRS_GET_LOCALIP_SENT,
  GPRS_LOCALIP_SET,
  GPRS_LOCALIP_ERR,
  GPRS_SET_RECV_FROM_SENT,
  GPRS_RECV_FROM_SET,
  GPRS_RECV_FROM_ERR,
  GPRS_SEND_MSG_SENT,
  GPRS_GOT_PROMPT,
  GPRS_GOT_PROMPT_ERR,
  GPRS_WAIT_FOR_SEND_OK,
  GPRS_DATA_SENT,
  GPRS_DATA_SEND_ERR,
  GPRS_GOT_RESPONSE,
  GPRS_NO_RESPONSE,
  GPRS_SET_SERVER_CONN_SENT,
  GPRS_SERVER_READY,
  GPRS_SERVER_ERR,
  GPRS_MESSAGE_RECEIVED,
  GPRS_CIPSHUT_SENT,
  GPRS_CIPSHUT_DONE,
  GPRS_CIPSHUT_ERR,
  GPRS_PING_SENT

} state_info;

bool sendSuccess;
// extern DmacDescriptor usartTXDescriptor, wbDmacMem;
// extern bool dataSendCompleteFlag;
// char recvBuffer[BUFF_SIZE];

unlck_auth_numbers auth_numbers;

void task1_thread(void) __attribute__((noreturn));
void task2_thread(void) __attribute__((noreturn));

result yutu_fw_erase_secure_flash(int location);
result yutu_fw_write_secure_flash(uint8_t* data, int location);
result yutu_fw_read_secure_flash(uint8_t* data, int location);
uint8_t* yutu_fw_get_secure_data(uint16_t loc);
void yutu_fw_register_alarms(callback arg);
// void yutu_fw_init(void);
void yutu_fw_gps_data_rdy_handler(callback arg);
void yutu_fw_reg_button_handler(callback arg);

char* yutu_fw_get_asset_name(void);
char* yutu_fw_get_latitude(void);
char*  yutu_fw_get_longitude(void);
char* yutu_fw_get_device_temp(void);
int yutu_fw_get_lock_status(void);
char* yutu_fw_get_open_status(void);
long yutu_fw_get_time(void);
char* yutu_if_get_date_time();
char* yutu_fw_get_tamper_status(void);
char* yutu_fw_get_devicekey(void);
char* yutu_fw_get_currspeed(void);
int yutu_fw_get_shackle_status(void);
char* yutu_fw_get_device_no(void);
char* yutu_fw_get_old_unlock_key(void);
void yutu_fw_set_old_unlock_key(uint8_t* old_key);
void yutu_fw_get_unlck_auth_numbers(uint8_t mem_locn,
				    unlck_auth_numbers* unlck_no);
result yutu_fw_unlock_device(void);
result yutu_fw_lock_device(void);
uint8_t yutu_fw_get_battery_percentage(void);
uint8_t yutu_fw_get_device_temperature(void);
void yutu_fw_set_reporting_interval(uint32_t milliseconds);
// result yutu_fw_bt_hfp_disconn();
// result yutu_fw_send_AT_cmd(char* cmd);
// result yutu_fw_bt_accept_sent(void);
// result yutu_fw_process_BT_URC(char* resp_buffer);
// result yutu_fw_echo_off(void);
// result yutu_fw_check_modem_ready(void);
// result do_MISC(char* recv_buffer);
// result yutu_fw_bt_init(char* hostname);
// result do_BT(char* recv_buffer);
// result yutu_fw_bt_power_on(void);
// result yutu_fw_bt_init(char* host_name);
// result yutu_fw_init_misc();
// result yutu_fw_init_BT();
// result yutu_fw_bt_sppsend_data();
// void yutu_fw_bt_process_command(char* rsp, uint16_t bytes);
// result yutu_fw_gsm_check_SIM_present(void);
// result yutu_fw_init_GSM(void);
// result yutu_fw_gsm_check_roaming_enabled(void);
// result do_GSM(char* recv_buffer);
// result yutu_fw_gprs_single_conn_mode(void);
// result yutu_fw_init_GPRS(void);
// result do_GPRS(char* recv_buffer);
// result yutu_fw_gprs_set_apn(void);
// result yutu_fw_gprs_enable_wireless(void);
// result yutu_fw_gprs_get_local_ip(void);
// result yutu_fw_gprs_set_recv_from(void);
// result yutu_fw_gprs_send_message(char* message);
// result yutu_fw_gprs_ping_google(void);
// result yutu_fw_gprs_setup_server_conn(char* server, char* port);
// result yutu_fw_gprs_cip_shut(void);

#endif /* YUTU_FW_DEFINES_H_ */
