#include "includes.h"

#define VerOfMon	  0x1209

// new version:
// PA1-TIM2-CH2->TI2FP2 CAP
// PA2,3 ->LED
// PA4,5-ADC
// PA6-TIM3-CH1->TI1FP1 CAP
// PB1,2->EXTI
// PB10,11 ->USART3
// PB12,13,14,15-SPI2 -> SPI2
// PA8-IO -> USB-DR
// PA9,10 - USART1-> USART1
// PA11,12 -USB/CAN -> USB/CAN
// PB6,7-I2C1->I2C1
// PB8,9TIM4->PWM


#define MAX_OnBoard_LED (4)
#define MAX_OnBoard_DVR (2)

#if defined(USB_ENABLED) && defined(CAN_ENABLED)
#  error conflict to have both USB and CAN enabled
#endif // USB_ENABLED && CAN_ENABLED

// =========================================================================
// MCU configurations
// =========================================================================
void RCC_Configuration(void);
void GPIO_Configuration(void);
void NVIC_Configuration(void);
#ifdef CAN_ENABLED
void CAN_Configuration(void);
#endif // CAN_ENABLED

void enablePWM(TIM_TypeDef* TIMx, uint16_t reload);
void ISBoard_RTC_Init(void);
uint32_t ISBoard_updateClock(uint32_t newTime);

uint8_t fd232 = 5;
uint8_t fd485 = 6;

// =========================================================================
// On-board IO Resources
// =========================================================================
const static IO_PIN OnBoardLEDs[MAX_OnBoard_LED] = 
{
#if (VerOfMon >= 0x1212)
	{GPIOA, GPIO_Pin_0},
#endif
	{GPIOA, GPIO_Pin_1},
	{GPIOA, GPIO_Pin_2},
	{GPIOA, GPIO_Pin_3},
#if (VerOfMon < 0x1212)
	{GPIOA, GPIO_Pin_4},
#endif
};

const static IO_PIN OnBoardDvrs[MAX_OnBoard_DVR] = // the drivers powered by S8550
{
	{GPIOB, GPIO_Pin_11},
	{GPIOB, GPIO_Pin_12},
};

const static IO_PIN MotionBit = {GPIOB, GPIO_Pin_7};

DS18B20  ds18b20;
// nRF24L01 nrf24l01 = { NULL, SPI2, {GPIOB, GPIO_Pin_1}, {GPIOB, GPIO_Pin_0}, EXTI_Line8, {1,2,3,4} };

// ---------------------------------------------------------------------------
// CAN bit rates - Registers setup
// ---------------------------------------------------------------------------

typedef struct _CAN_BaudRateConfig {
	uint8_t Prescaler;         //(1...64)Baud Rate Prescaler
	uint8_t SJW;         //(1...4) SJW time
	uint8_t BS1;      //(1...8) Phase Segment 1 time
	uint8_t BS2;      //(1...8) Phase Segment 2 time
} CAN_BaudRateConfig;

#ifdef CAN_ENABLED
const static CAN_BaudRateConfig CAN_BaudRateConfig500Kbps= {1, CAN_SJW_1tq, CAN_BS1_13tq, CAN_BS2_2tq};  // 500kbps, (1+13)/(1+13+2)=87.5%
#define CAN_BaudRate CAN_BaudRateConfig500Kbps
#endif // CAN_ENABLED

void ISBoard_setLed(uint8_t id, uint8_t on) // i.e. setLed(0~3) to turn on led 0~3, setLed(0x80|(0~3)) to turn off the led
{ GPIO_WriteBit(OnBoardLEDs[id % MAX_OnBoard_LED].port, OnBoardLEDs[id % MAX_OnBoard_LED].pin, (BitAction)(on?0:1)); }

void ISBoard_setDvr(uint8_t id, uint8_t on)
{ GPIO_WriteBit(OnBoardDvrs[id % MAX_OnBoard_DVR].port, OnBoardDvrs[id % MAX_OnBoard_DVR].pin, (BitAction)(on?0:1)); }

void ISBoard_485sendMode(uint8_t sendMode)
{ GPIO_WriteBit(GPIOA, GPIO_Pin_8, (BitAction)(sendMode?1:0)); }

