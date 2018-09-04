// sensor & ctrl node

#include "STC12C5AXX.h"
#include "defines.h"
#include <intrins.h>
#include <stdio.h>
#include <string.h>

const uint16 nodeId=0x1233;

// this water control takes Baud=9600 with 

// -------------------------------------------------------------------------------
// MCU pin connections
// -------------------------------------------------------------------------------

// PT2262 receiver
//   - D0~D3 connect to P2^0~3 Pin21~24
//   - VT connects to P0.7 Pin32
// self-study receiver
//   - D0~D3 connect to P2^4~7 Pin25~28
//   - VT connects to P0.6 pin33
// P0.7 and P0.6 joined to INT1/P3.3/pin13
sbit ByReceiverA       = P0^7; // VT
sbit ByReceiverB       = P0^6; // VT

// WG26 pad
//   - D0,1 connect to P0.4/Pin35, P0.5/Pin34, joined to INT0/P3.2/Pin12
sbit WG26_Exti =P3^2; // the interrupt pin: WG DATA0 and DATA1 connect to this pin thru a D each, 
sbit WG26_D0   =P0^4; // the yellow pin of the keyboard

// 485 module
//   - RxD, TxD connected to P3.0/pin10, P3.1/pin11
sbit MAX485_DE          = P3^6;

// LED
sbit LED_sig            = P0^0; // ??
sbit LED_epad           = P0^1;
sbit Beep_epad          = P0^2;

// -------------------------------------------------------------------------------
// Declarations
// -------------------------------------------------------------------------------
#define MSGCH_SEND_TO_UART        0
#define MSGCH_RECV_FROM_UART      1

volatile MsgLine xdata _pendingLines[MAX_PENDMSGS];

uint16 timer_msec =0, timer_sec =0;
uint8 blinkTick=0, blinkReload =0;

typedef struct _BlinkTick
{
	uint8 tick, reloadOn, reloadOff;
} BlinkTick;

BlinkTick kbLedTick = {0, 0, 0}, beepTick={0, 0, 0};

#define ledOn(_ON)    { LED_sig = (_ON) ? 1:0; }
#define kbLedOn(_ON)  { LED_epad = (_ON) ? 1:0; }
#define kbBeep(_ON)   { Beep_epad = (_ON) ? 1:0; }

void UART_485Dir(uint8 sendMode)   { MAX485_DE = sendMode ?1:0; }
bool wg26_read(uint8 wg26msg[4]);

typedef struct {
	char ch;
	uint8_t codes[4];
} wg26code;

static const wg26code epadKeyTable[] = {
	{ '0', "\0\1\2\3" },
	{ 0x00, "\0\1\2\3" }
};

bool wg26_read(uint8 wg26msg[4]);

// -------------------------------------------------------------------------------
// interrupt ISR_Exti0_WG26()
// -------------------------------------------------------------------------------
// interrupt of the wg26 signal from keypad
#define KB_MAX_INPUT (10)
char kbinput[KB_MAX_INPUT] ="";
uint kbinIdx =0; 
void ISR_Exti0_WG26() interrupt 0 // for EXTI0
{
	uint8 wg26msg[4], i;
	volatile MsgLine* outgoingLine= NULL;
	EX0=0;
	if (wg26_read(wg26msg))
	{
		// convert the received signals into charactors
		for (i=0; outgoingLine && epadKeyTable[i].ch; i++)
		{
			if ((epadKeyTable[i].codes[0] != wg26msg[0]) || (epadKeyTable[i].codes[1] != wg26msg[1]))
				continue;
			if ((epadKeyTable[i].codes[2] == wg26msg[2]) || (epadKeyTable[i].codes[3] != wg26msg[3]))
				continue;

			// find the matched code in the table
			break;
		}

		if (epadKeyTable[i].ch)	// a valid input from KB
		{
			kbinput[kbinIdx] = epadKeyTable[i].ch;
			if ('#' == kbinput[kbinIdx]) // the terminate key pressed
			{
				kbinput[++kbinIdx] ='\0'; kbinIdx =0; // terminate the string, and reset the buffer for next receiving
				outgoingLine = MsgLine_findLineToFill(MSGCH_SEND_TO_UART);
				if (outgoingLine)
				{
					sprintf(outgoingLine->msg, "POST %04x/kb=%s\r\n", nodeId, kbinput);
					outgoingLine->offset = 0; // ready to go
				}
  			}
			else { kbinIdx = (++kbinIdx) % (KB_MAX_INPUT-1); }
		}
	}

	WG26_Exti =1;
	EX0 =1;
	IE0 =0; // clear the interrupt EXTI0
}

