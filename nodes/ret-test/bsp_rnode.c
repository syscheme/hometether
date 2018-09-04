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
void RCC_Configuration(void);
void GPIO_Configuration(void);
void NVIC_Configuration(void);

void enablePWM(TIM_TypeDef* TIMx, uint16_t reload);
void RNode_RTC_Init(void);
uint32_t RNode_updateClock(uint32_t newTime);

uint8_t fd232 = 5;
uint8_t fd485 = 6;

// =========================================================================
// On-board IO Resources
// =========================================================================
const static IO_PIN MotionBit = {GPIOB, GPIO_Pin_7};

DS18B20  ds18b20;
nRF24L01 nrf24l01 = {0x00, 0x00, SPI1, {GPIOA, GPIO_Pin_4}, {GPIOB, GPIO_Pin_11}, 0, 0 };
SI4432   si4432   = {0x00, NULL, SPI1, {GPIOA, GPIO_Pin_8}, {GPIOB, GPIO_Pin_10}, EXTI_Line1, 0};
USART    TTY1   = {USART1,}; 

// Onboard LED pair
// ---------------------------------------------------------------------------
BlinkPair RNode_ledPair = {
	{GPIOB, GPIO_Pin_4}, {GPIOB, GPIO_Pin_5}, // onboard LEDs: PB4/5
};

#define USE_INTERNAL_CLK


// ============================================================================
// Function Name  : RNode_RCC_Config
// Description    : Configures the different system clocks.
// Input          : None
// Output         : None
// Return         : None
// ============================================================================
void RNode_RCC_Config(void)
{
#ifndef STM32F103_32MHz
#error RNODE v1 must run at 32Mhz, pls take STM32F103_32MHz as a global preprocessor
#endif // STM32F103_32MHz

	RCC_DeInit(); // RCC system reset(for debug purpose),take internal clk
	RCC_HSEConfig(RCC_HSE_ON); // Enable HSE

	if (SUCCESS == RCC_WaitForHSEStartUp()) // Wait till HSE is ready
	{
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable); // Enable Prefetch Buffer
		FLASH_SetLatency(FLASH_Latency_2); // Flash 2 wait state
		RCC_HCLKConfig(RCC_SYSCLK_Div1);   // HCLK = SYSCLK
		RCC_PCLK2Config(RCC_HCLK_Div1);    // AHB2 PCLK2 = HCLK
		RCC_PCLK1Config(RCC_HCLK_Div2);    // AHB1 PCLK1 = HCLK/2
		RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // ADC clock PCLK2/6 = 12MHz
		RCC_PLLConfig(RCC_PLLSource_HSE_Div2, RCC_PLLMul_9); // PLLCLK = 8MHz/2 * 9 = 36 MHz
	}
	else
	{
		RCC_DeInit(); // Enable HSI
		RCC_HSEConfig(RCC_HSE_OFF);
        RCC_HSICmd(ENABLE);     
		RCC_HCLKConfig(RCC_SYSCLK_Div1);   // HCLK = SYSCLK
		RCC_PCLK2Config(RCC_HCLK_Div1);    // AHB2 PCLK2 = HCLK
		RCC_PCLK1Config(RCC_HCLK_Div2);    // AHB1 PCLK1 = HCLK/2
		RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // ADC clock PCLK2/6 = 12MHz
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_9); // PLLCLK = 4MHz * 9 = 36 MHz
	}

	RCC_PLLCmd(ENABLE); // Enable PLL 

	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET); // Wait till PLL is ready

	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK); // Select PLL as system clock source
	while(RCC_GetSYSCLKSource() != 0x08); // Wait till PLL is used as system clock source

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