// ============================================================================
// Function Name  : RCC_Configuration
// Description    : Configures the different system clocks.
// Input          : None
// Output         : None
// Return         : None
// ============================================================================
void ISBoard_RCC_Config(void)
{
	ErrorStatus HSEStartUpStatus;   

	RCC_DeInit(); // RCC system reset(for debug purpose)
	RCC_HSEConfig(RCC_HSE_ON); // Enable HSE
	HSEStartUpStatus = RCC_WaitForHSEStartUp(); // Wait till HSE is ready

	if (HSEStartUpStatus == SUCCESS)
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

	// enable DMA1
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

}

// the DMA buf to receive ADC result
extern uint16_t ADC_DMAbuf[ADC_CHS];

void ISBoard_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	ADC_InitTypeDef  ADC_InitStruct;
	DMA_InitTypeDef  DMA_InitStruct;
	EXTI_InitTypeDef EXTI_InitStruct;
	SPI_InitTypeDef  SPI_InitStruct;

	int i;

	// section 1. about the onboard LEDs: PA1, PA2, PA3, PA4
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;	// because the onboard leds are enabled by vH, so take Push-Pull mode
	GPIO_InitStruct.GPIO_Pin   = 0;
	for (i=0; i <MAX_OnBoard_LED ; i++)
		GPIO_InitStruct.GPIO_Pin |= OnBoardLEDs[i].pin;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// turn off the LEDs
	for (i=0; i <MAX_OnBoard_LED ; i++)
		ISBoard_setLed(i, 0);

	// section 2. about the onboard drivers powered by S8550: PB11, B12
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_OD;	//开漏输出
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	// turn off the drivers
	for (i=0; i <MAX_OnBoard_DVR ; i++)
		ISBoard_setDvr(i, 0);

	// section 3. about the DS18B20 that is connected to B5
	// --------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_5;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_OD; //开漏输出
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	DS18B20_InitConfiguration(&ds18b20, (POneWireBusCtx) 0, 0x00);
	OWReset(NULL);

	// section 3. about the onboard ADC	that links to DMA1_Channel1
	// ------------------------------------------------------
#ifdef LUMIN_ADC
	// section 3.1 about the onboard Lumin ADC: PA7
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;	  //端口模式为模拟输入方式
	GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif // LUMIN_ADC

	// http://bbs.ednchina.com/BLOG_ARTICLE_146193.HTM
	// http://www.mculover.com/post/140.html
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t) &(ADC1->DR);
	DMA_InitStruct.DMA_MemoryBaseAddr     = (uint32_t) ADC_DMAbuf;
	DMA_InitStruct.DMA_DIR                = DMA_DIR_PeripheralSRC;
	DMA_InitStruct.DMA_BufferSize         = sizeof(uint16_t) * ADC_CHS;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 16bit ADC value
	DMA_InitStruct.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord; // 16bit ADC value
	DMA_InitStruct.DMA_MemoryInc          = DMA_MemoryInc_Enable; // receive buffer take increasing step
	DMA_InitStruct.DMA_PeripheralInc      = DMA_PeripheralInc_Disable; // single ADC1->DR, step=0
//#ifdef LUMIN_ADC
//	DMA_InitStruct.DMA_PeripheralInc      = DMA_PeripheralInc_Enable; // multi ADC1->DR, step=1
//#endif // LUMIN_ADC
	DMA_InitStruct.DMA_Mode               = DMA_Mode_Circular;  // continuous loop
	DMA_InitStruct.DMA_Priority           = DMA_Priority_Medium; 
	DMA_InitStruct.DMA_M2M                = DMA_M2M_Disable; // non-m2m copy

	DMA_Init(DMA1_Channel1, &DMA_InitStruct);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA1_Channel1, ENABLE); // Enable DMA1 channel1

	ADC_InitStruct.ADC_Mode               = ADC_Mode_Independent;	 //ADC1和ADC2工作的独立模式
	ADC_InitStruct.ADC_ScanConvMode       = ENABLE;		 //ADC设置为多通道扫描模式
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;	 //设置为连续转换模式
	ADC_InitStruct.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;	   //由软件控制开始转换（还有触方式等）
	ADC_InitStruct.ADC_DataAlign          = ADC_DataAlign_Right;	   //AD输出数值为右端对齐方式
	ADC_InitStruct.ADC_NbrOfChannel       = ADC_CHS;					   //指定要进行AD转换的信道1

	ADC_Init(ADC1, &ADC_InitStruct);						   //用上面的参数初始化ADC1

	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_55Cycles5);
	ADC_TempSensorVrefintCmd(ENABLE);

