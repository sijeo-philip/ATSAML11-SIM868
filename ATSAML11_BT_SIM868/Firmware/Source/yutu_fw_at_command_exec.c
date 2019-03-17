#include "yutu_fw_at_command_exec.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "MQTT_Packet_Writer.h"
#include "common.h"
#include "yutu_fw_bt.h"
#include "yutu_fw_gsm_gprs.h"
#include "yutu_fw_scheduler.h"
#include "yutu_fw_uart.h"

static char* clear_receive_buffer(char* rxbuffer, uint16_t buffsize);

static bool sendSuccess = false;

char responseRecvBuffer[ATCOMMAND_EXEC_RX_BUFF_SIZE];
char dataServer[ATCOMMAND_EXEC_CMD_BUFF_SIZE];
extern char btSendDataBuffer[300];
volatile extern BTProfile btProfile;
volatile extern unsigned short machineState;
atcommand_exec_event_t dataEvents;

volatile atcommand_exec_atcmd_queue_t cmdQueue;
atcommand_exec_urc_queue_struct_t urcEntityQueue;
volatile atcommand_exec_modem_status_t modemStatus = MODEM_EXECUTE_AT;
volatile data_packet_t mqttDataPacket;

bool clearBufferFlag = false;
bool modemRspWait = false;
bool responseRecvd = false;
bool s_atExecute = false;
uint16_t recvdBytes = 0;
// Should be used in SysTick Timer Handler for Timeout of response.
extern bool delayTimerStart;
extern uint32_t respTimeOut;
extern bool timeOutFlag;
extern char* rspRxPtr;

extern void at_response_processor_thread(void) __attribute__((noreturn));
extern void atcmd_dispatcher_thread(void) __attribute__((noreturn));
extern void atcmd_tester_thread(void) __attribute__((noreturn));

atcommand_exec_rsp_t at_command_default_callback(char* rsp);
bool global_urc_handler(char* rspStr, uint16_t len);

// Check if the AT Command is full of unexecuted commands
static bool at_command_queue_full_check(void)
{
  if (cmdQueue.commandCount >= ATCOMMAND_EXEC_CMD_QUEUE)
    return true;
  else
    return false;
}

// Delete AT Command
static void at_command_remove(atcommand_exec_at_command_t* atCommand)
{
  memset(atCommand->atCmd, '\0', sizeof(atCommand->atCmd));
  atCommand->repCount = 0;
  atCommand->rspFunc = NULL;
  atCommand->rspDelay = 0;
}

// Add AT Command to the tail of the queue
atcommand_exec_queue_status_t atcommand_exec_add_at_cmd(
    atcommand_exec_at_command_t* atCommand)
{
  if (at_command_queue_full_check() == true)
    return QUEUE_FULL;
  else
    {
      if (cmdQueue.commandCount > 0)
	cmdQueue.lastCmd =
	    (cmdQueue.lastCmd + 1) %
	    ATCOMMAND_EXEC_CMD_QUEUE;  // Increment the lastCount of the Queue
      // clear the AT command stored in the the last position stored in the
      // Queue
      memset(cmdQueue.atCmdQueue[cmdQueue.lastCmd].atCmd, '\0',
	     sizeof(cmdQueue.atCmdQueue[cmdQueue.lastCmd].atCmd));
      // Copy the command to the cleared position
      strcpy(cmdQueue.atCmdQueue[cmdQueue.lastCmd].atCmd, atCommand->atCmd);
      // Copy the repetition count to the queue
      cmdQueue.atCmdQueue[cmdQueue.lastCmd].repCount = atCommand->repCount;
      // Map the function pointer
      if (atCommand->rspFunc != NULL)
	cmdQueue.atCmdQueue[cmdQueue.lastCmd].rspFunc = atCommand->rspFunc;
      else
	cmdQueue.atCmdQueue[cmdQueue.lastCmd].rspFunc =
	    at_command_default_callback;
      // Copy the execution Delay
      cmdQueue.atCmdQueue[cmdQueue.lastCmd].rspDelay = atCommand->rspDelay;
      // Copy the status value on successful execution
      cmdQueue.atCmdQueue[cmdQueue.lastCmd].successStatus =
	  atCommand->successStatus;
      cmdQueue.commandCount++;
      return QUEUE_OPER_OK;
    }
}