/*
void RNode_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	ADC_InitTypeDef  ADC_InitStruct;
	DMA_InitTypeDef  DMA_InitStruct;
	EXTI_InitTypeDef EXTI_InitStruct;
	SPI_InitTypeDef  SPI_InitStruct;

	// section 1. about the onboard LEDs: PB4,PB5
	// ------------------------------------------------------
	// TODO: turn off JTAG
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;	// because the onboard leds are enabled by vH, so take Push-Pull mode
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_SetBits(GPIOB, GPIO_Pin_4 | GPIO_Pin_5); // turn off the leds
	RNode_Blink_init();


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
	// section 3.1 about the onboard Lumin ADC: PA0<->ADCx-IN0
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_0;
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
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0,  2, ADC_SampleTime_55Cycles5); //将ADC1信道1的转换通道7（PA0）的采样时间设置为55.5个周期
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

}

*/

uint8_t motionState(void) { return GPIO_ReadInputDataBit(MotionBit.port, MotionBit.pin)?0:1; }

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
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

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
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 4;
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	// enable the EXTI1 for SI4432
	NVIC_InitStruct.NVIC_IRQChannel                   = EXTI1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
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
//	RNode_setLed(2, (NewState != DISABLE)?1:0);
	RNode_setDvr(0, (NewState != DISABLE)?1:0);
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
// fputc(): portal of printf() to USART1
// =========================================================================
int fputc(int ch, FILE *f)
{
	// forward the printf() to USART1
	USART_SendData(USART1, (uint8_t) ch);
	while (!(USART1->SR & USART_FLAG_TXE)); // wait till sending finishes

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
// RNode_RTC_Init: onboard clock that may be with battery supply
// =========================================================================
void RNode_RTC_Init(void)
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
		RNode_updateClock(60*60*1); RNode_updateClock(60*60*2);

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

uint32_t RNode_getTime(struct tm* pTm)
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

uint32_t RNode_updateClock(uint32_t newTime)
{
	uint32_t itime = RNode_getTime(NULL);
	if (newTime<=0 || abs(newTime - itime) < 10)
		return itime;

	newTime &= 0x07ffffff;
	RTC_WaitForLastTask();  // wait until last write operation on RTC registers has finished
	RTC_SetCounter(newTime); // change the current time 
	RTC_WaitForLastTask();  // wait until last write operation on RTC registers has finished

	return newTime;
}

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
	
	TIM_TimeBaseStruct.TIM_Prescaler = 72 -1; // the counter resolution 36MHz/36 = 1MHz
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

void RNode_Timer_config1(void)
{
 TIM_TimeBaseInitTypeDef   TIM_TimeBaseStructure;

 TIM_DeInit(TIM2);

 TIM_TimeBaseStructure.TIM_Period=2000;		 //ARR的值
 TIM_TimeBaseStructure.TIM_Prescaler=0;
 TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; //采样分频
 TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
 TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
 TIM_PrescalerConfig(TIM2,0x8C9F,TIM_PSCReloadMode_Immediate);//时钟分频系数36000，所以定时器时钟为2K
 TIM_ARRPreloadConfig(TIM2, DISABLE);//禁止ARR预装载缓冲器
 TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);

 TIM_Cmd(TIM2, ENABLE);	//开启时钟

}

void RNode_PulseCapture_Config()
{
	GPIO_InitTypeDef GPIO_InitStruct;
	EXTI_InitTypeDef EXTI_InitStruct;

	// about the PB8,PB9 connects to ASK capture
	// --------------------------------------------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_8;
#if CCP_NUM>1
	GPIO_InitStruct.GPIO_Pin   |= GPIO_Pin_9;
#endif // CCP_NUM
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	// mapping the EXTI and PulseCapture's io
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);
	pulsechs[0].capPin.pin.port = GPIOB; pulsechs[0].capPin.pin.pin = GPIO_Pin_8;
#if CCP_NUM>1
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource9);
	pulsechs[1].capPin.pin.port = GPIOB; pulsechs[1].capPin.pin.pin = GPIO_Pin_9;
#endif // CCP_NUM

	// enable the EXTI
	EXTI_StructInit(&EXTI_InitStruct);
	EXTI_InitStruct.EXTI_Line    = EXTI_Line8 | EXTI_Line9;
	EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising_Falling;	// both edge
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	// enable the interrupts
	EXTI_ClearITPendingBit(EXTI_Line8);
	EXTI_GenerateSWInterrupt(EXTI_Line8);

	EXTI_ClearITPendingBit(EXTI_Line9);
	EXTI_GenerateSWInterrupt(EXTI_Line9);
}

