#include "bsp.h"

#define TIMER_RELOAD_Temperature  (60*2) // 2 min
#define TIMER_RELOAD_CurrentI     (60*2) // 2 min
#define INTV_PulseFrame	          (200) // 200usec

#define PULSE_CH_FRAME_MAXLEN 32

// -------------------------------------------------------------------------------
// MCU pin connections
// -------------------------------------------------------------------------------
#define PINNO_HEATER_CURRENT_I  (0)
#define PINNO_MCU_VOLT          (7)
sbit PIN_HEATER_CURRENT_I = P1^PINNO_HEATER_CURRENT_I;   // the ADC to test the current of heater power usage, pin3
sbit PIN_MCU_VOLT         = P1^PINNO_MCU_VOLT;   // the ADC to test the volts of MCU against 5v, pin10

#define FLAG(BIT)  (1<<BIT)
#define MASK(BIT)   FLAG(BIT)

sbit PIN_DS18B20 	      = P2^5;   // pin28, the io connects to DS18B20
sbit PIN_LED     	      = P2^4;   // pin27, connects to LED
sbit PIN_ASK_SENDER       = P2^3;   // pin26, connects to ASK sender 

// -------------------------------------------------------------------------------
// Declarations
// -------------------------------------------------------------------------------
// the timer counters
uint16 timerAdvCurrentI =0,  timerAdvTemperature =0;
uint16 timer_msec =0, timer_sec =0;

// about the blinking led
uint8 blinkTick=0, blinkReload =0;
#define ledOn(_ON)  { PIN_LED = (_ON) ? 1:0; }

// about the variables to sample temperature
int16 theTemperature =0;
uint16 inVolt =0;
uint16 prevCurrent=0, newCurrent=0;

uint8   byteStatus =0; // consist of the following flags
#define FLAG_DEBUG        (1<<7)
#define FLAG_CurrentSampl (1<<0)

//
int16 readDS18B20(); // unit = 0.1 degree
void initADC(void);
uint8 readADC(uint8 ch);

// -------------------------------------------------------------------------------
// interrupt ISR_timer0()
// -------------------------------------------------------------------------------
// the timer interrupt, once every 1 msec expected
void ISR_timer0() interrupt 1
{
	Timer0_reload();

	if (!(--timer_msec & 0x3ff))
	{
		// an approximate sec timer = 1msec *1024 
		timer_msec = 0x3ff;
		timer_sec--;
		
		// update timerAdvCurrentI
		if (timerAdvCurrentI)
			timerAdvCurrentI--;

		// update timerAdvTemperature
		if (timerAdvTemperature)
			timerAdvTemperature--;
	}

	if (!(timer_msec & 0x07f)) // approximately every 0.1sec
	{
		// approximately every 0.1sec
		if (!(blinkReload & 0x7f))  { ledOn(0); }
		else if (!(blinkTick-- & 0x7f))
		{
			blinkTick = (blinkReload & 0x7f) ^ (blinkTick & 0x80);
			if (blinkTick & 0x80)
			{
				if (blinkTick >0x8a)
					blinkTick = 0x8a;

				ledOn(1);
			}
			else ledOn(0);
		}
	}
}

void writeDS18B20byte(uint8 value)   
{
	uint8 j;

	for(j=0; j<8; j++)
	{
		if (value & 0x01)
		{ // write the bit=1
			PIN_DS18B20 = 0; // ANALOG_SIG=0;
			delayIOSet(); // ensure L remain greater than 1us but must reset to H before 15us
			PIN_DS18B20 = 1; // ANALOG_SIG=1;
			delayX10us(5);
		}
		else
		{
			// write the bit=0;
			PIN_DS18B20 = 0; // ANALOG_SIG=0;
			delayX10us(5); // ensure L remain greater than 60usec
			PIN_DS18B20 = 1; // ANALOG_SIG=1;
			delayIOSet();
		}

		value >>= 1;
	}
}