// Initialize the Queue Parameters
void atcommand_exec_at_cmd_queue_init()
{
  uint8_t i = 0;
  for (i = 0; i < ATCOMMAND_EXEC_CMD_QUEUE; i++)
    {  // Clears the Command Queue and resets all value.
      memset(cmdQueue.atCmdQueue[i].atCmd, '\0',
	     sizeof(cmdQueue.atCmdQueue[i].atCmd));
      cmdQueue.atCmdQueue[i].repCount = 0;
      cmdQueue.atCmdQueue[i].rspDelay = 0;
      cmdQueue.atCmdQueue[i].rspFunc = NULL;
      cmdQueue.atCmdQueue[i].successStatus = 0;
    }
  cmdQueue.firstCmd = 0;
  cmdQueue.lastCmd = 0;
  cmdQueue.currentCmd = 0;
  cmdQueue.repeatCount = 0;
  cmdQueue.commandCount = 0;
  cmdQueue.cmdQueueWrite = false;
  cmdQueue.currentCmdRet = AT_RSP_ERROR;
  cmdQueue.currentCmdStatus = NOK_OPER;
}

// Delete the command from head of the Queue to the one before the current to
// complete execution.
atcommand_exec_queue_status_t atcommand_exec_del_at_cmd_head(void)
{
  uint8_t i = 0;
  if (cmdQueue.firstCmd == cmdQueue.currentCmd) return QUEUE_ERROR;

  if (cmdQueue.commandCount == 0) return QUEUE_EMPTY;

  if (cmdQueue.currentCmd < cmdQueue.firstCmd)
    {
      for (i = cmdQueue.firstCmd; i < ATCOMMAND_EXEC_CMD_QUEUE; i++)
	{
	  at_command_remove(&cmdQueue.atCmdQueue[i]);
	  cmdQueue.commandCount--;
	}
      for (i = 0; i < cmdQueue.currentCmd; i++)
	{
	  at_command_remove(&cmdQueue.atCmdQueue[i]);
	  cmdQueue.commandCount--;
	}
    }
  else
    {
      for (i = cmdQueue.firstCmd; i < cmdQueue.currentCmd; i++)
	{
	  at_command_remove(&cmdQueue.atCmdQueue[i]);
	  cmdQueue.commandCount--;
	}
    }
  cmdQueue.firstCmd = cmdQueue.currentCmd;
  return QUEUE_OPER_OK;
}

// Add a  set of commands to the queue for execution
atcommand_exec_queue_status_t atcommand_exec_add_at_cmd_queue(
    atcommand_exec_at_command_t* atCmd, uint16_t cmdCount)
{
  uint16_t i;
  i = cmdCount;
  if ((cmdQueue.commandCount + cmdCount) > ATCOMMAND_EXEC_CMD_QUEUE)
    return QUEUE_ERROR;
  atcommand_exec_del_at_cmd_head();
  while (cmdCount)
    {
      atcommand_exec_add_at_cmd(&atCmd[i - cmdCount]);
      cmdCount--;
    }
  return QUEUE_OPER_OK;
}

// Delete AT Command from the head of the Queue
atcommand_exec_queue_status_t atcommand_exec_del_at_command_head(void)
{
  if (cmdQueue.commandCount == 0) return QUEUE_EMPTY;
  at_command_remove(&cmdQueue.atCmdQueue[cmdQueue.firstCmd]);
  if (cmdQueue.firstCmd == cmdQueue.currentCmd)
    cmdQueue.currentCmd = (cmdQueue.currentCmd + 1) % ATCOMMAND_EXEC_CMD_QUEUE;
  cmdQueue.firstCmd = (cmdQueue.firstCmd + 1) % ATCOMMAND_EXEC_CMD_QUEUE;
  cmdQueue.commandCount--;
  return QUEUE_OPER_OK;
}

/******************************************************************************************************************************/
/*Algorithm to issue a AT Command
call the queue init function
1. Clear the Recieve Buffer
2. Write AT Command to the Modem
3. Set the timeout for the Response
4. Assign the rspRxPtr to the recieve buffer
5. set rx_byte_count =0
6. the recieve buffer should be cleared after processing the response.

*/

void rsp_time_out_timer_start(void)
{
  delayTimerStart = false;
  respTimeOut = cmdQueue.atCmdQueue[cmdQueue.currentCmd].rspDelay * 0.05;
  timeOutFlag = false;
  delayTimerStart = true;
}

void rsp_time_out_timer_stop(void)
{
  delayTimerStart = false;
  respTimeOut = 0;
  timeOutFlag = false;
}

