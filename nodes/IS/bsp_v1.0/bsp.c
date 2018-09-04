#include "includes.h"

// onboard resources
DS18B20  ds18b20;

// ============================================================================
// Function Name  : RCC_Configuration
// Description    : Configures the different system clocks.
// Input          : None
// Output         : None
// Return         : None
// ============================================================================
void RCC_Configuration(void)
{
	ErrorStatus HSEStartUpStatus;   

	RCC_DeInit(); // RCC system reset(for debug purpose)
	RCC_HSEConfig(RCC_HSE_ON); // Enable HSE
	HSEStartUpStatus = RCC_WaitForHSEStartUp(); // Wait till HSE is ready

	if(HSEStartUpStatus == SUCCESS)
	{
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable); // Enable Prefetch Buffer
		FLASH_SetLatency(FLASH_Latency_2); // Flash 2 wait state
		RCC_HCLKConfig(RCC_SYSCLK_Div1);  // HCLK = SYSCLK
		RCC_PCLK2Config(RCC_HCLK_Div1);  // PCLK2 = HCLK
		RCC_PCLK1Config(RCC_HCLK_Div2); // PCLK1 = HCLK/2
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_16); // PLLCLK = 8MHz * 9 = 72 MHz
		RCC_PLLCmd(ENABLE); // Enable PLL 
		
		while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET); // Wait till PLL is ready
			
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK); // Select PLL as system clock source
		while(RCC_GetSYSCLKSource() != 0x08); // Wait till PLL is used as system clock source
	}

	// Enable GPIO clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);

	// Enable ADC1 and USART1 clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_USART1 | RCC_APB1Periph_SPI2, ENABLE);

	// enable SPI2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
}

// =========================================================================
// USART_open
// =========================================================================
void USART_open(USART_TypeDef* USARTx, uint16_t baudRateX100)
{
	USART_InitTypeDef USART_InitStruct;
	USART_ClockInitTypeDef  USART_ClockInitStruct;

	USART_ClockInitStruct.USART_Clock = USART_Clock_Disable;     //禁止USART时钟
	USART_ClockInitStruct.USART_CPOL = USART_CPOL_Low;           //时钟低时数据有效
	USART_ClockInitStruct.USART_CPHA = USART_CPHA_2Edge;         //数据在第二个时钟边沿捕获
	USART_ClockInitStruct.USART_LastBit = USART_LastBit_Disable; //最后数据位的时钟不输出到SCLK
	USART_ClockInit(USARTx, &USART_ClockInitStruct);	         // initialize usart clock with the above parameters

	USART_InitStruct.USART_BaudRate = baudRateX100 *100;         //设置波特率
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;     //设置数据长度为8位
	USART_InitStruct.USART_StopBits = USART_StopBits_1;			 //设置一个停止位
	USART_InitStruct.USART_Parity = USART_Parity_No ;			 //无校验位
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //禁止硬件流控制模式
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; //使能串口的收发功能
	USART_Init(USARTx, &USART_InitStruct);						 // initialize usart with the above parameters

	USART_Cmd(USARTx, ENABLE);	// open the usart
}

// ============================================================================
// Function Name  : GPIO_Configuration
// Description    : Configures the different GPIO ports.
// ============================================================================
void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
  
	// about the onboard LEDs that is drived by PA1~4
	// --------------------------------------------------
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;       //翻转速度为50M
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;		//端口模式为推拉输出方式
	GPIO_Init(GPIOA, &GPIO_InitStructure);				    // initialize the PA with the above parameters

    // turn off LEDs
	GPIO_WriteBit(GPIOA, GPIO_Pin_1, (BitAction) 0x01);
    GPIO_WriteBit(GPIOA, GPIO_Pin_2, (BitAction) 0x01);
    GPIO_WriteBit(GPIOA, GPIO_Pin_3, (BitAction) 0x01);
    GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction) 0x01);

	// about the DS18B20 that is connected to B5
	// --------------------------------------------------
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
 	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	DS18B20_InitConfiguration(&ds18b20, (POneWireBusCtx) 0, 0x00);
	OWReset(NULL);

	// about the Lumin ADC that is connected to A7
	// --------------------------------------------------
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;	  //端口模式为模拟输入方式
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// about the IR LEDs driven by S8550 that are connected to B11, B12
	// --------------------------------------------------
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;	//开漏输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOB, GPIO_Pin_11, (BitAction) 1); // turn off LED
	GPIO_WriteBit(GPIOB, GPIO_Pin_12, (BitAction) 1); // turn off LED

	// about the USART1: RX-PA10, TX-PA9
	// --------------------------------------------------
	// configure USART1 Rx (PA10) as input floating
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
  
 	// Configure USART1 Tx (PA09) as alternate function push-pull
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// about the nRF24L01, takes SPI2(B13-15), B0-CSN B1-CE A8-IRQ
	// --------------------------------------------------
	// step 1. about B0-CSB B1-CE
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// step 2. about A8-IRQ
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// step 3. the SPI2
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// SPI2 configuration
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode      = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize  = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL      = SPI_CPOL_Low; //?? SPI_CPOL_High=模式3，时钟空闲为高 //SPI_CPOL_Low=模式0，时钟空闲为低
	SPI_InitStructure.SPI_CPHA      = SPI_CPHA_1Edge; //SPI_CPHA_2Edge;//SPI_CPHA_1Edge, SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS       = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;//SPI_BaudRatePrescaler_2=18M;//SPI_BaudRatePrescaler_4=9MHz
	SPI_InitStructure.SPI_FirstBit  = SPI_FirstBit_MSB;//数据从高位开始发送
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStructure);
	SPI_Cmd(SPI2, ENABLE);   // enable SPI2
}


