

#ifndef _YUTU_FW_AT_COMMAND_EXEC_H_
#define _YUTU_FW_AT_COMMAND_EXEC_H_

#include <sam.h>
#include <stdbool.h>
#include <stdint.h>

#define ATCOMMAND_EXEC_CMD_QUEUE  		10
#define ATCOMMAND_EXEC_URC_QUEUE		15	
#define ATCOMMAND_EXEC_CMD_BUFF_SIZE	50	
#define ATCOMMAND_EXEC_RX_BUFF_SIZE		400

#define ATCOMMAND_EXEC_OVERTIME				60000
#define ATCOMMAND_EXEC_CMD_END				"\x0d\x0a"
#define ATCOMMAND_EXEC_CMD_CR					'\x0d'
#define ATCOMMAND_EXEC_CMD_LF					'\x0a'
#define ATCOMMAND_EXEC_CTRL_Z					"\x1a"

#define ATCOMMAND_EXEC_MDM_RDY				1
#define ATCOMMAND_EXEC_GPRS_RDY				1<<1
#define ATCOMMAND_EXEC_BT_RDY				1<<2
#define ATCOMMAND_EXEC_SERVER_RDY			1<<3
#define ATCOMMAND_EXEC_MQTT_RDY				1<<4
#define ATCOMMAND_EXEC_BT_HOST_SET			1<<5
#define ATCOMMAND_EXEC_BT_PWR_RDY			1<<6

extern char responseRecvBuffer[ATCOMMAND_EXEC_RX_BUFF_SIZE];


typedef enum
{
	AT_RSP_ERROR=-1,
	AT_RSP_CONTINUE=0,					//Continue to execute the next AT Comma
	AT_RSP_FUN_OVER=1,
	AT_RSP_FINISH=2,
	AT_RSP_WAIT,
}atcommand_exec_rsp_t;

typedef enum{QUEUE_FULL, QUEUE_EMPTY, QUEUE_ERROR, QUEUE_OPER_OK}atcommand_exec_queue_status_t;

typedef enum{MODEM_IDLE=0, MODEM_RSP_WAIT, MODEM_EXECUTE_AT, MODEM_RSP_RECVD}atcommand_exec_modem_status_t;

typedef atcommand_exec_rsp_t(*atcommand_exec_rsp_callback)(char* rspStr);

typedef int(*atcommand_exec_urc_callback)(char* rspStr, uint16_t len);

typedef enum{NOK_OPER=0,
	CMD1_OK,
	CMD2_OK,
	CMD3_OK,
	CMD4_OK,
	CMD5_OK,
}at_command_exec_rsp_status_t;

//typedef short atcommand_exec_machine_state_t;
//typedef	struct {
		//bool MDM_RDY;
		//bool GPRS_RDY;
		//bool BT_RDY;
		//bool SERVER_RDY;
		//bool MQTT_SERVER_RDY;
		//bool BT_HOST_SET;
		//bool BT_PWR_RDY;
		//}atcommand_exec_machine_state_t;

typedef union{
	 struct{
		  uint8_t BT_DATA_RD_RDY:1;
		  uint8_t GPRS_DATA_RD_RDY:1;
		  uint8_t BT_DATA_WR_RDY:1;
		  uint8_t GPRS_DATA_WR_RDY:1;
		  uint8_t BT_DATA_SENT:1;
		  uint8_t GPRS_DATA_SENT:1;
		  uint8_t reserved:2;
		}event_bit;
        uint8_t EVENT_STATUS_BYTE;
}atcommand_exec_event_t;
	 
	 
typedef struct{
	char atCmd[ATCOMMAND_EXEC_CMD_BUFF_SIZE];
	atcommand_exec_rsp_callback rspFunc;
	uint8_t repCount;
	uint32_t rspDelay;
	at_command_exec_rsp_status_t successStatus;
}atcommand_exec_at_command_t;


