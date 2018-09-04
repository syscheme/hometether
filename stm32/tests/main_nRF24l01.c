#include "common.h"
#include "ds18b20.h"

typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;		 
ErrorStatus HSEStartUpStatus;
void RCC_Configuration(void);								   //����ʱ�ӳ�ʼ������
void GPIO_Configuration(void);								   //����IO��ʼ������
GPIO_InitTypeDef GPIO_InitStructure;

DS18B20 ds18b20;

// ---------------------------------------------------------------------------
// main()
// ---------------------------------------------------------------------------
int main(void)												  //������
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


	/*����LED��ӦλIO��*/
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_1 | GPIO_Pin_4;		  //IO�˿ڵĵ�1�͵�4λ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;			  //��ת�ٶ�Ϊ50M
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;			  //�˿�ģʽΪ���������ʽ
	GPIO_Init(GPIOA, &GPIO_InitStructure);					  //�����ϼ���������ʼ��PA��


	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_0 | GPIO_Pin_1;		  //ͬ��
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
	RCC_DeInit();										//ʱ�ӿ��ƼĴ���ȫ���ָ�Ĭ��ֵ

	// Enable HSE
	RCC_HSEConfig(RCC_HSE_ON);						//�ⲿ����ʱ��Դ������8M����

	// Wait till HSE is ready
	HSEStartUpStatus = RCC_WaitForHSEStartUp();		//�ȴ��ⲿʱ�Ӿ���

	if (HSEStartUpStatus == SUCCESS)					//���ʱ�������ɹ�
	{
		// HCLK = SYSCLK, ����AHB�豸ʱ��Ϊϵͳʱ��1��Ƶ
		RCC_HCLKConfig(RCC_SYSCLK_Div1);

		// PCLK2 = HCLK, ����AHB2�豸ʱ��ΪHCLKʱ��1��Ƶ
		RCC_PCLK2Config(RCC_HCLK_Div1);

		// PCLK1 = HCLK/2, ����AHB1�豸ʱ��ΪHCLKʱ��2��Ƶ
		RCC_PCLK1Config(RCC_HCLK_Div2);

		// Flash 2 wait state, �趨�ڲ�FLASH�ĵ���ʱ����Ϊ2����
		FLASH_SetLatency(FLASH_Latency_2);
		// Enable Prefetch Buffer, ʹ��FLASHԤ��ȡ������
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

		// PLLCLK = 8MHz * 9 = 72 MHz, ����PLLʱ��Ϊ�ⲿ����ʱ�ӵ�9��Ƶ��8MHz * 9 = 72 MHz
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

		// Enable PLL
		RCC_PLLCmd(ENABLE);

		// Wait till PLL is ready
		while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

		// Select PLL as system clock source, ʹ��PLLʱ����Ϊϵͳʱ��Դ
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

		// Wait till PLL is used as system clock source, �ȴ�����ʱ��Դȷ��Ϊ�ⲿ���پ���8M����
		while(RCC_GetSYSCLKSource() != 0x08);
	}

	// Enble GPIOA��GPIOB��GPIOC, ʹ����APB2ʱ�ӿ��Ƶ������е�PA��PB�˿�
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
