#include "yutu_fw_hw_init.h"
#include "yutu_fw_uart.h"

void at_response_processor_thread(void)__attribute__((noreturn));
void atcmd_dispatcher_thread(void)__attribute__((noreturn));
void atcmd_tester_thread(void)__attribute__((noreturn));

void yutu_fw_init(void)
{
  yutu_fw_hw_init_sys();
  yutu_fw_hw_init_gpio();
  UART_SAFE_START
  yutu_fw_hw_init_uart_sercom();
  yutu_fw_hw_init_response_timer();
  UART_SAFE_END
  
}

void 
yutu_fw_scheduler_start(void){
	scheduler_os_init();
	scheduler_os_task_init(&atcmd_tester_thread);
	scheduler_os_task_init(&atcmd_dispatcher_thread);
	scheduler_os_task_init(&at_response_processor_thread);
	scheduler_os_start(20);
}
void yutu_fw_hw_init_sys()
{
  // PAC_SEC->WRCTRL.reg = PAC_WRCTRL_PERID(ID_PORT) | PAC_WRCTRL_KEY_CLR;
   PAC_SEC->WRCTRL.reg = PAC_WRCTRL_PERID(ID_DMAC) | PAC_WRCTRL_KEY_CLR;

  // Set to highest performance level.
  PM->INTFLAG.reg = PM_INTFLAG_PLRDY;
  PM->PLCFG.reg = PM_PLCFG_PLSEL_PL2;
  while (0 == PM->INTFLAG.bit.PLRDY);

  // Enable Internal Oscillator of 16MHz without Pre-scaler
  OSCCTRL->OSC16MCTRL.reg =
      OSCCTRL_OSC16MCTRL_ENABLE | OSCCTRL_OSC16MCTRL_FSEL_16;

  // Please disable the below lines in the release version when flashed to
  // actual hardware
  PORT_SEC->Group[0].PMUX[PIN_PA27].bit.PMUXO |=
      0x07;			  // Peripheral H is opted for PIN_PA27
  GCLK->GENCTRL->bit.OE |= 0x01;  // Enable output of GCLK0 to PIN_PA27
  NVIC->ISER[1] |= (1 << 2);      // Enable Timer 0 Interrupt for One shot timer
  NVIC->ISER[0] |= (1 << 28);     // Enable Sercom 1 usart RXC Interrupt.
	NVIC_EnableIRQ(DMAC_0_IRQn);
	//NVIC_SetPriority(DMAC_0_IRQn, 0x07);
}

void yutu_fw_hw_init_gpio()
{
  MCLK->APBAMASK.bit.PORT_ = 1;

  // Enable the below line in final release version
  // HW_INIT_SET_GPIO_INPUT(PIN_PA27);  //ACC_INT1 input from the accelerometer
  HW_INIT_SET_GPIO_INPUT(PIN_PA25);  // ACC_INT2 input from the accelerometer
  HW_INIT_SET_GPIO_INPUT(PIN_PA19);  // BOOT_PIN for Boot Loading
  HW_INIT_SET_GPIO_INPUT(PIN_PA05);  // INT Pin from the Battery Charger
  HW_INIT_SET_GPIO_INPUT(PIN_PA01);  // MCU_SW input from Tactile Switch

  // Secure mode for writing into the PORT Registers

  HW_INIT_SET_GPIO_OUTPUT(PIN_PA24);  // UART_SEL pin for switching the UART MUX SELECTION
  HW_INIT_SET_GPIO_OUTPUT(PIN_PA18);  // FLASH_WP# pin which is connected to the
				      // external flash for Write protection
  HW_INIT_SET_GPIO_OUTPUT(PIN_PA10);  // MIN- Signal to control the Motor
  HW_INIT_SET_GPIO_OUTPUT(PIN_PA11);  // MIN+ Signal to Control the Motor
  HW_INIT_SET_GPIO_OUTPUT(PIN_PA00);  // SIM_PWR Signal to Power ON/OFF the SIMCOM module
}