// -------------------------------------------------------------------------------
// interrupt ISR_Exti1_ASKRecvr()
// -------------------------------------------------------------------------------
// interrupt of ASK receiving that triggered by	VT of either receiver
void ISR_Exti1_ASKRecvr() interrupt 2 // for EXTI1
{
	uint8 v = P0;
	volatile MsgLine* outgoingLine= NULL;
	if (ByReceiverA)
	{
		v &= 0x0f;
	    ByReceiverA =0;
	}

	if (ByReceiverB)
	{
		v >>=4; v+=0x10;
	    ByReceiverB =0;
	}

	if (v)
	{
		outgoingLine = MsgLine_findLineToFill(MSGCH_SEND_TO_UART);
		if (outgoingLine)
		{
			sprintf(outgoingLine->msg, "POST %04x/askMsg=%02x\r\n", nodeId, v);
			outgoingLine->offset = 0; // ready to go
		}
 	}

	IE1 =0; // clear the interrupt EXTI1
}

bool tickBlink(BlinkTick* pBT)
{ 
	if (NULL == pBT || 0 == pBT->reloadOn)
		return false;
		
	if (0 == (pBT->tick-- & 0x7f))
	{
		pBT->tick = (pBT->reloadOn & 0x7f) ^ (pBT->tick & 0x80);
		if (0 == (pBT->tick & 0x80))
			pBT->tick = pBT->reloadOff & 0x7f;
	}

	return (pBT->tick & 0x80) ? true:false;
}

// -------------------------------------------------------------------------------
// interrupt ISR_timer0()
// -------------------------------------------------------------------------------
// the timer interrupt, once every 1 msec expected
void ISR_timer0() interrupt 1
{
	Timer0_reload();

	if (!(--timer_msec & 0x3ff))
	{
		// approximate 1 sec timer = 1msec *1024 
		timer_msec = 0x3ff;
		timer_sec--;
		
		// TODO: some 1 set timer logic here 
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

		// drive the LED of keyboard
		kbLedOn(tickBlink(&kbLedTick)?1:0); // tickPulse(kbLedTick, kbLedOn);

		// drive the Beep of keyboard
		kbBeep(tickBlink(&beepTick)?1:0); // kbBeep(tickBlink(&beepTick)?1:0);
	}
}

void BSP_Init()
{
	EA=0;  // disable interrupts
	IT0=1; // triggered by down edge

	Timer0_init(1); // initialize timer0 w/ interval=1msec
	UART_init(96);  // initialize uart1 with 8n1,9600bps

	EX0=1; // enable EXTI0
	EA=1;  // enable all interrupts
}

// -----------------------------
// The keyboard via Wiegand26
// -----------------------------
bool wg26_read(uint8 wg26msg[4])  // called from ISR_Exti0_WG26
{
	uint8 i=0, j=0, k=0;
	bit theBit = WG26_D0;
	wg26msg[0] = wg26msg[1] = wg26msg[2] = wg26msg[3] = 0;

	for (j =0; j <100;)
	{
		for (k=0; !WG26_Exti && k< 200; k++)
		{
			theBit = WG26_D0;
			delayX10us(1);
		}

		if (k>100) // take it as a noise
			return false;

		// determin which byte it belongs to
		if (0 == j || j>=25)
			k = 0;
		else if ( j >= 1 && j <9)
			k = 1;
		else if ( j >=9 && j<17)
			k = 2;
		else if ( j>=17 && j<25)
		    k = 3;

		j++;

		wg26msg[k] <<=1;
		wg26msg[k] |= theBit;

		delayX10us(30);
		for (i=0; WG26_Exti && i< 254; i++) // wait till the next signal
			delayX10us(2);

		theBit = WG26_D0;

		if (i>253)
			return false;
	}

	return true;
}

