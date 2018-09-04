#include "bsp.h"
#include "../htod.h"

extern void DQBoard_Config(void);
extern void DQBoard_NVIC_RegisterISRs(void);

#define ADC_SAMPLE_TIME      ADC_SampleTime_239Cycles5

void enablePWM(TIM_TypeDef* TIMx, uint16_t reload);

// =========================================================================
// RCC_Configuration
// =========================================================================
static void RCC_Configuration(void)
{
	ErrorStatus HSEStartUpStatus;   

	// RCC system reset(for debug purpose)
	RCC_DeInit();

	RCC_HSEConfig(RCC_HSE_ON); // Enable external HSE

	HSEStartUpStatus = RCC_WaitForHSEStartUp(); // Wait till HSE is ready
	if (SUCCESS == HSEStartUpStatus)
	{
		RCC_HCLKConfig(RCC_SYSCLK_Div1);  // HCLK  = SYSCLK
		RCC_PCLK2Config(RCC_HCLK_Div1);   // PCLK2 = HCLK
		RCC_PCLK1Config(RCC_HCLK_Div2);   // PCLK1 = HCLK/2

		FLASH_SetLatency(FLASH_Latency_2); // Flash Latency = 2 cycles
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable); // Enable Prefetch Buffer

		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9); // PLLCLK = 8MHz *9 = 72 MHz
		RCC_PLLCmd(ENABLE); // Enable PLL 

		while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET); // Wait till PLL is ready
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK); // Select PLL as system clock source
		while(RCC_GetSYSCLKSource() != 0x08); // Wait till PLL is used as system clock source
	}

	// Enable APBxPeriph clocks
#ifdef RCC_AHBPeriph_Devices
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_Devices, ENABLE);
#endif // RCC_APBPeriph_Devices

#ifdef RCC_APB1Periph_Devices
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_Devices, ENABLE);
#endif // RCC_APB1Periph_Devices

#ifdef RCC_APB2Periph_Devices
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_Devices, ENABLE);
#endif // RCC_APB2Periph_Devices

	RCC_ADCCLKConfig(RCC_PCLK2_Div8);  //ADC clock	RCC_PCLK2_Div6
}

// =========================================================================
// NVIC_Configuration
// =========================================================================
#if defined(ECU_WITH_ENC28J60) && !defined(EXTI_TEST)
#  define EXTI_TEST
#endif
// #define EXTI_TEST

static void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStruct;
#ifdef EXTI_TEST
	EXTI_InitTypeDef EXTI_InitStruct;
#endif // EXTI_TEST

#if defined (VECT_TAB_RAM)
#warning NVIC in RAM
	// Set the Vector Table base location at 0x20000000
	NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0); 
#elif defined(VECT_TAB_FLASH_IAP)
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x2000);
#else  // VECT_TAB_FLASH
	// Set the Vector Table base location at 0x08000000
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);   
#endif 

	// Configure the NVIC Preemption Priority Bits
	// FreeRTOS must take NVIC_PriorityGroup_4
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); //(NVIC_PriorityGroup_0);

	// TIM2 for every 1msec
	NVIC_InitStruct.NVIC_IRQChannel                   = TIM2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 1;
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	// DMA1 interrupts for ADC converstions
	NVIC_InitStruct.NVIC_IRQChannel                   = DMA1_Channel1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	// Enable the USART1 Interrupt
	// enable USART1 int
	NVIC_InitStruct.NVIC_IRQChannel                   = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	// Enable USART1 Receive interrupts
	// USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

#ifdef ECU_WITH_ENC28J60
	// about the exti7 for A7 that connects to IRQ of EN28J60
	EXTI_StructInit(&EXTI_InitStruct);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource7);
	EXTI_InitStruct.EXTI_Line    = EXTI_Line7;
	EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	EXTI_ClearITPendingBit(EXTI_Line7);
	EXTI_GenerateSWInterrupt(EXTI_Line7); //中断允许

	NVIC_InitStruct.NVIC_IRQChannel                   = EXTI9_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2; //强占优先级
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 0; //次优先级
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE; //通道中断使能
	NVIC_Init(&NVIC_InitStruct); //初始化中断

