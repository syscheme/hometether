// =========================================================================
// The water2 board is based on rnode version ??
// Its specifications are
//   1) it will never adapt nRF24L01, and only take Si4432
//   2) the pins of Si4432 was damaged, so it takes those of nRF24L01 instead except SDN and nSEL
//   3) pin B12,13 is taken to drive L9110S to control the valve
//   4) pin B7 is taken to drive the relay that powers the pumper
//   5) pin B6 is taken to capture ASK 315Mhz
//   6) the ADC PA3 is taken to read the input of water flow
//   7) pin B14 is taken to read mulitiple DS18B20s 
// =========================================================================

#include "includes.h"
#include "usart.h"

#define SWD_DEBUG

#ifdef RAM_DEBUG
#  define VECT_TAB_RAM
#  undef  SWD_DEBUG
#endif // RAM_DEBUG

#define VerOfMon	  0x1209

// new version:
// PA1-TIM2-CH2->TI2FP2 CAP
// PB4,5 ->LED0,1; PB4=JnTRST

#if defined(USB_ENABLED) && defined(CAN_ENABLED)
#  error conflict to have both USB and CAN enabled
#endif // USB_ENABLED && CAN_ENABLED

// =========================================================================
// MCU configurations
// =========================================================================
void     RNode_RTC_Init(void);
uint32_t RNode_updateClock(uint32_t newTime);

uint8_t fd232 = 5;
uint8_t fd485 = 6;

// =========================================================================
// On-board IO Resources
// =========================================================================
// nRF24L01 nrf24l01 = {0x00, 0x00, SPI1, {GPIOA, GPIO_Pin_4}, {GPIOB, GPIO_Pin_11}, 0, 0 };
// SI4432   si4432   = {0x00, NULL, SPI1, {GPIOA, GPIO_Pin_8}, {GPIOB, GPIO_Pin_10}, EXTI_Line1, 0};
// take nRF2401's B11 as SDN because of the damaged B10
SI4432   si4432   = {0x00, NULL, SPI1, {GPIOA, GPIO_Pin_8}, {GPIOB, GPIO_Pin_10}, EXTI_Line0, 0}; 
USART    COM1 = {USART1, {NULL,} , {NULL,}};

// Onboard LED pair
// ---------------------------------------------------------------------------
BlinkPair RNode_ledPair = {
	{GPIOB, GPIO_Pin_4}, {GPIOB, GPIO_Pin_5}, // onboard LEDs: PB4/5
};

// ============================================================================
// Function Name  : RNode_RCC_Config
// Description    : Configures the different system clocks.
// Input          : None
// Output         : None
// Return         : None
// ============================================================================
void RNode_RCC_Config(void)
{
	RCC_DeInit(); // RCC system reset(for debug purpose),take internal clk

#ifdef SYSCLK_FREQ_HSE
	RCC_HSEConfig(RCC_HSE_ON); // Enable HSE

	if (SUCCESS == RCC_WaitForHSEStartUp()) // Wait till HSE is ready
	{
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable); // Enable Prefetch Buffer
		FLASH_SetLatency(FLASH_Latency_2); // Flash 2 wait state
		RCC_HCLKConfig(RCC_SYSCLK_Div1);   // HCLK = SYSCLK
		RCC_PCLK2Config(RCC_HCLK_Div1);    // AHB2 PCLK2 = HCLK
		RCC_PCLK1Config(RCC_HCLK_Div2);    // AHB1 PCLK1 = HCLK/2
		RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // ADC clock PCLK2/6 = 12MHz
		// RCC_PLLConfig(RCC_PLLSource_HSE_Div2, RCC_PLLMul_9); // PLLCLK = 8MHz/2 * 9 = 36 MHz
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9); // PLLCLK = 8MHz * 9 = 72 MHz
		// FLASH_SetLatency(FLASH_Latency_2);
		// SystemFrequency = 8000000*9; // 72Mhz
	}
	else
#endif // SYSCLK_FREQ_HSE
	{
		RCC_DeInit(); // Enable HSI
		RCC_HSEConfig(RCC_HSE_OFF);
		RCC_HSICmd(ENABLE);     
		RCC_HCLKConfig(RCC_SYSCLK_Div1);   // HCLK = SYSCLK
		RCC_PCLK2Config(RCC_HCLK_Div1);    // AHB2 PCLK2 = HCLK
		RCC_PCLK1Config(RCC_HCLK_Div2);    // AHB1 PCLK1 = HCLK/2
		RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // ADC clock PCLK2/6 = 12MHz
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_9); // PLLCLK = 4MHz * 9 = 36 MHz
		// SystemFrequency = 4000000*9; // 36Mhz
	}

	RCC_PLLCmd(ENABLE); // Enable PLL 

	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET); // Wait till PLL is ready

	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK); // Select PLL as system clock source
	while(RCC_GetSYSCLKSource() != 0x08); // Wait till PLL is used as system clock source

	// SysTick_Config(SystemFrequency / 1000000);

	// Enable GPIO clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);

	// Enable ADC1 and USART1 clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_USART1 | RCC_APB1Periph_SPI2, ENABLE);

	// enable SPI2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	// enable DMA1
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
}


