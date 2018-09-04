// =========================================================================
//                 STM32F10x Peripherals Interrupt Handlers                  
//  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the
//  available peripheral interrupt handler's name please refer to the startup
//  file (startup_stm32f10x_xx.s).
// =========================================================================

#include "bsp.h"

#ifdef FreeRTOS
#  include "FreeRTOS.h"
#endif // FreeRTOS

// =========================================================================
// Customized ISRs for PECU
// =========================================================================

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

void ISR_ADC_EOC(void)
{
    ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
}

extern void RS485_OnMessage(char* msg);


void ISR_ADC1DMA(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC1))
       DMA_ClearITPendingBit(DMA1_IT_TC1); // DMA1_IT_GL1);
}

void ISR_EXTI(void)
{
}

// @brief  This function handles USARTz global interrupt request.
void ISR_TTY1(void)
{
	USART_doISR(&TTY1);
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
void  EXTI0_IRQHandler         (void)  {}
void  EXTI1_IRQHandler         (void)  {}
void  EXTI2_IRQHandler         (void)  {}
void  EXTI3_IRQHandler         (void)  {}
void  EXTI4_IRQHandler         (void)  {}
void  DMA1_Channel1_IRQHandler (void)  { ISR_ADC1DMA(); }
void  DMA1_Channel2_IRQHandler (void)  {}
void  DMA1_Channel3_IRQHandler (void)  {}
void  DMA1_Channel4_IRQHandler (void)  {}
void  DMA1_Channel5_IRQHandler (void)  {}
void  DMA1_Channel6_IRQHandler (void)  {}
void  DMA1_Channel7_IRQHandler (void)  {}
void  ADC12_IRQHandler         (void)  { ISR_ADC_EOC(); }
void  CAN_TX_IRQHandler (void)  {} // USB_HP_CAN_TX_IRQHandler (void)  {}
void  CAN_RX0_IRQHandler(void)  {} // USB_LP_CAN_RX0_IRQHandler(void)  {}
void  CAN_RX1_IRQHandler       (void)  {}
void  CAN_SCE_IRQHandler       (void)  {}
void  EXTI9_5_IRQHandler       (void)  {}
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
void  USART1_IRQHandler        (void)  { ISR_TTY1(); }
void  USART2_IRQHandler        (void)  {}
void  USART3_IRQHandler        (void)  {}
void  EXTI15_10_IRQHandler     (void)  {}
void  RTCAlarm_IRQHandler      (void)  {}
void  USBWakeUp_IRQHandler     (void)  {}
//////////////////
void  TIM8_BRK_IRQHandler      (void)  {}
void  TIM8_UP_IRQHandler       (void)  {}
void  TIM8_TRG_COM_IRQHandler  (void)  {}
void  TIM8_CC_IRQHandler       (void)  {}
void  ADC3_IRQHandler          (void)  {}
void  FSMC_IRQHandler          (void)  {}
void  SDIO_IRQHandler          (void)  {}

// stm32f10x only
void  TIM5_IRQHandler     (void)  {}
void  SPI3_IRQHandler     (void)  {}
void  UART4_IRQHandler     (void)  {}
void  UART5_IRQHandler     (void)  {}
void  TIM6_IRQHandler     (void)  {}
void  TIM7_IRQHandler     (void)  {}
void  DMA2_Channel1_IRQHandler     (void)  {}
void  DMA2_Channel2_IRQHandler     (void)  {}
void  DMA2_Channel3_IRQHandler     (void)  {}
void  DMA2_Channel4_5_IRQHandler     (void)  {}

// stm32f105/7 connectivity line only begin
void  DMA2_Channel5_IRQHandler     (void)  {}
void  ETH_IRQHandler     (void)  {}
void  ETH_WKUP_IRQHandler     (void)  {}
void  CAN2_TX_IRQHandler     (void)  {}
void  CAN2_RX0_IRQHandler     (void)  {}
void  CAN2_RX1_IRQHandler     (void)  {}
void  CAN2_SCE_IRQHandler     (void)  {}
void  OTG_FS_IRQHandler      (void)  {}
// stm32f105/7 connectivity line only end
///////////////////////////////
void  uHardFault_Handler      (void)  {}
void  vPortSVCHandler      (void)  {}
void  xPortPendSVHandler      (void)  {}
void  xPortSysTickHandler      (void)  {}
#endif // !FreeRTOS 