#endif // ECU_WITH_ENC28J60

//	// E0 takes interrupt 0
//	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0; //强占优先级
//	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0; //次优先级
//	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE; //通道中断使能
//
//	//  E0=>EXTI0
//	NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
//	NVIC_Init(&NVIC_InitStruct); //初始化中断
//
//	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource0);
// 	// Trigger type
//	EXTI_InitStruct.EXTI_Line = EXTI_Line0;
//	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
//	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling; //下降沿触发
//	EXTI_InitStruct.EXTI_LineCmd = ENABLE; //中断线使能
//	EXTI_Init(&EXTI_InitStruct); //初始化中断
//
//	EXTI_GenerateSWInterrupt(EXTI_Line0); //中断允许

/*
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0; //强占优先级
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0; //次优先级
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE; //通道中断使能

	//  E0=>EXTI0
	NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_Init(&NVIC_InitStruct); //初始化中断
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource0);

	//  E1=>EXTI1
	NVIC_InitStruct.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_Init(&NVIC_InitStruct); //初始化中断
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource1);

	//  E2=>EXTI2
	NVIC_InitStruct.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_Init(&NVIC_InitStruct); //初始化中断
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource2);

	//  E3=>EXTI3
	NVIC_InitStruct.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_Init(&NVIC_InitStruct); //初始化中断
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource3);

	// Trigger type
	EXTI_InitStruct.EXTI_Line = EXTI_Line0 | EXTI_Line1 | EXTI_Line2 | EXTI_Line3;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling; //下降沿触发
	EXTI_InitStruct.EXTI_LineCmd = ENABLE; //中断线使能
	EXTI_Init(&EXTI_InitStruct); //初始化中断

	EXTI_GenerateSWInterrupt(EXTI_Line0 | EXTI_Line1 | EXTI_Line2 | EXTI_Line3); //中断允许
// */

	// register onboard ISRs
	DQBoard_NVIC_RegisterISRs();
}

// =========================================================================
// External IO Resources
// =========================================================================
DS18B20 ds18b20s[CHANNEL_SZ_DS18B20];

// On SECU, the DS18B20 chips connect to PD0~7
const static OneWire OneWireBus[CHANNEL_SZ_DS18B20] = 
{
	{ PIN_DECL(GPIOD, GPIO_Pin_0) },
	{ PIN_DECL(GPIOD, GPIO_Pin_1) },
	{ PIN_DECL(GPIOD, GPIO_Pin_2) },
	{ PIN_DECL(GPIOD, GPIO_Pin_3) },
	{ PIN_DECL(GPIOD, GPIO_Pin_4) },
	{ PIN_DECL(GPIOD, GPIO_Pin_5) },
	{ PIN_DECL(GPIOD, GPIO_Pin_6) },
	{ PIN_DECL(GPIOD, GPIO_Pin_7) }
};

// the DMA buf to receive ADC result, should maps to the Lumin_ch[0]~ch[CHANNEL_SZ_LUMUN] of CanOpen OD
extern uint16_t* ADC_DMAbuf;

// On SECU, motion sensors connect to D8, E7~15
const static IO_PIN MotionChs[CHANNEL_SZ_MOTION] = 
{
	{ GPIOD, GPIO_Pin_8},
	{ GPIOE, GPIO_Pin_7},
	{ GPIOE, GPIO_Pin_8},
	{ GPIOE, GPIO_Pin_9},
	{ GPIOE, GPIO_Pin_10},
	{ GPIOE, GPIO_Pin_11},
	{ GPIOE, GPIO_Pin_12},
	{ GPIOE, GPIO_Pin_13},
	{ GPIOE, GPIO_Pin_14},
	{ GPIOE, GPIO_Pin_15}
};