// =========================================================================
// On board interrupts
// =========================================================================
void RNode_NVIC_Config(void)
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

	// enable the EXTI9_5 for pulse capture
	NVIC_InitStruct.NVIC_IRQChannel                   = EXTI9_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 2;
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	// enable the B0-EXTI0 for SI4432(beause of the damaged B1, take nRF24L01's B0 instead)
	NVIC_InitStruct.NVIC_IRQChannel                   = EXTI0_IRQn; // EXTI1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 2;
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	// enable USART1 int
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

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
/* sample from RTOS-demo
int fputc( int ch, FILE *f )
{
static unsigned short usColumn = 0, usRefColumn = mainCOLUMN_START;
static unsigned char ucLine = 0;

	if( ch != '\n' )
	{
		// Decrement the column position by 16
		usRefColumn -= mainCOLUMN_INCREMENT;

		// Increment the character counter
		usColumn++;
		if( usColumn == mainMAX_COLUMN )
		{
			ucLine += mainROW_INCREMENT;
			usRefColumn = mainCOLUMN_START;
			usColumn = 0;
		}
	}
	else
	{
		// Move back to the first column of the next line.
		ucLine += mainROW_INCREMENT;
		usRefColumn = mainCOLUMN_START;
		usColumn = 0;
	}

	// Wrap back to the top of the display.
	if( ucLine >= mainMAX_LINE )
	{
		ucLine = 0;
	}

	return ch;
}
*/
void RNode_blink( uint8_t mode, uint8_t reloadA, uint8_t reloadB, uint8_t blinks)
{
	BlinkPair_set(&RNode_ledPair, mode, reloadA, reloadB, blinks);
}

void RNode_reset(void)
{
	// blink Leds quickly for 3sec, then reset
	RNode_blink(3, 2, 2, 0); // or BlinkPair_set(&RNode_ledPair, 3, 2, 2, 30/(2+2));
	delayXmsec(3000);

	NVIC_SystemReset();
}

void RNode_Timer_config() // takes TIM2 as the source timer of 5usec or 200KHz
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

void RNode_LED_USART1_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

#ifdef SWD_DEBUG
	// only enable SWD, so reuse the other JTAG pins
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
#endif // JTAG_DEBUG

	// about the onboard LEDs: PB4,PB5
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;	// because the onboard leds are enabled by vH, so take Push-Pull mode
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_5;
#ifdef SWD_DEBUG
	// only enable SWD, so reuse the other JTAG pins
	GPIO_InitStruct.GPIO_Pin  |=  GPIO_Pin_4;
#endif // JTAG_DEBUG
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_SetBits(GPIOB, GPIO_Pin_4 | GPIO_Pin_5); // turn off the leds
	BlinkList_register(&RNode_ledPair);

	// about the onboard USART1(A9,10)
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Configure USART1 Tx (PA.09) as alternate function push-pull
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// USART_open(&COM1, baudX100);
}

