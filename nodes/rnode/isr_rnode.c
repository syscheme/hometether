#define  BSP_INT_MODULE
//#include <bsp.h>
//#include  "..\BSP\bsp.h"
#include "includes.h"
#include "nrf24l01.h"
#include "si4432.h"

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
void IntHandler_USBRX0(void)
{
	 USB_Istr();
}
#endif // USB_ENABLED

// ============================================
// true implementations of IntHandler_####()
// ============================================
uint32_t global_timer_msec =0;
uint16_t gcnt =0;
void ISR_OnTimer_1msec(void)
{
gcnt = TIM_GetCounter(TIM2);
	if (RESET != TIM_GetITStatus(TIM2, TIM_IT_Update))
	{
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	global_timer_msec++;
	gcnt++;

	if (0 == (global_timer_msec& 0x1f)) // equals %=32 => 32msec/blink-tick, almost 30fps
		BlinkList_doTick();
	}
}

/*
void TIM2_IRQHandler(void)
{
 
 if(TIM_GetITStatus(TIM2,TIM_IT_Update)!=RESET)
 {
  TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
  GPIO_WriteBit(GPIOC,GPIO_Pin_6,(BitAction)(1-GPIO_ReadOutputDataBit(GPIOC,GPIO_Pin_6)));
 }
}
*/
void ISR_PulseCapture(void)  // Exti Line 5~9
{
	static uint32_t lastCnt[CCP_NUM];
	uint8_t i, flags = RNode_readPlusePinStatus();
	uint32_t cnt = TIM_GetCounter(TIM2)/10 + global_timer_msec *100; // the counter in 10usec

	if (RESET != EXTI_GetITStatus(EXTI_Line8))	// B8 connects to IR receiver
	{
		i = RNode_indexByExtiLine(EXTI_Line8);
		if (i < CCP_NUM) // valid index
			PulseCapture_Capfill(&pulsechs[i], cnt - lastCnt[i], (flags & (1<<i))?TRUE:FALSE);
		lastCnt[i] = cnt;
	}

	if (RESET != EXTI_GetITStatus(EXTI_Line9))	// B8 connects to IR receiver
	{
		i = RNode_indexByExtiLine(EXTI_Line9);
		if (i < CCP_NUM) // valid index
			PulseCapture_Capfill(&pulsechs[i], cnt - lastCnt[i], (flags & (1<<i))?TRUE:FALSE);
		lastCnt[i] = cnt;
	}

	EXTI_ClearITPendingBit(EXTI_Line5 | EXTI_Line6 | EXTI_Line7 | EXTI_Line8 | EXTI_Line9);
}

void ISR_Si4432(void)  // Exti Line B1
{
	if (EXTI_GetITStatus(EXTI_Line1) != RESET)
	{
		SI4432_doISR(&si4432);
		EXTI_ClearITPendingBit(EXTI_Line1);
	}

}

#ifndef FreeRTOS

// ============================================
// Mapping IntHandler_####()
// ============================================
void  IntHandler_SysTick	   (void)  {}
void  IntHandler_PendSV 	   (void)  {}
void  IntHandler_WWDG          (void)  {}
void  IntHandler_PVD           (void)  {}
void  IntHandler_TAMPER        (void)  {}
void  IntHandler_RTC           (void)  {}
void  IntHandler_FLASH         (void)  {}
void  IntHandler_RCC           (void)  {}
void  IntHandler_EXTI0         (void)  {}
void  IntHandler_EXTI1         (void)  { ISR_Si4432(); }
void  IntHandler_EXTI2         (void)  {}
void  IntHandler_EXTI3         (void)  {}
void  IntHandler_EXTI4         (void)  {}
void  IntHandler_DMA1_CH1      (void)  { ISR_ADC1DMA(); }
void  IntHandler_DMA1_CH2      (void)  {}
void  IntHandler_DMA1_CH3      (void)  {}
void  IntHandler_DMA1_CH4      (void)  {}
void  IntHandler_DMA1_CH5      (void)  {}
void  IntHandler_DMA1_CH6      (void)  {}
void  IntHandler_DMA1_CH7      (void)  {}
void  IntHandler_ADC1_2        (void)  {}
void  IntHandler_USB_HP_CAN_TX (void)  {}
void  IntHandler_USB_LP_CAN_RX0(void)  {}
void  IntHandler_CAN_RX1       (void)  {}
void  IntHandler_CAN_SCE       (void)  {}
void  IntHandler_EXTI9_5       (void)  { ISR_PulseCapture(); }
void  IntHandler_TIM1_BRK      (void)  {}
void  IntHandler_TIM1_UP       (void)  {}
void  IntHandler_TIM1_TRG_COM  (void)  {}
void  IntHandler_TIM1_CC       (void)  {}
void  IntHandler_TIM2          (void)  { ISR_OnTimer_1msec(); }
void  IntHandler_TIM3          (void)  {}
void  IntHandler_TIM4          (void)  {}
void  IntHandler_I2C1_EV       (void)  {}
void  IntHandler_I2C1_ER       (void)  {}
void  IntHandler_I2C2_EV       (void)  {}
void  IntHandler_I2C2_ER       (void)  {}
void  IntHandler_SPI1          (void)  {}
void  IntHandler_SPI2          (void)  {}
void  IntHandler_USART1        (void)  {}
void  IntHandler_USART2        (void)  {}
void  IntHandler_USART3        (void)  {}
void  IntHandler_EXTI15_10     (void)  {}
void  IntHandler_RTCAlarm      (void)  {}
void  IntHandler_USBWakeUp     (void)  {}
#endif // !FreeRTOS 