/*
const IO_PIN IrLEDs[CHANNEL_SZ_IrLED] = {
	{GPIOE, GPIO_Pin_4},
	{GPIOE, GPIO_Pin_5},
	{GPIOE, GPIO_Pin_6},
	{GPIOE, GPIO_Pin_7},
};

const IO_PIN IrRecvs[CHANNEL_SZ_IrRecv] = {
	{GPIOE, GPIO_Pin_0},
	{GPIOE, GPIO_Pin_1},
	{GPIOE, GPIO_Pin_2},
	{GPIOE, GPIO_Pin_3},
};
*/

// blinks extends DQ leds, see the mapping of OnBoardLEDs in bsp_dq.c
BlinkPair EcuCtrl_dqLEDs[4] = {
	{ PIN_DECL(GPIOA, GPIO_Pin_1), PIN_DECL(NULL, 0), 0,0,0,0,0},
	{ PIN_DECL(GPIOA, GPIO_Pin_4), PIN_DECL(NULL, 0), 0,0,0,0,0},
	{ PIN_DECL(GPIOB, GPIO_Pin_0), PIN_DECL(NULL, 0), 0,0,0,0,0},
	{ PIN_DECL(GPIOB, GPIO_Pin_1), PIN_DECL(NULL, 0), 0,0,0,0,0}
};

// Control of relay
BlinkPair EcuCtrl_relays[4] = {
	{ PIN_DECL(GPIOD, GPIO_Pin_11), PIN_DECL(NULL, 0), 0,0,0,0,0},
	{ PIN_DECL(GPIOD, GPIO_Pin_12), PIN_DECL(NULL, 0), 0,0,0,0,0},
	{ PIN_DECL(GPIOD, GPIO_Pin_13), PIN_DECL(NULL, 0), 0,0,0,0,0},
	{ PIN_DECL(GPIOD, GPIO_Pin_14), PIN_DECL(NULL, 0), 0,0,0,0,0}
};

// about the ENC18J60 if adapted
#ifdef ECU_WITH_ENC28J60 
const uint8_t  MyIP[4]       = THIS_IP;
const uint16_t MyServerPort  = PORT_WWW;
const uint8_t  GroupIP[4]    = GROUP_IP;

ENC28J60 nic= {
			{0x00,0x1A,0x6B,0xCE,0x92,0x66},
			SPI2,
			PIN_DECL(GPIOD, GPIO_Pin_8),
			PIN_DECL(GPIOD, GPIO_Pin_9),
			};
#endif // ECU_WITH_ENC28J60