// Clear Recieve Buffer
static char* clear_receive_buffer(char* rxbuffer, uint16_t buffsize)
{
  memset(rxbuffer, '\0', buffsize);
  return rxbuffer;
}
// Send AT Command
void send_at_command(atcommand_exec_at_command_t cmd)
{
  cmdQueue.repeatCount = cmd.repCount;
  s_atExecute = true;
  yutu_fw_uart_send(cmd.atCmd, (uint16_t)strlen((const char*)cmd.atCmd));
  // if(modemRspWait==false)
  //{
  timeOutFlag = false;
  rsp_time_out_timer_start();
  modemRspWait = true;
  //}
}

/************************************************DEFAULT CALLBACK
 * FUNCTIONS*******************************************/
// Callback function for handling response from Server
int mqtt_response_handler(char* buffer, uint16_t len)
{
  int ret = -1;
  char tempBuff[100];

  buffer = strcpymarker(buffer, tempBuff, ':', '\x0d');
  buffer++;
  if (mqttDataPacket.packetType == NORMAL_PACKET)
    {
      memset(dataServer, '\0', sizeof(dataServer));
      strncpy(dataServer, buffer, sizeof(buffer));
      return 0;
    }
  else if ((buffer[0] == 0x20) && (buffer[1] == 0x02))  // Check for Packet ID
    {
      if ((buffer[3] >= 0x00) &&
	  (buffer[3] <= 0x05))  // Return the connect acknowledgement status.
	{
	  unsigned char retVal = buffer[3];
	  switch (retVal)
	    {
	      case 0x00:
		{
		  ret = 1;  // Value Returned for Successful connect to MQTT
			    // Server
		  break;
		}
	      case 0x01:
		{
		  ret = 2;  // Connection Refused, unacceptable protocol
		  break;
		}
	      case 0x02:
		{
		  ret = 3;  // Connection Refused, Identifier Rejected
		  break;
		}
	      case 0x03:
		{
		  ret = 4;  // Connection Refused, Server Unavailable
		  break;
		}
	      case 0x04:
		{
		  ret = 5;  // Connection Refused, Bad UserName / Password
		  break;
		}
	      case 0x05:
		{
		  ret = 6;  // Connection Refused, Unauthourized.
		  break;
		}
	      default:
		ret = -1;  // Invalid Data
		break;
	    }
	}
      ret = -1;  // Invalid Data
    }
  else
      // Publish Acknowledge processing
      if ((buffer[0] == 0x40) && (buffer[1] == 0x02))
    {
      // MQTT: DATA PUBLISH SUCCESS
      ret = 7;
    }
  else if ((buffer[0] == 0x50) && (buffer[1] == 0x02))
    {
      // MQTT: DATA PUBLISH RECIEVE SUCCESS
      ret = 8;
    }
  else if ((buffer[0] == 0x60) && (buffer[1] == 0x02))
    {
      // MQTT: DATA PUBLISH RELEASE SUCCESS
      ret = 9;
    }
  else if ((buffer[0] == 0x70) && (buffer[1] == 0x02))
    {
      //"MQTT: DATA PUBLISH COMPLETE SUCCESS
      ret = 10;
    }
  // subscribe ack handler
  else if ((buffer[0] == 0x90) && (buffer[1] == 0x03))  // Check for Packet ID
    {
      unsigned char retVal = buffer[4];
      switch (retVal)
	{
	  case 0x00:
	    {
	      // MQTT: Qos0-SUBSCRIBE SUCCESS
	      ret = 11;
	      break;
	    }
	  case 0x01:
	    {
	      // MQTT: Qos1-SUBSCRIBE SUCCESS
	      ret = 12;
	      break;
	    }
	  case 0x02:
	    {
	      // MQTT: Qos2-SUBSCRIBE SUCCESS
	      ret = 13;
	      break;
	    }
	  case 0x80:
	    {
	      // SUBSCRIPTION FAILED
	      ret = 14;
	      break;
	    }
	  default:
	    // MQTT [%s] : INVALID DATA INPUT
	    ret = -1;
	    break;
	}
    }
  else  // unsubscribe acknowledgement handler
      if ((buffer[0] == 0xB0) && (buffer[1] == 0x01))
    {
      // MQTT: DATA UNSUBSCRIBE SUCCESS
      ret = 15;
    }
  else  // Ping acknowledgement handler
      if (buffer[0] == 0xD0)
    {
      // MQTT: PING RESPONSE RECIEVED
      ret = 16;
    }
  ret = -1;
  // MQTT [%s] : INVALID DATA INPUT

  mqttDataPacket.packetType = NORMAL_PACKET;

  return ret;
}

