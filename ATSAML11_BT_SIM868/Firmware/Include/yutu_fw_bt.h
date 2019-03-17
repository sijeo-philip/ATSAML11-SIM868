#ifndef __YUTU_FW_BT_H__
#define __YUTU_FW_BT_H__
#include <stdbool.h>
#include <stdint.h>

typedef enum {BT_UNPAIRED=-1, BT_PAIRING,
	 BT_CONNECTING, 
	 BT_CONNECT, 
	 BT_HFPDISCONN, 
	 BT_SPPCONNECT, 
	 BT_SPPCONNECTED,
	  BTDISCONNECTED,
	  BT_PAIRED,
	  BT_HFPDISCONNECTED,
	   }BT_Status;

//typedef struct {
	//char deviceID[4];
	//char bytes[4];
	//char message[150];
//}BT_MSG_FRAME;

typedef void(*BT_Receive_Callback)(char*, char*);

typedef struct {
	BT_Status btStatus;
	uint8_t connectedDevicesCount;
	uint8_t hostName[20];
	uint8_t passCode[10];
	char connectDeviceHFPID[4];
	BT_Receive_Callback btCallback;
	char* btMsgFrame;
}	BTProfile;

/**********************************************************************************
* @Function 		: 	bt_init
* @Param				:	  [in] BTProfile*
								  	[out] uint8_t 0 for failure to initialize and 1 for success
*	@Description	: This function sets the hostname to the device and 
**********************************************************************************/
bool yutu_fw_bt_init(void);


/**********************************************************************************
* @Function : bt_power_on
* @Params		: [in] None
*							[out] uint8_t 1 to failure to connect and 0 for success
* @Description : This function power on
*************************************************************************************/
bool yutu_fw_bt_power_on(void);

/**********************************************************************************
* @Function : bt_power_off
* @Params		: [in] None
*							[out] uint8_t 1 to failure to connect and 0 for success
* @Description : This function power on
*************************************************************************************/
bool yutu_fw_bt_power_off(void);


/**********************************************************************************
* @Function : bt_disconnect
* @Params		: None
*							[out] uint8_t 1 to failure to disconnect and 0 for success
* @Description : This function is used to disconnect already paired device
*************************************************************************************/
bool yutu_fw_disconnect_bt(char* connectionNo);

/**********************************************************************************
* @Function : bt_send_data
* @Params		: [in] device id from pointer to the structure
							[in] uint8_t* data to be send to paired device
							[in] uint32_t bytes number of bytes to be send
*							[out] uint8_t 1 to failure to connect and 0 for success
* @Description : This function is used to send data to the connected device
*************************************************************************************/
bool yutu_fw_bt_send_data(char*, uint32_t);

/**********************************************************************************
* @Function : get_bt_Devices
* @Params		: [in] None 
*							[out] BTProfile * 
* @Description : This function gets the list of connected devices and paired devices 
								 updates the BTProfile Structure and returns the pointer to it.
*************************************************************************************/
//BTProfile* yutu_fw_get_bt_devices(uint8_t);


bool yutu_fw_unpair_bt(void);



bool yutu_fw_connect_to_bt(void);

#endif