// =========================================================================
// External IO Initial Configuration
// =========================================================================
static void External_Config(void)
{
	int i;
	GPIO_InitTypeDef GPIO_InitStruct;
	DMA_InitTypeDef  DMA_InitStruct;
	ADC_InitTypeDef  ADC_InitStruct;

#ifdef ECU_WITH_ENC28J60
	SPI_InitTypeDef  SPI_InitStruct;
#endif

	// section 1. about DS18B20s that is connected to PD0~7 on SECU
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3
		| GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_OD; // open-drain in/output mode because have pull-up resister on DS16B20
	GPIO_Init(GPIOD, &GPIO_InitStruct);

	for (i =0; i < CHANNEL_SZ_DS18B20; i++)
	{
		DS18B20_init(&ds18b20s[i], (OneWire*)&OneWireBus[i], 0x00);
		OneWire_resetBus((OneWire*)&OneWireBus[i]);
	}

	// section 2. the Lumin ADC connects to C0,C1,C2,C3,A2,A3,C4,C5 thru DMA1
	// C0-CH10, C1-CH11, C2-CH12, C3-CH13, A2-CH2, A3-CH3, C4-CH14, C5-CH15
	// ------------------------------------------------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	// PA2/3
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	//PC0/1/2/3/4/5
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStruct);

	// http://bbs.ednchina.com/BLOG_ARTICLE_146193.HTM
	// http://www.mculover.com/post/140.html
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t) &(ADC1->DR);
	DMA_InitStruct.DMA_MemoryBaseAddr     = (uint32_t) ADC_DMAbuf;
	DMA_InitStruct.DMA_DIR                = DMA_DIR_PeripheralSRC;
	DMA_InitStruct.DMA_BufferSize         = sizeof(uint16_t) * CHANNEL_SZ_ADC;
	DMA_InitStruct.DMA_PeripheralInc      = DMA_PeripheralInc_Disable; // single ADC1->DR, step=0
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 16bit ADC value
	DMA_InitStruct.DMA_MemoryInc          = DMA_MemoryInc_Enable; // receive buffer take increasing step
	DMA_InitStruct.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord; // 16bit ADC value
	DMA_InitStruct.DMA_Mode               = DMA_Mode_Circular;  // continuous loop
	DMA_InitStruct.DMA_Priority           = DMA_Priority_Medium; 
	DMA_InitStruct.DMA_M2M                = DMA_M2M_Disable; // non-m2m copy

	DMA_Init(DMA1_Channel1, &DMA_InitStruct);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA1_Channel1, ENABLE); // Enable DMA1 channel1

	// ADC1 configuration
	ADC_InitStruct.ADC_Mode               = ADC_Mode_Independent;
	ADC_InitStruct.ADC_ScanConvMode       = ENABLE;	  // !!!!
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;	  // !!!!
	ADC_InitStruct.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
	ADC_InitStruct.ADC_DataAlign          = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_NbrOfChannel       = CHANNEL_SZ_ADC;
	ADC_Init(ADC1, &ADC_InitStruct);

	// ADC1 regular channels configuration
	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SAMPLE_TIME);  // C0
	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 2, ADC_SAMPLE_TIME);  // C1
	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 3, ADC_SAMPLE_TIME);  // C2
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 4, ADC_SAMPLE_TIME);  // C3
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2,  5, ADC_SAMPLE_TIME);  // A2
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3,  6, ADC_SAMPLE_TIME);  // A3
	ADC_RegularChannelConfig(ADC1, ADC_Channel_14, 7, ADC_SAMPLE_TIME);  // C4
	ADC_RegularChannelConfig(ADC1, ADC_Channel_15, 8, ADC_SAMPLE_TIME);  // C5

	// the built-in temperature sensor
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_55Cycles5);
	ADC_TempSensorVrefintCmd(ENABLE);

	// enable ADC1 DMA and ADC1
	ADC_DMACmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE); 

	// enable ADC1 reset calibaration register, and wait till it gets completed
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));

	// start ADC1 calibaration, and wait till it finishes
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));

	// start ADC1 Software Conversion
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	// section 3. about the motion sensor that connect to D8, E7~15
	// ------------------------------------------------------
	//PE
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11
		| GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStruct);

	//PD8
	GPIO_InitStruct.GPIO_Pin =GPIO_Pin_8;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOD, &GPIO_InitStruct);

