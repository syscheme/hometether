#include "htcomm.h" 
#ifdef FreeRTOS // FreeRTOS style
#  include "FreeRTOSConfig.h"
#endif // FreeRTOS
// -----------------------------
// delay
// -----------------------------
void delayXusec(int16_t usec)
{
	// the following was from http://code.google.com/p/stm32-codes/source/browse/trunk/SDIO_5110_FW3.x/5110/delay.c
	uint8_t n;
	while(usec--)
	{
//#ifdef STM32F103_32MHz
		for (n =5 ; n >0; n--);  // for-loop(n=5) tested via STM32F105VC(RCC_PLLSource_HSI_Div2, RCC_PLLMul_9)
//#else
//		for (n =8 ; n != 0; n--);
//#endif // STM32F103_32MHz
	}
}

/*
void delayXusec(int16_t usec) 
{  
	SysTick->LOAD = usec*9;          //时间加载       
	SysTick->CTRL |= 0x01;             //开始倒数     
	while(!(SysTick->CTRL&(1<<16))); //等待时间到达  

	SysTick->CTRL =0X00000000;        //关闭计数器 
 	SysTick->VAL  =0X00000000;         //清空计数器      
} 
*/

#ifdef uIRQ_TIM2
void delayXmsec(int16_t msec)
{
	uint32_t exp_msec = gClock_calcExp(msec);
	while (gClock_msec != exp_msec)
		delayXusec(50);
}
#else
void delayXmsec(int16_t msec)
{
	while(msec--)
		delayXusec(1000);
}
#endif // uIRQ_TIM2
