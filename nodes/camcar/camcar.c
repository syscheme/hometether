#include "config.h"
#include "USART1.h"
#include "delay.h"

#include <stdio.h>
#include <string.h>

#define TIMER_RELOAD_Temperature  (60*2) // 2 min
#define TIMER_RELOAD_CurrentI     (60*2) // 2 min
#define INTV_PulseFrame	          (200) // 200usec

#define PULSE_CH_FRAME_MAXLEN 32

#define	Timer0_Reload	(65536UL -(MAIN_Fosc / 1000))		//Timer 0 中断频率, 1000次/秒

// -------------------------------------------------------------------------------
// MCU pin connections
// -------------------------------------------------------------------------------
#define FLAG(BIT)  (1<<BIT)
#define MASK(BIT)   FLAG(BIT)

// #define DIP16
#ifndef DIP16
// 408AD-DIP28
sbit PIN_TurnMotor_A    	= P2^5;  // PIN28->L293D-PIN15
sbit PIN_TurnMotor_B   		= P2^4;  // PIN27->L293D-PIN02
sbit PIN_PrimaryMotor_A 	= P2^3;  // PIN26->L293D-PIN10
sbit PIN_PrimaryMotor_B    	= P2^2;  // PIN25->L293D-PIN07
sbit PIN_LED_Signal         = P2^1;  // IAP15W205S-DIP16-PIN07, 408AD-PIN13

sbit PIN_Sonar_Trigger      = P2^6;  // PIN01
sbit PIN_Sonar_Echo         = P2^7;  // PIN02

sbit PIN_EXT0           	= P1^0;  // PIN03
sbit PIN_EXT1           	= P1^1;  // PIN04
sbit PIN_EXT2           	= P1^2;  // PIN05
sbit PIN_EXT3           	= P1^3;  // PIN06
sbit PIN_EXT4           	= P1^4;  // PIN07
sbit PIN_EXT5           	= P1^5;  // PIN08
sbit PIN_EXT6           	= P1^6;  // PIN09
sbit PIN_EXT7           	= P1^7;  // PIN10

#else
// DIP16
sbit PIN_TurnMotor_A    	= P1^2;  // IAP15W205S-DIP16-PIN01, 408AD-PIN05
sbit PIN_TurnMotor_B   		= P1^3;  // IAP15W205S-DIP16-PIN02, 408AD-PIN06
sbit PIN_PrimaryMotor_A 	= P1^4;  // IAP15W205S-DIP16-PIN03, 408AD-PIN07
sbit PIN_PrimaryMotor_B    	= P1^5;  // IAP15W205S-DIP16-PIN04, 408AD-PIN08

sbit PIN_Sonar_Trigger      = P1^0;  // IAP15W205S-DIP16-PIN15, 408AD-PIN03
sbit PIN_Sonar_Echo         = P1^1;  // IAP15W205S-DIP16-PIN16, 408AD-PIN04

sbit PIN_LED_Signal         = P5^5;  // IAP15W205S-DIP16-PIN07, 408AD-PIN13

sbit PIN_EXT0           	= P3^2;  // IAP15W205S-DIP16-PIN11, 408AD-PIN17
sbit PIN_EXT1           	= P3^3;  // IAP15W205S-DIP16-PIN12, 408AD-PIN18
sbit PIN_EXT2           	= P3^6;  // IAP15W205S-DIP16-PIN13, 408AD-PIN21
sbit PIN_EXT3           	= P3^7;  // IAP15W205S-DIP16-PIN14, 408AD-PIN22
#endif // IAP15W205S

uint8 timerPrimaryMotor =0, timerTurnMotor =0; 	// in 0.1msec

void move(uint8 durMsec, bool backward)
{
	if (durMsec<=0)
	{
		timerPrimaryMotor = 0;
		PrimaryMotor_reset();
		return;
	}

	if (backward)
	{
		PIN_PrimaryMotor_A =1;
		PIN_PrimaryMotor_B =0;
	}
	else
	{
		PIN_PrimaryMotor_A =0;
		PIN_PrimaryMotor_B =1;
	}

	timerPrimaryMotor = durMsec;
}

