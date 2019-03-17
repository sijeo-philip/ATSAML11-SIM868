#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "JSONParser.h"
#include "JSONWriter.h"

#include <stdint.h>
#include "common.h"
#include "sam.h"  // Device header
#include "yutu_fw_at_command_exec.h"
#include "yutu_fw_bt.h"
#include "yutu_fw_defines.h"
#include "yutu_fw_hw_init.h"
#include "yutu_fw_scheduler.h"
#include "yutu_fw_uart.h"
// Constants
//
#define MEM_AUTH_NUMBER 0x0001
#define BUFF_SIZE 300
//

// volatile uint32_t task1_counter=0;
// volatile uint32_t task2_counter=0;
// Opcodes
//

#define MOB_UNLOCK_REQ 112
#define MOB_UNLOCK_RESP 112
#define SVR_UNLOCK_REQ 3
#define MOB_SVR_UNLOCK_REQ 2
#define MOB_LOCK_REQ 117
#define MOB_LOCK_RESP 117
#define SVR_LOCK_REQ 8
#define DEV_REPORT_PACKET 8
#define DEV_UNLOCK_PACKET 9
#define SVR_GET_SYNC_PACKET 11
//
// end Opcodes

// SUCCESS/ERROR CODES
#define COMMAND_SUCCESS 40
#define COMMAND_FAILURE 41
#define INVALID_SGTIN_NUMBER 120
#define SHACKLE_NOT_IN_ERROR 42
#define SVR_OR_MOB_REQ_ERROR 43
#define ERROR_OPCODE 255
#define SVR_INVALID_OPCODE 40
#define MOB_INVALID_OPCODE 131
#define SVR_INVALID_DEVICE_ID 30
#define MOB_INVALID_DEVICE_ID 130
#define INVALID_OLD_UNLOCK_KEY 53
#define INVALID_NEW_UNLOCK_KEY 54
#define INVALID_JSON 75
#define INVALID_TYPE 76
#define UNLOCK_FAILED 77
#define LOCK_FAILED 78
#define DEVICEID_NOT_FOUND 78
#define MOBILE_NUMBER_NOT_FOUND 140
#define INVALID_MOBILE_NUMBER 141
#define MOBILE_CONNECT_RESPONSE 110
#define MOBILE_LOCATION_NOT_FOUND 142
#define UNLOCK_SUCCESS 83
#define LOCKING_FAILED 84
#define LOCK_REPORT_PACKET 85
#define MOB_CONNECT_REQ 110
#define MOBILE_LOCK 87
#define DEVICE_UNLCK 89
#define BUTTON_LOCK 90
#define SLEEP_ON_TIME_NOT_FOUND 91
#define REPORTING_INTERVAL_NOT_FOUND 92
#define TRIP_ID_NOT_FOUND 93
#define INVALID_TRIP_ID 94

// MANGESH CODE START
#define REGISTRATION_OPCODE 1
#define TRIP_START_OPCODE 26

// ERROR CODE START
#define ENCRYPTION_KEY_NOT_FOUND 41
#define MEMORY_READ_FAILURE 58
#define MEMORY_WRITE_FAILURE 59
#define MASTER_KEY_NOT_FOUND 60
#define BUTTON_CODE_NOT_FOUND 61
#define REPORT_PACKET_INTERVAL_NOT_FOUND 62
#define SLEEP_ON_NOT_FOUND 63
#define GPS_ON_OFF_NOT_FOUND 64
#define DEVICE_TYPE_NOT_FOUND 65
#define CLIENT_NOT_FOUND 66
#define DEVICE_NO_NOT_FOUND 67
#define ADMIN_PHONE_2_NOT_FOUND 68
#define ADMIN_PHONE_1_NOT_FOUND 72
#define UNLOCK_KEY_NOT_FOUND 69
#define ASSEET_NAME_NOT_FOUND 70
#define VERIFY_UNLOCK_NOT_FOUND 71
#define OPERATOR_NUMBER_NOT_FOUND 73
#define UNLOCK_TYPE_NOT_FOUND 74
#define TRIP_DURATION_NOT_FOUND 74
// end CODES
//
#define USER_GPS_ON_OFF_ADDR 0x0001
#define CLIENT_ADDR 0x0001
#define VERIFY_UNLOCK_ADDR 0x0001
#define USER_BUTTON_CODE_ADDR 0x0001
#define DEVICE_TYPE_ADDR 0x0001
#define DEVICE_NO_ADDR 0x0001
#define ASSET_NAME_ADDR 0x0001
#define DEVICE_ID_ADDR 0x0001
#define CONNECTED_MOBILE_ADDR 0x0001
#define MQTT_KEEP_ALIVE_DUR_ADDR 0x0001
#define MQTT_SUBSCRIBE_TOPIC_ADDR 0x0001
#define MQTT_PUBLISH_TOPIC_ADDR 0x0001
#define MQTT_PROTOCOL_NAME_ADDR 0x0001
#define MQTT_CLIENT_ID_ADDR 0x0001
#define MQTT_PASSWORD_ADDR 0x0001
#define MQTT_USERNAME_ADDR 0x0001
#define CLIENT_PH_ADDR 0x0001
#define OPERATOR_PH_ADDR 0x0001
#define SUPERVISOR_PH_ADDR 0x0001
#define ADMIN_PH_ADDR 0x0001
#define USER_DEEP_SLEEP_OFF_ADDR 0x0001
#define USER_DEEP_SLEEP_ON_ADDR 0x0001
#define USER_SLEEP_OFF_ADDR 0x0001
#define USER_SLEEP_ON_ADDR 0x0001
#define USER_REPORT_INTERVAL_ADDR 0x0001
#define USER_UNLOCK_KEY_ADDR 0x0001
#define USER_ENCRYPTION_KEY_ADDR 0x0001
#define USER_MASTER_KEY_ADDR 0x0001
#define DEF_DEEP_SLEEP_OFF_ADDR 0x0001
#define DEF_DEEP_SLEEP_ON_ADDR 0x0001
#define DEF_SLEEP_OFF_ADDR 0x0001
#define DEF_SLEEP_ON_ADDR 0x0001
#define DEF_REPORT_INTERVAL_ADDR 0x0001
#define DEF_UNLOCK_KEY_ADDR 0x0001
#define DEF_ENCRYPTION_KEY_ADDR 0x0001
#define DEF_MASTER_KEY_ADDR 0x0001
#define ENCRYPTION_KEY_ADDR 0x0001
#define PORT_ADDR 0x0001
#define IPADD_ADDR 0x0001
#define APN_ADDR 0x0001
#define CFG_MEM_ADDR 0x0001
#define SGTIN_ADDR 0x0001
#define TRIP_ID_ADDR 0x0001
//
//
//
//
// DESIGN
// TODO :
// 1 : store the unlock packet if there is no connection. at each sending
// interval check if n/w is available. if so send all packets and delete the
// queue. if not, queue again.
//
// END DESIGN
//

// Variables
//

//#define YUTU_DEBUG 1

struct jWriteControl testJSON;
struct jsonParser testReadJSON;
struct jsonReadToken* jsonPacket;

typedef enum
{
  MOBILE_UNLCK,
  REMOTE_UNLCK,
  BUTTON_UNLCK,
  SMS_UNLCK
} unlock_method;

typedef enum
{
  MOBILE_LCK,
  REMOTE_LCK,
  BUTTON_LCK,
  SMS_LCK
} lock_method;

typedef enum
{
  SINGLE_PRESS,
  DOUBLE_PRESS,
  LONG_PRESS,
  UNLOCK_PRESS
} key_press_type;

typedef struct
{
  uint32_t report_packet_interval;
  uint32_t sleep_on_time;
  bool verify_unlock;
  bool gps_on_off;
} config_params;

uint8_t recv_buff[BUFF_SIZE];
uint8_t send_buff[BUFF_SIZE];
char btMsgFrame[200];

struct ConfigParam
{
  uint8_t* MasterKey;
  uint8_t* EncryptionKey;
  uint8_t* UnlockKey;
  int ReportPacketInterval;  // in second
  int SleepON;
  int SleepOFF;
  int DeepSleepON;
  int DeepSleepOFF;
};

bool encryption_key_arrived = false;
// Extern Declarations
volatile extern BTProfile btProfile;
extern atcommand_exec_event_t dataEvents;
volatile extern unsigned short machineState;
extern unlck_auth_numbers auth_numbers;
//
// end Variables

// Declarations
//

//result yutu_fw_mobile_send_message(uint8_t* buff);
result yutu_fw_server_send_message(uint8_t* buff);
void yutu_fw_mobile_message_handler(BT_Receive_Callback arg,
				    BTProfile* bt_profile, char* btHostName,
				    char* btRcvBuffer);
// void yutu_fw_server_message_handler(message_callback arg);
result yutu_if_enable_gprs_conn(void);
result yutu_if_enable_bt_connConnection(void);
//

void yutu_fwap_alarms_handler(void* arg);
void yutu_fwap_mobile_request_handler(char* arg, char* response);
// void yutu_fwap_server_request_handler(request_packet* arg, char* response);
void yutu_fwap_report_packet_handler(void* arg);
void yutu_fwap_buttonpress_handler(void* data);
void yutu_fwap_create_report_packet(uint8_t* outdata);
void yutu_fwap_create_unlock_packet(uint8_t* outdata, uint8_t type);
void yutu_fwap_create_error_packet(char* outdata, char* deviceID,
				   uint8_t opcode, uint8_t error_code);
void yutu_fwap_create_lock_report_packet(uint8_t* outdata,
					 lock_method lock_type,
					 char* mobile_no);

void yutu_fwap_store_message(uint8_t* buffer, source src);

// Managesh code
//

result yutu_if_SendAcknowledge(source dest, int opcode);
void yutu_if_send_error_packet(int error_code);
result yutu_if_send_resistration_packet();
result yutu_if_BTSendMessage(char* packet);
result yutu_if_WriteConfigParam(struct ConfigParam* parameters, bool wh);
result yutu_if_ReadConfigParam(struct ConfigParam* parameters, bool wh);
result yutu_if_SendMessage(char* packet, source dest);
result yutu_if_end_trip_packet();
result yutu_if_store_trip_packet();
/*
 * @owner       Mohan
 * @brief       monolithic main contain all functionality
 * @Called by   System
 * @param[out]  None
 * @param[in]   None.
 * @return      int
 * @ DLD Reference:  DLD for IOT Lock Device-L11-3.1
 * @ Tracebility : Section 4.9.1
 */