GPIO_InitTypeDef GPIO_InitStruct;

// -----------------------------------
// USART1: A9,A10
// -----------------------------------
static void USART_Config(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_USART1, ENABLE);

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

void RNode_ADC_Config(void)
{
	DMA_InitTypeDef  DMA_InitStruct;
	ADC_InitTypeDef  ADC_InitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_ADC1 | RCC_APB2Periph_SPI1, ENABLE);

	// about the onboard ADC	that links to DMA1_Channel1
	// ------------------------------------------------------
#ifdef LUMIN_ADC
	// 1 about the onboard Lumin ADC: PA0<->ADC-IN0
	// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_0;
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
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0,  2, ADC_SampleTime_55Cycles5); //将ADC1信道1的转换通道7（PA0）的采样时间设置为55.5个周期
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
}

#define nRF_24L01     (1<<0)
#define nRF_Si4432    (1<<1)

void RNode_nRF_Config(uint8_t nRFChips)
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

	// about the adapted nRF24L01
	// ------------------------------------------------------
	if ( nRFChips & nRF_24L01 )
	{
		// step 1. about A4-CS B11-CE
		GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_4;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
		GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_11;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
		GPIO_Init(GPIOB, &GPIO_InitStruct);

		// step 2. about B0/exti0 -IRQ
		GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_0;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(GPIOB, &GPIO_InitStruct);
		GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);

		EXTI_StructInit(&EXTI_InitStruct);
		EXTI_InitStruct.EXTI_Line = EXTI_Line0;
		EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
		EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
		EXTI_InitStruct.EXTI_LineCmd = ENABLE;
		EXTI_Init(&EXTI_InitStruct);

		EXTI_ClearITPendingBit(EXTI_Line0);
		EXTI_GenerateSWInterrupt(EXTI_Line0); //中断允许
	}

	// about the adapted Si4432
	// ------------------------------------------------------
	if ( nRFChips & nRF_Si4432 )
	{
		// step 1. about A8-CS B10-CE
		
		GPIO_InitStruct.GPIO_Pin   = si4432.pinCSN.pin;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
		GPIO_Init(si4432.pinCSN.port, &GPIO_InitStruct);

		GPIO_InitStruct.GPIO_Pin   = si4432.pinSDN.pin;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
		GPIO_Init(si4432.pinSDN.port, &GPIO_InitStruct);

//*
		// step 2. about B1-IRQ
		GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_1;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(GPIOB, &GPIO_InitStruct);
		GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);

		EXTI_StructInit(&EXTI_InitStruct);
		EXTI_InitStruct.EXTI_Line = EXTI_Line1;
		EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
		EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
		EXTI_InitStruct.EXTI_LineCmd = ENABLE;
		EXTI_Init(&EXTI_InitStruct);

		EXTI_ClearITPendingBit(EXTI_Line1);
		EXTI_GenerateSWInterrupt(EXTI_Line1); //中断允许
// */
	}
}

uint32_t BSP_nodeId = 0;

void BSP_Init(void)
{
	// read the cpu id, and set it to the nRF modules
	BSP_nodeId = *((const uint32_t*)0xE000ED00); // CPU type ID , such as STM32F103XXX=0x411FC231
	BSP_nodeId = CPU_id();         // CPU "unqiue" ID (compressed from 96bits), such as my STM32F103RET6=0x2CDBE292
	nrf24l01.nodeId = si4432.nodeId = BSP_nodeId; 

	// System Clocks Configuration
	RNode_RCC_Config();   
//	RNode_RTC_Init();

	RNode_Timer_config();
	USART_Config(); 
//	RNode_ADC_Config();
//	RNode_PulseCapture_Config();
//	RNode_nRF_Config(nRF_Si4432);

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
	switch(extiLine)
	{
	case EXTI_Line8:
		return 0;	  // PulseCapture index
#if CCP_NUM >1
	case EXTI_Line9:
		return 1;
#endif // CCP_NUM
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