// Default Callback function for AT Commands

atcommand_exec_rsp_t at_command_default_callback(char* rsp)
{
  atcommand_exec_rsp_t ret = AT_RSP_CONTINUE;
  char* rspStrTable[3] = {"OK", "ERROR"};
  uint8_t i = 0;
  int8_t rspType = -1;
  char* p = rsp;
  while (p)
    {
      /*ignore \r\n*/
      while ((ATCOMMAND_EXEC_CMD_CR == *p) || (ATCOMMAND_EXEC_CMD_LF == *p))
	{
	  p++;
	}
      for (i = 0; i < sizeof(rspStrTable) / sizeof(rspStrTable[0]); i++)
	{
	  if (!strncmp(rspStrTable[i], (char*)p, strlen(rspStrTable[i])))
	    {
	      rspType = (int8_t)i;
	      break;
	    }
	}
      p = (char*)strchr((char*)p, 0x0a);
    }
  switch (rspType)
    {
      case 0:
	ret = AT_RSP_FINISH;
	break;

      case 1:
	ret = AT_RSP_ERROR;
	break;

      default:
	ret = AT_RSP_CONTINUE;
	break;
    }
  return ret;
}

// URC callback function
bool global_urc_handler(char* rspStr, uint16_t len)
{
  bool retVal = false;
  uint8_t i = 0;
  char* p = rspStr;
  while (p)
    {
      while ((ATCOMMAND_EXEC_CMD_CR == *p) || (ATCOMMAND_EXEC_CMD_LF == *p))
	{
	  p++;
	}
      for (i = 0; i < urcEntityQueue.urcQueueCount; i++)
	{
	  if (!strncmp((char*)urcEntityQueue.urcQueue[i].urcStr, (char*)p,
		       strlen((char*)urcEntityQueue.urcQueue[i].urcStr)))
	    {
	      if (urcEntityQueue.urcQueue[i].urcCallback)
		{
		  urcEntityQueue.urcQueue[i].urcCallback(p, len);
		  retVal = true;
		}
	      else
		{
		  retVal = false;
		}
	      rspRxPtr = clear_receive_buffer(responseRecvBuffer,
					      ATCOMMAND_EXEC_RX_BUFF_SIZE);
	    }
	  else
	    retVal = false;
	}
      p = (char*)memchr(p, 0x0a, 0);
    }

  return retVal;
}
// call back for AT+CPIN?
atcommand_exec_rsp_t atcommand_exec_cpin_status_callback(char* rsp)
{
  volatile atcommand_exec_rsp_t ret = AT_RSP_CONTINUE;
  volatile char* rspStrTable[3] = {"+CPIN: READY", "+CPIN: NOT READY", "ERROR"};
  volatile uint8_t i = 0;
  volatile int rspType = -1;
  volatile char* p = rsp;
  while (p)
    {
      /*Ignore \r\n*/
      while (ATCOMMAND_EXEC_CMD_CR == *p || ATCOMMAND_EXEC_CMD_LF == *p)
	{
	  p++;
	}
      for (i = 0; i < 3; i++)
	{
	  if (!strncmp(rspStrTable[i], (char*)p, strlen(rspStrTable[i])))
	    {
	      rspType = i;
	      break;
	    }
	}
      p = (char*)strchr((char*)p, 0x0a);
    }
  switch (rspType)
    {
      case 0:
	ret = AT_RSP_FINISH;
	break;
      case 1:
      case 2:
	ret = AT_RSP_ERROR;
	break;
      default:
	break;
    }
  return ret;
}