void HardFault_Handler()
{
  while (1)
    ;
}

int main(void)
{
  /* Initializes MCU, drivers and middleware */
  // atmel_start_init();

  yutu_fw_init();
  // yutu_fw_set_reporting_interval(1000);
  // yutu_fw_register_alarms(yutu_fwap_alarms_handler);
  yutu_fw_mobile_message_handler(yutu_fwap_mobile_request_handler, &btProfile,
				 "yukee_lock", btMsgFrame);
  // yutu_fw_server_message_handler(yutu_fwap_server_request_handler);
  // yutu_fw_gps_data_rdy_handler(yutu_fwap_report_packet_handler);
  // yutu_fw_reg_button_handler(yutu_fwap_buttonpress_handler);
  // if (yutu_if_send_resistration_packet() != SUCCESS)
  //{
  //// TODO : if registration is not done, what can you do?
  //}
  yutu_fw_scheduler_start();
  while (1)
    ;
}

/*
 * @owner       Mohan
 * @brief       whenever the interface layer fails to send message, we need to
 *              keep this packet for sending it later.
 * @Called by   Device
 * @param[out]  None
 * @param[in]   pointer to buffer and towards source type.
 * @return      None.
 * @ DLD Reference:  DLD for IOT Lock Device-L11-3.1
 * @ Tracebility : Section TBD
 */
void yutu_fwap_store_message(uint8_t* buffer, source src)
{
  // TODO : append this message and src info into a list.
  //        a timer will check if there is network.
  //        the timer handler should send all the pending messages and discard
  //        them from list.
  ;
}

// ERROR CODE END

char* yutu_fw_get_date_time() { return "13425464645"; }
typedef enum
{
  REQUEST,
  RESPONSE,
  ERROR,
  ONE_WAY
} TYPE;
/********************************************************************************************************
@function		: result yutu_if_end_trip_packet()
@Description	: This function use to store trip packet phone numbers in secure
location
@parameter		: [in] packet -> packet to be send
@return			: SUCCESS/FAILURE
**********************************************************************************************************/
result yutu_if_end_trip_packet()
{
  yutu_fw_erase_secure_flash(ADMIN_PH_ADDR);
  yutu_fw_erase_secure_flash(SUPERVISOR_PH_ADDR);
  yutu_fw_erase_secure_flash(OPERATOR_PH_ADDR);
  yutu_fw_erase_secure_flash(CLIENT_PH_ADDR);

  return SUCCESS;
}

/********************************************************************************************************
@function		: result yutu_if_store_trip_packet(char* packet)
@Description	: This function use to store trip packet phone numbers in secure
location
@parameter		: [in] packet -> packet to be send
@return			: SUCCESS/FAILURE
**********************************************************************************************************/
result yutu_if_store_trip_packet(uint8_t* packet)
{
  if (packet !=
      NULL)  // extract phone number from json packet and store on secure memory
    yutu_fw_write_secure_flash(packet, (int)ADMIN_PH_ADDR);
  else
    return FAILURE;
  if (packet !=
      NULL)  // extract phone number from json packet and store on secure memory
    yutu_fw_write_secure_flash(packet, (int)SUPERVISOR_PH_ADDR);
  else
    return FAILURE;
  if (packet !=
      NULL)  // extract phone number from json packet and store on secure memory
    yutu_fw_write_secure_flash(packet, (int)OPERATOR_PH_ADDR);
  else
    return FAILURE;
  if (packet !=
      NULL)  // extract phone number from json packet and store on secure memory
    yutu_fw_write_secure_flash(packet, (int)CLIENT_PH_ADDR);
  else
    return FAILURE;

  return SUCCESS;
}

/********************************************************************************************************
@function		: result yutu_if_BTSendMessage(char* packet)
@Description	: This function use to send packet to connected bluetooth device
@parameter		: [in] packet -> packet to be send
@return			: SUCCESS/FAILURE
**********************************************************************************************************/
result yutu_if_BTSendMessage(char* packet)
{
  // Sijeo API to send BT message
  return SUCCESS;
}
/********************************************************************************************************
@function		: result yutu_if_SendMessage(char* packet,source dest)
@Description	: This function use to send packet to connected bluetooth device
@parameter		: [in] packet -> packet to be send
				  [in] dest -> source (MOBILE/SERVER)
@return			: SUCCESS/FAILURE
**********************************************************************************************************/
result yutu_if_SendMessage(char* packet, source dest)
{
  result ret_val;
  // if(dest == SERVER)
  // ret_val =
  // yutu_fw_send_publish_packet(mqttconfiginf,sysinf,packet,pubtopic);
  // else
  ret_val = yutu_if_BTSendMessage(packet);

  return ret_val;
}

/********************************************************************************************************
@function		: result yutu_if_ReadConfigParam(ConfigParam
*parameters)
@Description	: This function use to read config parameter from secure memory
@parameter		: [in] parameters -> config parameter
@return			: SUCCESS/FAILURE
**********************************************************************************************************/
result yutu_if_ReadConfigParam(struct ConfigParam* parameters, bool wh)
{
  uint8_t* data = NULL;
  if (wh)
    {
      if (yutu_fw_read_secure_flash(data, (int)DEF_MASTER_KEY_ADDR) == FAILURE)
	return FAILURE;
      parameters->MasterKey = data;

      if (yutu_fw_read_secure_flash(data, (int)DEF_ENCRYPTION_KEY_ADDR) ==
	  FAILURE)
	return FAILURE;
      parameters->EncryptionKey = data;

      if (yutu_fw_read_secure_flash(data, (int)DEF_UNLOCK_KEY_ADDR) == FAILURE)
	return FAILURE;
      parameters->UnlockKey = data;

      if (yutu_fw_read_secure_flash(data, (int)DEF_REPORT_INTERVAL_ADDR) ==
	  FAILURE)
	return FAILURE;
      parameters->ReportPacketInterval = (int)data;

      if (yutu_fw_read_secure_flash(data, (int)DEF_SLEEP_ON_ADDR) == FAILURE)
	return FAILURE;
      parameters->SleepON = (int)data;

      if (yutu_fw_read_secure_flash(data, (int)DEF_SLEEP_OFF_ADDR) == FAILURE)
	return FAILURE;
      parameters->SleepOFF = (int)data;

      if (yutu_fw_read_secure_flash(data, (int)DEF_DEEP_SLEEP_ON_ADDR) ==
	  FAILURE)
	return FAILURE;
      parameters->DeepSleepON = (int)data;

      if (yutu_fw_read_secure_flash(data, (int)DEF_DEEP_SLEEP_OFF_ADDR) ==
	  FAILURE)
	return FAILURE;
      parameters->DeepSleepOFF = (int)data;
    }
  else
    {
      if (yutu_fw_read_secure_flash(data, (int)USER_MASTER_KEY_ADDR) == FAILURE)
	return FAILURE;
      parameters->MasterKey = data;

      if (yutu_fw_read_secure_flash(data, (int)USER_ENCRYPTION_KEY_ADDR) ==
	  FAILURE)
	return FAILURE;
      parameters->EncryptionKey = data;

      if (yutu_fw_read_secure_flash(data, (int)USER_UNLOCK_KEY_ADDR) == FAILURE)
	return FAILURE;
      parameters->UnlockKey = data;

      if (yutu_fw_read_secure_flash(data, (int)USER_REPORT_INTERVAL_ADDR) ==
	  FAILURE)
	return FAILURE;
      parameters->ReportPacketInterval = (int)data;

      if (yutu_fw_read_secure_flash(data, (int)USER_SLEEP_ON_ADDR) == FAILURE)
	return FAILURE;
      parameters->SleepON = (int)data;

      if (yutu_fw_read_secure_flash(data, (int)USER_SLEEP_OFF_ADDR) == FAILURE)
	return FAILURE;
      parameters->SleepOFF = (int)data;

      if (yutu_fw_read_secure_flash(data, (int)USER_DEEP_SLEEP_ON_ADDR) ==
	  FAILURE)
	return FAILURE;
      parameters->DeepSleepON = (int)data;

      if (yutu_fw_write_secure_flash(data, (int)USER_DEEP_SLEEP_OFF_ADDR) ==
	  FAILURE)
	return FAILURE;
      parameters->DeepSleepOFF = (int)data;
    }
  return SUCCESS;
}

/********************************************************************************************************
@function		: result yutu_if_send_resistration_packet()
@Description	: This function use send registration packet to server
@parameter		: [in] opcode-> opcode of request
@parameter		: [in] error_code-> error code
**********************************************************************************************************/
result yutu_if_send_resistration_packet(void)
{
  char buffer[BUFF_SIZE];
  struct jWriteControl response_data;
  uint8_t *deviceID = NULL, *SgtinLoc = NULL;

  // yutu_fw_read_secure_flash(deviceID, DEVICE_ID_ADDR);
  jwOpen(&response_data, (char*)buffer, BUFF_SIZE, JW_OBJECT, JW_COMPACT);
  if (yutu_fw_read_secure_flash(deviceID, DEVICE_ID_ADDR) != SUCCESS)
    {
      // TODO : handle error
    }
  jwObj_string(&response_data, "DeviceId", (char*)deviceID);
  jwObj_int(&response_data, "Opcode", REGISTRATION_OPCODE);
  jwObj_string(&response_data, "DateTime", yutu_fw_get_date_time());
  jwObj_int(&response_data, "Type", REQUEST);
  if (yutu_fw_read_secure_flash(SgtinLoc, SGTIN_ADDR) != SUCCESS)
    {
      // TODO : handle error
    }
  jwObj_string(&response_data, "SGTIN", (char*)SgtinLoc);
  jwEnd(&response_data);
  if (yutu_if_SendMessage(buffer, SERVER) != SUCCESS)
    yutu_fwap_store_message((uint8_t*)buffer, SERVER);

  return SUCCESS;
}
/********************************************************************************************************
@function		: void yutu_if_send_error_packet(int opcode,int
error_code)
@Description	: This function use to send error
@parameter		: [in] opcode-> opcode of request
@parameter		: [in] error_code-> error code
**********************************************************************************************************/
void yutu_if_send_error_packet(int error_code)
{
  char buffer[BUFF_SIZE];
  struct jWriteControl response_data;
  jwOpen(&response_data, (char*)buffer, BUFF_SIZE, JW_OBJECT, JW_COMPACT);
  uint8_t* deviceID = NULL;

  yutu_fw_read_secure_flash(deviceID, DEVICE_ID_ADDR);
  jwOpen(&response_data, (char*)buffer, BUFF_SIZE, JW_OBJECT, JW_COMPACT);
  if (yutu_fw_read_secure_flash(deviceID, DEVICE_ID_ADDR) != SUCCESS)
    {
      // yutu_if_send_error_packet(MEMORY_WRITE_FAILURE);
      return;
    }
  jwObj_string(&response_data, "DeviceId", (char*)deviceID);
  jwObj_int(&response_data, "Opcode", (int)ERROR_OPCODE);
  jwObj_string(&response_data, "DateTime", yutu_fw_get_date_time());
  jwObj_int(&response_data, "Type", RESPONSE);
  jwObj_int(&response_data, "Error", error_code);
  jwEnd(&response_data);
  if (yutu_if_SendMessage(buffer, SERVER) != SUCCESS)
    yutu_fwap_store_message((uint8_t*)buffer, SERVER);
}

