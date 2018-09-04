#include "bsp_stm32_RJ45_4x2.h"

static void enablePWM(TIM_TypeDef* TIMx, uint16_t reload);
static void CAN_Configuration(void);

#define fd232 (3)
#define fd485 (4)

#define f232 ((FILE*)fd232)
#define f485 ((FILE*)fd485)

uint8_t fdcan = 5;
uint8_t fdext = fd485;
uint8_t fdadm = fd232;
uint8_t fdeth = 6;

// =========================================================================
// On-board IO Resources
// =========================================================================
// 1) LED: A1, A4, B0, B1
#define MAX_OnBoard_LED (4)
const static IO_PIN OnBoardLEDs[MAX_OnBoard_LED] = 
{
	{GPIOA, GPIO_Pin_1},
	{GPIOA, GPIO_Pin_4},
	{GPIOB, GPIO_Pin_0},
	{GPIOB, GPIO_Pin_1}
};

USART RS232 = {USART1, {NULL,} , {NULL,}};
USART RS485 = {USART3, {NULL,} , {NULL,}};

// 2) Beep C6
// 3) RS485<->USART3(B10/11, DE-A8)
// 4) RS232<->USART1(A9/10)
// 5) CAN bus remapped to B8/9
// 6) TF card, C8/9/10/11/12, cmd=D2

// ---------------------------------------------------------------------------
// CAN bit rates - Registers setup
// ---------------------------------------------------------------------------

typedef struct _CAN_BaudRateConfig {
	uint8_t Prescaler;         //(1...64)Baud Rate Prescaler
	uint8_t SJW;         //(1...4) SJW time
	uint8_t BS1;      //(1...8) Phase Segment 1 time
	uint8_t BS2;      //(1...8) Phase Segment 2 time
} CAN_BaudRateConfig;

const static CAN_BaudRateConfig CAN_BaudRateConfig500Kbps= {1, CAN_SJW_1tq, CAN_BS1_13tq, CAN_BS2_2tq};  // 500kbps, (1+13)/(1+13+2)=87.5%

#define CAN_BaudRate CAN_BaudRateConfig500Kbps

/* old schema
#define	CO_OSCILATOR_FREQ (8)   // 8MHz HSE OSC
typedef struct _CAN_BaudRateConfig {
	uint8_t BRP;         //(1...64)Baud Rate Prescaler
	uint8_t SJW;         //(1...4) SJW time
	uint8_t PROP;        //(1...8) PROP time
	uint8_t PhSeg1;      //(1...8) Phase Segment 1 time
	uint8_t PhSeg2;      //(1...8) Phase Segment 2 time
} CAN_BaudRateConfig;

const static CAN_BaudRateConfig CAN_BaudRateTable[8] = {
#if CO_OSCILATOR_FREQ == 4
	{10,  1, 8, 8, 3}, //CAN=10kbps
	{5,   1, 8, 8, 3}, //CAN=20kbps
	{2,   1, 8, 8, 3}, //CAN=50kbps
	{1,   1, 5, 8, 2}, //CAN=125kbps
	{1,   1, 2, 4, 1}, //CAN=250kbps
	{1,   1, 1, 1, 1}, //CAN=500kbps
	{1,   1, 1, 1, 1}, //CAN=800kbps     //Not possible
	{1,   1, 1, 1, 1}  //CAN=1000kbps    //Not possible
#elif CO_OSCILATOR_FREQ == 8
	{25,  1, 5, 8, 2}, //CAN=10kbps
	{10,  1, 8, 8, 3}, //CAN=20kbps
	{5,   1, 5, 8, 2}, //CAN=50kbps
	{2,   1, 5, 8, 2}, //CAN=125kbps
	{1,   1, 5, 8, 2}, //CAN=250kbps
	{1,   1, 2, 4, 1}, //CAN=500kbps
	{1,   1, 1, 2, 1}, //CAN=800kbps
	{1,   1, 1, 1, 1}  //CAN=1000kbps
#elif CO_OSCILATOR_FREQ == 16
	{50,  1, 5, 8, 2}, //CAN=10kbps
	{25,  1, 5, 8, 2}, //CAN=20kbps
	{10,  1, 5, 8, 2}, //CAN=50kbps
	{4,   1, 5, 8, 2}, //CAN=125kbps
	{2,   1, 5, 8, 2}, //CAN=250kbps
	{1,   1, 5, 8, 2}, //CAN=500kbps
	{1,   1, 3, 4, 2}, //CAN=800kbps
	{1,   1, 2, 3, 2}  //CAN=1000kbps
#elif CO_OSCILATOR_FREQ == 20
	{50,  1, 8, 8, 3}, //CAN=10kbps
	{25,  1, 8, 8, 3}, //CAN=20kbps
	{10,  1, 8, 8, 3}, //CAN=50kbps
	{5,   1, 5, 8, 2}, //CAN=125kbps
	{2,   1, 8, 8, 3}, //CAN=250kbps
	{1,   1, 8, 8, 3}, //CAN=500kbps
	{1,   1, 8, 8, 3}, //CAN=800kbps     //Not possible
	{1,   1, 3, 4, 2}  //CAN=1000kbps
#elif CO_OSCILATOR_FREQ == 24
	{63,  1, 8, 8, 2}, //CAN=10kbps
	{40,  1, 4, 8, 2}, //CAN=20kbps
	{15,  1, 5, 8, 2}, //CAN=50kbps
	{6,   1, 5, 8, 2}, //CAN=125kbps
	{3,   1, 5, 8, 2}, //CAN=250kbps
	{2,   1, 3, 6, 2}, //CAN=500kbps
	{1,   1, 5, 7, 2}, //CAN=800kbps
	{1,   1, 3, 6, 2}  //CAN=1000kbps
#elif CO_OSCILATOR_FREQ == 32
	{64,  1, 8, 8, 8}, //CAN=10kbps
	{50,  1, 5, 8, 2}, //CAN=20kbps
	{20,  1, 5, 8, 2}, //CAN=50kbps
	{8,   1, 5, 8, 2}, //CAN=125kbps
	{4,   1, 5, 8, 2}, //CAN=250kbps
	{2,   1, 5, 8, 2}, //CAN=500kbps
	{2,   1, 3, 4, 2}, //CAN=800kbps
	{2,   1, 2, 3, 2}  //CAN=1000kbps
#elif CO_OSCILATOR_FREQ == 40
	{50,  1, 8, 8, 3}, //CAN=10kbps      //Not possible
	{50,  1, 8, 8, 3}, //CAN=20kbps
	{25,  1, 5, 8, 2}, //CAN=50kbps
	{10,  1, 5, 8, 2}, //CAN=125kbps
	{5,   1, 5, 8, 2}, //CAN=250kbps
	{2,   1, 8, 8, 3}, //CAN=500kbps
	{1,   1, 8, 8, 8}, //CAN=800kbps
	{2,   1, 2, 5, 2}  //CAN=1000kbps
#else
#  error define_CO_OSCILATOR_FREQ CO_OSCILATOR_FREQ not supported
#endif
};
*/

