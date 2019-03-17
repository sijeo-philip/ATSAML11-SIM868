#include "yutu_fw_gsm_gprs.h"
#include "yutu_fw_at_command_exec.h"
#include <string.h>
#include <stdint.h>
#include "common.h"


extern atcommand_exec_urc_queue_struct_t urcEntityQueue;
volatile extern atcommand_exec_atcmd_queue_t cmdQueue;
//volatile uint32_t task1Counter=0;
volatile unsigned short machineState=0;
gsm_gprs_config_t gprsConfig;
char* rspRxPtr;
volatile gsm_gprs_config_t gsmGPRSConfig;

/*************************************EXTERN CALLBACK FUCNTION**********************************************************/
//extern atcommand_exec_rsp_t atcommand_exec_cpin_status_callback(char* rsp);
//extern atcommand_exec_rsp_t atcommand_exec_creg_status_callback(char* rsp);
//extern atcommand_exec_rsp_t atcommand_exec_get_ipaddr_callback(char* rsp);
//extern atcommand_exec_rsp_t atcommand_exec_server_connect_callback(char* rsp);
//extern atcommand_exec_rsp_t atcommand_exec_send_gprs_packet_callback(char* rsp);
//extern int mqtt_response_handler(char* buffer, uint16_t len);
/************************************************************************************************************************/
static //Initialize the URC Queue
void yutu_fw_urc_queue_init(void)
{
	uint8_t i = 0;
  urcEntityQueue.urcQueueCount = 0;
	for(i=0;i<10;i++)
	{
		memset(urcEntityQueue.urcQueue[i].urcStr, '\0', sizeof(urcEntityQueue.urcQueue[i].urcStr));
		urcEntityQueue.urcQueue[i].urcCallback = NULL;
	}
	
}



static //Add URCs to the queue
atcommand_exec_queue_status_t yutu_fw_add_urc(atcommand_exec_urc_queue_t* urcQueue, uint8_t count)
{
	uint8_t i=0;
	yutu_fw_urc_queue_init();
	if(count>10)
		return QUEUE_ERROR;
	urcEntityQueue.urcQueueCount = count;
	for(i=0; i<count; i++)
	{
		strcpy((char*)urcEntityQueue.urcQueue[i].urcStr, (char*)urcQueue[i].urcStr);
		urcEntityQueue.urcQueue[i].urcCallback = urcQueue[i].urcCallback;
		
	}
	return QUEUE_OPER_OK;
}

//Callback for connection getting closed.
int set_server_connection_status_handler(char* buffer, uint16_t len)
{
	if(!strncmp(buffer, "CLOSED", 6)){
	machineState&=0xFFF7;
	return 1;
	}
	else
		return 0;
}

bool yutu_fw_gsm_gprs_init(void){
	
	volatile atcommand_exec_queue_status_t status= QUEUE_ERROR;
	volatile bool retVal =false;
	atcommand_exec_urc_queue_t urcQueue[10] ={ {"+CMTI: ", NULL},
	 {"RECV FROM:", mqtt_response_handler},
	 {"CLOSED",set_server_connection_status_handler},
	 {"+BTPAIR:",NULL},
	 {"SEND OK", yutu_fw_data_send_ack},
	 {"+BTPAIRING: ", yutu_fw_bt_pair},
	 {"+BTCONNECT:", yutu_fw_bt_connect },
	 {"+BTCONNECTING: ", yutu_fw_bt_profile_connect},	 
	 {"+BTDISCONN: ", yutu_fw_bt_disconnect},	   
	 {"+BTSPPDATA: ", yutu_fw_recv_bt_data},
	 
		  };
	atcommand_exec_at_command_t atCommand[5]={
		{"ATE0"ATCOMMAND_EXEC_CMD_END, NULL, 3, 3000, CMD1_OK}, 
		{"AT"ATCOMMAND_EXEC_CMD_END, NULL, 3, 3000, CMD2_OK},
		{"AT+CPIN?"ATCOMMAND_EXEC_CMD_END, atcommand_exec_cpin_status_callback, 3, 3000,CMD3_OK},
		{"AT+CREG?"ATCOMMAND_EXEC_CMD_END, atcommand_exec_creg_status_callback, 3, 3000, CMD4_OK},
	};
  atcommand_exec_at_cmd_queue_init();	
	yutu_fw_add_urc(urcQueue, 10);
	cmdQueue.cmdQueueWrite = true;
	status = atcommand_exec_add_at_cmd_queue(&atCommand[0], 4);
	cmdQueue.cmdQueueWrite = false;
	if(status == QUEUE_OPER_OK)
	{
		while(cmdQueue.commandCount!=0)
		{
		}
		if(cmdQueue.currentCmdStatus== CMD4_OK){
			machineState |= ATCOMMAND_EXEC_MDM_RDY;
			retVal= true; 
		}
		else 
			retVal =  false;
	}
	else
		retVal= false;
	atcommand_exec_at_cmd_queue_init();
	return retVal;
}

bool yutu_fw_gprs_set_apn(const char* apnValue, gsm_gprs_config_t* gprsgsmConfig)
{
	char *str;
	memset(gprsgsmConfig->apn, '\0', sizeof(gprsgsmConfig->apn));
	str = strcpy((char*)gprsgsmConfig->apn, (char*)apnValue);
	if(str !=NULL)
		return true;
	else
		return false;
	
}

//Connection to GPRS of Network

