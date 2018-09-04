#include "bsp.h"
#include "../htod.h"

#define ADC_SAMPLE_TIME      ADC_SampleTime_239Cycles5

// =========================================================================
// RCC_Configuration
// =========================================================================
static void RCC_Config(void)
{
	// This unit takes HSI at 36MHz, because HSE is dead   

	// RCC system reset(for debug purpose)
	RCC_DeInit();

	RCC_HSEConfig(RCC_HSE_OFF); // disable HSE because it was dead
	RCC_HSICmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);
	     
	RCC_HCLKConfig(RCC_SYSCLK_Div1);   // HCLK = SYSCLK
	RCC_PCLK2Config(RCC_HCLK_Div1);    // AHB2 PCLK2 = HCLK
	RCC_PCLK1Config(RCC_HCLK_Div2);    // AHB1 PCLK1 = HCLK/2
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // ADC clock PCLK2/6 = 12MHz
	
	// Configure and start PLL
	RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_9); // PLLCLK = 4MHz * 9 = 36 MHz
	RCC_PLLCmd(ENABLE); // enable PLL
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET); // wait till PLL becomes ready

	// set the SYSCLK to PLL
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK); 
	// Wait till PLL is confirmed as the SYSCLK:
	//   0x0-HSI as the source, 0x04-HSE, 0x08-PLL as the source
	while(RCC_GetSYSCLKSource() != 0x08); 

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

static void NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStruct;

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
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

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
}

// =========================================================================
// External IO Resources
// =========================================================================

// Control of relay
BlinkPair EcuCtrl_relays[4] = {
	{ PIN_DECL(GPIOD, GPIO_Pin_11), PIN_DECL(NULL, 0), 0,0,0,0,0},
	{ PIN_DECL(GPIOD, GPIO_Pin_12), PIN_DECL(NULL, 0), 0,0,0,0,0},
	{ PIN_DECL(GPIOD, GPIO_Pin_13), PIN_DECL(NULL, 0), 0,0,0,0,0},
	{ PIN_DECL(GPIOD, GPIO_Pin_14), PIN_DECL(NULL, 0), 0,0,0,0,0}
};

// -----------------------------------
// DS18B20
// -----------------------------------
DS18B20 ds18b20s[CHANNEL_SZ_DS18B20];
const OneWire dht11s[CHANNEL_SZ_DHT11] =
{
	{ PIN_DECL(GPIOB, GPIO_Pin_3) },
	{ PIN_DECL(GPIOB, GPIO_Pin_4) },
	{ PIN_DECL(GPIOB, GPIO_Pin_5) },
};

// On PECU, the DS18B20 chips connect to PD0~7
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

int i;
GPIO_InitTypeDef GPIO_InitStruct;
static void OneWire_Config(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOD, ENABLE);

	// section 1. about DS18B20s that are connected to PD0~7 on ECU
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin =0;
	for (i=0; i < CHANNEL_SZ_DS18B20; i++)
		GPIO_InitStruct.GPIO_Pin |= OneWireBus[i].pin.pin;

	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_OD; // open-drain in/output mode because have pull-up resister on DS18B20
	GPIO_Init(GPIOD, &GPIO_InitStruct);

	for (i =0; i < CHANNEL_SZ_DS18B20; i++)
	{
		DS18B20_init(&ds18b20s[i], (OneWire*)&OneWireBus[i], 0x00);
		OneWire_resetBus((OneWire*)&OneWireBus[i]);
	}

	// section 2. about DHT11s that are connected to PB3~5
	// ------------------------------------------------------
	// only enable SWD, so reuse the other JTAG pins
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE); //B4
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); //B3,B4

	GPIO_InitStruct.GPIO_Pin =0;
	for (i=0; i < CHANNEL_SZ_DHT11; i++)
		GPIO_InitStruct.GPIO_Pin |= dht11s[i].pin.pin;

	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_OD; // open-drain in/output mode because have pull-up resister on DS18B20
	GPIO_Init(dht11s[0].pin.port, &GPIO_InitStruct);
}

