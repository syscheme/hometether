#define  BSP_INT_MODULE
//#include <bsp.h>
//#include  "..\BSP\bsp.h"
#include "includes.h"
#include "nrf24l01.h"
#include "si4432.h"
#include "usart.h"

#include "pulses.h"

#ifdef FreeRTOS
#  include "FreeRTOS.h"
#endif // FreeRTOS

#define PLUSE_MAX             (40)
#define PLUSE_CAP_SAMPLE_USEC (50) // 50usec

// uint16_t pluscapbuf[PLUSE_MAX*2+2];
// uint8_t seq[50];
// PulseSeq pulses = {50, 0, seq};
// PendingPulseSeq pendingPulseSeq ={{50, 0, seq},0};

void ISR_ADC1DMA(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC1))
		DMA_ClearITPendingBit(DMA1_IT_GL1);
}

#ifdef USB_ENABLED
#include "usb_istr.h"
void USBRX0(void)_IRQHandler
{
	USB_Istr();
}
#endif // USB_ENABLED

// ============================================
// true implementations of ####()_IRQHandler
// ============================================
void ISR_OnTimer_1msec(void)
{
	if (RESET != TIM_GetITStatus(TIM2, TIM_IT_Update))
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		gClock_step1msec();

		if (0 == (gClock_msec% BLINK_INTERVAL_MSEC)) // equals %=32 => 32msec/blink-tick, almost 30fps
			BlinkList_doTick();
	}
}

void ISR_PulseCapture(void)  // Exti Line 5~9
{
	static uint32_t lastCnt[CCP_NUM];
	uint8_t i, flags = RNode_readPlusePinStatus();
	uint32_t cnt = TIM_GetCounter(TIM2)/10 + gClock_msec *100; // the counter in 10usec

	if (RESET != EXTI_GetITStatus(EXTI_Line6))	// B6/TIM4_C1 connects to ASK receiver
	{
		i = RNode_indexByExtiLine(EXTI_Line6);
		if (i < CCP_NUM) // valid index
			PulseCapture_Capfill(&pulsechs[i], cnt - lastCnt[i], (flags & (1<<i))?TRUE:FALSE);
		lastCnt[i] = cnt;
	}

	//	if (RESET != EXTI_GetITStatus(EXTI_Line9))	// B8 connects to IR receiver
	//	{
	//		i = RNode_indexByExtiLine(EXTI_Line9);
	//		if (i < CCP_NUM) // valid index
	//			PulseCapture_Capfill(&pulsechs[i], cnt - lastCnt[i], (flags & (1<<i))?TRUE:FALSE);
	//		lastCnt[i] = cnt;
	//	}

	EXTI_ClearITPendingBit(EXTI_Line5 | EXTI_Line6 | EXTI_Line7 | EXTI_Line8 | EXTI_Line9);
}

void ISR_Si4432(void) 
{
	// takes nRF24L01's IRQ B0 instead, because of the damaged B0
	// if (EXTI_GetITStatus(EXTI_Line1) != RESET)
	// {
	// 	EXTI_ClearITPendingBit(EXTI_Line1);
	// 	SI4432_doISR(&si4432);
	// }

	if (EXTI_GetITStatus(EXTI_Line0) != RESET)
	{
		EXTI_ClearITPendingBit(EXTI_Line0);
		SI4432_doISR(&si4432);
	}
}

// @brief  This function handles USARTz global interrupt request.
void ISR_USART1(void)
{
	USART_doISR(&COM1);
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