#ifdef LUMIN_ADC
	ADC_RegularChannelConfig(ADC1, ADC_Channel_7,  2, ADC_SampleTime_55Cycles5); //将ADC1信道1的转换通道7（PA7）的采样时间设置为55.5个周期
#endif // LUMIN_ADC

	// enable ADC1 DMA and ADC1
	ADC_DMACmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE); 

	// enable ADC1 reset calibaration register, and wait till it gets completed
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));

	// start ADC1 calibaration, and wait till it finishes
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));

	// start ADC1 Software Conversion	SHOULD BE MOVED TO TASK_Start
	// ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	// section 4. about the onboard USART3(B10/11, DE-A8)
	// ------------------------------------------------------
	// TODO:
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Configure USART1 Tx (PA.09) as alternate function push-pull
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// the pin DE of 485

#ifdef nRF24L01_ADAPTED
	// section 5. about the adapted nRF24L01
	// ------------------------------------------------------
	// step 1. about B0-CSB B1-CE
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	// step 3. the SPI2 (B13-15)
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

	// step 2. about A8-IRQ
	GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_8;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	// about the exti8 for A8 that connects to IRQ of nRF24l01
	EXTI_DeInit();

#ifndef NRF2401_POLL
	EXTI_StructInit(&EXTI_InitStruct);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource8);
	EXTI_InitStruct.EXTI_Line = EXTI_Line8;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	EXTI_ClearITPendingBit(EXTI_Line8);
	EXTI_GenerateSWInterrupt(EXTI_Line8); //中断允许
#endif // NRF2401_POLL

#endif // nRF24L01_ADAPTED

#ifdef CAN_ENABLED
	// section 6. about the onboard CAN bus
	// ------------------------------------------------------
	// TODO: CAN_RX=B8, 上拉输入; CAN_TX=B9, 复用推拉输出
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_8;	
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	CAN_Configuration();
#endif // CAN_ENABLED

	// section 7. about the motion state connected to B7
	// --------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = MotionBit.pin;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(MotionBit.port, &GPIO_InitStruct);

	// section 8. about the IR recv connected to A6
	// --------------------------------------------------
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	EXTI_StructInit(&EXTI_InitStruct);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource6);
	EXTI_InitStruct.EXTI_Line = EXTI_Line6;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	EXTI_ClearITPendingBit(EXTI_Line6);
	EXTI_GenerateSWInterrupt(EXTI_Line6); //中断允许
}

uint8_t motionState(void) { return GPIO_ReadInputDataBit(MotionBit.port, MotionBit.pin)?0:1; }
uint8_t irRecvBit(void) { return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)?1:0; }