// ============================================================================
// Function Name  : NVIC_Configuration
// Description    : Configures Vector Table base location.
// ============================================================================
void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStruct;

	//#ifdef  VECT_TAB_RAM
#if defined (VECT_TAB_RAM)
	// Set the Vector Table base location at 0x20000000
	NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0); 
#elif defined(VECT_TAB_FLASH_IAP)
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x2000);
#else  // VECT_TAB_FLASH
	// Set the Vector Table base location at 0x08000000
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);   
#endif 

	// Configure the NVIC Preemption Priority Bits
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

#ifndef NRF2401_POLL
 	// enable the EXTI9_5 for A8-IRQ of nRF2401
	NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
#endif // NRF2401_POLL
}

void BSP_Init(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	// System Clocks Configuration
	RCC_Configuration();   

	GPIO_Configuration();

	// NVIC configuration
	NVIC_Configuration();

	USART_open(USART1, 1152);

	// ADC1: configure channel 1 over PA7
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	 //ADC1和ADC2工作的独立模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;		 //ADC设置为多通道扫描模式
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;	 //设置为连续转换模式
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	   //由软件控制开始转换（还有触方式等）
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	   //AD输出数值为右端对齐方式
	ADC_InitStructure.ADC_NbrOfChannel = 1;					   //指定要进行AD转换的信道1
	ADC_Init(ADC1, &ADC_InitStructure);						   //用上面的参数初始化ADC1

	//将ADC1信道1的转换通道7（PA7）的采样时间设置为55.5个周期
	ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 1, ADC_SampleTime_55Cycles5);
	ADC_Cmd(ADC1, ENABLE);                               //使能ADC1
	ADC_ResetCalibration(ADC1);							 //重置ADC1校准寄存器
	while(ADC_GetResetCalibrationStatus(ADC1));			 //得到重置校准寄存器状态
	ADC_StartCalibration(ADC1);							 //开始校准ADC1
	while(ADC_GetCalibrationStatus(ADC1));				 //得到校准寄存器状态
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);				 //使能ADC1由软件控制开始转换

	// about the exti8 for A8 that connects to IRQ of nRF24l01
	EXTI_DeInit();
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource8);
    EXTI_InitStructure.EXTI_Line = EXTI_Line8;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

	EXTI_ClearITPendingBit(EXTI_Line8);

	EXTI_GenerateSWInterrupt(EXTI_Line8); //中断允许
}

void OWPortal_dqH(POneWireBusCtx pBusCtx)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_5);
}

void OWPortal_dqL(POneWireBusCtx pBusCtx)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_5);
}

uint8_t OWPortal_dqV(POneWireBusCtx pBusCtx)
{
	return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5);
}


CPU_INT32U  BSP_CPU_ClkFreq (void)
{
	RCC_ClocksTypeDef  rcc_clocks;
	RCC_GetClocksFreq(&rcc_clocks);

	return ((CPU_INT32U)rcc_clocks.HCLK_Frequency);
}

INT32U  OS_CPU_SysTickClkFreq (void)
{
	INT32U  freq = BSP_CPU_ClkFreq();
	return (freq);
}

#ifdef  DEBUG
// ============================================================================
// Function Name  : assert_failed
// Description    : Reports the name of the source file and the source line number
//                  where the assert_param error has occurred.
// Input          : - file: pointer to the source file name
//                  - line: assert_param error line source number
// ============================================================================
void assert_failed(u8* file, u32 line)
{ 
	// User can add his own implementation to report the file name and line number,
	// ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line)

	// Infinite loop
	while (1);
}
#endif