uint8 readDS18B20byte(void)
{
	uint8 i, value=0;

	for(i=0; i<8; i++)
	{
		value >>= 1;

		PIN_DS18B20 = 0; // ANALOG_SIG=1;
		delayIOSet(); // delay a bit to ensure the L remains greater than 1us
		PIN_DS18B20 = 1; // ANALOG_SIG=1;
		delayIOSet(); delayIOSet();
		// delayX10us(1); // the DS18B20 becomes valid after 15usec since the L

		value |= PIN_DS18B20 ? 0x80 : 0x00; // j= ANALOG_SIG; value |= (j<<7);

		delayX10us(5); // yield for the next bit
		PIN_DS18B20 =1;   delayIOSet();  // ANALOG_SIG=1;
	}

	return value;
}

void resetDS18B20(void)
{
	int i =0;

	for (i =1000; i && (PIN_DS18B20); i--)
		delayIOSet();
	delayX10us(50);
	PIN_DS18B20 =1;
}

int16 readDS18B20() // unit = 0.1 degree
{
	uint8 a =0x00, b=0x00;
	int16 temp;

	EA = 0; // disable all interrupts to avoid errors
	ledOn(1);

	// step 1. reset DS18B20
	resetDS18B20();

	// step 2. send the byte 0xcc and 0xbe
	writeDS18B20byte(0xcc);
	writeDS18B20byte(0xbe);

	// step 3 read the value
	a = readDS18B20byte(); // the low byte
	b = readDS18B20byte(); // the high byte

	// step 3.1 kick off DS18B20 convert by the way for next read
	resetDS18B20();
	writeDS18B20byte(0xcc);
	writeDS18B20byte(0x44);

	EA = 1; // enable the interrupts
	ledOn(0);

	// step 4. calcuate the true temperature value
	// step 4.1 comb into a 16bit integter
	temp = b;
	temp <<=8;
	temp |= a;

	// step 4.2 every 1 means 1/16 degree in C, convert it to dec via temp = Round(temp/16 *10 +0.5) = (temp*10 +8)/16
	temp = temp *10 +8; temp >>=4;
	if (b &0x80) // correct the sign
		temp |= 0xf000;

	// step 5. return the result
	return temp; // the return value is in 0.1 degree, for example, 238 mean 23.8C
}

uint8 message[10];
code const uint8 idata* idata_ptr = 0xf1; // the address to the 7bytes CpuId,
void sendMessage(const uint8* message, uint8 len); 

// -------------------------------------------------------------------------------
// main()
// -------------------------------------------------------------------------------
// the main loop
void main(void)
{
	uint8 i=0, headerLen=0;
	int16 tmp16 = 0;

	Timer0_init(1); // initialize timer0 w/ interval=1msec
	UART_init(96);  // initialize uart1 with 8n1,9600bps

	theTemperature = readDS18B20();

	message[headerLen++] = 'P'; // cmd = POST
	// read the CpuId and compress them into 4bytes
	message[headerLen++] = idata_ptr[0] ^ idata_ptr[3];
	message[headerLen++] = idata_ptr[1] ^ idata_ptr[4];
	message[headerLen++] = idata_ptr[2] ^ idata_ptr[5];
	message[headerLen++] = idata_ptr[2] ^ idata_ptr[5];

	while (1) // the heater pwr main loop
	{
		// step 1. read the current every 1 sec
		if (0 == (timer_sec & 0x01))
		{
			if (0 == (byteStatus & FLAG_CurrentSampl))
			{
				byteStatus |= FLAG_CurrentSampl;
				
				// take the average from 16 ADC reads
				for(i=0; i<16; i++)
					tmp16 += readADC(0);
				newCurrent = tmp16 >>4;

				for(i=0; i<16; i++)
					tmp16 += readADC(7);
				inVolt = tmp16 >>4;

				// step 1.1 test if the recent current change exceeds 25%
				tmp16 = (newCurrent > prevCurrent) ? (newCurrent - prevCurrent) : (prevCurrent - newCurrent);
				if (tmp16 > (prevCurrent>>2))
				{
					prevCurrent = newCurrent;
					timerAdvCurrentI =0;
				}
			}
		} else byteStatus &= ~FLAG_CurrentSampl;  // clear the flag

		if (0 == timerAdvTemperature)
		{
			// read the temperature twice and send the result
			theTemperature = readDS18B20();
			theTemperature = readDS18B20();
			message[headerLen] = 'T';
			(*(int16*) &message[headerLen+1]) = theTemperature;
			sendMessage(message, headerLen + 1 + sizeof(int16));

			// reload the timerTemperature
			timerAdvTemperature = TIMER_RELOAD_Temperature;
		}

		if (0 == timerAdvCurrentI)
		{
			message[headerLen] = 'I';
			(*(uint16*) &message[headerLen+1]) = newCurrent;
			sendMessage(message, headerLen + 1 + sizeof(uint16));

			// reload the timerCurrentI
			timerAdvCurrentI = TIMER_RELOAD_CurrentI;
		}
	}
// */
}