/********************************************************************************************************
@function		: result yutu_if_SendAcknowledge(source dest,int opcode)
@Description	: This function use send ack for required source
@parameter		: [in] dest-> destination to send
@parameter		: [in] opcode-> opcode for packet
**********************************************************************************************************/
void yutu_fwap_create_ack_packet(uint8_t* outdata, int opcode)
{
  char buffer[BUFF_SIZE];
  struct jWriteControl response_data;
  jwOpen(&response_data, (char*)buffer, BUFF_SIZE, JW_OBJECT, JW_COMPACT);
  uint8_t* deviceID = NULL;

  yutu_fw_read_secure_flash(deviceID, DEVICE_ID_ADDR);
  jwOpen(&response_data, (char*)buffer, BUFF_SIZE, JW_OBJECT, JW_COMPACT);
  if (yutu_fw_read_secure_flash(deviceID, DEVICE_ID_ADDR) != SUCCESS)
    {
      // TODO : handle error
    }
  jwObj_string(&response_data, "DeviceId", (char*)deviceID);
  jwObj_int(&response_data, "OpCode", (int)REGISTRATION_OPCODE);
  jwObj_string(&response_data, "DateTime", yutu_fw_get_date_time());
  jwObj_int(&response_data, "Type", RESPONSE);
  jwObj_string(&response_data, "Ack", "ACK");
  jwEnd(&response_data);
}
/*
 * @owner       Mohan
 * @brief       mobile side and server side request handler.
 * @Called by   interface layer
 * @param[out]  None
 * @param[in]   None
 * @return      None
 * @ DLD Reference:  DLD for IOT Lock Device-L11-3.1
 * @ Tracebility : Section 4.9.3, 4.5.2, 4.5.4, 4.5.9, 4.5.10, 4.8 case 1, 4.8.5
 */

struct jsonParser req_packet_parsed;