#ifdef CAN_ENABLED
// ----------------------------------------
// CAN_Configuration
// ----------------------------------------
uint8_t CAN_setBaud(uint8_t baud)
{
	CAN_InitTypeDef        CAN_InitStruct;
	const CAN_BaudRateConfig*    baudConf = NULL;

	// determin baud to take and the baudConf
	switch(baud)
	{
	case CanBaud_10Kbps:
	case CanBaud_20Kbps:
	case CanBaud_50Kbps:
	case CanBaud_125Kbps:
	case CanBaud_250Kbps:
	case CanBaud_500Kbps:
	case CanBaud_800Kpbs:
	case CanBaud_1Mbps:
	default:
		baudConf = &CAN_BaudRateConfig500Kbps;
		baud     = CanBaud_500Kbps;
		break;
	}

	// CAN cell init
	CAN_StructInit(&CAN_InitStruct);				  //将寄存器全部设置成默认值

	CAN_InitStruct.CAN_TTCM=DISABLE;			   //禁止时间触发通信方式
	CAN_InitStruct.CAN_ABOM=DISABLE;			   //禁止CAN总线自动关闭管理
	CAN_InitStruct.CAN_AWUM=DISABLE;			   //禁止自动唤醒模式
	CAN_InitStruct.CAN_NART=DISABLE;			   //禁止非自动重传模式
	CAN_InitStruct.CAN_RFLM=DISABLE;			   //禁止接收FIFO锁定
	CAN_InitStruct.CAN_TXFP=DISABLE;			   //禁止发送FIFO优先级
#ifdef CAN_LOOPBACK
	CAN_InitStruct.CAN_Mode=CAN_Mode_Loopback;
#else
	CAN_InitStruct.CAN_Mode=CAN_Mode_Normal;		  //设置CAN工作方式为正常收发模式
#endif

	CAN_InitStruct.CAN_SJW       =baudConf->SJW;			  //设置重新同步跳转的时间量子
	CAN_InitStruct.CAN_BS1       =baudConf->BS1;			  //设置字段1的时间量子数
	CAN_InitStruct.CAN_BS2       =baudConf->BS2;			  //设置字段2的时间量子数
	CAN_InitStruct.CAN_Prescaler =baudConf->Prescaler;				  //配置时间量子长度为1周期
	CAN_Init(CAN1, &CAN_InitStruct);			   //用以上参数初始化CAN1端口

	return baud;
}

void CAN_Configuration(void)
{
	CAN_FilterInitTypeDef  CAN_FilterInitStruct;

	// CAN register init
	CAN_DeInit(CAN1);									  //复位CAN1的所有寄存器

	CAN_setBaud(CanBaud_500Kbps);

	// CAN filter init
	CAN_FilterInitStruct.CAN_FilterNumber         =0;						//选择CAN过滤器0
	CAN_FilterInitStruct.CAN_FilterMode           =CAN_FilterMode_IdMask;	//初始化为标识/屏蔽模式
	CAN_FilterInitStruct.CAN_FilterScale          =CAN_FilterScale_32bit;	//选择过滤器为32位
	CAN_FilterInitStruct.CAN_FilterIdHigh         =0x0000;					//过滤器标识号高16位
	CAN_FilterInitStruct.CAN_FilterIdLow          =0x0000;					//过滤器标识号低16位
	CAN_FilterInitStruct.CAN_FilterMaskIdHigh     =0x0000;				    //根据模式选择过滤器标识号或屏蔽号的高16位
	CAN_FilterInitStruct.CAN_FilterMaskIdLow      =0x0000;				    //根据模式选择过滤器标识号或屏蔽号的低16位
	CAN_FilterInitStruct.CAN_FilterFIFOAssignment =CAN_FIFO0;		        //将FIFO 0分配给过滤器0
	CAN_FilterInitStruct.CAN_FilterActivation     =ENABLE;				    //使能过滤器
	CAN_FilterInit(&CAN_FilterInitStruct);
}
#endif // CAN_ENABLED

// =========================================================================
// On board interrupts
// =========================================================================
void ISBoard_NVIC_Config(void)
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

	// DMA1 interrupts for ADC converstions
	NVIC_InitStruct.NVIC_IRQChannel                   = DMA1_Channel1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	// enable the EXTI9_5 for A8-IRQ of nRF2401 and A6 of IR recv
	NVIC_InitStruct.NVIC_IRQChannel                   = EXTI9_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

#if defined(USB_ENABLED) || defined(CAN_ENABLED)
	NVIC_InitStruct.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 2;
 	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
 	NVIC_Init(&NVIC_InitStruct);
#endif // USB_ENABLED ||| CAN_ENABLED
}

#ifdef USB_ENABLED
// -----------------------------
// Set_USBClock()
// Description : Configures USB Clock input (48MHz)
// Argument: none.
// Return   : none.
// Note    : none.
// -----------------------------
void Set_USBClock(void)
{
  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5); // USBCLK = PLLCLK
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE); // Enable USB clock
}