#define PrimaryMotor_reset() { PIN_PrimaryMotor_A =0; PIN_PrimaryMotor_B =0; }
#define TurnMotor_reset()    { PIN_TurnMotor_A =0; PIN_TurnMotor_B =0; }
#define IS_FORWARD     ( 0==PIN_PrimaryMotor_A && 1==PIN_PrimaryMotor_B)

void turn(uint8 durMsec, bool right)
{
	if (durMsec<=0)
	{
		timerTurnMotor = 0;
		TurnMotor_reset();
		return;
	}

	if (right)
	{
		PIN_TurnMotor_A =1;
		PIN_TurnMotor_B =0;
	}
	else
	{
		PIN_TurnMotor_A =0;
		PIN_TurnMotor_B =1;
	}

	timerTurnMotor = durMsec;
}

// -------------------------------------------------------------------------------
// Declarations
// -------------------------------------------------------------------------------
// the timer counters
uint16 timer_msec =0, timer_sec =0;
uint8  recvdCmd[20];

// about the blinking led
uint8 blinkTick=3, blinkReload =10;
#define ledOn(_ON)  { PIN_LED_Signal = (_ON) ? 1:0; }

// about the variables to sample temperature
int16 distance, CONFIG_yieldDistance =0;

int16 Sonar_readDist();

// -------------------------------------------------------------------------------
// UART_config
// -------------------------------------------------------------------------------
void UART_config(void)
{
	COMx_InitDefine	COMx_InitStructure;					//结构定义
	COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;		//模式,       UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	COMx_InitStructure.UART_BRT_Use   = BRT_Timer2;			//使用波特率,   BRT_Timer1, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
	COMx_InitStructure.UART_BaudRate  = 9600ul; //115200ul;			//波特率, 一般 110 ~ 115200
	COMx_InitStructure.UART_RxEnable  = ENABLE;				//接收允许,   ENABLE或DISABLE
	COMx_InitStructure.BaudRateDouble = DISABLE;			//波特率加倍, ENABLE或DISABLE
	COMx_InitStructure.UART_Interrupt = ENABLE;				//中断允许,   ENABLE或DISABLE
	COMx_InitStructure.UART_Polity    = PolityLow;			//中断优先级, PolityLow,PolityHigh
	COMx_InitStructure.UART_P_SW      = 0; // UART1_SW_P16_P17;	//切换端口,   UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17(必须使用内部时钟)
	COMx_InitStructure.UART_RXD_TXD_Short = DISABLE;		//内部短路RXD与TXD, 做中继, ENABLE,DISABLE
	USART_Configuration(USART1, &COMx_InitStructure);		//初始化串口1 USART1,USART2

//	printf("STC15F2K60S2 UART1 Test Prgramme!\r\n");	//SUART1发送一个字符串
}