///*
//	// section 4. about the IR sender that connect to E4/5/6, B7
//	// ------------------------------------------------------
//	//PE
//	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;  //TODO should take GPIO_Mode_Out_OD(open-drain) because have pull-up resister already
//	GPIO_Init(GPIOE, &GPIO_InitStruct);
//
//	//PB
//	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_7;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP; //TODO should take GPIO_Mode_Out_OD(open-drain)
//	GPIO_Init(GPIOB, &GPIO_InitStruct);
//
//	// TODO: section 5. about the IR recv that connect to E0~E3
//	// ------------------------------------------------------
//	// ref: http://www.zdh1909.com/html/stm32/10912.html
//
//	//  GPIO of the IR recvrs
//	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//	GPIO_Init(GPIOE, &GPIO_InitStruct);
//
//	// see also NVIC_Configuration() for interrupt registrations
//*/
	// section 6. about the relays that connect to D11~14
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; // GPIO_Speed_2MHz
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP; // TODO should take GPIO_Mode_Out_OD(open-drain) because have pull-up resister already
	GPIO_Init(GPIOD, &GPIO_InitStruct);
	for (i =0; i < CHANNEL_SZ_Relay; i++)
	{
		SECU_setRelay(i, 0);
		BlinkList_register(&EcuCtrl_relays[i]);
	}

	// DQ onboard LEDs
	for (i =sizeof(EcuCtrl_dqLEDs) / sizeof(EcuCtrl_dqLEDs[0])-1; i >=0; i--)
		BlinkList_register(&EcuCtrl_dqLEDs[i]);

#ifdef ECU_WITH_ENC28J60
	// section 7. we take SPI2 to connect to external ENC28J60 NIC
	// ------------------------------------------------------
	// step 7.1. about D8-CS D9-RESET
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD, &GPIO_InitStruct);
	GPIO_SetBits(GPIOD, GPIO_Pin_8 | GPIO_Pin_9);

	// step 7.2. the SPI2 (B13-15)
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	// SPI2 configuration
	SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStruct.SPI_Mode      = SPI_Mode_Master;
	SPI_InitStruct.SPI_DataSize  = SPI_DataSize_8b;
	SPI_InitStruct.SPI_CPOL      = SPI_CPOL_Low; //?? SPI_CPOL_High=模式3，时钟空闲为高 //SPI_CPOL_Low=模式0，时钟空闲为低
	SPI_InitStruct.SPI_CPHA      = SPI_CPHA_1Edge; //SPI_CPHA_2Edge;//SPI_CPHA_1Edge, SPI_CPHA_2Edge;
	SPI_InitStruct.SPI_NSS       = SPI_NSS_Soft;
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // SPI_BaudRatePrescaler_8=9MHz <=24L01的最大SPI时钟为10Mhz
	SPI_InitStruct.SPI_FirstBit  = SPI_FirstBit_MSB;//数据从高位开始发送
	SPI_InitStruct.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStruct);
	SPI_Cmd(SPI2, ENABLE);   // enable SPI2

	// step 7.3. about A7-IRQ
	GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif // ECU_WITH_ENC28J60

}

void DQBoard_Timer_config() // takes TIM2 as the source timer of 5usec or 200KHz
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_DeInit(TIM2);

	// TIM_InternalClockConfig(TIM2);// takes internal clock, which is APB1/PCLK1 = HCLK/2 = SYSCLK/2 = 36MHZ, but the system will double it by default, so 72MHZ

	TIM_TimeBaseStruct.TIM_Prescaler = 72 -1; // the counter resolution 72MHz/72 = 1MHz
	TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStruct.TIM_Period = 1000 -1; // interrupt when counts 1000 => interrupt every 1msec
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct);
	TIM_PrescalerConfig(TIM2, 72-1, TIM_PSCReloadMode_Immediate);

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_ARRPreloadConfig(TIM2, DISABLE);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM2, ENABLE);  // enable TIM2
}

void RTC_Init(void);

// =========================================================================
// BSP_Init
// =========================================================================
void BSP_Init(void)
{
	RCC_Configuration(); // system Clocks Configuration
	DQBoard_Config();
	DQBoard_Timer_config();
	External_Config();
	
	NVIC_Configuration(); // NVIC configuration
	RTC_Init(); // initialize the battery supplied clock
	// Timer_init(); // enable OS-independent SysTick

#ifdef ECU_WITH_ENC28J60 
	// initialize the NIC
	ENC28J60_init(&nic);

	//init the ethernet/ip layer:
	init_ip_arp_udp_tcp(nic.macaddr, MyIP, MyServerPort);
#endif // ECU_WITH_ENC28J60 
}


