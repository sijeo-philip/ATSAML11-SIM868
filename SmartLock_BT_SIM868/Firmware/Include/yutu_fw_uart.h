/*
 *					HEADER FILE
 * yutu_fw_uart.h
 *
 * Created: 11-Feb-19 13:15:02 PM
 * Description : These functions are used to read and write from the UART
 * Author : Sijeo Philip
 */ 

#ifndef _YUTU_FW_UART_H_
#define _YUTU_FW_UART_H_
#include <stdint.h>
#include <stdbool.h>

#define UART_SAFE_START  {\
	HW_INIT_UART_SERCOM->USART.CTRLA.reg &= 0xFFFFFFDF;  // Disable UART
#define UART_SAFE_END	HW_INIT_UART_SERCOM->USART.CTRLA.reg|=SERCOM_USART_CTRLA_ENABLE;}

#define UART_DMAC_SAFE_START {\
	DMAC->CHCTRLA.bit.ENABLE =0;\
	DMAC->CTRL.bit.DMAENABLE =0; 
	
#define UART_DMAC_SAFE_END   DMAC->CHCTRLA.bit.ENABLE =1;\
							DMAC->CTRL.bit.DMAENABLE =1;\
}

uint16_t yutu_fw_uart_recieve(char* recvBuffer);

bool yutu_fw_uart_send(char* txBuffer, int size);


#endif