void yutu_fw_hw_init_uart_sercom()
{
  HW_INIT_CONFIGURE_PMUX(RX);  // Port configuration is done for PIN_PA17 to function as USART_RX
  HW_INIT_CONFIGURE_PMUX(TX);  // Port configuration is done for PIN_PA16 to function as USART_TX

  MCLK->HW_INIT_UART_SERCOM_MASK_REG.reg |=
      HW_INIT_UART_SERCOM_MASK_BIT;  // Enable the APBCCLOCK for the sercom 1  peripheral
  // GCLK Generator 0 is enable to supply clock to SERCOM1_CORE
  GCLK->PCHCTRL[HW_INIT_UART_SERCOM_GCLK_ID].reg |=
      GCLK_PCHCTRL_GEN(0) | GCLK_PCHCTRL_CHEN;
  // Wait till the clock stabilizes as write synchronization is required.
  while (0 == (GCLK->PCHCTRL[HW_INIT_UART_SERCOM_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));
	//Enable clock to the AHB bus of DMAC for the TX of SERCOM1 USART
	MCLK->AHBMASK.bit.DMAC_ |= MCLK_AHBMASK_DMAC;  
	  // Initializing the SERCOM for USART
  HW_INIT_UART_SERCOM->USART.CTRLA.reg |=
      SERCOM_USART_CTRLA_DORD | SERCOM_USART_CTRLA_MODE(1 /*INT CLOCK*/) |
      SERCOM_USART_CTRLA_FORM(0 /*USART*/) | SERCOM_USART_CTRLA_SAMPR(1) |
      HW_INIT_UART_SERCOM_RXPO | HW_INIT_UART_SERCOM_TXPO;
  // UART sercom 1 RX and TX is enabled on PAD0 and PAD1 and the data size is
  // configured as 8 bit/ 1 byte
  HW_INIT_UART_SERCOM->USART.CTRLB.reg |=
      SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_RXEN |
      SERCOM_USART_CTRLB_CHSIZE(0 /*8 BIT*/);

#define BAUD_VAL (16000000 / (16 * HW_INIT_UART_BAUDRATE))
#define FP_VAL ((16000000 / HW_INIT_UART_BAUDRATE - 16 * BAUD_VAL) / 2)

  // The baud rate is configured to 115200 w.r.t to the clock frequency of 16MHz
  HW_INIT_UART_SERCOM->USART.BAUD.reg =
      SERCOM_USART_BAUD_FRACFP_BAUD(BAUD_VAL) |
      SERCOM_USART_BAUD_FRACFP_FP(FP_VAL);
  HW_INIT_UART_SERCOM->USART.INTENSET.reg |= SERCOM_USART_INTENSET_RXC;
}

/*
// Call this function before UART initialization.
 void yutu_fw_hw_init_rxdma(void* recvBuffer, DmacDescriptor *dmacDesc,
DmacDescriptor *wbDmacMem)
{
	
	usartRXDescriptor.BTCNT =1024;		// block is of size 1024 bytes.
	usartRXDescriptor.BTCTRL = 0x080B;	// Destination address
increment, Event Strobe when
											// block transfer is complete, channel will be disabled with
											// last block transfer with  a block interrupt. beat size is 1 byte
	usartRXDescriptor.DESCADDR =0;
	usartRXDescriptor.DESCADDR = recvBuffer;
	usartRXDescriptor.SRCADDR = &SERCOM1->USART.DATA.reg;
	DMAC->BASEADDR.reg = dmacDesc;
	DMAC->WRBADDR.reg = wbDmacMem;
	DMAC->CHID.bit.ID = 0;
	DMAC->CHCTRLB.bit.TRIGSRC = SERCOM1_DMAC_ID_RX;
	DMAC->CHCTRLB.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT;
	DMAC->CHCTRLA.bit.ENABLE =1;   // Enable the Channel
	DMAC->CTRL.bit.DMAENABLE =1; // Enable the DMA
}
*/

void yutu_fw_hw_init_response_timer()
{
  MCLK->APBCMASK.bit.TC0_ |= MCLK_APBCMASK_TC0;
  GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(0);
  PAC->NONSECC.bit.TC0_;
  TC0->COUNT16.CTRLBSET.reg |=
      TC_CTRLBSET_DIR | TC_CTRLBSET_CMD_RETRIGGER;  // Timer is counting down.
  while (TC0->COUNT16.SYNCBUSY.bit.CTRLB == 1);
  TC0->COUNT16.WAVE.bit.WAVEGEN = 1;  // Match Frequencey with CC0 Register.
  TC0->COUNT16.CC->reg = 4000;       // 250us delay generated
  while (TC0->COUNT16.SYNCBUSY.bit.CC0 == 1);
  TC0->COUNT16.INTENSET.reg = TC_INTENSET_MC1;
  TC0->COUNT16.CTRLBSET.reg |= TC0->COUNT16.CTRLA.bit.ENABLE = 1;
  while (TC0->COUNT16.SYNCBUSY.bit.ENABLE == 1);
}