void yutu_ffwap_mobile_request_handler(char* req, char* response)
{
  char* temp =
      "{\"deviceID\":\"01010101010\",\"opCode\":110,\"dateTime\":"
      "\"13425464645\",\"type\":1,\"encryptionKey\":\"a2b23asdfs\","
      "\"batteryPercentage\":21,\"lockStatus\":0,\"deviceTemp\":21}";
  strcpy(response, temp);
}
struct jWriteControl resp_packet;
void yutu_fwap_mobile_request_handler(char* req, char* response)
{
  //// char* con_json_pack =
  ////"{\"deviceID\":\"12134456799\",\"opCode\":110,\"dateAndTime\":"
  ////"\"02134401222019\",\"type\":0,\"SGTIN\":\"1234333566666\"}";

  memset(&req_packet_parsed, '\0', sizeof(req_packet_parsed));
  int err = JSONStructInit(&req_packet_parsed, req);
  if (err != JDATA_NOK)
    {
      while (req_packet_parsed.parserStatus != JSON_END)
	err = JSONParseData(&req_packet_parsed);
      volatile struct jsonReadToken* req_json_packet_sID;
      volatile struct jsonReadToken* req_json_packet_opcode;
      volatile struct jsonReadToken* req_json_packet_type;
      req_json_packet_sID = SearchToken(&req_packet_parsed, "\"sID\"");
      req_json_packet_opcode = SearchToken(&req_packet_parsed, "\"op\"");
      req_json_packet_type = SearchToken(&req_packet_parsed, "\"type\"");
      bool errorFlag = false;
      if (!req_json_packet_opcode)
		{
	  //char err_buff[BUFF_SIZE];
	  //memset(err_buff, '\0', BUFF_SIZE);
	  yutu_fwap_create_error_packet(
	      response, req_json_packet_sID->jsonValue,
	      (uint8_t)convert_string_to_integer(req_json_packet_opcode->jsonValue),
	      MOB_INVALID_OPCODE);
	  	  return;
	}
      // int opcode = atoi(req_json_packet_opcode->jsonValue);
      int opcode = convert_string_to_integer(req_json_packet_opcode->jsonValue);
      if (opcode != 110)
			{
	  if (!req_json_packet_sID)
	    {
	      //char err_buff[BUFF_SIZE];
	      //memset(err_buff, '\0', BUFF_SIZE);
	      yutu_fwap_create_error_packet(
		  response, req_json_packet_sID->jsonValue,
		  (uint8_t)convert_string_to_integer(req_json_packet_opcode->jsonValue),
		  MOB_INVALID_DEVICE_ID);
	      //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
	      return;
	    }
	  char* device_id = req_json_packet_sID->jsonValue;

	  char* devID1 = "\"12134456799\"";
	  char* devID2 = "\"01010101010\"";
	  /*
	    *    if (yutu_fw_read_secure_flash((uint8_t*)devID, DEVICE_ID_ADDR)
	   * != SUCCESS)
	   *   {
	   * // TODO Handle error
	   *   }
	   */

	  char* json_devID = req_json_packet_sID->jsonValue;
	  if ((strcmp(json_devID, devID1) != 0) &&
	      (strcmp(json_devID, devID2) != 0))
	    {
	      //char err_buff[BUFF_SIZE];
	      //memset(err_buff, '\0', BUFF_SIZE);
	      yutu_fwap_create_error_packet(
		  response, req_json_packet_sID->jsonValue,
		  (uint8_t)convert_string_to_integer(req_json_packet_opcode->jsonValue),
		  MOB_INVALID_DEVICE_ID);
	      //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
	      return;
	    }
	}
     if (!req_json_packet_type)
	{
	  //char err_buff[BUFF_SIZE];
	  //memset(err_buff, '\0', BUFF_SIZE);
	  yutu_fwap_create_error_packet(
	      response, req_json_packet_sID->jsonValue,
	      (uint8_t)convert_string_to_integer(req_json_packet_opcode->jsonValue), INVALID_JSON);
	  //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
	  return;
	}

      switch (opcode)
	{
	  case MOB_CONNECT_REQ:
	    {
	     struct jsonReadToken* devSGTIN =
		  SearchToken(&req_packet_parsed, "\"SGTIN\"");
	      char* sgtin = "\"1234333566666\"";
	      /* : currently not able to store the SGTIN
	       *   if (yutu_fw_read_secure_flash((uint8_t*)sgtin, SGTIN_ADDR) !=
	       *   SUCCESS)
	       * {
	       *   // TODO : handle error
	       * }
	       */
	      char* json_SGTIN = devSGTIN->jsonValue;
	      if (strcmp(json_SGTIN, sgtin) != 0)
		{
		  //char err_buff[BUFF_SIZE];
		  //memset(err_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(
		      response, req_json_packet_sID->jsonValue,
		      (uint8_t)convert_string_to_integer(req_json_packet_opcode->jsonValue),
		      INVALID_SGTIN_NUMBER);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
		  return;
		}

	      /*
	      *   struct jsonReadToken* mobile_ph_no =
	      *   SearchToken(&req_packet_parsed, "\"mobileNumber\"");
	      *   if (!mobile_ph_no)
	      * {
	      *   char err_buff[BUFF_SIZE];
	      *   memset(err_buff, '\0', BUFF_SIZE);
	      *   yutu_fwap_create_error_packet(
	      *       err_buff, req_json_packet_devID->jsonValue,
	      *       (uint8_t)atoi(req_json_packet_opcode->jsonValue),
	      *       MOBILE_NUMBER_NOT_FOUND);
	      *   result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
	      *   return;
	      * }
	      *   long mobile_number = atoi(mobile_ph_no->jsonValue);
	      *   yutu_fw_get_unlck_auth_numbers(MEM_AUTH_NUMBER,
	      * &auth_numbers); if (mobile_number !=
	      * auth_numbers.admin_auth_number && mobile_number !=
	      * auth_numbers.supervisor_auth_number && mobile_number !=
	      * auth_numbers.operator_auth_number && mobile_number !=
	      * auth_numbers.client_auth_number)
	      * {
	      *   char err_buff[BUFF_SIZE];
	      *   memset(err_buff, '\0', BUFF_SIZE);
	      *   yutu_fwap_create_error_packet(
	      *       err_buff, req_json_packet_devID->jsonValue,
	      *       (uint8_t)atoi(req_json_packet_opcode->jsonValue),
	      *       INVALID_MOBILE_NUMBER);
	      *   result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
	      *   // TODO : Also send illegal connection attempt to Server
	      *   return;
	      * }
	      */
	      // send response.
	      //char conn_resp_buff[BUFF_SIZE];
	      //memset(conn_resp_buff, '\0', BUFF_SIZE);

	      char* date_time = yutu_fw_get_date_time();
	      /* : currently not stored in device. Hence comment.
	       *   if (yutu_fw_read_secure_flash((uint8_t*)devID,
	       * DEVICE_ID_ADDR) != SUCCESS)
	       * {
	       *   // TODO : handle error
	       * }
	       */
	      char* no_quote_sID = "01010101010";
	      memset(&resp_packet, '\0', sizeof(resp_packet));
	      jwOpen(&resp_packet, (char*)response, BUFF_SIZE, JW_OBJECT,
		     JW_COMPACT);
	      jwObj_string(&resp_packet, "sID", no_quote_sID);
	      jwObj_int(&resp_packet, "op", MOBILE_CONNECT_RESPONSE);
	      jwObj_string(&resp_packet, "DAT", date_time);
	      jwObj_int(&resp_packet, "type", RESPONSE);
	      char* enc_key = "a2b23asdfs";
	      if (yutu_fw_read_secure_flash((uint8_t*)enc_key,
					    ENCRYPTION_KEY_ADDR) != SUCCESS)
		{
		  // TODO : Handle Error
		}

	      jwObj_string(&resp_packet, "enKey", enc_key);
	      uint8_t batt_percent = yutu_fw_get_battery_percentage();
	      jwObj_int(&resp_packet, "batt", batt_percent);
	      lock_status ls = yutu_fw_get_lock_status();
	      jwObj_int(&resp_packet, "lckSt", ls);
	      uint8_t dev_temp = yutu_fw_get_device_temperature();
	      jwObj_int(&resp_packet, "dTemp", dev_temp);
	      jwEnd(&resp_packet);
	      //char* resp_ptr = conn_resp_buff;
	      //strcpy(response, conn_resp_buff);
	      return;
	    }
	    break;

	  case MOB_UNLOCK_REQ:
	    {
	      struct jsonReadToken* tripID;
	      tripID = SearchToken(&req_packet_parsed, "\"TripID\"");
	      if (!tripID)
		{
		  // TODO : optional
		}
	      /*
	       *   uint8_t* tripIDinDev = NULL;
	       *   if (yutu_fw_read_secure_flash(tripIDinDev, TRIP_ID_ADDR) !=
	       *   SUCCESS)
	       * {
	       *   // TODO : handle error
	       * }
	       */

	      /*
	       *   if (atoi(tripID->jsonValue) != atoi((char*)tripIDinDev))
	       * {
	       *   uint8_t err_buff[BUFF_SIZE];
	       *   memset(err_buff, '\0', BUFF_SIZE);
	       *   yutu_fwap_create_error_packet(err_buff, INVALID_TRIP_ID);
	       *   if (yutu_if_send_message(err_buff, MOBILE) != SUCCESS)
	       *     yutu_fwap_store_message(err_buff, MOBILE);
	       *   return;
	       * }
	       */

	      char* sID = "01010101010";
	      /* no need to check for unlock.
	       *   if (yutu_fw_get_shackle_status() == SHACKLE_OUT)
	       * {
	       *   char resp_buff[BUFF_SIZE];
	       *   memset(resp_buff, '\0', BUFF_SIZE);
	       *   yutu_fwap_create_error_packet(resp_buff, sID, opcode,
	       *                 SHACKLE_NOT_IN_ERROR);
	       *   result res =
	       * yutu_fw_mobile_send_message((uint8_t*)resp_buff); return;
	       * }
	       */
	      // send the request to server.
	      struct jsonReadToken* jsonPacket;
	      jsonPacket = SearchToken(&req_packet_parsed, "\"mobNo\"");
	      if (!jsonPacket)
		{
		  //char resp_buff[BUFF_SIZE];
		  //memset(resp_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(response, sID, opcode,
						MOBILE_NUMBER_NOT_FOUND);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)resp_buff);
		  return;
		}
	      char* mobile_number = jsonPacket->jsonValue;
	      char* saved_mobileno = "\"8867830282\"";
	      if (strcmp(mobile_number, saved_mobileno) != 0)
		{
		  //char err_buff[BUFF_SIZE];
		  //memset(err_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(response, sID, opcode,
						INVALID_MOBILE_NUMBER);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
		  // TODO : report illegal number based unlock attempt to
		  // server.
		  return;
		}
	      /*
	       * : currently not storing the mobile number
	       *   yutu_fw_get_unlck_auth_numbers(MEM_AUTH_NUMBER,
	       * &auth_numbers);
	       * if (mobile_number != auth_numbers.admin_auth_number &&
	       * mobile_number != auth_numbers.supervisor_auth_number &&
	       * mobile_number != auth_numbers.operator_auth_number &&
	       * mobile_number != auth_numbers.client_auth_number)
	       *   {
	       * char err_buff[BUFF_SIZE];
	       * memset(err_buff, '\0', BUFF_SIZE);
	       * yutu_fwap_create_error_packet(err_buff, devID, opcode,
	       *                   INVALID_MOBILE_NUMBER);
	       * result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
	       * // TODO : report illegal number based unlock attempt to
	       * // server.
	       * return;
	       *   }
	       */
	      // create unlock req packet
	      // to server and send.
	      //char unlk_buff[300];
	      //memset(unlk_buff, '\0', 300);
	      char* date_time = (char*)yutu_fw_get_date_time();
	      /*
	       *   nothing stored in the flash now.if
	       * (yutu_fw_read_secure_flash( (uint8_t*)devID, DEVICE_ID_ADDR) !=
	       *                   SUCCESS)
	       *   {
	       * // TODO : handle error
	       *   }
	       */

	      char* devID = "123456789013";
	      memset(&resp_packet, '\0', sizeof(resp_packet));
	      jwOpen(&resp_packet, response, BUFF_SIZE, JW_OBJECT, JW_COMPACT);
	      jwObj_string(&resp_packet, "dID", devID);
	      jwObj_int(&resp_packet, "op", MOB_SVR_UNLOCK_REQ);
	      jwObj_string(&resp_packet, "DAT", date_time);
	      jwObj_int(&resp_packet, "type", REQUEST);
	      struct jsonReadToken* mob_lat =
		  SearchToken(&req_packet_parsed, "\"lat\"");

	     if (!mob_lat)
			{
		  //char err_buff[BUFF_SIZE];
		  //memset(err_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(response, (char*)devID, opcode,
						MOBILE_LOCATION_NOT_FOUND);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
			}
	     struct jsonReadToken* mob_lon =
		  SearchToken(&req_packet_parsed, "\"lon\"");

	      if (!mob_lon)
				{
		  //char err_buff[BUFF_SIZE];
		  //memset(err_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(response, (char*)devID, opcode,
						MOBILE_LOCATION_NOT_FOUND);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
			}

	      char* tripIDinDev = "123";
	      jwObj_string(&resp_packet, "mLat", mob_lat->jsonValue);
	      jwObj_string(&resp_packet, "mLon", mob_lon->jsonValue);
	      jwObj_string(&resp_packet, "dLat", yutu_fw_get_latitude());
	      jwObj_string(&resp_packet, "dLon", yutu_fw_get_longitude());
	      jwObj_string(&resp_packet, "mobNo", mobile_number);
	      jwObj_string(&resp_packet, "unlckType", "MOBILE_UNLCK");
	      jwObj_string(&resp_packet, "tripID", tripIDinDev);
	      jwEnd(&resp_packet);
	      //char* unlk_buff_ptr = NULL;
	      //unlk_buff_ptr = unlk_buff;
	      // strcpy(response, unlk_buff_ptr);
	      // return;

	      // TODO : assume that  server has given old and new unlock key
	      // compare old unlock key
	      // if match, then unlock, send unlock and report packet.
	      // else report unauth unlock to server.
	      //
	      char* json_from_server =
		  "{\"dID\":\"12134456799\",\"op\":2,\"DAT\":"
		  "\"02134401222019\",\"type\":1,\"oKey\":"
		  "\"33333333\",\"nKey\":"
		  "\"44444444\"}";

	      memset(&req_packet_parsed, '\0', sizeof(req_packet_parsed));
	      int err = JSONStructInit(&req_packet_parsed, json_from_server);
	      if (err != JDATA_NOK)
				{
				while (req_packet_parsed.parserStatus != JSON_END)
		    err = JSONParseData(&req_packet_parsed);
		  struct jsonReadToken* old_key =
		      SearchToken(&req_packet_parsed, "\"oKey\"");

		  char* devID = "12134456799";
		  int opcode = 2;
		  if (!old_key)
		    {
		      //char err_buff[BUFF_SIZE];
		      //memset(err_buff, '\0', BUFF_SIZE);
		      yutu_fwap_create_error_packet(response, devID, opcode,
						    UNLOCK_KEY_NOT_FOUND);
		      //result res =
			  ////yutu_fw_mobile_send_message((uint8_t*)err_buff);
		      ////;
		    }
		  struct jsonReadToken* new_key =
		      SearchToken(&req_packet_parsed, "\"nKey\"");

		  if(!new_key)
		    {
		      //char err_buff[BUFF_SIZE];
		      //memset(err_buff, '\0', BUFF_SIZE);
		      yutu_fwap_create_error_packet(response, devID, opcode,
						    UNLOCK_KEY_NOT_FOUND);
		      //result res =
			  //yutu_fw_mobile_send_message((uint8_t*)err_buff);
		      //;
		    }
		  if (strlen(old_key->jsonValue) != 10)
		    {
		      //char err_buff[BUFF_SIZE];
			  
		      //memset(err_buff, '\0', BUFF_SIZE);
		      yutu_fwap_create_error_packet(response, devID, opcode,
						    INVALID_OLD_UNLOCK_KEY);
		      //result res =
			  //yutu_fw_mobile_send_message((uint8_t*)err_buff);
		    }
		  if (strlen(new_key->jsonValue) != 10)
		    {
		      //char err_buff[BUFF_SIZE];
		      //memset(err_buff, '\0', BUFF_SIZE);
		      yutu_fwap_create_error_packet(response, devID, opcode,
						    INVALID_NEW_UNLOCK_KEY);
		      //result res =
			  //yutu_fw_mobile_send_message((uint8_t*)err_buff);
		    }

		  char* device_old_unlck_key = yutu_fw_get_old_unlock_key();

		  if (strcmp(device_old_unlck_key, old_key->jsonValue) != 0)
		    {
		      // TODO : Hack attempt. report to server.
		    }
		  yutu_fw_set_old_unlock_key((uint8_t*)new_key->jsonValue);
		  if (yutu_fw_unlock_device() == SUCCESS)
		    {
		      //uint8_t unlck_buff[BUFF_SIZE];
		      //memset(unlck_buff, '\0', BUFF_SIZE);
		      if (convert_string_to_integer(req_json_packet_opcode->jsonValue) ==
			  MOB_SVR_UNLOCK_REQ)  // NOTE : device initiated means
			// mobile
			yutu_fwap_create_unlock_packet(response,
						       MOBILE_UNLCK);
		     else
			yutu_fwap_create_unlock_packet(response,
						       REMOTE_UNLCK);
		      //yutu_fw_server_send_message((uint8_t*)unlck_buff);

		      if(opcode ==
			  MOB_SVR_UNLOCK_REQ)  // NOTE : mobile initiated
			{
			  //uint8_t resp_buff[BUFF_SIZE];
			  //memset(resp_buff, '\0', BUFF_SIZE);
			  char* date_time = yutu_fw_get_date_time();
			  /* if (yutu_fw_read_secure_flash(                   */
			   /*     (uint8_t*)devID, DEVICE_ID_ADDR) != SUCCESS) */
			  /*   {                                              */
			  /*     // TODO : error                              */
			  /*   }                                              */

			  memset(&resp_packet, '\0', sizeof(resp_packet));
			  //char unlck_resp_buff[BUFF_SIZE];
			  jwOpen(&resp_packet, response, BUFF_SIZE,
				JW_OBJECT, JW_COMPACT);
			  jwObj_string(&resp_packet, "sID", (char*)devID);
			  jwObj_int(&resp_packet, "op", MOB_UNLOCK_REQ);
			  jwObj_string(&resp_packet, "DAT", date_time);
			  jwObj_int(&resp_packet, "res", SUCCESS);
			  jwObj_int(&resp_packet, "type", RESPONSE);
			  lock_status ls = yutu_fw_get_lock_status();
			  jwObj_int(&resp_packet, "lckSt", UNLOCKED);
			  jwEnd(&resp_packet);
			  //char* resp_ptr = unlck_resp_buff;
			  //strcpy(response, resp_ptr);
			  return;
			}
		    }
		  else
		    {
		      //uint8_t unlck_buff[BUFF_SIZE];
		      //memset(unlck_buff, '\0', BUFF_SIZE);
		      if (convert_string_to_integer(req_json_packet_opcode->jsonValue) ==
			  MOB_SVR_UNLOCK_REQ)  // NOTE : device initiated means
			// mobile
			yutu_fwap_create_unlock_packet(response,
						      MOBILE_UNLCK);
		      else
			yutu_fwap_create_unlock_packet(response,
						       REMOTE_UNLCK);
		      //yutu_fw_server_send_message((uint8_t*)unlck_buff);

		      if (opcode ==
			  MOB_SVR_UNLOCK_REQ)  // NOTE : mobile initiated
			{
			  //uint8_t resp_buff[BUFF_SIZE];
			  //memset(resp_buff, '\0', BUFF_SIZE);
			  char* date_time = yutu_fw_get_date_time();
			  /* if (yutu_fw_read_secure_flash(                   */
			  /*     (uint8_t*)devID, DEVICE_ID_ADDR) != SUCCESS) */
			  /*   {                                              */
			  /*     // TODO : error                              */
			  /*   }                                              */

			  //char unlck_resp_buff[BUFF_SIZE];
			  //memset(unlck_resp_buff, '\0', BUFF_SIZE);
			  memset(&resp_packet, '\0', sizeof(resp_packet));

			  jwOpen(&resp_packet, response, BUFF_SIZE,
				JW_OBJECT, JW_COMPACT);
			  jwObj_string(&resp_packet, "sID", (char*)devID);
			  jwObj_int(&resp_packet, "op", MOB_UNLOCK_REQ);
			  jwObj_string(&resp_packet, "DAT", date_time);
			  jwObj_int(&resp_packet, "res", FAILURE);
			  jwObj_int(&resp_packet, "type", RESPONSE);
			  lock_status ls = yutu_fw_get_lock_status();
			  jwObj_int(&resp_packet, "lckSt", ls);
			  jwEnd(&resp_packet);
			  //char* resp_ptr = unlck_resp_buff;
			  //strcpy(response, resp_ptr);
			  return;
			}
		    }
		}
	    }
	    break;
	  case MOB_LOCK_REQ:
	    {
	     char* sID = "01010101010";
	     if (yutu_fw_get_shackle_status() == SHACKLE_OUT)
		{
		  //char resp_buff[BUFF_SIZE];
		  //memset(resp_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(response, sID, opcode,
						SHACKLE_NOT_IN_ERROR);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)resp_buff);
		  return;
		}
	     // send the request to server.
	     struct jsonReadToken* jsonPacket;
	      jsonPacket = SearchToken(&req_packet_parsed, "\"mobNo\"");
	      if (!jsonPacket)
		{
		  //char resp_buff[BUFF_SIZE];
		  //memset(resp_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(response, sID, opcode,
						MOBILE_NUMBER_NOT_FOUND);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)resp_buff);
		  return;
		}
	      char* mobile_number = jsonPacket->jsonValue;
	      char* saved_mobileno = "\"8867830282\"";
	      if (strcmp(mobile_number, saved_mobileno) != 0)
		{
		  //char err_buff[BUFF_SIZE];
		  //memset(err_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(response, sID, opcode,
						INVALID_MOBILE_NUMBER);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
		  // TODO : report illegal number based unlock attempt to
		  // server.
		  return;
		}
	      /*
	      * : currently not storing the mobile number
	      *   yutu_fw_get_unlck_auth_numbers(MEM_AUTH_NUMBER,
	      * &auth_numbers);
	      * if (mobile_number != auth_numbers.admin_auth_number &&
	      * mobile_number != auth_numbers.supervisor_auth_number &&
	      * mobile_number != auth_numbers.operator_auth_number &&
	      * mobile_number != auth_numbers.client_auth_number)
	      *   {
	      * char err_buff[BUFF_SIZE];
	      * memset(err_buff, '\0', BUFF_SIZE);
	      * yutu_fwap_create_error_packet(err_buff, devID, opcode,
	      *                   INVALID_MOBILE_NUMBER);
	      * result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
	      * // TODO : report illegal number based unlock attempt to
	      * // server.
	      * return;
	      *   }
	      */
	      // create unlock req packet
	      // to server and send.
	      //char unlk_buff[300];
	      //memset(unlk_buff, '\0', 300);
	      /*
	       *   nothing stored in the flash now.if
	       * (yutu_fw_read_secure_flash( (uint8_t*)devID, DEVICE_ID_ADDR) !=
	       *                   SUCCESS)
	       *   {
	       * // TODO : handle error
	       *   }
	       */

	      struct jsonReadToken* mob_lat =
		  SearchToken(&req_packet_parsed, "\"lat\"");

	      if (!mob_lat)
		{
		  //char err_buff[BUFF_SIZE];
		  //memset(err_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(response, (char*)sID, opcode,
						MOBILE_LOCATION_NOT_FOUND);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
		}
	      struct jsonReadToken* mob_lon =
		  SearchToken(&req_packet_parsed, "\"lon\"");

	      if (!mob_lon)
		{
		  //char err_buff[BUFF_SIZE];
		  //memset(err_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(response, (char*)sID, opcode,
						MOBILE_LOCATION_NOT_FOUND);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
		}

	      if (yutu_fw_lock_device() != SUCCESS)
		{
		  //char err_buff[BUFF_SIZE];
		  //memset(err_buff, '\0', BUFF_SIZE);
		  yutu_fwap_create_error_packet(response, sID, opcode,
						LOCKING_FAILED);
		  //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
		  return;
		}

	      /*
	       *   char* conn_mobile = NULL;
	       *   if (yutu_fw_read_secure_flash((uint8_t*)conn_mobile,
	       *                 CONNECTED_MOBILE_ADDR) != SUCCESS)
	       * {
	       *   // TODO : Handle error
	       * }
	       */

	      //uint8_t resp_buff[BUFF_SIZE];
	      //memset(resp_buff, '\0', BUFF_SIZE);
	      yutu_fwap_create_lock_report_packet(response, MOBILE_LCK,
						  mobile_number);
	      // yutu_fw_server_send_message(resp_buff);
	      // send success/fail to mobile
	      //uint8_t mob_resp_buff[BUFF_SIZE];
	      //memset(mob_resp_buff, '\0', BUFF_SIZE);
	      char* date_time = yutu_fw_get_date_time();

	      /*
	       *   if (yutu_fw_read_secure_flash((uint8_t*)devID,
	       * DEVICE_ID_ADDR) != SUCCESS)
	       * {
	       *   // TODO : Handle error
	       * }
	       */
	      //char lck_resp_buff[BUFF_SIZE];
	      //memset(lck_resp_buff, '\0', BUFF_SIZE);

	      memset(&resp_packet, '\0', sizeof(resp_packet));
	      jwOpen(&resp_packet, response, BUFF_SIZE, JW_OBJECT,
		    JW_COMPACT);
	      jwObj_string(&resp_packet, "sID", sID);
	      jwObj_int(&resp_packet, "op", MOB_LOCK_RESP);
	      jwObj_string(&resp_packet, "DAT", date_time);
	      jwObj_int(&resp_packet, "type", RESPONSE);
	      jwObj_int(&resp_packet, "res", SUCCESS);
	      uint8_t batt_percent = yutu_fw_get_battery_percentage();
	      jwObj_int(&resp_packet, "batt", batt_percent);
	      lock_status ls = yutu_fw_get_lock_status();
	      jwObj_int(&resp_packet, "lckSt", ls);
	      uint8_t dev_temp = yutu_fw_get_device_temperature();
	      jwObj_int(&resp_packet, "dTemp", dev_temp);
	      jwEnd(&resp_packet);
	      //char* resp_ptr = lck_resp_buff;
	      //strcpy(response, resp_ptr);
	      return;
	    }
	    break;
	  default:
	    {
	      // TODO : INVALID_OPCODE
	      //char err_buff[BUFF_SIZE];
	      char* devID = "111111111111";
	      //memset(err_buff, '\0', BUFF_SIZE);
	      yutu_fwap_create_error_packet(response, devID, MOB_INVALID_OPCODE,
					    FAILURE);
	      //result res = yutu_fw_mobile_send_message((uint8_t*)err_buff);
	    }
	    break;
	}
    }
  else
    {
      // TODO :  json parse error
      //char err_buff[BUFF_SIZE];
      //memset(err_buff, '\0', BUFF_SIZE);
      yutu_fwap_create_error_packet(response, NULL, 0, INVALID_JSON);
      //yutu_fw_mobile_send_message((uint8_t*)err_buff);
    }
}