// -----------------------------
// USB_Cable_Config()
// Description : Software Connection/Disconnection of USB Cable
// Argument: none.
// Return   : none.
// -----------------------------
void USB_Cable_Config (FunctionalState NewState)
{
//	ISBoard_setLed(2, (NewState != DISABLE)?1:0);
	ISBoard_setDvr(0, (NewState != DISABLE)?1:0);
//  if (NewState != DISABLE)
//    GPIO_ResetBits(GPIOC, GPIO_Pin_3);
//  else
//    GPIO_SetBits(GPIOC, GPIO_Pin_3);
}

// -----------------------------
// Enter_LowPowerMode()
// Description : Power-off system clocks and power while entering suspend mode
// Argument: none.
// Return   : none.
// -----------------------------
void Enter_LowPowerMode(void)
{
}

// -----------------------------
// Leave_LowPowerMode()
// Description : Restores system clocks and power while exiting suspend mode
// Argument    : none.
// Return      : none.
// -----------------------------
void Leave_LowPowerMode(void)
{
}

#endif // USB_ENABLED


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

// =========================================================================
// fputc(): portal of printf() to USART1
// =========================================================================
// #define DBG_TO_485
int fputc(int ch, FILE *f)
{
	if (((uint8_t)f) == fd485)
	{
		ISBoard_485sendMode(1);
		// forward the printf() to USART3
		USART_SendData(USART3, (uint8_t) ch);
		while (!(USART3->SR & USART_FLAG_TXE)); // wait till sending finishes
		ISBoard_485sendMode(0);
	}
	else
	{
		// forward the printf() to USART1
		USART_SendData(USART1, (uint8_t) ch);
		while (!(USART1->SR & USART_FLAG_TXE)); // wait till sending finishes
	}

	return (ch);
}

//param reload - value refers to PitchReload, 0 means turn off the PWM
void enablePWM(TIM_TypeDef* TIMx, uint16_t reload)
{
	TIM_TimeBaseInitTypeDef TimeBaseStruct;
	TIM_OCInitTypeDef  TIM_OCInitStruct;

	TIM_DeInit(TIMx); // reset TIMx

	if (reload <=0)
		return;

	// Time Base configuration
	TimeBaseStruct.TIM_Prescaler         = 0;				   //预分频数为0,不分频
	TimeBaseStruct.TIM_CounterMode       = TIM_CounterMode_Up; //计数方式为顺序计数模式，增大型
	TimeBaseStruct.TIM_Period            = reload;             //设置计数器溢出后的重载初值
	TimeBaseStruct.TIM_ClockDivision     = 0x00;               //配置时钟分隔值
	TimeBaseStruct.TIM_RepetitionCounter = 0x0;                //循环计数次数值

	TIM_TimeBaseInit(TIMx,&TimeBaseStruct); // initialize TIM

	// Channel 1 Configuration in PWM mode
	TIM_OCInitStruct.TIM_OCMode       = TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState  = TIM_OutputState_Enable;  //使能输出比较状态
	TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Enable; //使能定时器互补输出               
	TIM_OCInitStruct.TIM_Pulse        = 5000;                    //设置脉宽
	TIM_OCInitStruct.TIM_OCPolarity   = TIM_OCPolarity_Low;      //输出比较极性为低
	TIM_OCInitStruct.TIM_OCIdleState  = TIM_OCIdleState_Set;	 //打开空闲状态选择关闭

	TIM_OC1Init(TIMx, &TIM_OCInitStruct);  //initialize the channel of TIM

	TIM_Cmd(TIMx, ENABLE); // TIM8 counter enable
	TIM_CtrlPWMOutputs(TIMx, ENABLE);  // enable PWM
}  

