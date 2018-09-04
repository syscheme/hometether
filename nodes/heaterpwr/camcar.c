#include "bsp.h"
#include <stdio.h>
#include <string.h>

#define TIMER_RELOAD_Temperature  (60*2) // 2 min
#define TIMER_RELOAD_CurrentI     (60*2) // 2 min
#define INTV_PulseFrame	          (200) // 200usec

#define PULSE_CH_FRAME_MAXLEN 32

// -------------------------------------------------------------------------------
// MCU pin connections
// -------------------------------------------------------------------------------
#define FLAG(BIT)  (1<<BIT)
#define MASK(BIT)   FLAG(BIT)

sbit PIN_PrimaryMotor_A 	= P0^0;  
sbit PIN_PrimaryMotor_B    	= P0^1; 
sbit PIN_TurnMotor_A    	= P0^2; 
sbit PIN_TurnMotor_B   		= P0^3;

sbit PIN_LED_Signal         = P0^4;

sbit PIN_Sonar_Trigger      = P0^5;
sbit PIN_Sonar_Echo         = P0^6;

uint8 timerPrimaryMotor =0, timerTurnMotor =0; 

void move(uint8 durMsec, bool backward)
{
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

#define PrimaryMotor_reset() { PIN_PrimaryMotor_A = PIN_PrimaryMotor_B =0; }
#define TurnMotor_reset()    { PIN_TurnMotor_A = PIN_TurnMotor_B =0; }
#define IS_FORWARD     ( 0==PIN_PrimaryMotor_A && 1==PIN_PrimaryMotor_B)

void turn(uint8 durMsec, bool right)
{
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
uint16 timerAdvCurrentI =0,  timerAdvTemperature =0;
uint16 timer_msec =0, timer_sec =0;

// about the blinking led
uint8 blinkTick=0, blinkReload =0;
#define ledOn(_ON)  { PIN_LED_Signal = (_ON) ? 1:0; }

// about the variables to sample temperature
uint16 distance, CONFIG_yieldDistance =0;

uint16 readDistance();

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
		
		// update timerTurnMotor
		if (timerTurnMotor)
		{
			if (0 == --timerTurnMotor)
				TurnMotor_reset();
		}

		// update timerPrimaryMotor
		if (timerPrimaryMotor)
		{
			if (0 == --timerPrimaryMotor)
				PrimaryMotor_reset();
		}
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

void execCmd(const char* cmd)
{
	char c1,c2;
	uint8 i=0;
	int16 tmp16 = 0;

	int c= sscanf(cmd, "MOVE %c%c %d", &c1, &c2, &tmp16); //command: MOVE {F|B}{L|R|D} <msec>
	if (3 == c)
	{
		if (tmp16 >200)
			tmp16 = 200;

		move(tmp16, ('B' == c1) ?1:0);

		if ('L' == c2 || 'R' ==c2)
			turn(tmp16, ('R' == c2) ?1:0);
		return;
	}

	//command: SET P2=<uint8>
	c= sscanf(cmd, "SET P2=%d", &tmp16);
	if (c>0)
	{
		P2 = tmp16 & 0xff;
		return;
	}

	//command: GET DIST
	if (0 == strncmp(cmd, "GET DIST", sizeof("GET DIST")-1))
	{
		printf("DIST=%d\r\n", distance);
		return;
	}

	//command: SET YIELDDIST=<millimeter>
	c= sscanf(cmd, "SET YIELDDIST=%d", &tmp16);
	if (c>0)
	{
		CONFIG_yieldDistance = tmp16;
		return;
	}
}

// -------------------------------------------------------------------------------
// main()
// -------------------------------------------------------------------------------
// the main loop
void main(void)
{
	uint8 i=0;
	int16 tmp16 = 0;

	Timer0_init(1); // initialize timer0 w/ interval=1msec
	UART_init(96);  // initialize uart1 with 8n1,9600bps

	// initialize actions
	turn(500, 0); delayXms(800); turn(500, 1); delayXms(800);
	move(500, 0); delayXms(800); move(500, 1); delayXms(800);

	while (1) // the main loop of cam-car
	{
		// step 1. read the distance ahead
		distance = readDistance();

		// step 1.1. stop moving if CONFIG_yieldDistance is set
		if (CONFIG_yieldDistance >10)
		{
			if (distance >0 && distance < CONFIG_yieldDistance && IS_FORWARD && !(PIN_TurnMotor_A || PIN_TurnMotor_B))
				PrimaryMotor_reset();
		}

		// step	2. process the command received
		if ('\0' != recvdCmd[0])
		{
			execCmd(recvdCmd);
			recvdCmd[0] = '\0';
		}
	} // end of main loop

}

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
