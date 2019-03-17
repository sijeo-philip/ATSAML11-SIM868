#include "yutu_fw_bt.h"
#include "string.h"
#include "yutu_fw_at_command_exec.h"

volatile BTProfile btProfile;
volatile extern unsigned short machineState;
char btSendDataBuffer[300];
extern char* rspRxPtr;
volatile extern atcommand_exec_atcmd_queue_t cmdQueue;

bool yutu_fw_bt_init()
{
  volatile atcommand_exec_queue_status_t status;
  volatile bool retVal = false;
  char tempCommand[25] = "AT+BTHOST=";
  atcommand_exec_at_command_t atCommand;
  strcat((char*)tempCommand, btProfile.hostName);
  strcat((char*)tempCommand, ATCOMMAND_EXEC_CMD_END);
  strcpy(atCommand.atCmd, (char*)tempCommand);
  atCommand.rspDelay = 3000;
  atCommand.repCount = 3;
  atCommand.rspFunc = NULL;
  atCommand.successStatus = CMD1_OK;
  char* cmdPtr = atCommand.atCmd;
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
	  machineState|=ATCOMMAND_EXEC_BT_HOST_SET;
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

bool yutu_fw_bt_power_on(void)
{
  volatile atcommand_exec_queue_status_t status;
  volatile bool retVal = false;
  atcommand_exec_at_command_t atCommand = {
      "AT+BTPOWER=1" ATCOMMAND_EXEC_CMD_END, NULL, 3, 3000, CMD1_OK};
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
	  machineState|=ATCOMMAND_EXEC_BT_PWR_RDY;
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

bool yutu_fw_bt_power_off(void)
{
  atcommand_exec_queue_status_t status;
  bool retVal;
  atcommand_exec_at_command_t atCommand = {
      "AT+BTPOWER=0" ATCOMMAND_EXEC_CMD_END, NULL, 3, 3000, CMD1_OK};

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
	  machineState&=0xFFBF;
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

bool yutu_fw_disconnect_bt(char* connectionNo)
{
  volatile atcommand_exec_queue_status_t status;
  volatile bool retVal = false;
  atcommand_exec_at_command_t atCommand = {"AT+BTDISCONN=", NULL, 1, 3000,
					   CMD1_OK};
  strncat(atCommand.atCmd, connectionNo, strlen(connectionNo));
  strncat(atCommand.atCmd, ATCOMMAND_EXEC_CMD_END, 2);
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
	retVal = true;
      else
	retVal = false;
    }
  else
    retVal = false;
  atcommand_exec_at_cmd_queue_init();
  return retVal;
}

bool yutu_fw_bt_send_data(char* data, uint32_t bytes)
{
  volatile atcommand_exec_queue_status_t status;
  volatile bool retVal = false;
  atcommand_exec_at_command_t atCommand = {
      "AT+BTSPPSEND" ATCOMMAND_EXEC_CMD_END, send_bt_data_callback, 1, 3000,
      CMD1_OK};
  //	memset(btSendDataBuffer, '\0', sizeof(btSendDataBuffer));
  //	strncpy(btSendDataBuffer, (char*)data, bytes);
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

// Function to connect to BT on match of passcode.
bool yutu_fw_connect_to_bt(void)
{
  volatile atcommand_exec_queue_status_t status;
  volatile bool retVal = false;
  atcommand_exec_at_command_t atCommand = {
      "AT+BTPAIR=1,1" ATCOMMAND_EXEC_CMD_END, NULL, 3, 50000, CMD1_OK};
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
	retVal = true;
      else
	retVal = false;
    }
  else
    retVal = false;

  atcommand_exec_at_cmd_queue_init();
  return retVal;
}