//
//
//
//
// void yutu_fwap_server_request_handler(request_packet* req, char*
// response)
//{
// char* con_json_pack =
//"{\"deviceID\":\"12134456799\",\"opCode\":110,\"dateAndTime\":"
//"\"02134401222019\",\"type\":0,\"SGTIN\":\"1234333566666\"}";
//
//// int err = JSONStructInit(&req_packet_parsed, (char*)req->packet);
// int err = JSONStructInit(&req_packet_parsed, con_json_pack);
// if (err != JDATA_NOK)
//{
// while (req_packet_parsed.parserStatus != JSON_END)
// err = JSONParseData(&req_packet_parsed);
//
// volatile struct jsonReadToken* req_json_packet_devID;
// volatile struct jsonReadToken* req_json_packet_opcode;
// volatile struct jsonReadToken* req_json_packet_type;
// req_json_packet_devID = SearchToken(&req_packet_parsed,
// "\"deviceID\""); req_json_packet_opcode =
// SearchToken(&req_packet_parsed, "\"opCode\""); req_json_packet_type =
// SearchToken(&req_packet_parsed, "\"type\""); bool errorFlag = false; if
// (!req_json_packet_opcode)
//{
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(
// err_buff, req_json_packet_devID->jsonValue,
//(uint8_t)atoi(req_json_packet_opcode->jsonValue),
// SVR_INVALID_OPCODE);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
// return;
//}
// int opcode = atoi(req_json_packet_opcode->jsonValue);
// if (!req_json_packet_devID)
//{
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(
// err_buff, req_json_packet_devID->jsonValue,
//(uint8_t)atoi(req_json_packet_opcode->jsonValue),
// SVR_INVALID_DEVICE_ID);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
// return;
//}
//
// char* devID = NULL;
// if (yutu_fw_read_secure_flash((uint8_t*)devID, DEVICE_ID_ADDR) !=
// SUCCESS)
//{
//// TODO Handle error
//}
//
// if (strcmp(req_json_packet_devID->jsonValue, devID) != 0)
//{
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(
// err_buff, req_json_packet_devID->jsonValue,
//(uint8_t)atoi(req_json_packet_opcode->jsonValue),
// SVR_INVALID_DEVICE_ID);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
// return;
//}
// if (!req_json_packet_type)
//{
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(
// err_buff, req_json_packet_devID->jsonValue,
//(uint8_t)atoi(req_json_packet_opcode->jsonValue), INVALID_JSON);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
// return;
//}
//
// switch (atoi(req_json_packet_opcode->jsonValue))
//{
///*
//*       case REGISTRATION_OPCODE:
//*         {
//*           struct jsonReadToken *encryption_key,
//* *report_packet_interval, *sleep_on_time, *verify_unlock,
//* *gps_on_off;
//*
//*           encryption_key =
//*           SearchToken(&req_packet_parsed, "\"EncryptionKey\"");
//*           if (encryption_key)
//*         {
//*           uint8_t* Encryption_key =
//* (uint8_t*)encryption_key->jsonValue; if
//* (yutu_fw_write_secure_flash( Encryption_key,
//* (int)USER_ENCRYPTION_KEY_ADDR) != FAILURE)
//*             {
//*               uint8_t err_buff[BUFF_SIZE];
//*               memset(err_buff, '\0', BUFF_SIZE);
//*               yutu_fwap_create_error_packet(err_buff,
//*                             MEMORY_WRITE_FAILURE);
//*               if (yutu_if_send_message(err_buff, SERVER) !=
//* SUCCESS) yutu_fwap_store_message(err_buff, SERVER); return;
//*             }
//*           uint8_t resp_buff[BUFF_SIZE];
//*           memset(resp_buff, '\0', BUFF_SIZE);
//*           yutu_fwap_create_ack_packet(resp_buff,
//* REGISTRATION_OPCODE); if (yutu_if_send_message(resp_buff,
//* SERVER)
//* != SUCCESS) yutu_fwap_store_message(resp_buff, SERVER);
//*           encryption_key_arrived = true;
//*           return;
//*         }
//*           if (encryption_key_arrived)
//*         {
//*           sleep_on_time =
//*               SearchToken(&req_packet_parsed,
//* "\"SleepOnTime\""); if (!sleep_on_time)
//*             {
//*               uint8_t err_buff[BUFF_SIZE];
//*               memset(err_buff, '\0', BUFF_SIZE);
//*               yutu_fwap_create_error_packet(err_buff,
//*                             SLEEP_ON_TIME_NOT_FOUND);
//*               if (yutu_if_send_message(err_buff, SERVER) !=
//* SUCCESS) yutu_fwap_store_message(err_buff, SERVER); return;
//*             }
//*           verify_unlock =
//*               SearchToken(&req_packet_parsed,
//* "\"VerifyUnlock\""); if (!verify_unlock)
//*             {
//*               uint8_t err_buff[BUFF_SIZE];
//*               memset(err_buff, '\0', BUFF_SIZE);
//*               yutu_fwap_create_error_packet(err_buff,
//*                             VERIFY_UNLOCK_NOT_FOUND);
//*               if (yutu_if_send_message(err_buff, SERVER) !=
//* SUCCESS) yutu_fwap_store_message(err_buff, SERVER); return;
//*             }
//*           gps_on_off = SearchToken(&req_packet_parsed,
//* "\"GPSOnOff\""); if (!gps_on_off)
//*             {
//*               uint8_t err_buff[BUFF_SIZE];
//*               memset(err_buff, '\0', BUFF_SIZE);
//*               yutu_fwap_create_error_packet(err_buff,
//*                             GPS_ON_OFF_NOT_FOUND);
//*               if (yutu_if_send_message(err_buff, SERVER) !=
//* SUCCESS) yutu_fwap_store_message(err_buff, SERVER); return;
//*             }
//*           report_packet_interval =
//*               SearchToken(&req_packet_parsed,
//* "\"ReportingInterval\""); if (!report_packet_interval)
//*             {
//*               uint8_t err_buff[BUFF_SIZE];
//*               memset(err_buff, '\0', BUFF_SIZE);
//*               yutu_fwap_create_error_packet(
//*               err_buff, REPORTING_INTERVAL_NOT_FOUND);
//*               if (yutu_if_send_message(err_buff, SERVER) !=
//* SUCCESS) yutu_fwap_store_message(err_buff, SERVER); return;
//*             }
//*           config_params cfg;
//*           cfg.report_packet_interval =
//*               atoi(report_packet_interval->jsonValue);
//*           cfg.sleep_on_time = atoi(sleep_on_time->jsonValue);
//*           cfg.verify_unlock = atoi(verify_unlock->jsonValue);
//*           cfg.gps_on_off = atoi(gps_on_off->jsonValue);
//*           if (yutu_fw_write_secure_flash((uint8_t*)&cfg,
//*                          CFG_MEM_ADDR) != SUCCESS)
//*             {
//*               uint8_t err_buff[BUFF_SIZE];
//*               memset(err_buff, '\0', BUFF_SIZE);
//*               yutu_fwap_create_error_packet(err_buff,
//*                             MEMORY_WRITE_FAILURE);
//*               if (yutu_if_send_message(err_buff, SERVER) !=
//* SUCCESS) yutu_fwap_store_message(err_buff, SERVER); return;
//*             }
//*           uint8_t resp_buff[BUFF_SIZE];
//*           memset(resp_buff, '\0', BUFF_SIZE);
//*           yutu_fwap_create_ack_packet(resp_buff,
//* REGISTRATION_OPCODE); if (yutu_if_send_message(resp_buff,
//* SERVER)
//* != SUCCESS) yutu_fwap_store_message(resp_buff, SERVER);
//*         }
//*           else
//*         {
//*           uint8_t err_buff[BUFF_SIZE];
//*           memset(err_buff, '\0', BUFF_SIZE);
//*           yutu_fwap_create_error_packet(err_buff,
//*                         ENCRYPTION_KEY_NOT_FOUND);
//*           if (yutu_if_send_message(err_buff, SERVER) !=
//* SUCCESS) yutu_fwap_store_message(err_buff, SERVER);
//*         }
//*         }
//*         break;
//*/
// case SVR_GET_SYNC_PACKET:
//{
// uint8_t rep_buff[BUFF_SIZE];
// memset(rep_buff, '\0', BUFF_SIZE);
//
// yutu_fwap_create_report_packet(rep_buff);
// result res = yutu_fw_server_send_message((uint8_t*)rep_buff);
//}
// break;
// case MOB_SVR_UNLOCK_REQ:
// case SVR_UNLOCK_REQ:
//{
//// compare old unlock key
//// if match, then unlock, send unlock and report packet.
//// else report unauth unlock to server.
////
// struct jsonReadToken* jsonPacket;
// jsonPacket = SearchToken(&req_packet_parsed, "\"oldUnlockKey\"");
// if (!jsonPacket)
//{
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(err_buff, devID, opcode,
// UNLOCK_KEY_NOT_FOUND);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
// return;
//}
// if (strlen(jsonPacket->jsonValue) != 32)
//{
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(err_buff, devID, opcode,
// INVALID_OLD_UNLOCK_KEY);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
// return;
//}
// uint8_t* device_old_unlck_key = yutu_fw_get_old_unlock_key();
//
// if (strcmp((char*)device_old_unlck_key, jsonPacket->jsonValue) !=
// 0)
//{
//// TODO : Hack attempt. report to server.
//}
// jsonPacket = SearchToken(&req_packet_parsed, "\"newUnlockKey\"");
// if (!jsonPacket)
//{
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(err_buff, devID, opcode,
// UNLOCK_KEY_NOT_FOUND);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
// return;
//}
// if (strlen(jsonPacket->jsonValue) != 32)
//{
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(err_buff, devID, opcode,
// INVALID_NEW_UNLOCK_KEY);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
// return;
//}
// yutu_fw_set_old_unlock_key((uint8_t*)jsonPacket->jsonValue);
// if (yutu_fw_unlock_device() == SUCCESS)
//{
// uint8_t unlck_buff[BUFF_SIZE];
// memset(unlck_buff, '\0', BUFF_SIZE);
// if (atoi(req_json_packet_opcode->jsonValue) ==
// MOB_SVR_UNLOCK_REQ)  // NOTE : device initiated means
//// mobile
// yutu_fwap_create_unlock_packet(unlck_buff, MOBILE_UNLCK);
// else
// yutu_fwap_create_unlock_packet(unlck_buff, REMOTE_UNLCK);
// yutu_fw_server_send_message((uint8_t*)unlck_buff);
//
// if (atoi(req_json_packet_opcode->jsonValue) ==
// MOB_SVR_UNLOCK_REQ)  // NOTE : mobile initiated
//{
// uint8_t resp_buff[BUFF_SIZE];
// memset(resp_buff, '\0', BUFF_SIZE);
// char* date_time = yutu_fw_get_date_time();
// uint8_t* devID = NULL;
// if (yutu_fw_read_secure_flash((uint8_t*)devID,
// DEVICE_ID_ADDR) != SUCCESS)
//{
//// TODO : error
//}
//
// struct jWriteControl unlck_req;
// jwOpen(&unlck_req, (char*)resp_buff, BUFF_SIZE, JW_OBJECT,
// JW_COMPACT);
// jwObj_string(&unlck_req, "deviceID", (char*)devID);
// jwObj_int(&unlck_req, "opCode", MOB_UNLOCK_REQ);
// jwObj_string(&unlck_req, "dateTime", date_time);
// jwObj_int(&unlck_req, "result", SUCCESS);
// jwObj_int(&unlck_req, "type", RESPONSE);
// uint8_t batt_percent = yutu_fw_get_battery_percentage();
// jwObj_int(&unlck_req, "batteryPercentage", batt_percent);
// lock_status ls = yutu_fw_get_lock_status();
// jwObj_int(&unlck_req, "lockStatus", ls);
// uint8_t dev_temp = yutu_fw_get_device_temperature();
// jwObj_int(&unlck_req, "deviceTemp", dev_temp);
// jwEnd(&unlck_req);
// yutu_fw_mobile_send_message((uint8_t*)resp_buff);
//}
//}
// else
//{
// char resp_buff[BUFF_SIZE];
// struct jWriteControl unlck_req;
// jwOpen(&unlck_req, resp_buff, BUFF_SIZE, JW_OBJECT,
// JW_COMPACT);
// jwObj_string(&unlck_req, "deviceID", (char*)devID);
// jwObj_int(&unlck_req, "opCode", MOB_UNLOCK_REQ);
// char* date_time = yutu_fw_get_date_time();
// jwObj_string(&unlck_req, "dateTime", date_time);
// jwObj_int(&unlck_req, "result", FAILURE);
// jwObj_int(&unlck_req, "type", RESPONSE);
// uint8_t batt_percent = yutu_fw_get_battery_percentage();
// jwObj_int(&unlck_req, "batteryPercentage", batt_percent);
// lock_status ls = yutu_fw_get_lock_status();
// jwObj_int(&unlck_req, "lockStatus", ls);
// uint8_t dev_temp = yutu_fw_get_device_temperature();
// jwObj_int(&unlck_req, "deviceTemp", dev_temp);
// jwEnd(&unlck_req);
// yutu_fw_mobile_send_message((uint8_t*)resp_buff);
//
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(err_buff, devID, opcode,
// UNLOCK_FAILED);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
//}
//}
// break;
// case SVR_LOCK_REQ:
//{
// if (yutu_fw_lock_device() != SUCCESS)
//{
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(err_buff, devID, opcode,
// LOCK_FAILED);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
// return;
//}
// uint8_t resp_buff[BUFF_SIZE];
// memset(resp_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_lock_report_packet(resp_buff, REMOTE_LCK, NULL);
// result res = yutu_fw_server_send_message((uint8_t*)resp_buff);
///*   TODO : no need to send to mobile here since it is server
//* area here.
//*           // send success/fail to mobile
//*           uint8_t mob_resp_buff[BUFF_SIZE];
//*           memset(mob_resp_buff, '\0', BUFF_SIZE);
//*           char* date_time = yutu_fw_get_date_time();
//*           uint8_t* devID = NULL;
//*           if (yutu_fw_read_secure_flash(devID,
//* DEVICE_ID_ADDR)
//* != SUCCESS)
//*         {
//*           // TODO : handle error
//*         }
//*
//*           struct jWriteControl unlck_req;
//*           jwOpen(&unlck_req, (char*)mob_resp_buff, BUFF_SIZE,
//* JW_OBJECT, JW_COMPACT); jwObj_string(&unlck_req, "deviceID",
//* (char*)devID); jwObj_int(&unlck_req, "opCode", MOB_LOCK_REQ);
//*           jwObj_string(&unlck_req, "dateTime", date_time);
//*           jwObj_int(&unlck_req, "result", LOCK_SUCCESS);
//*           jwObj_int(&unlck_req, "type", 0);
//*           jwEnd(&unlck_req);
//*           if (yutu_if_send_message(mob_resp_buff, MOBILE) !=
//* SUCCESS) yutu_fwap_store_message(mob_resp_buff, MOBILE);
//*/
//}
// break;
// default:
//{
//// TODO : INVALID_OPCODE
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(err_buff, devID, SVR_INVALID_OPCODE,
// FAILURE);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
//}
// break;
//}
//}
// else
//{
//// TODO :  json parse error
// char err_buff[BUFF_SIZE];
// memset(err_buff, '\0', BUFF_SIZE);
// yutu_fwap_create_error_packet(err_buff, NULL, 0, INVALID_JSON);
// result res = yutu_fw_server_send_message((uint8_t*)err_buff);
// return;
//}
//}

/*
 * @owner       Mohan
 * @brief       alarms like, temperature, low battery, tamper etc handler
 * @Called by   firmware layer
 * @param[out]  None
 * @param[in]   data to be passed to callback
 * @return      None
 */
void yutu_fwap_alarms_handler(void* data) {}
/*
 * @owner       Mohan
 * @brief       gps data ready handler. when gps data ready, report packet
 * is generated and sent to the server.
 * @Called by   firmware layer
 * @param[out]  None
 * @param[in]   data to be passed to callback
 * @return      None
 */

void yutu_fwap_create_error_packet(char* outdata, char* deviceID,
				   uint8_t opcode, uint8_t error_code)
{
  char* date_time = yutu_fw_get_date_time();
  struct jWriteControl error_packet;
  jwOpen(&error_packet, (char*)outdata, BUFF_SIZE, JW_OBJECT, JW_COMPACT);
  jwObj_string(&error_packet, "deviceID", deviceID);
  jwObj_int(&error_packet, "opCode", opcode);
  jwObj_string(&error_packet, "dateTime", date_time);
  jwObj_int(&error_packet, "type", 3);
  jwObj_int(&error_packet, "errorCode", error_code);
  jwEnd(&error_packet);
}

/*
 * @owner       Mohan
 * @brief       creates Report Data returns handle to buffer
 * @Called by   yutu_fwap_report_packet_handler
 * @param[out]  None
 * @param[in]   None
 * @return     handle to report data buffer.
 * @ DLD Reference:  DLD for IOT Lock Device-L11-3.1
 * @ Tracebility : Section 4.11.1, 4.11.3, 4.5.5, 4.5.6
 */
void yutu_fwap_create_report_packet(uint8_t* outdata)
{
  struct jWriteControl report_packet;
  jwOpen(&report_packet, (char*)outdata, BUFF_SIZE, JW_OBJECT, JW_COMPACT);
  uint8_t* devID = NULL;
  if (yutu_fw_read_secure_flash((uint8_t*)devID, DEVICE_ID_ADDR) != SUCCESS)
    {
      // TODO : handle error
    }
  jwObj_string(&report_packet, "DeviceID", (char*)devID);
  char* date_time = yutu_fw_get_date_time();
  jwObj_string(&report_packet, "DateTime", date_time);
  jwObj_int(&report_packet, "OpCode", DEV_REPORT_PACKET);
  jwObj_int(&report_packet, "Type", 1);
  char* asset_name = yutu_fw_get_asset_name();
  jwObj_string(&report_packet, "asset_name", asset_name);
  jwObj_string(&report_packet, "client", "YuTu");
  jwObj_object(&report_packet, "location");
  jwObj_string(&report_packet, "type", "Point");
  jwObj_array(&report_packet, "coordinates");
  char* latitude = yutu_fw_get_latitude();
  jwArr_string(&report_packet, latitude);
  char* longitude = yutu_fw_get_longitude();
  jwArr_string(&report_packet, longitude);
  jwEnd(&report_packet);
  jwEnd(&report_packet);
  jwObj_string(&report_packet, "device_type", "LockTracking");
  int temp = yutu_fw_get_device_temperature();
  jwObj_int(&report_packet, "device temperature", temp);
  int lock_status = yutu_fw_get_lock_status();
  jwObj_int(&report_packet, "lockstatus", lock_status);
  char* open_status = yutu_fw_get_open_status();
  jwObj_string(&report_packet, "isOpen", open_status);
  long time = yutu_fw_get_time();
  jwObj_int(&report_packet, "timestamp", time);
  char* tamper = yutu_fw_get_tamper_status();
  jwObj_string(&report_packet, "tampered", tamper);
  char* devkey = yutu_fw_get_devicekey();
  jwObj_string(&report_packet, "device_key", devkey);
  char* speed = yutu_fw_get_currspeed();
  jwObj_string(&report_packet, "Current speed", speed);
  int shackle_status = yutu_fw_get_shackle_status();
  jwObj_int(&report_packet, "shackelstatus", shackle_status);
  char* devno = yutu_fw_get_device_no();
  jwObj_string(&report_packet, "device_no", devno);
  jwEnd(&report_packet);
}

/*
 * @owner       Mohan
 * @brief       creates unlock Report Data and return handle to buffer.
 * @Called by   yutu_fwap_request_handler
 * @param[out]  None
 * @param[in]   None
 * @return      Handle to unlock report packet.
 * @ DLD Reference:  DLD for IOT Lock Device-L11-3.1
 * @ Tracebility : Section 4.5.7
 */
void yutu_fwap_create_unlock_packet(uint8_t* outdata, uint8_t unlock_type)
{
  struct jWriteControl unlck_report_data;
  jwOpen(&unlck_report_data, (char*)outdata, BUFF_SIZE, JW_OBJECT, JW_COMPACT);
  uint8_t* devID = NULL;
  if (yutu_fw_read_secure_flash((uint8_t*)devID, DEVICE_ID_ADDR) != SUCCESS)
    {
      // TODO : handle error
      return;
    }
  jwObj_string(&unlck_report_data, "deviceID", (char*)devID);
  char* date_time = yutu_fw_get_date_time();
  jwObj_string(&unlck_report_data, "dateTime", date_time);
  jwObj_int(&unlck_report_data, "opCode", DEV_UNLOCK_PACKET);
  jwObj_int(&unlck_report_data, "unlockMethod", unlock_type);
  jwObj_object(&unlck_report_data, "deviceLocation");
  jwObj_string(&unlck_report_data, "latitude", yutu_fw_get_latitude());
  jwObj_string(&unlck_report_data, "longitude", yutu_fw_get_longitude());
  jwEnd(&unlck_report_data);
  jwObj_int(&unlck_report_data, "type", ONE_WAY);
  jwEnd(&unlck_report_data);
}

/*
 * @owner       Mohan
 * @brief       creates Report Data and sends to Server
 * @Called by   firmware layer
 * @param[out]  None
 * @param[in]   data to be passed to callback
 * @return      None
 * @ DLD Reference:  DLD for IOT Lock Device-L11-3.1
 * @ Tracebility : Section 4.11.1
 */
void yutu_fwap_report_packet_handler(void* data)
{
  uint8_t rep_buff[BUFF_SIZE];
  memset(rep_buff, '\0', BUFF_SIZE);

  yutu_fwap_create_report_packet(rep_buff);
  yutu_fw_server_send_message((uint8_t*)rep_buff);
}

/*
 * @owner       Mohan
 * @brief       creates lock Report packet and sends to Server
 * @Called by   yutu_fwap_request_handler
 * @param[out]  outdata
 * @param[in]   None
 * @return      None
 * @ DLD Reference:  DLD for IOT Lock Device-L11-3.1
 * @ Tracebility : Section 4.11.1
 */

void yutu_fwap_create_lock_report_packet(uint8_t* outdata,
					 lock_method lock_type, char* mobile_no)
{
  struct jWriteControl report_packet;
  jwOpen(&report_packet, (char*)outdata, BUFF_SIZE, JW_OBJECT, JW_COMPACT);
  uint8_t* devID = NULL;
  if (yutu_fw_read_secure_flash((uint8_t*)devID, DEVICE_ID_ADDR) != SUCCESS)
    {
      // TODO : handle error
    }
  jwObj_string(&report_packet, "deviceID", (char*)devID);
  char* date_time = yutu_fw_get_date_time();
  jwObj_string(&report_packet, "dateTime", date_time);
  jwObj_int(&report_packet, "opCode", LOCK_REPORT_PACKET);
  jwObj_int(&report_packet, "lockMethod", lock_type);
  jwObj_int(&report_packet, "type", ONE_WAY);
  jwObj_string(&report_packet, "mobileNo", mobile_no);
  jwEnd(&report_packet);
}

/*
 * @owner       Mohan
 * @brief       button press event handler
 * @Called by   firmware layer
 * @param[out]  None
 * @param[in]   data to be passed to callback
 * @return      None
 * @ DLD Reference:  DLD for IOT Lock Device-L11-3.1
 * @ Tracebility : Section 4.9.3
 */
void yutu_fwap_buttonpress_handler(void* data)
{
  //
  key_press_type* press_type = (key_press_type*)data;
  switch (*press_type)
    {
      case SINGLE_PRESS:  // TODO : ??
	{
	}
	break;
      case DOUBLE_PRESS:  // TODO : ??
	{
	}
	break;
      case LONG_PRESS:
	{
	  if (yutu_fw_lock_device() != SUCCESS)
	    {
	      char err_buff[BUFF_SIZE];
	      memset(err_buff, '\0', BUFF_SIZE);
	      uint8_t* devID = NULL;
	      if (yutu_fw_read_secure_flash((uint8_t*)devID, DEVICE_ID_ADDR) !=
		  SUCCESS)
		{
		  // TODO : handle error
		}
	      yutu_fwap_create_error_packet(err_buff, (char*)devID, BUTTON_LOCK,
					    LOCKING_FAILED);
	      yutu_fw_server_send_message((uint8_t*)err_buff);
	      return;
	    }
	  uint8_t resp_buff[BUFF_SIZE];
	  memset(resp_buff, '\0', BUFF_SIZE);
	  yutu_fwap_create_lock_report_packet(resp_buff, BUTTON_LOCK, NULL);
	  yutu_fw_server_send_message((uint8_t*)resp_buff);
	}
	break;
      case UNLOCK_PRESS:
	{
	  if (yutu_fw_unlock_device() == SUCCESS)
	    {
	      // if send failed, then store the packet in some location.
	      uint8_t unlck_buff[BUFF_SIZE];
	      memset(unlck_buff, '\0', BUFF_SIZE);

	      yutu_fwap_create_unlock_packet(unlck_buff, BUTTON_UNLCK);
	      yutu_fw_server_send_message((uint8_t*)unlck_buff);
	    }
	}
	break;
      default:
	break;
    }
}

// all these are if layer APIs.
void yutu_fw_mobile_message_handler(BT_Receive_Callback arg,
				    BTProfile* bt_profile, char* btHostName,
				    char* btRcvBuffer)
{
  machineState&=0xFFDF;
  memset(bt_profile->hostName, '\0', sizeof(bt_profile->hostName));
  strcpy((char*)bt_profile->hostName, btHostName);
  bt_profile->btCallback = arg;
  bt_profile->btStatus = BT_UNPAIRED;
  bt_profile->btMsgFrame = btRcvBuffer;
}
// void yutu_fw_server_message_handler(message_callback arg) {}
// result yutu_fw_mobile_send_message(uint8_t* buff) { return SUCCESS; }
// result yutu_fw_server_send_message(uint8_t* buff) { return SUCCESS; }

// end if API

/*
 * @owner       Sijeo
 * @brief       Json print test routine.
 * @Called by   main().
 * @param[out]  None
 * @param[in]   None.
 * @return      void
 */
void jWriteTest()
{
  char buffer[1024];
  unsigned int buflen = 1024;
  int err;

  // printf("\rA JSON object example :\r\n");

  jwOpen(&testJSON, buffer, buflen, JW_OBJECT, JW_COMPACT);

  jwObj_string(&testJSON, "key", "value");
  jwObj_int(&testJSON, "int", 10);
  jwObj_double(&testJSON, "double", 1.265487);
  jwObj_null(&testJSON, "null Thing");
  jwObj_bool(&testJSON, "boolean", 1);
  jwObj_array(&testJSON, "EmptyArray");
  jwEnd(&testJSON);
  jwObj_array(&testJSON, "anArray");
  jwArr_string(&testJSON, "array one");
  jwArr_int(&testJSON, -1);
  jwArr_double(&testJSON, 12.568970);
  jwArr_null(&testJSON);
  jwArr_bool(&testJSON, 0);
  jwArr_object(&testJSON);
  jwObj_string(&testJSON, "objArr1", "value1");
  jwObj_string(&testJSON, "objArr2", "value2");
  jwEnd(&testJSON);
  jwArr_array(&testJSON);
  jwArr_int(&testJSON, 1);
  jwArr_int(&testJSON, 2);
  jwArr_int(&testJSON, 3);
  jwEnd(&testJSON);
  jwEnd(&testJSON);
  jwObj_object(&testJSON, "Empty Object");
  jwEnd(&testJSON);
  jwObj_object(&testJSON, "anObject");
  jwObj_string(&testJSON, "msg", "Object in Object");
  jwObj_string(&testJSON, "msg2", "Object in object 2nd");
  jwEnd(&testJSON);
  jwObj_string(&testJSON, "ObjectEntry", "This is the last one");
  jwClose(&testJSON);

  // printf(buffer);
  // printf(buffer1);
  // printf("\r\n\r\n\r\n");
  err = JSONStructInit(&testReadJSON, buffer);
  // printf("\nThe number of tokens parsed %d \n",
  // testReadJSON.numTokens); printf("Buffer initialized for reading \n
  // "); printf("The value of error is %d \n", err);
  if (err != JDATA_NOK)
    {
      while (testReadJSON.parserStatus != JSON_END)
	err = JSONParseData(&testReadJSON);
      // printf("The Error code is %d\n", err);
      // printf("The Number of Tokens parsed are %d\n",
      // testReadJSON.numTokens); printf("The value of key parsed is
      // %s\n", testReadJSON.jData[2].keyValue); printf("The value of data
      // parsed is %s\n", testReadJSON.jData[5].jsonValue); printf("The
      // Value of Stack Position is %d", testReadJSON.parserStatus);
      jsonPacket = SearchToken(&testReadJSON, "\"anObject\"");
      if (jsonPacket != 0)
	{
	  // printf("Packet Found!!!!\n");
	  // printf("The Value of the entered key is %s\n",
	  // jsonPacket->jsonValue);
	}
    }
}

char* yutu_fw_get_asset_name(void) { return "YUTU01-001"; }
char* yutu_fw_get_latitude(void) { return "78.4750203"; }
char* yutu_fw_get_longitude(void) { return "17.4172617"; }
uint8_t yutu_fw_get_device_temperature(void) { return 21; }
uint8_t yutu_fw_get_battery_percentage(void) { return 21; }
char* yutu_fw_get_open_status(void) { return "no"; }
long yutu_fw_get_time(void) { return 1545108774; }
char* yutu_fw_get_tamper_status(void) { return "yes"; }
char* yutu_fw_get_devicekey(void) { return "IHPX2LQDKZAT6LGUUW4U"; }
char* yutu_fw_get_currspeed(void) { return "35"; }
int yutu_fw_get_shackle_status(void) { return SHACKLE_IN; }
char* yutu_fw_get_device_no(void) { return "YUTU01"; }
char* yutu_fw_get_old_unlock_key() { return "33333333"; }
void yutu_fw_set_old_unlock_key(uint8_t* old_key) {}

void yutu_fw_get_unlck_auth_numbers(uint8_t mem_locn,
				    unlck_auth_numbers* unlck_no)
{
  auth_numbers.admin_auth_number = 2222222222;
  auth_numbers.supervisor_auth_number = 2222222222;
  auth_numbers.operator_auth_number = 2222222222;
  auth_numbers.client_auth_number = 3333333333;
  unlck_no = &auth_numbers;
}
result yutu_fw_unlock_device(void) { return SUCCESS; }
result yutu_fw_lock_device(void) { return SUCCESS; }
int yutu_fw_get_lock_status(void) { return LOCKED; }
result yutu_fw_mobile_send_message(uint8_t* buff) { ; }
result yutu_fw_erase_secure_flash(int location) { return SUCCESS; }
result yutu_fw_read_secure_flash(uint8_t* data, int location)
{
  return SUCCESS;
}
result yutu_fw_write_secure_flash(uint8_t* data, int location)
{
  return SUCCESS;
}
result yutu_fw_server_send_message(uint8_t* buff) { ; }