// -------------------------------------------------------------------------------
// initialize ADC
// -------------------------------------------------------------------------------
#define ADC_POWER    (0x80)
#define ADC_FLAG	(1<<4)	//软件清0
#define ADC_START	(1<<3)	//自动清0

#define ADC_SPEED_MASK    (3<<5)
#define ADC_SPEED_90T	  (3<<5)
#define ADC_SPEED_180T	  (2<<5)
#define ADC_SPEED_360T	  (1<<5)
#define ADC_SPEED          ADC_SPEED_360T

void initADC()
{
	P1M1 |= MASK(PINNO_HEATER_CURRENT_I) | MASK(PINNO_MCU_VOLT);
	P1M0 &= ~(MASK(PINNO_HEATER_CURRENT_I) | MASK(PINNO_MCU_VOLT));
	P1ASF = MASK(PINNO_HEATER_CURRENT_I) | MASK(PINNO_MCU_VOLT); //设置 P1 口为 AD 口
	ADC_RES = 0; //清除结果寄存器
	ADC_CONTR = ADC_POWER | ADC_SPEED | ADC_START | PINNO_HEATER_CURRENT_I;
	delayIOSet(); delayIOSet(); //ADC 上电并延时
}

// -------------------------------------------------------------------------------
// read ADC
// -------------------------------------------------------------------------------
uint8 readADC(uint8 ch)
{
//	uint16 adc = 0;
	ch &= 0x07;
	ADC_CONTR = ADC_POWER | ADC_SPEED | ADC_START | ch;
	_nop_(); // wait for 4 nop()s
	_nop_();
	_nop_();
	_nop_();
	while (!(ADC_CONTR & ADC_FLAG)); // wait for ADC conversion gets done
	ADC_CONTR &= ~ADC_FLAG; // close ADC
//	adc = ADC_RES; adc<<=2;
//	adc |= ADC_RESL & 0x3;
	return ADC_RES; // return ADC result
}
// */

///////////
#define	ADC_P10		(1<<0)	//IO引脚 Px.0
#define	ADC_P11		(1<<1)	//IO引脚 Px.1
#define	ADC_P12		(1<<2)	//IO引脚 Px.2
#define	ADC_P13		(1<<3)	//IO引脚 Px.3
#define	ADC_P14		(1<<4)	//IO引脚 Px.4
#define	ADC_P15		(1<<5)	//IO引脚 Px.5
#define	ADC_P16		(1<<6)	//IO引脚 Px.6
#define	ADC_P17		(1<<7)	//IO引脚 Px.7
#define	ADC_P1_All	0xFF	//IO所有引脚

#define ADC_90T		(3<<5)
#define ADC_180T	(2<<5)
#define ADC_360T	(1<<5)
#define ADC_540T	0
#define ADC_FLAG	(1<<4)	//软件清0
#define ADC_START	(1<<3)	//自动清0
#define ADC_CH0		0
#define ADC_CH1		1
#define ADC_CH2		2
#define ADC_CH3		3
#define ADC_CH4		4
#define ADC_CH5		5
#define ADC_CH6		6
#define ADC_CH7		7

#define ADC_RES_H2L8	1
#define ADC_RES_H8L2	0

typedef struct
{
	uint8	ADC_Px;			//设置要做ADC的IO,	ADC_P10 ~ ADC_P17,ADC_P1_All
	uint8	ADC_Speed;		//ADC速度			ADC_90T,ADC_180T,ADC_360T,ADC_540T
	uint8	ADC_Power;		//ADC功率允许/关闭	ENABLE,DISABLE
	uint8	ADC_AdjResult;	//ADC结果调整,	ADC_RES_H2L8,ADC_RES_H8L2
	uint8	ADC_Priority;		//优先级设置	PolityHigh,PolityLow
	uint8	ADC_Interrupt;	//中断允许		ENABLE,DISABLE
} ADC_InitTypeDef;