bool yutu_fw_gprs_on(gsm_gprs_config_t *gprsgsmConfig)
{
	volatile atcommand_exec_queue_status_t status;
	volatile bool retVal =false;
	char tempStr[30] = "AT+SAPBR=3,1,\"APN\",\"";
  atcommand_exec_at_command_t atCommand[6]= {
		{"AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""ATCOMMAND_EXEC_CMD_END, NULL, 3, 3000, CMD1_OK}, 
		{.rspFunc = NULL, .repCount=3, .rspDelay = 3000, .successStatus=CMD2_OK },
		{"AT+SAPBR=1,1"ATCOMMAND_EXEC_CMD_END, NULL, 3, 10000, CMD3_OK},
		{"AT+SAPBR=2,1"ATCOMMAND_EXEC_CMD_END, atcommand_exec_get_ipaddr_callback, 3, 3000, CMD4_OK},
	};
	strcat(tempStr, (const char*)gprsgsmConfig->apn);
	strcat(tempStr, "\""ATCOMMAND_EXEC_CMD_END);
	memset(atCommand[1].atCmd, '\0', sizeof(atCommand[1].atCmd));
	strcpy(atCommand[1].atCmd, tempStr);
	atcommand_exec_at_cmd_queue_init();	
	cmdQueue.cmdQueueWrite = true;
	status = atcommand_exec_add_at_cmd_queue(atCommand, 4);
	cmdQueue.cmdQueueWrite = false;
	if(status == QUEUE_OPER_OK)
	{
		while(cmdQueue.commandCount!=0)
		{
		}
		if(cmdQueue.currentCmdStatus==CMD4_OK)
		{
			machineState|=ATCOMMAND_EXEC_GPRS_RDY;
			retVal =true;
		}
		else
			 retVal=false;
	}
	else 
		retVal = false;
	atcommand_exec_at_cmd_queue_init();
	return retVal;
}

//Connect to the Server
bool yutu_fw_connect_server(gsm_gprs_server_credentials_t* serverProfile){
	volatile atcommand_exec_queue_status_t status;
	volatile bool retVal=false;
	atcommand_exec_at_command_t atCommand[2] = {
														{"AT+CIPSRIP=1"ATCOMMAND_EXEC_CMD_END, NULL, 3, 10000, CMD1_OK},
													{.rspFunc=atcommand_exec_server_connect_callback, .repCount=1, .rspDelay=60000,.successStatus=CMD2_OK},
													};
	strcpy(atCommand[1].atCmd, "AT+CIPSTART=\"TCP\",\"");
	strncat(atCommand[1].atCmd, serverProfile->serverIP, strlen(serverProfile->serverIP));
	strcat(atCommand[1].atCmd, "\",\"");
	strncat(atCommand[1].atCmd, serverProfile->serverPort, strlen(serverProfile->serverPort));
  strcat(atCommand[1].atCmd, "\""ATCOMMAND_EXEC_CMD_END);
  atcommand_exec_at_cmd_queue_init();	
	cmdQueue.cmdQueueWrite=true;
	status = atcommand_exec_add_at_cmd_queue(atCommand,2);
	cmdQueue.cmdQueueWrite=false;
	if(status == QUEUE_OPER_OK)
	{
		while(cmdQueue.commandCount!=0)
		{}
			
		if(cmdQueue.currentCmdStatus==CMD2_OK)
		{
			machineState|=ATCOMMAND_EXEC_SERVER_RDY;
			retVal=true;
		}
		else 
			retVal=false;
	}
	else
	retVal=false;
	atcommand_exec_at_cmd_queue_init();
	return retVal;
}

//Set Server IP and Port
bool yutu_fw_set_server_ip(char* ipAddress, gsm_gprs_server_credentials_t* serverProfile){
	char* tempPtr;
	memset(serverProfile->serverIP, '\0', sizeof(serverProfile->serverIP));
	tempPtr = strncpy(serverProfile->serverIP, ipAddress, strlen(ipAddress));
  if(tempPtr == NULL)
		return false;
	else
		return true;
	
}

bool yutu_fw_set_server_port(char* port, gsm_gprs_server_credentials_t* serverProfile){
	char *tempPtr;
	memset(serverProfile->serverPort, '\0', sizeof(serverProfile->serverPort));
	tempPtr = strncpy(serverProfile->serverPort, port, strlen(port));
	if(tempPtr == NULL)
		return false ;
	else
		return true;
	
}
//Send Data to the Conneceted Server

bool yutu_fw_send_gprs_data(void)
{
  volatile atcommand_exec_queue_status_t status;
	volatile bool retVal=false;
	atcommand_exec_at_command_t atCommand[2] = {{"AT+CIPSEND"ATCOMMAND_EXEC_CMD_END, atcommand_exec_send_gprs_packet_callback, 1, 60000,CMD1_OK},
														//{"DELAY", NULL, 1, 5000},
														};
	atcommand_exec_at_cmd_queue_init();														
  cmdQueue.cmdQueueWrite=true;												
	status = atcommand_exec_add_at_cmd_queue(atCommand, 1);
	cmdQueue.cmdQueueWrite=false;												
	if(status == QUEUE_OPER_OK)
	{
		while(cmdQueue.commandCount!=0)
		{}
			
		if(cmdQueue.currentCmdStatus==CMD1_OK)
		{
			retVal=true;
		}
		else 
			retVal=false;
	}
	else
		retVal=false;
	atcommand_exec_at_cmd_queue_init();
	return retVal;
	
}