void DQBoard_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	int i;

	// section 1. about the onboard LEDs: PA1, PA4, PB0, PB1
	// ------------------------------------------------------
	//PA
	GPIO_InitStruct.GPIO_Pin =GPIO_Pin_1 | GPIO_Pin_4;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;	// because the onboard leds are enabled by vH, so take Push-Pull mode
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	//PB
	GPIO_InitStruct.GPIO_Pin =GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;  // because the onboard leds are enabled by vH, so take Push-Pull mode
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	// section 2. about the onboard beep that connects to C6
	// ------------------------------------------------------
	// GPIOC Configuration: TIM8 Channel 1 output, see PWM_Beep sample for PWM
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP; // Push-Pull mode for PWM
	GPIO_Init(GPIOC, &GPIO_InitStruct);	

	// section 2. about the onboard RS485<->USART3(B10/11, DE-A8)
	// ------------------------------------------------------
	// Configure USART3 Tx  as alternate function push-pull
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;	//��ת�ٶ�Ϊ50M
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;	//�˿�ģʽΪ�������������ʽ
	GPIO_Init(GPIOB, &GPIO_InitStruct);				//�����ϼ���������ʼ��PB��

	// Configure USART3 Rx as input floating				       
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_11;			  //IO�˿ڵĵ�11λ
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;	  //�˿�ģʽΪ�������뷽ʽ
	GPIO_Init(GPIOB, &GPIO_InitStruct);					  //�����ϼ���������ʼ��PB��

	// the pin DE
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_8;			  //IO�˿ڵĵ�8λ
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;		  //��ת�ٶ�Ϊ50M
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;		  //�˿�ģʽΪ���������ʽ
	GPIO_Init(GPIOA, &GPIO_InitStruct);					  //�����ϼ���������ʼ��PA��

	USART_open(&RS485, 96); // good to take no faster than 19200 at 485 messaging
	DQBoard_485sendMode(0);

	// 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); // enable the clock on USART3

	// section 3. about the onboard RS232<->USART1(A9/10)
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

	USART_open(&RS232, 1152);
	// TODO: USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // enable the USART receive interrupt when receive data register is not empty

	// section 5. about the onboard CAN bus remapped to B8/9
	// ------------------------------------------------------
	// CAN_RX=B8, ��������; CAN_TX=B9, �����������
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_8;	
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_PinRemapConfig(GPIO_Remap1_CAN1, ENABLE);  //��CAN�˿��ض��嵽PB8��PB9����ʹ��Ĭ�϶˿ڣ�

	CAN_Configuration();

	//TODO section 6. about the onboard TF card, C8/9/10/11/12, cmd=D2
	// ------------------------------------------------------
	/*	// 6.1 GPIO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// 6.2 RCC clock to SDIO
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_SDIO, ENABLE);

	SDIO_DeInit();
	*/

	//TODO section 7. about the onboard keyboard

	// turn off the LEDs
	for (i=0; i <MAX_OnBoard_LED ; i++)
		DQBoard_setLed(i, 0);

	DQBoard_Timer_config();
}

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
  CAN_StructInit(&CAN_InitStruct);				  //���Ĵ���ȫ�����ó�Ĭ��ֵ

  CAN_InitStruct.CAN_TTCM=DISABLE;			   //��ֹʱ�䴥��ͨ�ŷ�ʽ
  CAN_InitStruct.CAN_ABOM=DISABLE;			   //��ֹCAN�����Զ��رչ���
  CAN_InitStruct.CAN_AWUM=DISABLE;			   //��ֹ�Զ�����ģʽ
  CAN_InitStruct.CAN_NART=DISABLE;			   //��ֹ���Զ��ش�ģʽ
  CAN_InitStruct.CAN_RFLM=DISABLE;			   //��ֹ����FIFO����
  CAN_InitStruct.CAN_TXFP=DISABLE;			   //��ֹ����FIFO���ȼ�
#ifdef CAN_LOOPBACK
	CAN_InitStruct.CAN_Mode=CAN_Mode_Loopback;
#else
	CAN_InitStruct.CAN_Mode=CAN_Mode_Normal;		  //����CAN������ʽΪ�����շ�ģʽ
#endif

/*
   CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;		   //��������ͬ����ת��ʱ������
  CAN_InitStructure.CAN_BS1=CAN_BS1_8tq;		   //�����ֶ�1��ʱ��������
  CAN_InitStructure.CAN_BS2=CAN_BS2_7tq;		   //�����ֶ�2��ʱ��������
  CAN_InitStructure.CAN_Prescaler=1;			   //����ʱ�����ӳ���Ϊ5����
*/
	CAN_InitStruct.CAN_SJW       =baudConf->SJW;			  //��������ͬ����ת��ʱ������
	CAN_InitStruct.CAN_BS1       =baudConf->BS1;			  //�����ֶ�1��ʱ��������
	CAN_InitStruct.CAN_BS2       =baudConf->BS2;			  //�����ֶ�2��ʱ��������
	CAN_InitStruct.CAN_Prescaler =baudConf->Prescaler;				  //����ʱ�����ӳ���Ϊ1����
  CAN_Init(CAN1, &CAN_InitStruct);			   //�����ϲ�����ʼ��CAN1�˿�

  return baud;
 }

void CAN_Configuration(void)
{
	CAN_FilterInitTypeDef  CAN_FilterInitStruct;

	// CAN register init
	CAN_DeInit(CAN1);									  //��λCAN1�����мĴ���

	CAN_setBaud(CanBaud_500Kbps);

	// CAN filter init
	CAN_FilterInitStruct.CAN_FilterNumber         =0;						//ѡ��CAN������0
	CAN_FilterInitStruct.CAN_FilterMode           =CAN_FilterMode_IdMask;	//��ʼ��Ϊ��ʶ/����ģʽ
	CAN_FilterInitStruct.CAN_FilterScale          =CAN_FilterScale_32bit;	//ѡ�������Ϊ32λ
	CAN_FilterInitStruct.CAN_FilterIdHigh         =0x0000;					//��������ʶ�Ÿ�16λ
	CAN_FilterInitStruct.CAN_FilterIdLow          =0x0000;					//��������ʶ�ŵ�16λ
	CAN_FilterInitStruct.CAN_FilterMaskIdHigh     =0x0000;				    //����ģʽѡ���������ʶ�Ż����κŵĸ�16λ
	CAN_FilterInitStruct.CAN_FilterMaskIdLow      =0x0000;				    //����ģʽѡ���������ʶ�Ż����κŵĵ�16λ
	CAN_FilterInitStruct.CAN_FilterFIFOAssignment =CAN_FIFO0;		        //��FIFO 0�����������0
	CAN_FilterInitStruct.CAN_FilterActivation     =ENABLE;				    //ʹ�ܹ�����
	CAN_FilterInit(&CAN_FilterInitStruct);
}