// Callback for AT+CREG?
atcommand_exec_rsp_t atcommand_exec_creg_status_callback(char* rsp)
{
  volatile atcommand_exec_rsp_t ret = AT_RSP_CONTINUE;
  volatile char* rspStrTable[3] = {"+CREG: 0,1", "+CREG: 0,5", "ERROR"};
  volatile uint8_t i = 0;
  volatile int rspType = -1;
  volatile char* p = rsp;

  while (p)
    {
      /*Ignore \r\n*/
      while (ATCOMMAND_EXEC_CMD_CR == *p || ATCOMMAND_EXEC_CMD_LF == *p)
	{
	  p++;
	}
      for (i = 0; i < 3; i++)
	{
	  if (!strncmp(rspStrTable[i], (char*)p, strlen(rspStrTable[i])))
	    {
	      rspType = i;
	      break;
	    }
	}
      p = (char*)strchr((char*)p, 0x0a);
    }
  switch (rspType)
    {
      case 0:
	// nwParams.network_status = CONNECTED_LOCAL;
	ret = AT_RSP_FINISH;
	break;
      case 1:
	// nwParams.network_status = CONNECTED_ROAMING;
	ret = AT_RSP_FINISH;
	break;
      default:
	// nwParams.network_status = DISCONNECTED;
	ret = AT_RSP_ERROR;
	cmdQueue.currentCmdStatus = NOK_OPER;
	break;
    }
  return ret;
}
// Get IP address after connecting with GPRS
atcommand_exec_rsp_t atcommand_exec_get_ipaddr_callback(char* rsp)
{
  volatile atcommand_exec_rsp_t ret = AT_RSP_CONTINUE;
  volatile char* rspStrTable[3] = {"+SAPBR: 1,1,", "+SAPBR: 1,3,", "ERROR"};
  volatile uint8_t i = 0;
  volatile int rspType = -1;
  volatile char* p = rsp;
  char tempNum[25];
  while (p)
    {
      /*Ignore \r\n*/
      while (ATCOMMAND_EXEC_CMD_CR == *p || ATCOMMAND_EXEC_CMD_LF == *p)
	{
	  p++;
	}
      for (i = 0; i < 3; i++)
	{
	  if (!strncmp(rspStrTable[i], p, strlen(rspStrTable[i])))
	    {
	      rspType = i;
	      break;
	    }
	}
      p = (char*)strchr((char*)p, 0x0a);
    }
  switch (rspType)
    {
      case 0:
	p = rsp;
	i = 0;
	p = (char*)strchr((char*)p, '"');
	p++;
	while ('"' != *p)
	  {
	    tempNum[i] = *p;
	    i++;
	    p++;
	  }
	tempNum[i] = '\0';
	memset(gprsConfig.ip_address, '\0', sizeof(gprsConfig.ip_address));
	strcpy(gprsConfig.ip_address, tempNum);
	ret = AT_RSP_FINISH;
	break;
      default:
	ret = AT_RSP_ERROR;
	break;
    }
  return ret;
}

// Server Connect Callback
atcommand_exec_rsp_t atcommand_exec_server_connect_callback(char* rsp)
{
  volatile atcommand_exec_rsp_t ret = AT_RSP_CONTINUE;
  volatile char* rspStrTable[4] = {"OK", "ERROR\r\n\r\nALREADY CONNECT",
				   "CONNECT OK",
				   "STATE: IP STATUS\r\n\r\nCONNECT FAIL"};
  volatile uint8_t i = 0;
  volatile int rspType = -1;
  volatile char* p = rsp;
  while (p)
    {
      /*Ignore \r\n*/
      while (ATCOMMAND_EXEC_CMD_CR == *p || ATCOMMAND_EXEC_CMD_LF == *p)
	{
	  p++;
	}
      for (i = 0; i < 4; i++)
	{
	  if (!strncmp(rspStrTable[i], (char*)p, strlen(rspStrTable[i])))
	    {
	      rspType = i;
	      break;
	    }
	}
      p = (char*)strchr(p, 0x0a);
    }
  switch (rspType)
    {
      case 0:
	ret = AT_RSP_WAIT;
	break;

      case 1:
      case 2:
	ret = AT_RSP_FINISH;
	break;

      default:
	ret = AT_RSP_ERROR;
	break;
    }
  return ret;
}

// Callback for Sending the Data to server
atcommand_exec_rsp_t atcommand_exec_send_gprs_packet_callback(char* rsp)
{
  char* p = rsp;
  while ((*p == ATCOMMAND_EXEC_CMD_CR) || (*p == ATCOMMAND_EXEC_CMD_LF))
    {
      p++;
    }
  if (*p == '>')
    {
      mqttDataPacket.data[mqttDataPacket.totalSize] = '\x1A';
      yutu_fw_uart_send(mqttDataPacket.data, mqttDataPacket.totalSize);
      return AT_RSP_WAIT;
    }
  else
    {
      if (!strncmp(p, "SEND OK", 7))
	return AT_RSP_FINISH;
      else
	return AT_RSP_ERROR;
    }
}