void	ADC_init(ADC_InitTypeDef *ADCx);
void	ADC_powerCtrl(uint8 pwr);
uint16  ADC_read10b(uint8 channel);	//channel = 0~7

#define P1n_pure_input(bitn)		P1M1 |=  (bitn),	P1M0 &= ~(bitn)

//========================================================================
// 函数: void	ADC_init(ADC_InitTypeDef *ADCx)
// 描述: ADC初始化程序.
// 参数: ADCx: 结构参数,请参考adc.h里的定义.
// 返回: none.
// 版本: V1.0, 2012-10-22
//========================================================================
void	ADC_init(ADC_InitTypeDef *ADCx)
{
	P1n_pure_input(ADCx->ADC_Px); //把ADC口设置为高阻输入

	P1ASF = ADCx->ADC_Px;
	ADC_CONTR = (ADC_CONTR & ~ADC_90T) | ADCx->ADC_Speed;

	if(ADCx->ADC_Power)
		ADC_CONTR |= 0x80;
	else
		ADC_CONTR &= 0x7F;

//	if (ADCx->ADC_AdjResult == ADC_RES_H2L8)
//		PCON2 |=  (1<<5);	//10位AD结果的高2位放ADC_RES的低2位，低8位在ADC_RESL。
//	else
//		PCON2 &= ~(1<<5);	//10位AD结果的高8位放ADC_RES，低2位在ADC_RESL的低2位。
	
//	EADC = (ADCx->ADC_Interrupt) ? 1:0; //中断允许
	EADC=0;

	PADC = (ADCx->ADC_Priority)	? 1:0;		//优先级设置	PolityHigh,PolityLow
}

//========================================================================
// 函数: void	ADC_PowerControl(uint8 pwr)
// 描述: ADC电源控制程序.
// 参数: pwr: 电源控制,ENABLE或DISABLE.
// 返回: none.
// 版本: V1.0, 2012-10-22
//========================================================================
void	ADC_powerCtrl(uint8 pwr)
{
	if (pwr)
		ADC_CONTR |= 0x80;
	else
		ADC_CONTR &= 0x7f;
}

//========================================================================
// 函数: uint16	Get_ADC10bitResult(uint8 channel)
// 描述: 查询法读一次ADC结果.
// 参数: channel: 选择要转换的ADC.
// 返回: 10位ADC结果.
// 版本: V1.0, 2012-10-22
//========================================================================
uint16	ADC_read10b(uint8 channel)	//channel = 0~7
{
	uint16	adc;
	uint8	i;

	if(channel > ADC_CH7)	return	1024;	//错误,返回1024,调用的程序判断	
	ADC_RES = 0;
	ADC_RESL = 0;

	ADC_CONTR = (ADC_CONTR & 0xe0) | ADC_START | channel; 
	_nop_(); _nop_(); _nop_(); _nop_();// wait for 4 nop()s, 对ADC_CONTR操作后要4T之后才能访问

	for(i=0; i<250; i++)		//超时
	{
		if(ADC_CONTR & ADC_FLAG)
		{
			ADC_CONTR &= ~ADC_FLAG;
//			if(PCON2 &  (1<<5))		//10位AD结果的高2位放ADC_RES的低2位，低8位在ADC_RESL。
//			{
				adc = (uint16)(ADC_RES & 3);
				adc = (adc << 8) | ADC_RESL;
//			}
//			else		//10位AD结果的高8位放ADC_RES，低2位在ADC_RESL的低2位。
//			{
				adc = (uint16)ADC_RES;
				adc = (adc << 2) | (ADC_RESL & 3);
//			}

			return	adc;
		}
	}

	return	0xffff;	//错误,返回1024,调用的程序判断
}