// =========================================================================
// On board interrupts
// =========================================================================
void DQBoard_NVIC_RegisterISRs(void)
{
}

// =========================================================================
// fputc(): portal of printf() to USART1
// =========================================================================
// #define DBG_TO_485
int fputc(int ch, FILE *f)
{
	if (f == f485)
	{
		DQBoard_485sendMode(1);
		// forward the printf() to USART3
		USART_SendData(USART3, (uint8_t) ch);
		while (!(USART3->SR & USART_FLAG_TXE)); // wait till sending finishes
		DQBoard_485sendMode(0);
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
	TimeBaseStruct.TIM_Prescaler         = 0;				   //Ԥ��Ƶ��Ϊ0,����Ƶ
	TimeBaseStruct.TIM_CounterMode       = TIM_CounterMode_Up; //������ʽΪ˳�����ģʽ��������
	TimeBaseStruct.TIM_Period            = reload;             //���ü��������������س�ֵ
	TimeBaseStruct.TIM_ClockDivision     = 0x00;               //����ʱ�ӷָ�ֵ
	TimeBaseStruct.TIM_RepetitionCounter = 0x0;                //ѭ����������ֵ

	TIM_TimeBaseInit(TIMx,&TimeBaseStruct); // initialize TIM

	// Channel 1 Configuration in PWM mode
	TIM_OCInitStruct.TIM_OCMode       = TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState  = TIM_OutputState_Enable;  //ʹ������Ƚ�״̬
	TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Enable; //ʹ�ܶ�ʱ���������               
	TIM_OCInitStruct.TIM_Pulse        = 5000;                    //��������
	TIM_OCInitStruct.TIM_OCPolarity   = TIM_OCPolarity_Low;      //����Ƚϼ���Ϊ��
	TIM_OCInitStruct.TIM_OCIdleState  = TIM_OCIdleState_Set;	 //�򿪿���״̬ѡ��ر�

	TIM_OC1Init(TIMx, &TIM_OCInitStruct);  //initialize the channel of TIM

	TIM_Cmd(TIMx, ENABLE); // TIM8 counter enable
	TIM_CtrlPWMOutputs(TIMx, ENABLE);  // enable PWM
}  

void DQBoard_beep(uint16_t reload, uint16_t msec) // i.e. DQBoard_beep(mid_1) make the board sing "Do"
{ enablePWM(TIM8, reload); delayXmsec(msec); enablePWM(TIM8, 0); }

void DQBoard_setLed(uint8_t id, uint8_t on) // i.e. setLed(0~3) to turn on led 0~3, setLed(0x80|(0~3)) to turn off the led
{ GPIO_WriteBit(OnBoardLEDs[id % MAX_OnBoard_LED].port, OnBoardLEDs[id % MAX_OnBoard_LED].pin, (BitAction)(on?1:0)); }

void DQBoard_485sendMode(uint8_t sendMode)
{ GPIO_WriteBit(GPIOA, GPIO_Pin_8, (BitAction)(sendMode?1:0)); }

// =========================================================================
// RTC_Init: onboard clock with battery supply
// =========================================================================
void RTC_Init(void)
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
		DQBoard_updateClock(60*60*1); DQBoard_updateClock(60*60*2);

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

uint32_t DQBoard_getTime(struct tm* pTm)
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

uint32_t DQBoard_updateClock(uint32_t newTime)
{
	uint32_t itime = DQBoard_getTime(NULL);
	if (newTime<=0 || abs(newTime - itime) < 10)
		return itime;

	newTime &= 0x07ffffff;
	RTC_WaitForLastTask();  // wait until last write operation on RTC registers has finished
	RTC_SetCounter(newTime); // change the current time 
	RTC_WaitForLastTask();  // wait until last write operation on RTC registers has finished

	return newTime;
}

void DQBoard_reset(void)
{
	uint8_t i, j;

	for (j=0; j <3; j++)
	{
		for (i=0; i <MAX_OnBoard_LED ; i++)
			DQBoard_setLed(i, 1);
		delayXmsec(300);

		for (i=0; i <MAX_OnBoard_LED ; i++)
			DQBoard_setLed(i, 0);
		delayXmsec(300);
	}

	for (i=0; i <MAX_OnBoard_LED ; i++)
		DQBoard_setLed(i, 1);
	delayXmsec(1000);

	for (i=0; i <MAX_OnBoard_LED ; i++)
		DQBoard_setLed(i, 1);

	NVIC_SystemReset();
}