// -------------------------------------------------------------------------------
// interrupt ISR_timer0()
// -------------------------------------------------------------------------------
// the timer interrupt, once every 1 msec expected
void ISR_timer0() interrupt 1
{
	if (++timer_msec >1000)
	{
		// an approximate sec timer = 1msec *1024 
		timer_msec = 0;
		timer_sec++;
	}


	if (0 == (timer_msec %100)) // every 0.1sec
	{
		// update timerTurnMotor
		if (timerTurnMotor)
			timerTurnMotor--;
		else timerTurnMotor =0;

		// update timerPrimaryMotor
		if (timerPrimaryMotor)
			timerPrimaryMotor--;
		else timerPrimaryMotor =0;

		if (0 == timerTurnMotor)
			TurnMotor_reset();

		if (0 == timerPrimaryMotor)
			PrimaryMotor_reset();

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

void execCmd(const char* cmd)
{
	char c1,c2,c;
	int16 tmp16 = 0;

	//command GET DIST: GD
	if (0 == strncmp(cmd, "GD", 2))
	{
		printf("200 DIST=%u\r\n", distance);
		return;
	}

	if (0 == strncmp(cmd, "MS", 2))
	{
		printf("200 MS=P%c%c#%d", PIN_PrimaryMotor_A?'1':'0', PIN_PrimaryMotor_B?'1':'0', (int)timerPrimaryMotor);
		printf(",T%c%c#%d\r\n", PIN_TurnMotor_A?'1':'0', PIN_TurnMotor_B?'1':'0', (int)timerTurnMotor);
		return;
	}

	//command MOVE: M{F|B}{L|R|D}<msec>
	c= sscanf(cmd, "M%c%c%u", &c1, &c2, &tmp16);
	if (3 == c)
	{
		if (tmp16 >100)
			tmp16 = 100;

		move(tmp16, ('B' == c1) ?1:0);

		if ('L' == c2 || 'R' ==c2)
			turn(tmp16, ('R' == c2) ?1:0);

		printf("200 MS=P%c%c#%d", PIN_PrimaryMotor_A?'1':'0', PIN_PrimaryMotor_B?'1':'0', (int)timerPrimaryMotor);
		printf(",T%c%c#%d\r\n", PIN_TurnMotor_A?'1':'0', PIN_TurnMotor_B?'1':'0', (int)timerTurnMotor);
		return;
	}

	//command SET PortOther: SP=<uint8>
	c= sscanf(cmd, "SP%u", &tmp16);
	if (c >0)
	{
#ifndef DIP16
		P1 = tmp16 &0x0f;
		printf("200 SP=%02x\r\n", P1);
#else
		// remap 0x0f to P3^2,3,6,7
		c2 = tmp16 &0x0f;
		PIN_EXT0 = (c2 & (1<<0))? 1:0;
		PIN_EXT1 = (c2 & (1<<1))? 1:0;
		PIN_EXT2 = (c2 & (1<<2))? 1:0;
		PIN_EXT3 = (c2 & (1<<3))? 1:0;
		P1 = tmp16 &0x0f;
		printf("200 SP=%02x\r\n", tmp16);
#endif
		return;
	}

	//command SET YIELDDIST: YD=<millimeter>
	c= sscanf(cmd, "YD%u", &tmp16);
	if (c>0)
	{
		CONFIG_yieldDistance = tmp16;
		printf("200 YD=%u\r\n", CONFIG_yieldDistance);
		return;
	}
	
	//command BLINK: B%uP%u
	c= sscanf(cmd, "B%uP%u", &c1, &tmp16);
	if (2 == c)
	{
		if (tmp16 >200)
			tmp16 = 200;
		if (c1> tmp16)
			c1 = 1; 
		if (tmp16 <2 || c1<=0)
			printf("416 BadValue\r\n");
			
		blinkTick=c1, blinkReload =tmp16;
		printf("200 BLN=%u/%u\r\n", blinkTick, blinkReload);
		return;
	}

	// printf("400 %s\r\n", cmd);
}

// -------------------------------------------------------------------------------
// main()
// -------------------------------------------------------------------------------
// the main loop
void main(void)
{		   

	uint16 i=0, stampLastDist=0;
//	int16 tmp16 = 0;

	UART_config();

	Timer0_1T();
	Timer0_AsTimer();
	Timer0_16bitAutoReload();
	Timer0_Load(Timer0_Reload);
	Timer0_InterruptEnable();
	Timer0_Run();

	EA = 1;
	
	// initialize actions
//	turn(500, 0); delayXms(800); turn(500, 1); delayXms(800);
//	move(500, 0); delayXms(800); move(500, 1); delayXms(800);
	PrimaryMotor_reset();
	TurnMotor_reset();

	while (0) // the main loop of cam-car
	{
		printf("stamp=%d.%03d\r\n", timer_sec, timer_msec);
		delayXms(250); delayXms(250);
		delayXms(250); delayXms(250);
	}

	while (1) // the main loop of cam-car
	{
		if (COM1.RX_TimeOut >0)
		{
			execCmd(RX1_Buffer);
			COM1.RX_TimeOut =0;
			delayXms(10);
			continue;
		}

		delayXms(100);
//		printf("heatbeat\r\n");

		// step 1. read the dis tance ahead
		distance = Sonar_readDist();

		// step 1.1. stop moving if CONFIG_yieldDistance is set
		if (distance>0 && CONFIG_yieldDistance >10)
		{
			if (distance >0 && distance < CONFIG_yieldDistance && IS_FORWARD && !(PIN_TurnMotor_A || PIN_TurnMotor_B))
				PrimaryMotor_reset();
		}

		if (stampLastDist == timer_sec) // every 1sec
			continue;

		stampLastDist = timer_sec;
	//	if (distance>0)
	//		printf("DIST=%dmm\r\n", distance);

		// step	2. process the command received
	} // end of main loop

}
/*
void cbOnLineReceived(u8 comId, u8* cmd)
{
	printf("got line: %s\r\n", cmd);
//	execCmd(cmd);
}
*/		


/*
// -------------------------------------------------------------------------------
// Ordinal code
// -------------------------------------------------------------------------------
enum OrdinalCode{
	// section of queries 0x00 - 0x01f
	ocReadDist      =0x00,   // read the status parameters
	ocMoveForward,           // read the temperatures
	ocMoveBackward,            // read the port bytes of P0, P1, P2, P3
	ocTurnLeft,
	ocTurnRight,
	ocLedOn,

	// section of configurations  0x30 - 0x3f
	ocConfigDist =0x30, // set the CONFIG_TimeoutPump, CONFIG_TimeoutValve2to1, CONFIG_TimeoutYield, CONFIG_TimeoutIdle
	ocReadP2,
	ocSetP2,
	// end of OrdinalCode
	ocMax
};

// -------------------------------------------------------------------------------
// serial port ISR_uart()
// -------------------------------------------------------------------------------
// the interrupt of serial port communication
void ISR_uart(void) interrupt 4  
{
	uint8 tmp, oc;
	uint16 w;
	if (0 == RI)
		return;

	ET0 =0;
	ledOn(1);

	// RI  =0;  tmp = SBUF; //
	oc = UART_readbyte();

	switch (oc)
	{
	case ocReadDist: // read the state parameters
		UART_writebyte(oc);
		UART_writeword(distance);
		break;

	case ocReadP2: // read the status of P2
		UART_writebyte(oc);
		UART_writebyte(P2);
		break;

	case ocSetP2: // set the status of P2
		P2 = UART_readbyte();
		UART_writebyte(oc);
		UART_writebyte(P2);
		break;

	case ocMoveForward: // move forward
		tmp = UART_readbyte();
		move(tmp, 0);
		UART_writebyte(oc);
		UART_writebyte(tmp);
		break;

	case ocMoveBackward: // move backward
		tmp = UART_readbyte();
		move(tmp, 1);
		UART_writebyte(oc);
		UART_writebyte(tmp);
		break;

	case ocTurnLeft:
		tmp = UART_readbyte();
		turn(tmp, 0);
		UART_writebyte(oc);
		UART_writebyte(tmp);
		break;

	case ocTurnRight:
		tmp = UART_readbyte();
		turn(tmp, 1);
		UART_writebyte(oc);
		UART_writebyte(tmp);
		break;

	case ocConfigDist: // change the configuration of yield distance
		w = UART_readword();
		if (w < 500)
			CONFIG_yieldDistance = w;
		UART_writebyte(oc);
		UART_writeword(CONFIG_yieldDistance);
		break;
	}

	ledOn(0);
	ET0 =1;
}
// */

// get the distance in milli-meter
int16 Sonar_readDist()
{
	u16 tmp16 = 0, stampStart, t16;
	
	PIN_Sonar_Trigger=0;

	stampStart= timer_msec;
	PIN_Sonar_Trigger=1;
	delayX10us(2); 
	PIN_Sonar_Trigger=0;

	while(0 == PIN_Sonar_Echo) //等待Echo回波引脚变高电平
	{
		if ((timer_msec - stampStart) >100)
			return -1;
	}

	stampStart= timer_msec;
	t16 = (((uint16)TH0) <<8) | TL0;

	while(1 == PIN_Sonar_Echo)
	{
		if ((timer_msec - stampStart) >50)
			return -1;
	}

	t16 =((((uint16)TH0) <<8) | TL0) - t16;
	tmp16 = timer_msec - stampStart;
	if (tmp16 >1000)
		return -1;

	if (t16 & 0x8000)
	{
		t16 += 0xffff;
		tmp16--;
	}

	t16 /= (MAIN_Fosc / 1000 /10); //0.1msec
	t16 %=10;
	// printf("t16=%d tmp16=%d\r\n", t16, tmp16);	// TX1_write2buff(theTemperature&0xff);//SUART1发送一个字符串
	t16 += tmp16*10;
	tmp16 = t16 * (344/2/10); // mm
	
	return tmp16;
}
// */