

#ifndef _YUTU_FW_GSM_GPRS_H_
#define _YUTU_FW_GSM_GPRS_H_

#include "yutu_fw_at_command_exec.h"

typedef struct 
{
	char apn[25];
	char user[10];
	char password[10];
	char ip_address[25];
	uint8_t signal_strength;
	uint8_t cellid;
	char sms_mode;
	uint16_t smsCount;
}gsm_gprs_config_t;


typedef struct
{
	char serverIP[25];
	char serverPort[10];
}gsm_gprs_server_credentials_t;

extern gsm_gprs_config_t gprsConfig;
/*******************************************************************************************************
@Function 	:	yutu_fw_gsm_gprs_init
@Input		:	None
@Output		:	None
@Return		: This function checks the presence of SIM, network registration and Module connectivity
*********************************************************************************************************/
bool yutu_fw_gsm_gprs_init(void);

/***********************************************************************************************
*@Function 			:	  gprs_on
*@Input					:  [in] pointer to GPRS configurations  
*@Output				:		None
*@Return				:		bool
*@Description 	:   This function will connect the device internet through GPRS and 
											return true on succesful connection
*************************************************************************************************/
bool yutu_fw_gprs_on(gsm_gprs_config_t*);

/***********************************************************************************************
*@Function 			:	  gprs_off
*@Input					:  None 
*@Output				:	 None
*@Return				:	 bool.
*@Description 	:   This function will disconnect if the system is connected to GPRS
*************************************************************************************************/
bool yutu_fw_gprs_off(void);

/***********************************************************************************************
*@Function			:	  gprs_set_apn
*@Input					:   grps_config_t* configuration structure
									  const uint8_t* value to be stored as APN in the system
*@Return				:	  bool return true on successful apn setting  and false on failure .
*@Description 		:   This function will set the apn to the network for GPRS connectivity
*************************************************************************************************/
bool yutu_fw_gprs_set_apn(const char*, gsm_gprs_config_t*);

 /************************************************************************************************
* @Function			: 	set_server_ip
 @Input					:   char* The pointer to IP Address of the Server to be connected
										gsm_gprs_config_t*
 @Return				:		bool returns true on success and false otherwise
 @Description		:	  This function takes the IP address of the Server from the User and stores in 
											nwParams structure for further use.
 ************************************************************************************************/
bool yutu_fw_set_server_ip(char*, gsm_gprs_server_credentials_t*);

 /************************************************************************************************
* @Function			: 	set_server_port
 @Input					:   char* The pointer to port of the Server to be connected
										gsm_gprs_config_t*
@Return				:	bool returns true on success and false otherwise
 @Description		:	  This function takes the PORT of the Server from the User and stores in 
											nwParams structure for further use.
 ************************************************************************************************/
bool yutu_fw_set_server_port(char*, gsm_gprs_server_credentials_t*);

//uint8_t yutu_fw_register_server_response_callback(urc_callback);
/************************************************************************************************
* @Function			: 	check_server_status
 @Input					:   None
 @Return				: 	bool --> returns 1 if the device is not connected else returns 0
 @Description		:	  This function checks if the TCP session is already connected if yes it will return 
										true else return false
 ************************************************************************************************/
bool yutu_fw_check_server_status(void);

/************************************************************************************************
* @Function			: 	connect_server
 @Input					:   None
 @Return				: 	bool --> returns 1 if the device is not connected else returns 0
 @Description		:	  This function connects to the Server and returns true on successful connection
 ************************************************************************************************/
bool yutu_fw_connect_server(gsm_gprs_server_credentials_t*);

/************************************************************************************************
* @Function			: 	connection_close
 @Input					:   None
 @Return				: 	bool --> returns true if the connection is closed successfully else false
 @Description		:	  This function disconnects to the Server and returns true if successful
 ************************************************************************************************/
bool yutu_fw_server_connection_close(void);

 /************************************************************************************************
* @Function 			: 	send_gprs_data
 @Inputs					:   char* --> pointer to the string to be sent
											uint16_t --> No.of bytes to be send over the Server
 @Return							bool --> returns true if the data is send else returns false
 @Description			:	  This function sends the DATA over established TCP connection to the Server
 ************************************************************************************************/
bool yutu_fw_send_gprs_data(void);


#endif