void BSP_config(void)
{
	ADC_InitTypeDef		ADC_InitStructure;				//结构定义
	ADC_InitStructure.ADC_Px        = ADC_P10;			//设置要做ADC的IO,	ADC_P10 ~ ADC_P17(或操作),ADC_P1_All
	ADC_InitStructure.ADC_Speed     = ADC_90T;			//ADC速度			ADC_90T,ADC_180T,ADC_360T,ADC_540T
	ADC_InitStructure.ADC_Power     = 1;			//ADC功率允许/关闭	ENABLE,DISABLE
	ADC_InitStructure.ADC_AdjResult = ADC_RES_H8L2;		//ADC结果调整,	ADC_RES_H2L8,ADC_RES_H8L2
	ADC_InitStructure.ADC_Priority  = 0;		//优先级设置	PolityHigh,PolityLow
	ADC_InitStructure.ADC_Interrupt = 0;			//中断允许		ENABLE,DISABLE
	ADC_init(&ADC_InitStructure);					//初始化
	ADC_powerCtrl(1);							//单独的ADC电源操作函数, ENABLE或DISABLE
}

// -----------------------------
// PulseChannel_sendFrame()
// -----------------------------
typedef void (*SetPulsePin_f) (uint8_t high);
#define PulseFrame_sendPulse(durH, durL)	sender(1); delayX10us(durH/10); sender(0); delayX10us(durL/10) 
#define PulseFrame_sendByte(B, durIntv)		for(i=0, tmp=B; i<8; i++, tmp<<=1) { \
												if (0x80 & tmp) { PulseFrame_sendPulse(durIntv*3, durIntv); } \
												else { PulseFrame_sendPulse(durIntv, durIntv*3); } }

uint8_t PulseChannel_sendFrame(uint8_t* msg, uint8_t len, SetPulsePin_f sender, uint8_t repeatTimes, uint8_t plusWideUsec)
{
	int i =0, j=0;
	uint8_t crc8=0, leadingByte=0, tmp=0;
#ifdef PULSEFRAME_ENCRYPY
	uint8_t idxEncrypt =0;
#endif // PULSEFRAME_ENCRYPY

	if (NULL ==msg || NULL==sender || len <=0)
		return 0;

	if (repeatTimes <=0)
		repeatTimes =1;

	if (len >PULSE_CH_FRAME_MAXLEN)
		len = PULSE_CH_FRAME_MAXLEN;

	if (plusWideUsec<=0)
		plusWideUsec = INTV_PulseFrame;

	// step 1. assemble the leadingByte
	// bit  7    6    5    4    3    2    1    0    
	//     +----+----+----+----+----+----+----+----+
	//     |  1    0    1 |      dataLen           |
	//     +----+----+----+----+----+----+----+----+
	leadingByte = 0xa0 | len;

	// step 2. calculate the crc checksum
	crc8= calcCRC8(msg, len);

#ifdef PULSEFRAME_ENCRYPY
	// step 2.1 encrypt the data
	idxEncrypt = (~leadingByte) ^ crc8;
	for (j=0; j< len; j++)
		msg[j] ^= encrypt_table[idxEncrypt^j];
#endif // PULSEFRAME_ENCRYPY

	// step 3. send the frame
	while (repeatTimes--)
	{
		//step 3.1 the sync signal: H=(128-4)*a, L=4*a
		PulseFrame_sendPulse(plusWideUsec *31, plusWideUsec);

		//step 3.2 the leading byte
		PulseFrame_sendByte(leadingByte, plusWideUsec);
		PulseFrame_sendByte(~leadingByte, plusWideUsec);

		//step 3.3 the frame payload
		for (j=0; j< len; j++)
			PulseFrame_sendByte(msg[j], plusWideUsec);

		//step 3.4 send the CRC byte
		PulseFrame_sendByte(crc8, plusWideUsec);
	}

	// the ending pulse
	PulseFrame_sendPulse(plusWideUsec*20, plusWideUsec*4); 

	return len;
}

void ASKPin_set(uint8_t high)
{
	PIN_ASK_SENDER = high?1:0;
}

void sendMessage(const uint8* message, uint8 len)
{
	PulseChannel_sendFrame(message, len, ASKPin_set, 2, INTV_PulseFrame);

	while (len--)
		UART_writebyte(*message);

	UART_writebyte(0x0a);
	UART_writebyte(0x0d);
}