/********************************************BT PROCCESSING**************************************************/
atcommand_exec_rsp_t send_bt_data_callback(char* rsp)
{
  char* p = rsp;
  while ((*p == ATCOMMAND_EXEC_CMD_CR) || (*p == ATCOMMAND_EXEC_CMD_LF))
    {
      p++;
    }
  if (*p == '>')
    {
      strncat(btSendDataBuffer, "\x1A", 1);
      yutu_fw_uart_send(btSendDataBuffer, strlen(btSendDataBuffer));
      dataEvents.event_bit.BT_DATA_SENT = 1;
      return AT_RSP_FINISH;
    }
  else
    return AT_RSP_ERROR;
}

// Callback for BT Connect for authorized Passcode received
int yutu_fw_bt_pair(char* rspStr, uint16_t count)
{
  uint8_t* p = rspStr;
  char tempString[30];
  char* tempStr;
  //	uint16_t bytes;
  if (p != NULL)
    {
      while ((*p == ATCOMMAND_EXEC_CMD_CR) || (*p == ATCOMMAND_EXEC_CMD_LF))
	{
	  p++;
	}
      tempStr = strcpymarker((char*)p, tempString, '"', ',');
      memset(tempString, '\0', sizeof(tempString));
      tempStr = strcpymarker(tempStr, tempString, ',', ',');
      memset(tempString, '\0', sizeof(tempString));
      tempStr = strcpymarker(tempStr, tempString, ',', 0x0D);
    }

  // if(!strncmp(tempString, (char*)btProfile.passCode, bytes))
  btProfile.btStatus = BT_PAIRING;
  // else
  //	 yutu_fw_disconnect_bt();
  rspRxPtr =
      clear_receive_buffer(responseRecvBuffer, ATCOMMAND_EXEC_RX_BUFF_SIZE);
  return 0;
}

// Connect BT to SPP profile.
bool yutu_fw_spp_connect()
{
  volatile atcommand_exec_queue_status_t status;
  volatile bool retVal = false;
  atcommand_exec_at_command_t atCommand = {"AT+BTACPT=1" ATCOMMAND_EXEC_CMD_END,
					   NULL, 3, 3000, CMD1_OK};

  atcommand_exec_at_cmd_queue_init();
  cmdQueue.cmdQueueWrite = true;
  status = atcommand_exec_add_at_cmd(&atCommand);
  cmdQueue.cmdQueueWrite = false;

  if (status == QUEUE_OPER_OK)
    {
      while (cmdQueue.commandCount != 0)
	{
	}
      if (cmdQueue.currentCmdStatus == CMD1_OK)
	{
	  machineState|=ATCOMMAND_EXEC_BT_RDY;
	  retVal = true;
	}
      else
	retVal = false;
    }
  else
    retVal = false;
  atcommand_exec_at_cmd_queue_init();
  return retVal;
}

// check  BT connect request to SPP profile
int yutu_fw_bt_profile_connect(char* rsp, uint16_t bytes)
{
  char* p = rsp;
  char tempChar[150];
  while ((*p == ATCOMMAND_EXEC_CMD_CR) || (*p == ATCOMMAND_EXEC_CMD_LF))
    {
      p++;
    }
  p = strcpymarker((char*)p, tempChar, '"', '"');  // Name of the device
  memset(tempChar, '\0', sizeof(tempChar));
  p++;
  strcpymarker(p, tempChar, '"', '"');  // Name of profile
  if (!strncmp(tempChar, "SPP", 3))
    {
      btProfile.btStatus = BT_SPPCONNECT;
      // if(yutu_fw_spp_connect())
      //{
      // btProfile.btStatus = BT_CONNECTED;
      //}
      // else
      // btProfile.btStatus = BT_DISCONNECTED;
    }
  rspRxPtr =
      clear_receive_buffer(responseRecvBuffer, ATCOMMAND_EXEC_RX_BUFF_SIZE);
  return 1;
}