void configBlink(BlinkTick* pPT, char* msg)
{
	uint16 val;
	char* q = strstr(msg, "ond=");
	if (NULL == msg || NULL ==pPT)
		return;

	if (NULL != q)
	{
		q+= sizeof("ond=")-1;
		hex2int(q, &val);  //RF_masterId = val;
		pPT->reloadOn = val; pPT->reloadOff =0;
	}

	q = strstr(msg, "offd=");
	if (NULL != q)
	{
		q+= sizeof("offd=")-1;
		hex2int(q, &val);  //RF_masterPort = val %5;
		pPT->reloadOff =val;
	}

	pPT->tick =0; // force to take the current configuration
}


// -------------------------------------------------------------------------------
// OnMessage()
// -------------------------------------------------------------------------------
// process of an incoming message
void OnMessage(char* msg)
{
	const char* p =NULL, *q=NULL;
// TODO:
	uint16 val = strlen(msg);
	if (val<=0)
		return;
		
	// step 2. check if it is about the local node 
	p =	strstr((const char*)msg, "POST "); if (NULL == p) return;
	msg = p;
	p = strstr(msg, "/ASK_send?");
	if (NULL != p)
	{ 
		// message to configure the RF_master
		p += sizeof("/ASK_send?")-1;
		q = strstr(p, "code=");
		if (NULL != q)
		{
			q+= sizeof("code=")-1;
			hex2int(q, &val);  //RF_masterId = val;
		}

		q = strstr(p, "profId=");
		if (NULL != q)
		{
			q+= sizeof("profId=")-1;
			hex2int(q, &val);  //RF_masterPort = val %5;
		}

		//TODO: send the ASK message out
	}

	// about the beep configuration
	p = strstr(msg, "/beep?");
	if (NULL != p)
	{ 
		// message to configure the RF_master
		p += sizeof("/beep?")-1;
		configBlink(&beepTick, p);
	}

	// about the led on keyboard configuration
	p = strstr(msg, "/kbled?");
	if (NULL != p)
	{ 
		// message to configure the RF_master
		p += sizeof("/kbled?")-1;
		configBlink(&kbLedTick, p);
	}
}

// -------------------------------------------------------------------------------
// main()
// -------------------------------------------------------------------------------
// the main loop
void main()
{
	uint8 i,j;
	BSP_Init();

	while (1)
	{
		// step 1. check if there are any pending ASK message to UART
		for (i =0; i < MAX_PENDMSGS; i++)
		{
			if (MSGCH_SEND_TO_UART != _pendingLines[i].chId)
				continue; // message not to UART

			if (0 == _pendingLines[i].timeout || _pendingLines[i].offset >0)
				continue;  // idle or incomplete message

			// process a pending outgoing message
			//step 1.0. reserve timeout to hold this message during processing
			_pendingLines[i].timeout = 0xff;

			//step 1.1. switch the 485 direction as send
			UART_485Dir(1);

			//step 1.2. send the string
			for (j=0; _pendingLines[i].msg[j] && j< _pendingLines[i].offset; j++)
				UART_writebyte(_pendingLines[i].msg[j]);

			UART_writebyte(0);

			//step End. reset timeout to be idle after processing
			_pendingLines[i].timeout = 0;
		}

		//step 1.INF. switch the 485 direction as receiv
		UART_485Dir(0);

		// step 2. check if there are any incoming message to process
		for (i =0; i < MAX_PENDMSGS; i++)
		{
			if (MSGCH_RECV_FROM_UART != _pendingLines[i].chId || 0 == _pendingLines[i].timeout || _pendingLines[i].offset >0)
				continue;  // idle or incomplete message

			// process a pending incoming message
			//step 1.0. reserve timeout to hold this message during processing
			_pendingLines[i].timeout = 0xff;
			
			OnMessage(_pendingLines[i].msg);

			//step End. reset timeout to be idle after processing
			_pendingLines[i].timeout = 0;
		}

		// TODO: more processings
	}
}

// -------------------------------------------------------------------------------
// serial port ISR_uart()
// -------------------------------------------------------------------------------
// the interrupt of serial port communication
void ISR_uart(void) interrupt 4  
{
	uint8 i;
	static uint8 buf[20];
	if (0 == RI)
		return;

	ET0 =0;
	ledOn(1);

	UART_485Dir(0);

	for(i=0, buf[i]=UART_readbyte(); buf[i] && i<sizeof(buf); buf[++i]=UART_readbyte());
		
	MsgLine_recv(MSGCH_RECV_FROM_UART, buf, i);

	ledOn(0);
	UART_485Dir(0);
	ET0 =1;
}


