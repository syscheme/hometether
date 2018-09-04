// =========================================================================
//                 STM32F10x Peripherals Interrupt Handlers                  
//  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the
//  available peripheral interrupt handler's name please refer to the startup
//  file (startup_stm32f10x_xx.s).
// =========================================================================

#include "bsp.h"
#include "secu.h"

#ifdef FreeRTOS
#  include "FreeRTOS.h"
#endif // FreeRTOS

// =========================================================================
// Customized ISRs for SECU
// =========================================================================
#if 0
// --------------------------------
// ISR_RTC()
// --------------------------------
void ISR_RTC(void)
{
	uint32_t counter =0;
	if (RTC_GetITStatus(RTC_IT_SEC) == RESET)
		return;

	// clear the RTC Second interrupt
	RTC_ClearITPendingBit(RTC_IT_SEC);
	// wait until last write operation on RTC registers has finished
	RTC_WaitForLastTask();

	// enable time update
	// TimeDisplay = 1;

	// Reset RTC Counter every week
	counter = RTC_GetCounter();

	// format the counter: the highest byte means the day of week, and lower 3-byte means the second of the day
	if (counter & 0xffffff == 60*60*24)
	{
		counter >>=24;
		counter = ++counter %7;
		counter <<=24;

		RTC_SetCounter(counter);
		RTC_WaitForLastTask(); // wait until last write operation on RTC registers has finished
	}
}
#endif // 0

#ifdef SECU_CAN
// --------------------------------
// ISR_CAN1_RX()
// --------------------------------
extern void canReceiveLoop(uint8_t);

void ISR_CAN1_RX(void)
{
	canReceiveLoop(MSG_CH_CAN1);
}
#endif // SECU_CAN

// --------------------------------
// ISR_OnTimer_1msec()
// --------------------------------
void ISR_OnTimer_1msec(void)
{
	if (RESET != TIM_GetITStatus(TIM2, TIM_IT_Update))
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		gClock_step1msec();
	}
}


extern void RS485_OnMessage(char* msg);

void ISR_RS485(void)
{
	static uint8_t buf[10], i=0;
	for (i=0; i < sizeof(buf) -1 && RESET != USART_GetFlagStatus(USART1, USART_FLAG_RXNE); i++)
		buf[i] = USART_ReceiveData(USART1);
		
	if (i>0)
		MsgLine_recv(MSG_CH_RS485_Received, buf, i);

/*
	static uint8_t buf[64], buf_i=0;
	while (RESET != USART_GetFlagStatus(USART3, USART_FLAG_RXNE))
	{ 
		buf[buf_i] = USART_ReceiveData(USART3);
		if ('\r' != buf[buf_i] && '\n' != buf[buf_i])
		{
			buf_i = ++buf_i % (sizeof(buf)-2);
			continue;
		}

		// a line just received
		buf[buf_i++] = '\0'; // NULL terminate the string
		buf_i =0; // reset for next receiving

		if ('\0' == buf[0]) // empty line
			continue;

		// a valid line here
		processTxtMessage(fext, (char*)buf);
	}
*/

	USART_ClearITPendingBit(USART3, USART_IT_RXNE);
}

void ISR_ADC1DMA(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC1))
       DMA_ClearITPendingBit(DMA1_IT_GL1);
}

void ISR_EXTI(void)
{
#ifdef EN28J60_INT
	if (EXTI_GetITStatus(EXTI_Line7) != RESET)	// handling of IR receiver as EXTi
	{
		GW_nicReceiveLoop(&nic);
		EXTI_ClearITPendingBit(EXTI_Line7);
	}
#endif // EN28J60_INT
}

// @brief  This function handles USARTz global interrupt request.
void ISR_RS232(void)
{
	USART_doISR(&RS232);
/*
	uint8_t ch;
	if (USART_GetITStatus(COM1.port, USART_IT_TXE) != RESET)
	{
		// !!!MUST NOT!!! CALL USART_ClearITPendingBit(USART1, USART_IT_TXE);

		// Read one byte from the outgoing FIFO
		if (ERR_SUCCESS == FIFO_pop(&COM1.txFIFO, &ch))
			IO_USART_putc(COM1.port, ch);
		else USART_ITConfig(COM1.port, USART_IT_TXE, DISABLE); // will be turned on by USART_transmit()
	}

	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		// Read one byte from the receive data register
		ch = IO_USART_getc(COM1.port);
		USART_ClearITPendingBit(COM1.port, USART_IT_RXNE);

		FIFO_push(&COM1.rxFIFO, &ch); 
	}
*/
}

#ifndef FreeRTOS

// ============================================
// Mapping ####_IRQHandler() to .s
// ============================================
void  SysTick_IRQHandler	   (void)  {}
void  PendSV_IRQHandler 	   (void)  {}
void  WWDG_IRQHandler          (void)  {}
void  PVD_IRQHandler           (void)  {}
void  TAMPER_IRQHandler        (void)  {}
void  RTC_IRQHandler           (void)  {}
void  FLASH_IRQHandler         (void)  {}
void  RCC_IRQHandler           (void)  {}
void  EXTI0_IRQHandler         (void)  { ISR_Si4432(); }  // {} <= takes nRF24L01's IRQ B0 instead, because of the damaged B0
void  EXTI1_IRQHandler         (void)  {} // { ISR_Si4432(); } <= takes nRF24L01's IRQ B0 instead, because of the damaged B0
void  EXTI2_IRQHandler         (void)  {}
void  EXTI3_IRQHandler         (void)  {}
void  EXTI4_IRQHandler         (void)  {}
void  DMA1_CH1_IRQHandler      (void)  { ISR_ADC1DMA(); }
void  DMA1_CH2_IRQHandler      (void)  {}
void  DMA1_CH3_IRQHandler      (void)  {}
void  DMA1_CH4_IRQHandler      (void)  {}
void  DMA1_CH5_IRQHandler      (void)  {}
void  DMA1_CH6_IRQHandler      (void)  {}
void  DMA1_CH7_IRQHandler      (void)  {}
void  ADC_IRQHandler           (void)  {}
void  USB_HP_CAN_TX_IRQHandler (void)  {}
void  USB_LP_CAN_RX0_IRQHandler(void)  {}
void  CAN_RX1_IRQHandler       (void)  {}
void  CAN_SCE_IRQHandler       (void)  {}
void  EXTI9_5_IRQHandler       (void)  { ISR_PulseCapture(); }
void  TIM1_BRK_IRQHandler      (void)  {}
void  TIM1_UP_IRQHandler       (void)  {}
void  TIM1_TRG_COM_IRQHandler  (void)  {}
void  TIM1_CC_IRQHandler       (void)  {}
void  TIM2_IRQHandler          (void)  { ISR_OnTimer_1msec(); }
void  TIM3_IRQHandler          (void)  {}
void  TIM4_IRQHandler          (void)  {}
void  I2C1_EV_IRQHandler       (void)  {}
void  I2C1_ER_IRQHandler       (void)  {}
void  I2C2_EV_IRQHandler       (void)  {}
void  I2C2_ER_IRQHandler       (void)  {}
void  SPI1_IRQHandler          (void)  {}
void  SPI2_IRQHandler          (void)  {}
void  USART1_IRQHandler        (void)  { ISR_USART1(); }
void  USART2_IRQHandler        (void)  {}
void  USART3_IRQHandler        (void)  {}
void  EXTI15_10_IRQHandler     (void)  {}
void  RTCAlarm_IRQHandler      (void)  {}
void  USBWakeUp_IRQHandler     (void)  {}

#endif // !FreeRTOS 