void RNode_ADC_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	DMA_InitTypeDef  DMA_InitStruct;
	ADC_InitTypeDef  ADC_InitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_ADC1 | RCC_APB2Periph_SPI1, ENABLE);

	// about the onboard ADC that links to DMA1_Channel1
	// ------------------------------------------------------
	// 1 about input of waterflow ADC-IN3: PA3
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_3;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;	  //端口模式为模拟输入方式
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// http://bbs.ednchina.com/BLOG_ARTICLE_146193.HTM
	// http://www.mculover.com/post/140.html
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t) &(ADC1->DR);
	DMA_InitStruct.DMA_MemoryBaseAddr     = (uint32_t) ADC_DMAbuf;
	DMA_InitStruct.DMA_DIR                = DMA_DIR_PeripheralSRC;
	DMA_InitStruct.DMA_BufferSize         = sizeof(uint16_t) * ADC_CHS;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 16bit ADC value
	DMA_InitStruct.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord; // 16bit ADC value
	DMA_InitStruct.DMA_MemoryInc          = DMA_MemoryInc_Enable;        // receive buffer take increasing step
	DMA_InitStruct.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;   // single ADC1->DR, step=0
	DMA_InitStruct.DMA_Mode               = DMA_Mode_Circular;  // continuous loop
	DMA_InitStruct.DMA_Priority           = DMA_Priority_Medium; 
	DMA_InitStruct.DMA_M2M                = DMA_M2M_Disable; // non-m2m copy

	DMA_Init(DMA1_Channel1, &DMA_InitStruct);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA1_Channel1, ENABLE); // Enable DMA1 channel1

	ADC_InitStruct.ADC_Mode               = ADC_Mode_Independent;	 //ADC1和ADC2工作的独立模式
	ADC_InitStruct.ADC_ScanConvMode       = ENABLE;		             //ADC设置为多通道扫描模式
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;	                 //设置为连续转换模式
	ADC_InitStruct.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;  //由软件控制开始转换（还有触方式等）
	ADC_InitStruct.ADC_DataAlign          = ADC_DataAlign_Right;	 //AD输出数值为右端对齐方式
	ADC_InitStruct.ADC_NbrOfChannel       = ADC_CHS;				 //指定要进行AD转换的信道1

	ADC_Init(ADC1, &ADC_InitStruct);						         //用上面的参数初始化ADC1

	// the internal temperature
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);
	ADC_TempSensorVrefintCmd(ENABLE);

	// A7 is taken as the input of water-flow status
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3,  2, ADC_SampleTime_239Cycles5); //将ADC1-IN3(PA3)的采样时间设置为55.5个周期

	// enable ADC1 DMA and ADC1
	ADC_DMACmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE); 

	// enable ADC1 reset calibaration register, and wait till it gets completed
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));

	// start ADC1 calibaration, and wait till it finishes
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));

	// start ADC1 software conversion
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void RNode_nRF_Config()
{
	GPIO_InitTypeDef GPIO_InitStruct;
	EXTI_InitTypeDef EXTI_InitStruct;
	SPI_InitTypeDef  SPI_InitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_SPI1, ENABLE);

	// about the SPI1 (A5-7)
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// SPI1 configuration
	SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStruct.SPI_Mode      = SPI_Mode_Master;
	SPI_InitStruct.SPI_DataSize  = SPI_DataSize_8b;
	SPI_InitStruct.SPI_CPOL      = SPI_CPOL_Low; //?? SPI_CPOL_High=模式3，时钟空闲为高 //SPI_CPOL_Low=模式0，时钟空闲为低
	SPI_InitStruct.SPI_CPHA      = SPI_CPHA_1Edge; //SPI_CPHA_2Edge;//SPI_CPHA_1Edge, SPI_CPHA_2Edge;
	SPI_InitStruct.SPI_NSS       = SPI_NSS_Soft;
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // SPI_BaudRatePrescaler_8=9MHz <=24L01的最大SPI时钟为10Mhz
	SPI_InitStruct.SPI_FirstBit  = SPI_FirstBit_MSB;//数据从高位开始发送
	SPI_InitStruct.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStruct);
	SPI_Cmd(SPI1, ENABLE);   // enable SPI2

	// about the adapted Si4432
	// ------------------------------------------------------
	// step 1. about A8-CS B10-SDN
	GPIO_InitStruct.GPIO_Pin   = si4432.pinCSN.pin;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(si4432.pinCSN.port, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin   = si4432.pinSDN.pin;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(si4432.pinSDN.port, &GPIO_InitStruct);

	// step 2. about B0-IRQ (because of the damaged B1, take nRF24L01's B0 instead)
	GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_0; // GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0); // GPIO_PinSource1);

	EXTI_StructInit(&EXTI_InitStruct);
	EXTI_InitStruct.EXTI_Line    = EXTI_Line0; // EXTI_Line1;
	EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	EXTI_ClearITPendingBit(EXTI_Line0);	 // EXTI_Line1
	EXTI_GenerateSWInterrupt(EXTI_Line0); //中断允许
}

void Water_Config_IO(void);

void BSP_Init(void)
{
	// read the cpu id, and set it to the nRF modules
	nidThis = *((const uint32_t*)0xE000ED00); // CPU type ID , such as STM32F103XXX=0x411FC231
	nidThis = CPU_id();         // CPU "unqiue" ID (compressed from 96bits), such as my STM32F103RET6=0x2CDBE292
	si4432.nodeId = nidThis; 

	// System Clocks Configuration
	RNode_RCC_Config();   
	//	RNode_RTC_Init();

	RNode_Timer_config();
	RNode_LED_USART1_Config(); 
	RNode_ADC_Config();
	RNode_nRF_Config();

	Water_Config_IO();
	Water_populate_DS18B20s();

	// NVIC configuration
	RNode_NVIC_Config();
}

