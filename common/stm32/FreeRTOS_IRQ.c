#include "FreeRTOSConfig.h"
#include "types.h"

//////////
// http://www.freertos.org/Debugging-Hard-Faults-On-Cortex-M-Microcontrollers.html
void readRegistersFromStack( uint32_t *pulFaultStackAddress )
{
	// These are volatile to try and prevent the compiler/linker optimising them
	// away as the variables never actually get used.  If the debugger won't show the
	// values of the variables, make them global my moving their declaration outside
	// of this function.
	volatile uint32_t r0;
	volatile uint32_t r1;
	volatile uint32_t r2;
	volatile uint32_t r3;
	volatile uint32_t r12;
	volatile uint32_t lr; // Link register.
	volatile uint32_t pc; // Program counter.
	volatile uint32_t psr;// Program status register.

	volatile uint32_t hfsr;// hard status fault
	volatile uint16_t ufsr;// usage fault status
	volatile uint8_t  bfsr;// bus fault status
	volatile uint8_t  mfsr;// memory management fault status

	volatile uint32_t bfar;// bus fault
	volatile uint32_t cfar;// mem fault
	volatile uint32_t dfar;// debug fault
	volatile uint32_t afar;// assitent fault
	static uint32_t t=0;

	r0  = pulFaultStackAddress[0];
	r1  = pulFaultStackAddress[1];
	r2  = pulFaultStackAddress[2];
	r3  = pulFaultStackAddress[3];

	r12 = pulFaultStackAddress[4];
	lr  = pulFaultStackAddress[5];
	pc  = pulFaultStackAddress[6];
	psr = pulFaultStackAddress[7];

	 // hard fault p126:
	 //  b31 - due to debug
	 //  b30 - escalated from bus-fault(bfar), memory-fault or usage fault
	 //  b01 - due to access NVIC
	hfsr = *((volatile uint32_t*) 0xe000ed2c);

	 // usage fault, 16-bit, p126:
	 //  b9  - divided by 0
	 //  b8  - unaligned access
	 //  b3  - intend to execute co-processor
	 //  b2  - intend to set PC to illegal value, stack overflow for FreeRTOS: stack too small or leak
	 //  b1  - intend to switch to ARM state from CM3
	 //  b0  - undefined instruction
	ufsr = *((volatile uint16_t*) 0xe000ed2a);

	 // bus fault status p123:
	 //  b7 - see bfar for details
	 //  b4 - push stack error
	 //  b3 - pop stack error
	 //  b2 - impresis error
	 //  b1 - precis error
	 //  b0 - read instruction from bus error
	bfsr = *((volatile uint8_t*) 0xe000ed29);

	 // memory management fault status 8-bit p124:
	 //  b7 - see mmar for details
	 //  b4 - push stack error
	 //  b3 - pop stack error
	 //  b1 - access data error
	 //  b0 - read instruction error
	mfsr = *((volatile uint8_t*) 0xe000ed28);

	bfar = *((volatile uint32_t*) 0xe000ed38);
	cfar = *((volatile uint32_t*) 0xe000ed28);
	dfar = *((volatile uint32_t*) 0xe000ed30);
	afar = *((volatile uint32_t*) 0xe000ed3c);
	
 	t = bfar + cfar + dfar + afar + r12 +lr +pc +psr + (hfsr +ufsr +bfsr +mfsr);

	// When the following line is hit, the variables contain the register values.
	for(;;);
}