// =========================================================================
// BSP_CPU_ClkFreq
// =========================================================================
uint32_t BSP_CPU_ClkFreq (void)
{
	RCC_ClocksTypeDef  rcc_clocks;
	RCC_GetClocksFreq(&rcc_clocks);

	return ((uint32_t)rcc_clocks.HCLK_Frequency);
}

uint32_t OS_CPU_SysTickClkFreq (void)
{
	uint32_t freq = BSP_CPU_ClkFreq();
	return (freq);
}


#ifdef  DEBUG
// =========================================================================
// assert_failed()
// Reports the name of the source file and the source line number where the
// assert_param error has occurred.
// param file  pointer to the source file name
// param line  assert_param error line source number
// =========================================================================
void assert_failed(u8* file, u32 line)
{ 
	// User can add his own implementation to report the file name and line number,
	// ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line)

	// Infinite loop
	while (1);
}
#endif

// ======================================================================
// portal of 1-wire
// On SEICB, the DS18B20 chips connects to PD0~7
// refer to secu.sch
// ======================================================================
void BSP_initDS18B20()
{
	uint8_t i;
	for (i=0; i < CHANNEL_SZ_DS18B20; i++)
	{
		DS18B20_init(&ds18b20s[i], (OneWire*) &OneWireBus[i], 0x00);
	}
}

// ======================================================================
// the motion state
// ======================================================================
#define ReadMotionBit(IOPin) (GPIO_ReadInputDataBit(IOPin.port, IOPin.pin) ? 1:0)
uint16_t MotionState(void)
{
	uint16_t state =0;
	int i =0;
	for (i=0; i <CHANNEL_SZ_MOTION; i++)
	{
		state <<=1;
		state |= ReadMotionBit(MotionChs[i]);
	}

	return state;
}

// ======================================================================
// beep
// ======================================================================
const Syllable startSong[] = {
	 {low_5, 4},
	 {mid_1, 6}, {low_7, 2}, {low_6, 4}, {low_5, 4},
	 {low_5, 8}, {low_3, 4}, {low_5, 4},
	 {low_4, 6}, {low_3, 2}, {low_4, 4}, {low_2, 4}, 
	 {low_1, 12}, {0,0}
};

// ======================================================================
// relay
// ======================================================================
void SECU_setRelay(uint8_t id, uint8_t on)
{
	id %= CHANNEL_SZ_Relay;
	if (on)
		BlinkPair_set(&EcuCtrl_relays[id], BlinkPair_FLAG_A_ENABLE, 100, 0, 0);
	else
		BlinkPair_set(&EcuCtrl_relays[id], BlinkPair_FLAG_A_ENABLE, 0, 100, 0);
}

// ======================================================================
// DQ Onboard LEDs as blinkers
// ======================================================================
void ECU_blinkLED(uint8_t id, uint8_t dsecEbOn, uint8_t dsecEbOff)
{
	id %= sizeof(EcuCtrl_dqLEDs) / sizeof(EcuCtrl_dqLEDs[0]);
	BlinkPair_set(&EcuCtrl_dqLEDs[id], BlinkPair_FLAG_A_ENABLE, dsecEbOn, dsecEbOff, 2);
}

#ifdef ECU_WITH_ENC28J60
// ======================================================================
// NIC control
// ======================================================================
void NIC_sendPacket(void* nic, uint8_t* packet, uint16_t packetLen)
{
	static uint8_t cFail = MAX_NIC_SEND_FAIL;
	if (NULL == nic || NULL ==packet || packetLen<=0)
		return;

	if (ERR_SUCCESS != ENC28J60_sendPacket((ENC28J60*) nic, packet, packetLen))
	{
		cFail = MAX_NIC_SEND_FAIL;
		return;
	}

	if (--cFail)
		return;

	// continuous send fail, reset the NIC
	ENC28J60_init(nic);
}
#endif // ECU_WITH_ENC28J60