void RNode_BSP_Start(void)
{
	// start ADC1 Software Conversion	SHOULD BE MOVED TO TASK_Start
	//	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

uint8_t RNode_indexByExtiLine(uint32_t extiLine)
{
	uint8_t i=0;
	for (i =0; i < CCP_NUM; i++)
	{
		if (extiLine == pulsechs[i].capPin.pin.pin)
			return i;
	}

	return 0xff; // invalid
}

PulseCapture pulsechs[CCP_NUM];
// static uint8_t      pulseL =0; //value of LOWs
uint8_t RNode_readPlusePinStatus()
{
	uint8_t flags =0, i;
	for (i =0; i < CCP_NUM; i++)
	{
		flags <<=1;
		if (pulsechs[i].capPin.pin.port)
			flags |= PulsePin_get(pulsechs[i].capPin);
	}

	return flags;
}

// =========================================================================
// Water IO Resources
// =========================================================================
const static IO_PIN PumperPin = {GPIOB, GPIO_Pin_7}; // B7
const static IO_PIN ValvePinPair[2] = { {GPIOB, GPIO_Pin_12}, {GPIOB, GPIO_Pin_13} };	 // B12,13

DS18B20  ds18b20s[DS18B20_MAX];
const static OneWire onewire = { {GPIOB, GPIO_Pin_14} }; // B14
const static IO_PIN Cap315MhzPin = {GPIOB, GPIO_Pin_6};  // B6->TIM4_C1

hterr Water_cbDS18B20Detected(const OneWire* hostPin, OneWireAddr_t devAddr)
{
	int i=0;
	for (i=0; i <DS18B20_MAX; i++)
	{
		if (NULL == ds18b20s[i].bus)
			break;
	}

	if (i>=DS18B20_MAX)
		return ERR_OVERFLOW;

	ds18b20s[i].bus = hostPin, ds18b20s[i].rom = devAddr;
	return ERR_SUCCESS;
}

void Water_populate_DS18B20s(void)
{
	memset(ds18b20s, 0x00, sizeof(ds18b20s));
	OneWire_resetBus(&onewire);
	OneWire_scanForDevices(&onewire, Water_cbDS18B20Detected);
	OneWire_resetBus(&onewire);
}

void Water_Config_IO(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	// B7 to drive the relay of pumper
	// --------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = PumperPin.pin;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_OD; //开漏输出
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(PumperPin.port, &GPIO_InitStruct);

	// B12,13 to drive the motor H-bridge
	// --------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = ValvePinPair[0].pin;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP; //推挽输出
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(ValvePinPair[0].port, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin   = ValvePinPair[1].pin;
	GPIO_Init(ValvePinPair[1].port, &GPIO_InitStruct);

	// B14 connect to DS18B20 chips thru onewire
	// http://linfengdu.blog.163.com/blog/static/1177107320103821648418/
	// --------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = onewire.pin.pin;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_OD; //开漏输出
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(onewire.pin.port, &GPIO_InitStruct);

}

void Water_Config_PulseCapture()
{
	GPIO_InitTypeDef GPIO_InitStruct;
	EXTI_InitTypeDef EXTI_InitStruct;

	PulseCapture_init(&pulsechs[0]);

	// B6/TIM4_C1 connect to 315Mhz ASK receiver
	// --------------------------------------------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStruct.GPIO_Pin   = Cap315MhzPin.pin;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Cap315MhzPin.port, &GPIO_InitStruct);

	// mapping the EXTI and PulseCapture's io
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource6);
	pulsechs[0].capPin.pin = Cap315MhzPin;

	// enable the EXTI
	EXTI_StructInit(&EXTI_InitStruct);
	EXTI_InitStruct.EXTI_Line    = EXTI_Line6;
	EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising_Falling;	// both edge
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	// enable the interrupts
	EXTI_ClearITPendingBit(EXTI_Line6);
	EXTI_GenerateSWInterrupt(EXTI_Line6);
}

void Water_enablePumper(uint8_t enable)
{
	pinSET(PumperPin, enable?0:1);
}

uint8_t Water_isPumping(void)
{
	return pinGET(PumperPin)?0:1;
}

void Water_setLoopMode(LoopMode mode)
{
	switch (mode)
	{
	case LoopMode_IDLE0:
		pinSET(ValvePinPair[0], 0);
		pinSET(ValvePinPair[1], 0);
		break;

	case LoopMode_FULL:
		pinSET(ValvePinPair[0], 1);
		pinSET(ValvePinPair[1], 0);
		break;

	case LoopMode_NORMAL:
		pinSET(ValvePinPair[0], 0);
		pinSET(ValvePinPair[1], 1);
		break;

	case LoopMode_IDLE:
	default:
		pinSET(ValvePinPair[0], 1);
		pinSET(ValvePinPair[1], 1);
		break;
	}
}