int yutu_fw_bt_connect(char* rspStr, uint16_t count)
{
  volatile char* p = rspStr;
  volatile char* tempPtr;

  volatile bool retVal = false;
  while ((*p == ATCOMMAND_EXEC_CMD_LF) || (*p == ATCOMMAND_EXEC_CMD_CR))
    {
      p++;
    }
  tempPtr = strcpymarker(p, btProfile.connectDeviceHFPID, ' ', ',');
  tempPtr = strstr(p, "HFP");
  if (tempPtr != NULL)
    {
      btProfile.btStatus = BT_HFPDISCONN;
    }
  else
    tempPtr = strstr(p, "SPP");
  if (tempPtr != NULL)
    btProfile.btStatus = BT_SPPCONNECT;
  else
    retVal = false;
  rspRxPtr =
      clear_receive_buffer(responseRecvBuffer, ATCOMMAND_EXEC_RX_BUFF_SIZE);
  return retVal;
}
int yutu_fw_bt_disconnect(char* rspStr, uint16_t count)
{
  char* p = rspStr;
  char tempChar[150];
  while ((*p == ATCOMMAND_EXEC_CMD_CR) || (*p == ATCOMMAND_EXEC_CMD_LF))
    {
      p++;
    }
  p = strcpymarker((char*)p, tempChar, '"', '"');  // Name of the device
  memset(tempChar, '\0', sizeof(tempChar));
  p++;
  strcpymarker(p, tempChar, '"', '"');  // Name of profile
  if (!strncmp(tempChar, "SPP", 3))
    {
      btProfile.btStatus = BTDISCONNECTED;
      // if(yutu_fw_spp_connect())
      //{
      // btProfile.btStatus = BT_CONNECTED;
      //}
      // else
      // btProfile.btStatus = BT_DISCONNECTED;
      machineState&=0xFFFB;
    }

  rspRxPtr =
      clear_receive_buffer(responseRecvBuffer, ATCOMMAND_EXEC_RX_BUFF_SIZE);
}

int yutu_fw_recv_bt_data(char* rspStr, uint16_t count)
{
  volatile char* p = rspStr;
  volatile char* tempPtr;
  volatile char tempChar[4];
  while ((*p == ATCOMMAND_EXEC_CMD_LF) || (*p == ATCOMMAND_EXEC_CMD_CR))
    {
      p++;
    }
  memset(btProfile.btMsgFrame, '\0', sizeof(btProfile.btMsgFrame));
  p = strcpymarker((char*)p, tempChar, ',', ',');
  p = strcpymarker((char*)p, btProfile.btMsgFrame, ',',
		   '\r');  // Store Message into bt Message Buffer
  dataEvents.event_bit.BT_DATA_RD_RDY = 1;
  return 0;
}
// Checking the send ok recieved from GPRS or BT Data Send
int yutu_fw_data_send_ack(char* rspStr, uint16_t count)
{
  if (dataEvents.event_bit.BT_DATA_SENT == 1)
    {
      dataEvents.event_bit.BT_DATA_WR_RDY = 1;
      dataEvents.event_bit.BT_DATA_SENT = 0;
    }
}
/**************************************RESPONSE PROCESSING THREAD*******************************************/
void at_response_processor_thread(void)
{
  bool ret = false;
  while (1)
    {
      if ((recvdBytes = yutu_fw_uart_recieve(rspRxPtr)) > 0)
	{
	  ret = global_urc_handler(rspRxPtr, recvdBytes);
	  if ((ret == false) && (s_atExecute == true))
	    {
	      s_atExecute = false;
	      responseRecvd = true;
	      modemRspWait = false;
	      rsp_time_out_timer_stop();
	      if (cmdQueue.atCmdQueue[cmdQueue.currentCmd].rspFunc)
		{
		  cmdQueue.currentCmdRet =
		      cmdQueue.atCmdQueue[cmdQueue.currentCmd].rspFunc(
			  rspRxPtr);
		  switch (cmdQueue.currentCmdRet)
		    {
		      case AT_RSP_ERROR:
			if ((cmdQueue.atCmdQueue[cmdQueue.currentCmd]
				 .repCount) > 0)
			  {
			    cmdQueue.atCmdQueue[cmdQueue.currentCmd].repCount--;
			    cmdQueue.currentCmdStatus = NOK_OPER;
			  }
			else
			  atcommand_exec_at_cmd_queue_init();
			modemStatus = MODEM_RSP_WAIT;
			break;
		      case AT_RSP_CONTINUE:
		      case AT_RSP_FINISH:
			if (cmdQueue.commandCount > 0)
			  {
			    cmdQueue.currentCmdStatus =
				cmdQueue.atCmdQueue[cmdQueue.currentCmd]
				    .successStatus;
			    cmdQueue.currentCmd = (cmdQueue.currentCmd + 1) %
						  ATCOMMAND_EXEC_CMD_QUEUE;
			    cmdQueue.commandCount--;
			  }
			modemStatus = MODEM_RSP_WAIT;
			break;
		      case AT_RSP_WAIT:
			modemStatus = MODEM_IDLE;
			rsp_time_out_timer_start();
			break;
		      case AT_RSP_FUN_OVER:
			break;
		      default:
			break;
		    }
		}
	    }
	}
    }
}

