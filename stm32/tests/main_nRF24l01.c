#include "common.h"
#include "ds18b20.h"

typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;		 
ErrorStatus HSEStartUpStatus;
void RCC_Configuration(void);								   //申明时钟初始化函数
void GPIO_Configuration(void);								   //申明IO初始化函数
GPIO_InitTypeDef GPIO_InitStructure;

DS18B20 ds18b20;

// ---------------------------------------------------------------------------
// main()
// ---------------------------------------------------------------------------
int main(void)												  //主函数
{

	int i=0;
	uint16_t temp=0;
#ifdef DEBUG
	debug();
#endif

	// initialize system clock
	RCC_Configuration();

	// configure GPIO
	GPIO_Configuration();

	// Setup SysTick Timer for 1 msec interrupts
	for (i=0; i <100 && SysTick_Config(SystemFrequency / 1000); i++);

	DS18B20_InitConfiguration(&ds18b20, GPIOB, 0);

	// read the temperature from a DS18B20 chip, the temperature value is in 0.1C

	// LED blinks
	while(1)
	{ 
		temp = DS18B20_read(&ds18b20);
/*
		GPIO_WriteBit(GPIOB, GPIO_Pin_0, (BitAction)0x01);		 //enable LED
		for(i=0;i<800000;i++);
		GPIO_WriteBit(GPIOB, GPIO_Pin_0, (BitAction)0x00);		 //reset LED
*/
		//for(i=0;i<500000;i++);
		GPIO_WriteBit(GPIOB, GPIO_Pin_1, (BitAction)0x01);
		for(i=0;i<800000;i++);
		GPIO_WriteBit(GPIOB, GPIO_Pin_1, (BitAction)0x00);
		//for(i=0;i<500000;i++);
		GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)0x01);
		for(i=0;i<600000;i++);
		GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)0x00);
		//for(i=0;i<500000;i++);
		GPIO_WriteBit(GPIOA, GPIO_Pin_1, (BitAction)0x01);
		for(i=0;i<600000;i++);
		GPIO_WriteBit(GPIOA, GPIO_Pin_1, (BitAction)0x00);
		//for(i=0;i<500000;i++);

	}

}

static __IO uint32_t _timeTick;
/**
* @brief  Inserts a delay time.
* @param nTime: specifies the delay time length, in milliseconds.
* @retval : None
*/
void delayMs(__IO uint32_t msec)
{
	uint32_t to = _timeTick - msec;
	while ((to & (1<<31)) ? to <_timeTick : to >_timeTick);
}

void Tick_Decrement(void)
{
	_timeTick --;
}

/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures Leds and Keys.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void GPIO_Configuration(void)
{


	/*配置LED对应位IO口*/
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_1 | GPIO_Pin_4;		  //IO端口的第1和第4位
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;			  //翻转速度为50M
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;			  //端口模式为推拉输出方式
	GPIO_Init(GPIOA, &GPIO_InitStructure);					  //用以上几个参数初始化PA口


	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_0 | GPIO_Pin_1;		  //同上
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}



/*******************************************************************************
* Function Name  : RCC_Configuration
* Description    : Configures the different system clocks.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RCC_Configuration(void)
{
	// RCC system reset(for debug purpose)
	RCC_DeInit();										//时钟控制寄存器全部恢复默认值

	// Enable HSE
	RCC_HSEConfig(RCC_HSE_ON);						//外部高速时钟源开启（8M晶振）

	// Wait till HSE is ready
	HSEStartUpStatus = RCC_WaitForHSEStartUp();		//等待外部时钟就绪

	if (HSEStartUpStatus == SUCCESS)					//如果时钟启动成功
	{
		// HCLK = SYSCLK, 定义AHB设备时钟为系统时钟1分频
		RCC_HCLKConfig(RCC_SYSCLK_Div1);

		// PCLK2 = HCLK, 定义AHB2设备时钟为HCLK时钟1分频
		RCC_PCLK2Config(RCC_HCLK_Div1);

		// PCLK1 = HCLK/2, 定义AHB1设备时钟为HCLK时钟2分频
		RCC_PCLK1Config(RCC_HCLK_Div2);

		// Flash 2 wait state, 设定内部FLASH的的延时周期为2周期
		FLASH_SetLatency(FLASH_Latency_2);
		// Enable Prefetch Buffer, 使能FLASH预存取缓冲区
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

		// PLLCLK = 8MHz * 9 = 72 MHz, 配置PLL时钟为外部高速时钟的9倍频，8MHz * 9 = 72 MHz
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

		// Enable PLL
		RCC_PLLCmd(ENABLE);

		// Wait till PLL is ready
		while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

		// Select PLL as system clock source, 使用PLL时钟作为系统时钟源
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

		// Wait till PLL is used as system clock source, 等待所用时钟源确认为外部高速晶振，8M晶振。
		while(RCC_GetSYSCLKSource() != 0x08);
	}

	// Enble GPIOA、GPIOB、GPIOC, 使能由APB2时钟控制的外设中的PA，PB端口
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB , ENABLE);
}

#if 0
void SysTick_Configuration(void)
{
	// Select AHB clock(HCLK) as SysTick clock source
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

	// Set SysTick Priority to 3
	NVIC_SystemHandlerPriorityConfig(SystemHandler_SysTick, 3, 0);

	// SysTick interrupt each 1ms with HCLK equal to 72MHz
	SysTick_SetReload(72000);

	// Enable the SysTick Interrupt
	SysTick_ITConfig(ENABLE);
}


#ifdef  DEBUG
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(u8* file, u32 line)
{
	/* User can add his own implementation to report the file name and line number,
	ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif
/******************* (C) COPYRIGHT 2007 STMicroelectronics *****END OF FILE****/
*/

#endif //0