__asm void uHardFault_Handler(void)	// void uHardFault_Handler(void) {  for(;;); }
{
	PRESERVE8
	IMPORT readRegistersFromStack 

	tst lr, #4
	ite eq
	mrseq r0, msp
	mrsne r0, psp
	ldr r1, [r0, #24]
  	B readRegistersFromStack

}

volatile uint8_t _interruptDepth =0; //the depth of interrupts
void WWDG_IRQHandler(void)
{
#ifdef uIRQ_WWDG
	_interruptDepth++;
	uIRQ_WWDG();
	_interruptDepth--;
#endif // WWDG
}

void PVD_IRQHandler(void)
{
#ifdef uIRQ_PVD
	_interruptDepth++;
	uIRQ_PVD();
	_interruptDepth--;
#endif // PVD
}

void TAMPER_IRQHandler(void)
{
#ifdef uIRQ_TAMPER
	_interruptDepth++;
	uIRQ_TAMPER();
	_interruptDepth--;
#endif // TAMPER
}

void RTC_IRQHandler(void)
{
#ifdef uIRQ_RTC
	_interruptDepth++;
	uIRQ_RTC();
	_interruptDepth--;
#endif // RTC
}

void FLASH_IRQHandler(void)
{
#ifdef uIRQ_FLASH
	_interruptDepth++;
	uIRQ_FLASH();
	_interruptDepth--;
#endif // FLASH
}

void RCC_IRQHandler(void)
{
#ifdef uIRQ_RCC
	_interruptDepth++;
	uIRQ_RCC();
	_interruptDepth--;
#endif // RCC
}

void EXTI0_IRQHandler(void)
{
#ifdef uIRQ_EXTI0
	_interruptDepth++;
	uIRQ_EXTI0();
	_interruptDepth--;
#endif // EXTI0
}

void EXTI1_IRQHandler(void)
{
#ifdef uIRQ_EXTI1
	_interruptDepth++;
	uIRQ_EXTI1();
	_interruptDepth--;
#endif // EXTI1
}

void EXTI2_IRQHandler(void)
{
#ifdef uIRQ_EXTI2
	_interruptDepth++;
	uIRQ_EXTI2();
	_interruptDepth--;
#endif // EXTI2
}

void EXTI3_IRQHandler(void)
{
#ifdef uIRQ_EXTI3
	_interruptDepth++;
	uIRQ_EXTI3();
	_interruptDepth--;
#endif // EXTI3
}

void EXTI4_IRQHandler(void)
{
#ifdef uIRQ_EXTI4
	_interruptDepth++;
	uIRQ_EXTI4();
	_interruptDepth--;
#endif // EXTI4
}

void DMA1_Channel1_IRQHandler(void)
{
#ifdef uIRQ_DMA1_Channel1
	_interruptDepth++;
	uIRQ_DMA1_Channel1();
	_interruptDepth--;
#endif // DMA1_Channel1
}

void DMA1_Channel2_IRQHandler(void)
{
#ifdef uIRQ_DMA1_Channel2
	_interruptDepth++;
	uIRQ_DMA1_Channel2();
	_interruptDepth--;
#endif // DMA1_Channel2
}

void DMA1_Channel3_IRQHandler(void)
{
#ifdef uIRQ_DMA1_Channel3
	_interruptDepth++;
	uIRQ_DMA1_Channel3();
	_interruptDepth--;
#endif // DMA1_Channel3
}

void DMA1_Channel4_IRQHandler(void)
{
#ifdef uIRQ_DMA1_Channel4
	_interruptDepth++;
	uIRQ_DMA1_Channel4();
	_interruptDepth--;
#endif // DMA1_Channel4
}

void DMA1_Channel5_IRQHandler(void)
{
#ifdef uIRQ_DMA1_Channel5
	_interruptDepth++;
	uIRQ_DMA1_Channel5();
	_interruptDepth--;
#endif // DMA1_Channel5
}

void DMA1_Channel6_IRQHandler(void)
{
#ifdef uIRQ_DMA1_Channel6
	_interruptDepth++;
	uIRQ_DMA1_Channel6();
	_interruptDepth--;
#endif // DMA1_Channel6
}

void DMA1_Channel7_IRQHandler(void)
{
#ifdef uIRQ_DMA1_Channel7
	_interruptDepth++;
	uIRQ_DMA1_Channel7();
	_interruptDepth--;
#endif // DMA1_Channel7
}

void ADC12_IRQHandler(void)
{
#ifdef uIRQ_ADC
	_interruptDepth++;
	uIRQ_ADC();
	_interruptDepth--;
#endif // ADC
}

#ifdef STM32F10X_CL
void CAN_TX_IRQHandler(void)
#else
void USB_HP_CAN_TX_IRQHandler(void)
#endif
{
#ifdef uIRQ_CAN_TX
	_interruptDepth++;
	uIRQ_CAN_TX();
	_interruptDepth--;
#endif // uIRQ_CAN_TX
}

#ifdef STM32F10X_CL
void CAN_RX0_IRQHandler(void)
#else
void USB_LP_CAN_RX0_IRQHandler(void)
#endif
{
#ifdef uIRQ_CAN_RX0
	_interruptDepth++;
	uIRQ_CAN_RX0();
	_interruptDepth--;
#endif // uIRQ_CAN_RX0
}

void CAN_RX1_IRQHandler(void)
{
#ifdef uIRQ_CAN_RX1
	_interruptDepth++;
	uIRQ_CAN_RX1();
	_interruptDepth--;
#endif // CAN_RX1
}

void CAN_SCE_IRQHandler(void)
{
#ifdef uIRQ_CAN_SCE
	_interruptDepth++;
	uIRQ_CAN_SCE();
	_interruptDepth--;
#endif // CAN_SCE
}

void EXTI9_5_IRQHandler(void)
{
#ifdef uIRQ_EXTI9_5
	_interruptDepth++;
	uIRQ_EXTI9_5();
	_interruptDepth--;
#endif // EXTI9_5
}

void TIM1_BRK_IRQHandler(void)
{
#ifdef uIRQ_TIM1_BRK
	_interruptDepth++;
	uIRQ_TIM1_BRK();
	_interruptDepth--;
#endif // TIM1_BRK
}

void TIM1_UP_IRQHandler(void)
{
#ifdef uIRQ_TIM1_UP
	_interruptDepth++;
	uIRQ_TIM1_UP();
	_interruptDepth--;
#endif // TIM1_UP
}

void TIM1_TRG_COM_IRQHandler(void)
{
#ifdef uIRQ_TIM1_TRG_COM
	_interruptDepth++;
	uIRQ_TIM1_TRG_COM();
	_interruptDepth--;
#endif // TIM1_TRG_COM
}

void TIM1_CC_IRQHandler(void)
{
#ifdef uIRQ_TIM1_CC
	_interruptDepth++;
	uIRQ_TIM1_CC();
	_interruptDepth--;
#endif // TIM1_CC
}

void TIM2_IRQHandler(void)
{
#ifdef uIRQ_TIM2
	_interruptDepth++;
	uIRQ_TIM2();
	_interruptDepth--;
#endif // TIM2
}

void TIM3_IRQHandler(void)
{
#ifdef uIRQ_TIM3
	_interruptDepth++;
	uIRQ_TIM3();
	_interruptDepth--;
#endif // TIM3
}

void TIM4_IRQHandler(void)
{
#ifdef uIRQ_TIM4
	_interruptDepth++;
	uIRQ_TIM4();
	_interruptDepth--;
#endif // TIM4
}

void I2C1_EV_IRQHandler(void)
{
#ifdef uIRQ_I2C1_EV
	_interruptDepth++;
	uIRQ_I2C1_EV();
	_interruptDepth--;
#endif // I2C1_EV
}

void I2C1_ER_IRQHandler(void)
{
#ifdef uIRQ_I2C1_ER
	_interruptDepth++;
	uIRQ_I2C1_ER();
	_interruptDepth--;
#endif // I2C1_ER
}

void I2C2_EV_IRQHandler(void)
{
#ifdef uIRQ_I2C2_EV
	_interruptDepth++;
	uIRQ_I2C2_EV();
	_interruptDepth--;
#endif // I2C2_EV
}

void I2C2_ER_IRQHandler(void)
{
#ifdef uIRQ_I2C2_ER
	_interruptDepth++;
	uIRQ_I2C2_ER();
	_interruptDepth--;
#endif // I2C2_ER
}

void SPI1_IRQHandler(void)
{
#ifdef uIRQ_SPI1
	_interruptDepth++;
	uIRQ_SPI1();
	_interruptDepth--;
#endif // SPI1
}

void SPI2_IRQHandler(void)
{
#ifdef uIRQ_SPI2
	_interruptDepth++;
	uIRQ_SPI2();
	_interruptDepth--;
#endif // SPI2
}

void USART1_IRQHandler(void)
{
#ifdef uIRQ_USART1
	_interruptDepth++;
	uIRQ_USART1();
	_interruptDepth--;
#endif // USART1
}

void USART2_IRQHandler(void)
{
#ifdef uIRQ_USART2
	_interruptDepth++;
	uIRQ_USART2();
	_interruptDepth--;
#endif // USART2
}

void USART3_IRQHandler(void)
{
#ifdef uIRQ_USART3
	_interruptDepth++;
	uIRQ_USART3();
	_interruptDepth--;
#endif // USART3
}

void EXTI15_10_IRQHandler(void)
{
#ifdef uIRQ_EXTI15_10
	_interruptDepth++;
	uIRQ_EXTI15_10();
	_interruptDepth--;
#endif // EXTI15_10
}

void RTCAlarm_IRQHandler(void)
{
#ifdef uIRQ_RTCAlarm
	_interruptDepth++;
	uIRQ_RTCAlarm();
	_interruptDepth--;
#endif // RTCAlarm
}

void USBWakeUp_IRQHandler(void)
{
#ifdef uIRQ_USBWakeUp
	_interruptDepth++;
	uIRQ_USBWakeUp();
	_interruptDepth--;
#endif // USBWakeUp
}

void TIM8_BRK_IRQHandler(void) // STM32F10xHD only interrupt
{
#ifdef uIRQ_TIM8_BRK
	_interruptDepth++;
	uIRQ_TIM8_BRK();
	_interruptDepth--;
#endif // uIRQ_TIM8_BRK
}

void TIM8_UP_IRQHandler(void) // STM32F10xHD only interrupt
{
#ifdef uIRQ_TIM8_UP
	_interruptDepth++;
	uIRQ_TIM8_UP();
	_interruptDepth--;
#endif // TIM8_UP
}

void TIM8_TRG_COM_IRQHandler(void) // STM32F10xHD only interrupt
{
#ifdef uIRQ_TIM8_TRG_COM
	_interruptDepth++;
	uIRQ_TIM8_TRG_COM();
	_interruptDepth--;
#endif // uIRQ_TIM8_TRG_COM
}

void TIM8_CC_IRQHandler(void) // STM32F10xHD only interrupt
{
#ifdef uIRQ_TIM8_CC
	_interruptDepth++;
	uIRQ_TIM8_CC();
	_interruptDepth--;
#endif // uIRQ_TIM8_CC
}

void ADC3_IRQHandler(void)
{
#ifdef uIRQ_ADC3
	_interruptDepth++;
	uIRQ_ADC3();
	_interruptDepth--;
#endif // uIRQ_ADC3
}

void FSMC_IRQHandler(void) // STM32F10xHD only interrupt
{
#ifdef uIRQ_FSMC
	_interruptDepth++;
	uIRQ_FSMC();
	_interruptDepth--;
#endif // uIRQ_FSMC
}

void SDIO_IRQHandler(void) // STM32F10xHD only interrupt
{
#ifdef uIRQ_SDIO
	_interruptDepth++;
	uIRQ_SDIO();
	_interruptDepth--;
#endif // uIRQ_SDIO
}

void TIM5_IRQHandler(void) // STM32F10xHD+ interrupt
{
#ifdef uIRQ_TIM5
	_interruptDepth++;
	uIRQ_TIM5();
	_interruptDepth--;
#endif // uIRQ_TIM5
}

void SPI3_IRQHandler(void) // STM32F10xHD+ interrupt
{
#ifdef uIRQ_SPI3
	_interruptDepth++;
	uIRQ_SPI3();
	_interruptDepth--;
#endif // uIRQ_SPI3
}

void UART4_IRQHandler(void) // STM32F10xHD+ interrupt
{
#ifdef uIRQ_UART4
	_interruptDepth++;
	uIRQ_UART4();
	_interruptDepth--;
#endif // uIRQ_UART4
}

void UART5_IRQHandler(void)	// STM32F10xHD+ interrupt
{
#ifdef uIRQ_UART5
	_interruptDepth++;
	uIRQ_UART5();
	_interruptDepth--;
#endif // uIRQ_UART5
}

void TIM6_IRQHandler(void) // STM32F10xHD+ interrupt
{
#ifdef uIRQ_TIM6
	_interruptDepth++;
	uIRQ_TIM6();
	_interruptDepth--;
#endif // uIRQ_TIM6
}

void TIM7_IRQHandler(void) // STM32F10xHD+ interrupt
{
#ifdef uIRQ_TIM7
	_interruptDepth++;
	uIRQ_TIM7();
	_interruptDepth--;
#endif // uIRQ_TIM7
}

void DMA2_Channel1_IRQHandler(void) // STM32F10xHD+ interrupt
{
#ifdef uIRQ_DMA2_Channel1
	_interruptDepth++;
	uIRQ_DMA2_Channel1();
	_interruptDepth--;
#endif // uIRQ_DMA2_Channel1
}

void DMA2_Channel2_IRQHandler(void)	// STM32F10xHD+ interrupt
{
#ifdef uIRQ_DMA2_Channel2
	_interruptDepth++;
	uIRQ_DMA2_Channel2();
	_interruptDepth--;
#endif // uIRQ_DMA2_Channel2
}

void DMA2_Channel3_IRQHandler(void)	// STM32F10xHD+ interrupt
{
#ifdef uIRQ_DMA2_Channel3
	_interruptDepth++;
	uIRQ_DMA2_Channel3();
	_interruptDepth--;
#endif // uIRQ_DMA2_Channel3
}

void DMA2_Channel4_5_IRQHandler(void) // STM32F10xHD+ interrupt
{
#ifdef uIRQ_DMA2_Channel4_5
	_interruptDepth++;
	uIRQ_DMA2_Channel4_5();
	_interruptDepth--;
#endif // uIRQ_DMA2_Channel4_5
}

#ifdef STM32F10X_CL

void DMA2_Channel5_IRQHandler(void) // STM32F10x CL only interrupt
{
#ifdef uIRQ_DMA2_Channel5
	_interruptDepth++;
	uIRQ_DMA2_Channel5();
	_interruptDepth--;
#endif // uIRQ_DMA2_Channel5
}

void ETH_IRQHandler(void) // STM32F10x CL only interrupt
{
#ifdef uIRQ_ETH
	_interruptDepth++;
	uIRQ_ETH();
	_interruptDepth--;
#endif // uIRQ_ETH
}

void ETH_WKUP_IRQHandler(void) // STM32F10x CL only interrupt
{
#ifdef uIRQ_ETH_WKUP
	_interruptDepth++;
	uIRQ_ETH_WKUP();
	_interruptDepth--;
#endif // uIRQ_ETH_WKUP
}

void CAN2_TX_IRQHandler(void) // STM32F10x CL only interrupt
{
#ifdef uIRQ_CAN2_TX
	_interruptDepth++;
	uIRQ_CAN2_TX();
	_interruptDepth--;
#endif // uIRQ_CAN2_TX
}

void CAN2_RX0_IRQHandler(void) // STM32F10x CL only interrupt
{
#ifdef uIRQ_CAN2_RX0
	_interruptDepth++;
	uIRQ_CAN2_RX0();
	_interruptDepth--;
#endif // uIRQ_CAN2_RX0
}

void CAN2_RX1_IRQHandler(void) // STM32F10x CL only interrupt
{
#ifdef uIRQ_CAN2_RX1
	_interruptDepth++;
	uIRQ_CAN2_RX1();
	_interruptDepth--;
#endif // uIRQ_CAN2_RX1
}

void CAN2_SCE_IRQHandler(void) // STM32F10x CL only interrupt
{
#ifdef uIRQ_CAN2_SCE
	_interruptDepth++;
	uIRQ_CAN2_SCE();
	_interruptDepth--;
#endif // uIRQ_CAN2_SCE
}

void OTG_FS_IRQHandler(void) // STM32F10x CL only interrupt
{
#ifdef uIRQ_OTG_FS
	_interruptDepth++;
	uIRQ_OTG_FS();
	_interruptDepth--;
#endif // uIRQ_OTG_FS
}

#endif // STM32F10X_CL