// -----------------------------------
// Lumine ADC<-> DMA
// -----------------------------------
// the DMA buf to receive ADC result, should maps to the Lumin_ch[0]~ch[CHANNEL_SZ_LUMUN] of CanOpen OD
extern uint16_t ADC_DMAbuf[];
static void ADC_Config(void)
{
	DMA_InitTypeDef  DMA_InitStruct;
	ADC_InitTypeDef  ADC_InitStruct;

	// section 2. the Lumin ADC connects to A1,A2,A3,A4,A5, C0,C1,C3 thru DMA1
	// A1-CH1, A2-CH2, A3-CH3, A4-CH4, A5-CH5, C0-CH10, C1-CH11, C3-CH13
	// ------------------------------------------------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_ADC1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	// PA1-5
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// PC0,1,3
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_3;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStruct);

	// http://bbs.ednchina.com/BLOG_ARTICLE_146193.HTM
	// http://www.mculover.com/post/140.html
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t) &(ADC1->DR);
	DMA_InitStruct.DMA_MemoryBaseAddr     = (uint32_t) ADC_DMAbuf;
	DMA_InitStruct.DMA_DIR                = DMA_DIR_PeripheralSRC;
	DMA_InitStruct.DMA_BufferSize         = CHANNEL_SZ_ADC*2;
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
	ADC_DeInit(ADC1);
	ADC_InitStruct.ADC_Mode               = ADC_Mode_Independent;
	ADC_InitStruct.ADC_ScanConvMode       = ENABLE;	  // !!!!
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;	  // !!!!
	ADC_InitStruct.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
	ADC_InitStruct.ADC_DataAlign          = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_NbrOfChannel       = CHANNEL_SZ_ADC;
	ADC_Init(ADC1, &ADC_InitStruct);

	// ADC1 regular channels configuration
	// the built-in temperature sensor
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_55Cycles5);

	// the lumin ADCs
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1,  2, ADC_SAMPLE_TIME);  // A1
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2,  3, ADC_SAMPLE_TIME);  // A2
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3,  4, ADC_SAMPLE_TIME);  // A3
	ADC_RegularChannelConfig(ADC1, ADC_Channel_4,  5, ADC_SAMPLE_TIME);  // A4
	ADC_RegularChannelConfig(ADC1, ADC_Channel_5,  6, ADC_SAMPLE_TIME);  // A5
	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 7, ADC_SAMPLE_TIME);  // C0
	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 8, ADC_SAMPLE_TIME);  // C1
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 9, ADC_SAMPLE_TIME);  // C3

	// enable the built-in temperature sensor
	ADC_TempSensorVrefintCmd(ENABLE);

	// enable ADC1 DMA and ADC1
	ADC_DMACmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE); 

	// enable ADC1 reset calibaration register, and wait till it gets completed
	for (ADC_ResetCalibration(ADC1); ADC_GetResetCalibrationStatus(ADC1); );

	// start ADC1 calibaration, and wait till it finishes
	for (ADC_StartCalibration(ADC1); ADC_GetCalibrationStatus(ADC1); );

	// start ADC1 Software Conversion
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	//ADC_ExternalTrigConvCmd(ADC1, ENABLE);
	//ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
}

// -----------------------------------
// Motion
// -----------------------------------
// On PECU, motion sensors connect to D8, E7~15
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

static void Motion_Config(void)
{
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
}

static void Relay_Config(void)
{
	// section 6. about the relays that connect to D11~14
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; // GPIO_Speed_2MHz
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP; // TODO should take GPIO_Mode_Out_OD(open-drain) because have pull-up resister already
	GPIO_Init(GPIOD, &GPIO_InitStruct);
	for (i =0; i < CHANNEL_SZ_Relay; i++)
	{
		PECU_setRelay(i, 0);
		BlinkList_register(&EcuCtrl_relays[i]);
	}
}

// -----------------------------------
// USART1: A9,A10
// -----------------------------------
static void USART_Config(void)
{
	// section 7. about the onboard TTY1<->USART1(A9/10)
	// ------------------------------------------------------
	// Configure USART1 Tx(A9) as alternate function push-pull
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Configure USART3 Rx(A10) as input floating				       
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	USART_open(&TTY1, 1152);
}

// -----------------------------------
// TIM2
// -----------------------------------
void Timer_Config()  // tested via via STM32F105VC(RCC_PLLSource_HSI_Div2, RCC_PLLMul_9)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_DeInit(TIM2);

	// TIM_InternalClockConfig(TIM2);// takes internal clock, which is APB1/PCLK1 = HCLK/2 = SYSCLK/2 = 36MHZ, but the system will double it by default, so 72MHZ

	TIM_TimeBaseStruct.TIM_Prescaler = 36 -1; // the counter resolution 36MHz/36 = 1MHz
	TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStruct.TIM_Period = 1000 -1; // interrupt when counts 1000 => interrupt every 1msec
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct);
	TIM_PrescalerConfig(TIM2, 36 -1, TIM_PSCReloadMode_Immediate);

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_ARRPreloadConfig(TIM2, DISABLE);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM2, ENABLE);  // enable TIM2
}

// =========================================================================
// BSP_Init
// =========================================================================
uint32_t BSP_typeId, BSP_nodeId = 0;

void BSP_Init(void)
{
	BSP_typeId = CPU_typeId(); // CPU type ID , such as STM32F103XXX=0x411FC231
	BSP_nodeId = CPU_id();     // CPU "unqiue" ID (compressed from 96bits), such as my STM32F103RET6=0x2CDBE292
	RCC_Config(); // system Clocks Configuration
	Timer_Config();

	OneWire_Config();
	ADC_Config();
	Motion_Config();
	//Relay_Config();
	USART_Config();

	NVIC_Config(); // NVIC configuration
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
// relay
// ======================================================================
void PECU_setRelay(uint8_t id, uint8_t on)
{
	id %= CHANNEL_SZ_Relay;
	if (on)
		BlinkPair_set(&EcuCtrl_relays[id], BlinkPair_FLAG_A_ENABLE, 100, 0, 0);
	else
		BlinkPair_set(&EcuCtrl_relays[id], BlinkPair_FLAG_A_ENABLE, 0, 100, 0);
}

//#define fd232 (3)
//#define fd485 (4)
//
//#define f232 ((FILE*)fd232)
//#define f485 ((FILE*)fd485)
//
//uint8_t fdcan = 5;
//uint8_t fdext = fd485;
//uint8_t fdadm = fd232;
//uint8_t fdeth = 6;

USART TTY1 = {USART1, {NULL,} , {NULL,}};
// USART RS485 = {USART3, {NULL,} , {NULL,}};

// =========================================================================
// fputc(): portal of printf() to USART1
// =========================================================================
int fputc(int ch, FILE *f)
{
	// forward the printf() to USART1
	USART_SendData(USART1, (uint8_t) ch);
	while (!(USART1->SR & USART_FLAG_TXE)); // wait till sending finishes

	return (ch);
}

