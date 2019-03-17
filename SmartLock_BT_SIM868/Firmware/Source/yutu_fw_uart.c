#include "yutu_fw_uart.h"

#include "saml11e16a.h"
#include <string.h>
#include <stdbool.h>

static char buffer[300];
static uint16_t byteCount=0;
static uint8_t startRecvFlag=0;
static uint16_t recvdData;
static uint16_t responseTimeDelay=0;
static bool recvBufferFullFlag = false;
volatile static bool dataSendCompleteFlag = false;
DmacDescriptor usartTXDescriptor, wbDmacMem;
uint16_t byteCountTx;

uint16_t yutu_fw_uart_recieve(char* recvBuffer){
	 if((recvBufferFullFlag==true)&&(recvBuffer!=NULL))
	 {
	 memset(recvBuffer,'\0', sizeof(recvBuffer));
	 strncpy(recvBuffer, buffer, strlen(buffer));
	 memset(buffer, '\0', sizeof(buffer));
		recvBufferFullFlag = false;
	return recvdData;
	 }
	 else
	 {
		 return 0;
	 }
}


void TC0_Handler(void)
{
	TC0->COUNT16.INTFLAG.reg |= TC_INTFLAG_MC0;
	if(startRecvFlag==1)
	{
		responseTimeDelay--;
		if(responseTimeDelay==0)
		{
			recvdData = byteCount;
			byteCount=0;
			startRecvFlag=0;
			recvBufferFullFlag = true;
		}
	}
	
}


void SERCOM1_2_Handler(void)
{
	if(SERCOM1->USART.INTFLAG.bit.RXC==1)
	{
		if(byteCount==0)
		{
			buffer[byteCount] = (char)SERCOM1->USART.DATA.reg;
			startRecvFlag = 1;
			responseTimeDelay = 2 ;
			byteCount++;
		}
		else
		{
		  buffer[byteCount] = (char)SERCOM1->USART.DATA.reg;
		  byteCount++;
			responseTimeDelay = 2;
		}
	}   // for UART Recieve Interrupt
}


bool yutu_fw_uart_send(char* txBuffer, int size){
	byteCountTx = 0;
	while(byteCountTx<=size-1)
	{
		SERCOM1->USART.DATA.reg = *(txBuffer+byteCountTx);
		while(SERCOM1->USART.INTFLAG.bit.TXC==0);
		byteCountTx++;
	}
	return true;
}

//bool yutu_fw_uart_send(char* txBuffer, uint16_t size){
//	//DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_SWRST;
//	__disable_irq();
//	UART_DMAC_SAFE_START
//	dataSendCompleteFlag = false;
//	DMAC->CHCTRLB.reg |= DMAC_CHCTRLB_TRIGACT_BEAT;
//	//usartTXDescriptor.BTCTRL.reg =0x1408;	// Source addressincrement, Event Strobe when
//											// block transfer is complete, channel will be disabled with
//											// last block transfer with  a block interrupt. beat size is 1 
//	usartTXDescriptor.BTCTRL.reg |= DMAC_BTCTRL_BEATSIZE_BYTE|DMAC_BTCTRL_BLOCKACT_INT|DMAC_BTCTRL_SRCINC|DMAC_BTCTRL_STEPSEL_SRC|DMAC_BTCTRL_STEPSIZE_X1;
//	usartTXDescriptor.BTCNT.reg=size;
//	usartTXDescriptor.SRCADDR.reg = (uint32_t)txBuffer+size;
//	usartTXDescriptor.DSTADDR.reg = (uint32_t)&SERCOM1->USART.DATA.reg;
//	usartTXDescriptor.BTCTRL.reg |= DMAC_BTCTRL_VALID;
//	usartTXDescriptor.DESCADDR.reg =0;
//	DMAC->BASEADDR.reg =(uint32_t)&usartTXDescriptor;
//	DMAC->WRBADDR.reg = (uint32_t)&wbDmacMem;
//	DMAC->CHID.reg |= DMAC_CHID_ID(0);
//	DMAC->PRICTRL0.bit.LVLPRI1=2;
//	//DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_RUNSTDBY;
//	//DMAC->CTRL.bit.LVLEN0=1;
//	DMAC->CHINTENSET.reg |= DMAC_CHINTENSET_TCMPL;
//	UART_DMAC_SAFE_END
//	__enable_irq();
//	DMAC->SWTRIGCTRL.bit.SWTRIG0 =1;
//	while(dataSendCompleteFlag==false);
//		return dataSendCompleteFlag;
//}

//void DMAC_0_Handler(void){
//	if(DMAC->CHINTFLAG.bit.TCMPL==1)
//	{
//		dataSendCompleteFlag=true;
//	}
//}


