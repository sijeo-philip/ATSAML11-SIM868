
/*
 *					HEADER FILE
 * yutu_fw_hw_init.h
 *
 * Created: 02-Feb-19 9:15:02 PM
 * Description : These functions are used to initialize the Hardware peripherals
 *				 Which includes the GPIOs, UART, SPI and I2C
 * Author : Sijeo Philip
 */

#ifndef _YUTU_FW_HW_INIT_H_
#define _YUTU_FW_HW_INIT_H_
#define HW_INIT_UART_TX_PIN PIN_PA16C_SERCOM1_PAD0
#define HW_INIT_UART_TX_MUX MUX_PA16C_SERCOM1_PAD0

#define HW_INIT_UART_RX_PIN PIN_PA17C_SERCOM1_PAD1
#define HW_INIT_UART_RX_MUX MUX_PA17C_SERCOM1_PAD1

#define HW_INIT_UART_SERCOM SERCOM1
#define HW_INIT_UART_SERCOM_GCLK_ID SERCOM1_GCLK_ID_CORE
#define HW_INIT_UART_SERCOM_MASK_REG APBCMASK
#define HW_INIT_UART_SERCOM_MASK_BIT MCLK_APBCMASK_SERCOM1
#define HW_INIT_UART_SERCOM_TXPO SERCOM_USART_CTRLA_TXPO(0 /*PAD0*/)
#define HW_INIT_UART_SERCOM_RXPO SERCOM_USART_CTRLA_RXPO(1 /*PAD1*/)

#define HW_INIT_UART_BAUDRATE 115200

#include "saml11e16a.h"

#define HW_INIT_SET_GPIO_INPUT(pin)             \
  {                                             \
    PORT_SEC->Group[0].DIRCLR.reg = (1 << pin); \
    PORT_SEC->Group[0].OUTSET.reg = (1 << pin); \
    PORT_SEC->Group[0].PINCFG[pin].reg =        \
	PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;  \
    PORT_SEC->Group[0].CTRL.reg |= (1 << pin);  \
  }

#define HW_INIT_SET_GPIO_OUTPUT(pin)            \
  {                                             \
    PORT_SEC->Group[0].DIRSET.reg = (1 << pin); \
    PORT_SEC->Group[0].OUTCLR.reg = (1 << pin); \
  }

#define HW_INIT_CONFIGURE_PMUX(dir)                                        \
  do                                                                       \
    {                                                                      \
      PORT_SEC->Group[0].PINCFG[HW_INIT_UART_##dir##_PIN].reg |=           \
	  PORT_PINCFG_PMUXEN;                                              \
      if (HW_INIT_UART_##dir##_PIN & 1)                                    \
	PORT_SEC->Group[0].PMUX[HW_INIT_UART_##dir##_PIN >> 1].bit.PMUXO = \
	    HW_INIT_UART_##dir##_MUX;                                      \
      else                                                                 \
	PORT_SEC->Group[0].PMUX[HW_INIT_UART_##dir##_PIN >> 1].bit.PMUXE = \
	    HW_INIT_UART_##dir##_MUX;                                      \
    }                                                                      \
  while (0)

void yutu_fw_hw_init_dma(void* recvBuffer);

/************************************************************************/
/* Function		: yutu_fw_sys_init
 */
/* Description	: This function initializes the sys clocks
 */
/* Input		: None
 */
/* Output		: None
 */
/* Return		: None */
/************************************************************************/

void yutu_fw_hw_init_sys(void);
/************************************************************************/
/* Function		: yutu_fw_gpio_init
 */
/* Description	: This function initializes the GPIOs for the function  */
/* Input		: None
 */
/* Output		: None
 */
/* Return		: None */
/************************************************************************/

void yutu_fw_hw_init_gpio(void);

/************************************************************************/
/* Function		: yutu_fw_uart_sercom_init
 */
/* Description	: This function initializes the UART for AT commands	*/
/* Input		: None
 */
/* Output		: None
 */
/* Return		: None */
/************************************************************************/
void yutu_fw_hw_init_uart_sercom(void);

/************************************************************************/
/* Function		: yutu_fw_i2c_sercom_init																	*/
/* Description	: This function initializes the I2C for I2C peripherals	*/
/* Input		: None																											*/
/* Output		: None																											*/
/* Return		: None 																											*/
/************************************************************************/

void yutu_fw_hw_init_i2c_sercom(void);

/************************************************************************/
/* Function		: yutu_fw_spi_sercom_init
 */
/* Description	: This function initializes the SPI for SPI peripherals	*/
/* Input		: None
 */
/* Output		: None
 */
/* Return		: None */
/************************************************************************/

void yutu_fw_hw_init_spi_sercom(void);

/************************************************************************/
/* Function		: yutu_fw_hw_init_response_timer(void)
 */
/* Description	: This function initializes TIMER 0 for at command resp */
/*				  time out to indicate response complete
 */
/* Input		: None
 */
/* Output		: None
 */
/* Return		: None */
/************************************************************************/

void yutu_fw_hw_init_response_timer(void);

/************************************************************************/
/* Function		: yutu_fw_init
 */
/* Description	: This function initializes the firmware internals */
/* Input		: None
 */
/* Output		: None
 */
/* Return		: None */
/************************************************************************/

void yutu_fw_init(void);

void
yutu_fw_scheduler_start(void);
#endif