void atcmd_dispatcher_thread(void)
{
  while (1)
    {
      switch (modemStatus)
	{
	  case MODEM_EXECUTE_AT:
	    if ((cmdQueue.commandCount > 0) &&
		(cmdQueue.cmdQueueWrite == false) && (s_atExecute == false))
	      {
		rspRxPtr = clear_receive_buffer(responseRecvBuffer,
						ATCOMMAND_EXEC_RX_BUFF_SIZE);
		responseRecvd = false;
		send_at_command(cmdQueue.atCmdQueue[cmdQueue.currentCmd]);
		modemStatus = MODEM_RSP_WAIT;
	      }
	    break;
	  case MODEM_RSP_WAIT:
	    if ((timeOutFlag == true) && (modemRspWait == true))
	      {
		if (cmdQueue.repeatCount > 0)
		  {
		    cmdQueue.atCmdQueue[cmdQueue.currentCmd].repCount--;
		    s_atExecute = false;
		  }
		if ((cmdQueue.atCmdQueue[cmdQueue.currentCmd].repCount == 0) &&
		    (cmdQueue.commandCount > 0))
		  {
		    cmdQueue.currentCmd =
			(cmdQueue.currentCmd + 1) % ATCOMMAND_EXEC_CMD_QUEUE;
		    cmdQueue.commandCount--;
		    s_atExecute = false;
		  }
		modemStatus = MODEM_EXECUTE_AT;
		modemRspWait = false;
		rsp_time_out_timer_stop();
	      }
	    if (responseRecvd == true) modemStatus = MODEM_EXECUTE_AT;
	    break;
	  case MODEM_RSP_RECVD:
	    break;
	  case MODEM_IDLE:
	    if (timeOutFlag == true)
	      {
		rsp_time_out_timer_stop();
	      }
	    break;
	  default:
	    break;
	}
    }
}

//	At command tester
void atcmd_tester_thread(void)
{
  while (1)
    {
      if ((machineState&0x0001)==0)
	{
	  sendSuccess = yutu_fw_gsm_gprs_init();
	}
      // if(((machineState&0x0002)==0)&&((machineState&0x0001)==0x0001))
      //{
      // yutu_fw_gprs_set_apn("internet",&gprsConfig);
      // sendSuccess = yutu_fw_gprs_on(&gprsConfig);
      //}
      // if(((machineState&0x0008)==0)&&((machineState&0x0003)==0x0003))
      //{
      // yutu_fw_set_server_ip("m15.cloudmqtt.com",&mqttServerProfile);
      // yutu_fw_set_server_port("17016", &mqttServerProfile);
      // yutu_fw_connect_server(&mqttServerProfile);
      //}
      if (((machineState&0x0020)==0) &&
	  ((machineState&0x0001)==0x0001))
	{
	  yutu_fw_bt_init();
	}
      if (((machineState&0x0040)==0) &&
	  ((machineState&0x0021)==0x0021))
	{
	  yutu_fw_bt_power_on();
	}
      if(((machineState&0x0061)==0x0061)&&((machineState&0x0004)==0))
	{
	  switch(btProfile.btStatus)
	    {
	      case BT_PAIRING:
		if (yutu_fw_connect_to_bt()) btProfile.btStatus = BT_PAIRED;
		break;
	      case BT_HFPDISCONN:
		if (yutu_fw_disconnect_bt(btProfile.connectDeviceHFPID))
		  btProfile.btStatus = BT_HFPDISCONNECTED;
		break;
	      case BT_SPPCONNECT:
		if (yutu_fw_spp_connect()) btProfile.btStatus = BT_SPPCONNECTED;
		break;
	      default:
		break;
	    }
	 
	}
	 if (dataEvents.event_bit.BT_DATA_RD_RDY == 1)
	 {
		 memset(btSendDataBuffer, '\0', sizeof(btSendDataBuffer));
		 btProfile.btCallback(btProfile.btMsgFrame, btSendDataBuffer);
		 yutu_fw_bt_send_data(btSendDataBuffer, strlen(btSendDataBuffer));
		 dataEvents.event_bit.BT_DATA_RD_RDY = 0;
	 }
    }
}