// =========================================================================
// ISBoard_RTC_Init: onboard clock that may be with battery supply
// =========================================================================
void ISBoard_RTC_Init(void)
{
	// initialize the system clock
	if (0xA5A5 != BKP_ReadBackupRegister(BKP_DR1))
	{
		// Backup data register value is not correct or not yet programmed (when the first time the program is executed)
		// RTC Configuration */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE); // enable PWR and BKP clocks
		PWR_BackupAccessCmd(ENABLE); // allow access to BKP domain

		BKP_DeInit(); // reset BKP domain

		RCC_LSEConfig(RCC_LSE_ON); // enable LSE
		while(RESET == RCC_GetFlagStatus(RCC_FLAG_LSERDY)); // wait till LSE is ready

		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);  // select LSE as RTC clock Source  
		RCC_RTCCLKCmd(ENABLE); // enable RTC Clock


#ifdef RTCClockOutput_Enable  
		// Disable the Tamper Pin, to output RTCCLK/64 on Tamper pin, the tamper functionality must be disabled
		BKP_TamperPinCmd(DISABLE);
		BKP_RTCCalibrationClockOutputCmd(ENABLE); // enable RTC Clock Output on Tamper Pin
#endif 

		RTC_WaitForSynchro();	// wait for RTC registers synchronization
		RTC_WaitForLastTask(); // wait until last write operation on RTC registers has finished

		RTC_ITConfig(RTC_IT_SEC, ENABLE);	 // enable the RTC Second
		RTC_WaitForLastTask(); // wait until last write operation on RTC registers has finished

		// set RTC prescaler: set RTC period to 1sec
		RTC_SetPrescaler(32767); // RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1)
		RTC_WaitForLastTask(); // wait until last write operation on RTC registers has finished

		// adjust time to Sun 02:00:00
		ISBoard_updateClock(60*60*1); ISBoard_updateClock(60*60*2);

		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);    
	}
	else
	{
		if (RESET != RCC_GetFlagStatus(RCC_FLAG_PORRST))
		{
			// the case of power off/on just occurred
		}
		else if (RESET != RCC_GetFlagStatus(RCC_FLAG_PINRST))
		{
			// the case of reset pin has been pressed
		}

		// in both cases, the clock will still be restored/continued from the battery supplied
		RTC_WaitForSynchro();  // wait for RTC registers synchronization

		RTC_ITConfig(RTC_IT_SEC, ENABLE); // enable the RTC Second
		RTC_WaitForLastTask(); // wait until last write operation on RTC registers has finished
	}

	RCC_ClearFlag(); // clear reset flags
}

uint32_t ISBoard_getTime(struct tm* pTm)
{
	uint32_t itime = RTC_GetCounter();
	if (NULL != pTm)
	{
		pTm->tm_wday  = itime >>24;
		itime &= 0xffffff;
		pTm->tm_hour = itime/3600;
		itime %= 3600;
		pTm->tm_min  = itime /60;
		pTm->tm_sec %= 60;
	}

	return itime;
}

uint32_t ISBoard_updateClock(uint32_t newTime)
{
	uint32_t itime = ISBoard_getTime(NULL);
	if (newTime<=0 || abs(newTime - itime) < 10)
		return itime;

	newTime &= 0x07ffffff;
	RTC_WaitForLastTask();  // wait until last write operation on RTC registers has finished
	RTC_SetCounter(newTime); // change the current time 
	RTC_WaitForLastTask();  // wait until last write operation on RTC registers has finished

	return newTime;
}

void ISBoard_reset(void)
{
	uint8_t i, j;

	// blink ALL leds synchronously
	for (j=0; j <4; j++)
	{
		for (i=0; i <MAX_OnBoard_LED ; i++)
			ISBoard_setLed(i, 0);
		delayXmsec(300);

		for (i=0; i <MAX_OnBoard_LED ; i++)
			ISBoard_setLed(i, 1);
		delayXmsec(300);
	}

	delayXmsec(700);

	NVIC_SystemReset();
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

void BSP_Init(void)
{
	// System Clocks Configuration
	ISBoard_RCC_Config();   

	ISBoard_GPIO_Config();

	ISBoard_RTC_Init();

	// NVIC configuration
	ISBoard_NVIC_Config();

	USART_open(USART1, 1152);
}

void BSP_Start(void)
{
	// start ADC1 Software Conversion	SHOULD BE MOVED TO TASK_Start
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

CPU_INT32U BSP_CPU_ClkFreq (void)
{
	RCC_ClocksTypeDef  rcc_clocks;
	RCC_GetClocksFreq(&rcc_clocks);

	return ((CPU_INT32U)rcc_clocks.HCLK_Frequency);
}

INT32U OS_CPU_SysTickClkFreq(void)
{
	INT32U  freq = BSP_CPU_ClkFreq();
	return (freq);
}