typedef struct{
	atcommand_exec_at_command_t atCmdQueue[ATCOMMAND_EXEC_CMD_QUEUE];
	uint8_t firstCmd;								//The first command in the queue	
	uint8_t lastCmd;								//The last command in the queue to be executed
	uint8_t currentCmd;							//The current AT Command to be executed/under execution
	uint8_t repeatCount;						//No. of repeats left for the current command
	uint8_t commandCount;						//No. of commands to be executed.
	bool cmdQueueWrite;							//should be set to true when the AT commands are written to the queue
	atcommand_exec_rsp_t currentCmdRet;	// this contains the value of the current AT command processing return.
	at_command_exec_rsp_status_t currentCmdStatus;
}atcommand_exec_atcmd_queue_t;

typedef struct{
	char urcStr[30];
	atcommand_exec_urc_callback urcCallback;
}atcommand_exec_urc_queue_t;

typedef struct {
	atcommand_exec_urc_queue_t urcQueue[ATCOMMAND_EXEC_URC_QUEUE];
	uint16_t urcQueueCount;
}atcommand_exec_urc_queue_struct_t;


/***********************************************FUNCTION FOR AT COMMAND QUEUE MANAGEMENT**********************************/
/**************************************************************************************************************************
@Function	: atcommand_exec_add_at_cmd
@Input		:	atcommand_exec_at_command_t , AT command to be added to the queue
@Output		:	None
@Return 	:	atcommand_exec_queue_status_t , status of the queue operation done
@Description : This function is used to add at command to the execution queue
************************************************************************************************************************/
atcommand_exec_queue_status_t atcommand_exec_add_at_cmd(atcommand_exec_at_command_t * atCommand);

/************************************************************************************************************************
@Function	:	atcommand_exec_at_queue_init
@Input		:	None
@Output		:	None
@Return		:	char* - address of UART_RX_BUFFER
@Description	:	This function deletes and initializes the complete queue by erasing the complete structure
***********************************************************************************************************************/
void atcommand_exec_at_cmd_queue_init(void);

/**********************************************************************************************************************
@Function	:	atcommand_exec_del_at_cmd_head
@Input		:	None
@Output		:	None
@Return 	: atcommand_exec_queue_status_t - status of the queue operation 
@Description	: This function deletes all the command from the head to the current command to be executed/under execution
************************************************************************************************************************/
atcommand_exec_queue_status_t atcommand_exec_del_at_cmd_head(void);

/***********************************************************************************************************************
@Function	:	atcommand_exec_add_at_cmd_queue
@Input		:	atcommand_exec_at_command_t *, uint16_t commandCount
@Output		: none
@Return		: atcommand_exec_queue_status_t - status of the queue operation 
@Description	: This function adds commands to the existing queue and returns result as per the operations
************************************************************************************************************************/

atcommand_exec_queue_status_t atcommand_exec_add_at_cmd_queue(atcommand_exec_at_command_t*, uint16_t);



atcommand_exec_queue_status_t atcommand_exec_del_at_command_head(void);

void send_at_command(atcommand_exec_at_command_t cmd);


//*****************************************************CALLBACKS FOR AT COMMANDS******************************************

atcommand_exec_rsp_t atcommand_exec_cpin_status_callback(char* rsp);
atcommand_exec_rsp_t atcommand_exec_creg_status_callback(char* rsp);
atcommand_exec_rsp_t atcommand_exec_get_ipaddr_callback(char* rsp);
atcommand_exec_rsp_t atcommand_exec_server_connect_callback(char* rsp);
atcommand_exec_rsp_t atcommand_exec_send_gprs_packet_callback(char* rsp);
atcommand_exec_rsp_t send_bt_data_callback(char* rsp);
//Callback for connection getting closed.
int set_server_connection_status_handler(char* buffer, uint16_t len);
int mqtt_response_handler(char* buffer, uint16_t len);
int yutu_fw_bt_connect(char* rspStr, uint16_t count);
int yutu_fw_bt_pair(char* rspStr, uint16_t count);
int yutu_fw_bt_profile_connect(char* rspStr, uint16_t count);
int yutu_fw_bt_disconnect(char* rspStr, uint16_t count);
int yutu_fw_recv_bt_data(char* rspStr, uint16_t count);
int yutu_fw_data_send_ack(char* rspStr, uint16_t count);
#endif