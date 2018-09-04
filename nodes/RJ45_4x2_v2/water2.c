// sensor & ctrl node
#include "includes.h"
#include "textmsg.h"

// -------------------------------------------------------------------------------
// The byte of state
// -------------------------------------------------------------------------------
__IO__ uint8_t   byteStatus =0; // consist of the following flags
#define FLAG_DEBUG      (1<<7)
#define FLAG_FullLoop   (1<<0)
#define FLAG_Yield      (1<<1)
#define FLAG_Init       (1<<2)

// -------------------------------------------------------------------------------
// Ordinal code
// -------------------------------------------------------------------------------
enum OrdinalCode{
	// section of queries 0x00 - 0x01f
	ocQueryStatus   =0x00,   // read the status parameters
	ocReadTemperatures,      // read the temperatures
	ocQueryPorts,            // read the port bytes of P0, P1, P2, P3
	ocReadTickCount =0x1f,

	// section of user commands	0x20 - 0x2f
	ocStartPumping  =0x20, // user request to start pumping, the highest bit=1 indicates to take full loop
	ocForceStatus   =0x2f, // force to set the status byte

	// section of configurations  0x30 - 0x3f
	ocConfigTimeout =0x30, // set the CONFIG_TimeoutPump, CONFIG_TimeoutValve2to1, CONFIG_TimeoutYield, CONFIG_TimeoutIdle
	ocConfigTemperatures,   // set the CONFIG_IntvOnTempDiff, CONFIG_TempDiff2Loop, MAX_TempToStop

	// end of OrdinalCode
	ocMax
};

// -------------------------------------------------------------------------------
// Configurations
// -------------------------------------------------------------------------------
#define CONST(_DEBUGVAL, _NORMALVAL) ((FLAG_DEBUG & byteStatus) ? (_DEBUGVAL)  : (_NORMALVAL)) 

#define DEFAULT_Timeout_Pump      CONST(   10, 3*60)  // 3min
#define DEFAULT_Timeout_Valve2to1 CONST(   10, 20)
#define DEFAULT_Timeout_Yield     CONST(   10, 5*60)  // 5min
#define DEFAULT_Refresh_Yield     CONST(   5,  3*60)  // 3min
#define DEFAULT_Timeout_Idle      CONST(   20, 30*60)  // half hr
#define DEFAULT_Timeout_ReadTemp  CONST(   5,  10)     // 10 sec
#define MIN_Timeout_Pump          CONST(    5, 10)
#define MIN_Timeout_Valve2to1     CONST(    5,  8)
#define MIN_Timeout_Yield         CONST(    5, 30)
#define MIN_IntvOnTempDiff        CONST(    5, 60)
#define MIN_Timeout_Idle          CONST( 1*60, 10*60)  // 10min

#define MAX_Timeout_Pump          (20*60) // 20 min
#define MAX_Timeout_Valve2to1     (5*60)  // 5 min
#define MAX_Timeout_Yield         (20*60) // 30 min

#define MAX_LowestTemp            (400)   // 40C
#define MAX_TempToStop			  (600)   // 60C
#define DEFAULT_TempToStop        (420)   // 42C
#define MIN_TempDiff2Loop         (200)   // 20C

uint16_t CONFIG_TimeoutPump           = 0;
uint16_t CONFIG_TimeoutValve2to1      = 0;
uint16_t CONFIG_TimeoutYield          = 0;
uint16_t CONFIG_TimeoutReadTemp       = 0;

// uint16_t CONFIG_IntvOnTempDiff        = 0;
// uint16_t CONFIG_LowestTemp            = 0;
uint16_t CONFIG_TempToStop            = DEFAULT_TempToStop;  // if any temperature sensor reaches this value, the pump will be forced to stop
uint16_t CONFIG_TempDiff2Loop         = 0;
uint16_t CONFIG_TimeoutIdle           = 0;

// -------------------------------------------------------------------------------
// Declarations
// -------------------------------------------------------------------------------
// the following timers are in second
uint16_t timerPump =0,  timerValve2to1 =0, timerYield=0, timerIdle=0;
uint16_t timer_readTemp =0, timerIdleBlink =0;
__IO__ uint16_t timeoutRF=DEFAULT_Timeout_RF;
// uint16_t timer_msec =0;

int16_t  temperatures[DS18B20_MAX]={0,0,};
uint8_t  cTemperatures=0;
uint16_t ADC_DMAbuf[ADC_CHS*2];


#define DEC_VAR_BY_TIL_ZERO(VAR, BY) {if (VAR > BY) VAR -= BY; else VAR = 0;}

void Water_stepTimers()
{
	static uint32_t msecLast=0;
	uint16_t secdiff = ( (gClock_msec - msecLast) & 0xffff) /1000;

	if (secdiff <=0) // in second
		return;

	msecLast = gClock_msec;

	// update the pumper's timeout
	DEC_VAR_BY_TIL_ZERO(timerPump, secdiff);

	// update 2-1 valve's timeout
	DEC_VAR_BY_TIL_ZERO(timerValve2to1, secdiff);

	// update 2-1 yield timeout
	DEC_VAR_BY_TIL_ZERO(timerYield, secdiff);

	DEC_VAR_BY_TIL_ZERO(timer_readTemp, secdiff);
	DEC_VAR_BY_TIL_ZERO(timerIdleBlink, secdiff);
	DEC_VAR_BY_TIL_ZERO(timeoutRF, secdiff);

	// update the idle timeout if configured
	if (CONFIG_TimeoutIdle >=MIN_Timeout_Idle)
		DEC_VAR_BY_TIL_ZERO(timerIdle, secdiff);
}


// -------------------------------------------------------------------------------
// isInUse()
// -------------------------------------------------------------------------------
//@param outerLoop 1 - if to test the activities on outer loop
//@return 1 if there are some activity
static uint8_t isInUse(uint8_t veranda)
{
	// the input pin takes ADC: 14K base-to-3v3, 22K any-to-GND, 5K veranda-to-GND
	// 0 - no water consumption
	// 26% - any-in-use, take 20~30%
	// 82% - any-in-use, take 75~100%

	if (ADCVAL_WaterFlow > (ADC_VALUE_MAX * 90/100))
		return 0;

	if (veranda && (ADCVAL_WaterFlow < (ADC_VALUE_MAX * 50/100)))
		return 1;

	return (veranda ? 0 :1);
	/*
	bit tmp = InUse_any;
	do {
		tmp = InUse_any;
		delayXmsec(10);
	} while (tmp != InUse_any); // wait till the flow sensor gets stable

	if (veranda)
		return !InUse_veranda;

#ifdef DEBUG
	return !tmp || !InUse_veranda;
#else
	return !tmp;
#endif
*/
}

// -------------------------------------------------------------------------------
// Water_refreshTemperatures()
// -------------------------------------------------------------------------------
void Water_refreshTemperatures(void)
{
	uint8_t i=0;
	//IRQ_DISABLE();
	for (i=0; i < DS18B20_MAX && ds18b20s[i].bus; i++)
		temperatures[i] =  DS18B20_read(&ds18b20s[i]);
	//IRQ_ENABLE();
	cTemperatures = i;
	if (CONFIG_TimeoutReadTemp >0)
		timer_readTemp = CONFIG_TimeoutReadTemp;
}

// -------------------------------------------------------------------------------
// Water_init()
// -------------------------------------------------------------------------------
uint16_t fullLoopAtIdleRounds = 0;
void Water_init(void)
{
	CONFIG_TimeoutPump           = DEFAULT_Timeout_Pump;
	CONFIG_TimeoutValve2to1      = DEFAULT_Timeout_Valve2to1;
	CONFIG_TimeoutYield          = DEFAULT_Timeout_Yield;
	CONFIG_TimeoutReadTemp       = DEFAULT_Timeout_ReadTemp;

	Water_refreshTemperatures();

	timerValve2to1 = 0;
	timerPump = MIN_Timeout_Pump; // pump for a while everytime when MCU restarts to confirm the event
	byteStatus |= FLAG_Init;
}

// -------------------------------------------------------------------------------
// Water_do1round()
// -------------------------------------------------------------------------------
//@return msec to sleep for next
uint16_t Water_do1round(void)
{
	uint8_t i;

	// step 0. step all the related Timers
	Water_stepTimers();

	// step 1. refresh the temperature-reading if timer reached
	if (CONFIG_TimeoutReadTemp >0 && 0 == timer_readTemp)
	{
		timer_readTemp = CONFIG_TimeoutReadTemp; 	
		Water_refreshTemperatures();
	}

	// step 2. determin timerPump and FLAG_FullLoop
	if (0==(byteStatus & FLAG_FullLoop) && isInUse(1)) // someone is using the hot water at verandas, change join the verandas to take full loop
		byteStatus |= FLAG_FullLoop;

	if (!timerPump)
	{
		if (0==(byteStatus & (FLAG_Init | FLAG_Yield)) && isInUse(0))
			timerPump = CONFIG_TimeoutPump;
	}

	// force to turn off the pump if one of the temprature exceeds CONFIG_HighestTemp 
	if (CONFIG_TimeoutReadTemp >0 && CONFIG_TempToStop >0 && timerPump)
	{
		for (i =0; i< cTemperatures; i++)
		{
			if (temperatures[i] <1000 && temperatures[i] < CONFIG_TempToStop)
				break; // discontinue if there is any temp has not yet been heated enough
		}

		if (i >= cTemperatures) // all the temperatures appeared higher than CONFIG_TempToStop, stop the pumper
			timerPump =0;
	}

	// step 2.1 signal the LED per status of pump and valve
	if (timerPump)
		RNode_blink(BlinkPair_FLAG_A_ENABLE, 8, 20, 0);

	if (timerValve2to1)
		RNode_blink(BlinkPair_FLAG_A_ENABLE, 8, 10, 0);

	// step 3. drive the loop selection if needed
	if (timerValve2to1)
		Water_setLoopMode((LoopMode)((byteStatus & FLAG_FullLoop) ? LoopMode_FULL : LoopMode_NORMAL));
	else Water_setLoopMode(LoopMode_IDLE);

	// case 1. running when timerPump>0
	if (timerPump)
	{
		if (!Water_isPumping())
		{
			Water_enablePumper(1);

			// start yield as the pumper has been recently run
			byteStatus |= FLAG_Yield;
			timerYield  = CONFIG_TimeoutYield;
		}

		return NEXT_SLEEP_MIN; // no need to follow if pumper is running
	}

	// case 2. turn off when timerPump==0
	if (Water_isPumping()) // timerPump rearched 0 but pumper is still working, turn it off
	{
		Water_enablePumper(0);

		// reset the loop selector to the inner loop
		byteStatus    &= ~FLAG_FullLoop;
		timerValve2to1 = CONFIG_TimeoutValve2to1;

		// reset the timer for yielding
		byteStatus |= FLAG_Yield;
		timerYield = CONFIG_TimeoutYield;
		if (byteStatus & FLAG_Init)
		{
			byteStatus &= ~FLAG_Init;
			timerYield  = 10;
		}

		// reset the timer for idle
		if (CONFIG_TimeoutIdle >=MIN_Timeout_Idle)
			timerIdle = CONFIG_TimeoutIdle;

		return NEXT_SLEEP_MIN;
	}

	// enter the idle state here: timerPump==0 && isPumping()==0
	// -------------------------------------------------------------

	// step 5.1 reset the yield when the yield timeout has been reached
	if (0 == timerYield && (byteStatus & FLAG_Yield))
		byteStatus &= ~FLAG_Yield;

	// case 3. to yield
	if (byteStatus & FLAG_Yield)
	{
		RNode_blink(BlinkPair_FLAG_A_ENABLE, 8, 40, 0);

		// if water used during first 2min of yield period, refresh field timer to count again
		if (timerYield < CONFIG_TimeoutYield && (/* timerYield >(CONFIG_TimeoutYield /2) ||*/ timerYield > (CONFIG_TimeoutYield - DEFAULT_Refresh_Yield)) && isInUse(0))
			timerYield = CONFIG_TimeoutYield;

		return NEXT_SLEEP_MIN *20;
	}

	// case 4. complete idle
	if (0 == timerIdleBlink)
	{
		RNode_blink(BlinkPair_FLAG_A_ENABLE, 8, 8, 1);
		timerIdleBlink = 5; // blink once every 5sec when idle
	}

	return NEXT_SLEEP_MAX;
}

// -------------------------------------------------------------------------------
// Water_doMainLoop()
// -------------------------------------------------------------------------------
// the main loop
void Water_doMainLoop(void)
{
	uint16_t nextSleep =0;

	Water_init();

	while (1)
	{
		if (nextSleep <NEXT_SLEEP_MIN)
			nextSleep = NEXT_SLEEP_MIN;
		else if (nextSleep > NEXT_SLEEP_MAX)
			nextSleep = NEXT_SLEEP_MAX;
		delayXmsec(nextSleep);

		nextSleep = Water_do1round();
	}
}

FIFO usartRX, usartTX;
void USART_read(uint8_t* data, uint8_t len)
{
	while (len--)
		*data = USART_ReceiveData(USART1);
}

void USART_write(uint8_t* data, uint8_t len)
{
	while (len--)
		USART_SendData(USART1, *data++);
}

uint8_t UART_readbyte()
{
	uint8_t data =0;
	USART_read((uint8_t*)&data, 1);
	return data;
}

uint16_t UART_readword()
{
	uint16_t data =0;
	USART_read((uint8_t*)&data, 2);
	return data;
}

void UART_485Dir(uint8_t tx)
{
}

#define UART_writebyte(B) USART_write((uint8_t*)&B, 1);
#define UART_writeword(W) USART_write((uint8_t*)&W, 2);

// -------------------------------------------------------------------------------
// serial port ISR_uart()
// -------------------------------------------------------------------------------
#if  0
void ISR_uart(void) 
{
	uint8_t tmp, oc;
	uint16_t w;
	RNode_blink(BlinkPair_FLAG_B_ENABLE, 8, 8, 3);

	UART_485Dir(0);

	oc = UART_readbyte();
	switch (oc)
	{
	case ocQueryStatus: // read the state parameters
		UART_485Dir(1);
		UART_writebyte(oc);

		UART_writebyte(byteStatus);
		UART_writeword(timerPump);
		UART_writeword(timerValve2to1);
		UART_writeword(timerYield);

		break;

	case ocReadTemperatures: // read the temperatures
		UART_485Dir(1);
		UART_writebyte(oc);
		UART_writebyte(cTemperatures);

		for (tmp =0 ; tmp< cTemperatures; tmp++)
			UART_writeword(temperatures[tmp]);

		timer_readTemp =0; // force to refresh the temperature reading in the next loop within main()
		break;

	case ocReadTickCount: // read the tick count
		UART_485Dir(1);
		UART_writebyte(oc);

		//		UART_writeword(testc);
		w = gClock_msec & 0xffff;
		UART_writeword(w);
		//		testc=timer_msec=0;
		break;

	case ocStartPumping: // user request to start pumping, the highest bit=1 indicates to take full loop
		w = UART_readword();
		if (w & 0x8000)
		{
			w &= 0x7fff;
			timerValve2to1 = CONFIG_TimeoutValve2to1;
			byteStatus |= FLAG_FullLoop;
		}

		if (w > CONFIG_TimeoutPump *10)
			w = CONFIG_TimeoutPump *10;

		timerPump = w;

		// echo with timerPump
		UART_485Dir(1);
		UART_writebyte(oc);

		UART_writeword(timerPump);
		break;

	case ocForceStatus: // force to set the status byte
		tmp = UART_readbyte();
		byteStatus = tmp;

		// echo with byteStatus
		UART_485Dir(1);
		UART_writebyte(oc);
		UART_writebyte(byteStatus);

		break;

	case ocConfigTimeout: // set the CONFIG_TimeoutPump, CONFIG_TimeoutValve2to1, CONFIG_TimeoutYield, CONFIG_TimeoutIdle
		w = UART_readword();
		if (0 !=w)
		{
			if (w > MAX_Timeout_Pump)
				w = MAX_Timeout_Pump;
			if (w < MIN_Timeout_Pump)
				w = MIN_Timeout_Pump;
			CONFIG_TimeoutPump = w;
		}

		w = UART_readword();
		if (0 !=w)
		{
			if (w > MAX_Timeout_Valve2to1)
				w = MAX_Timeout_Valve2to1;
			if (w < MIN_Timeout_Valve2to1)
				w = MIN_Timeout_Valve2to1;
			CONFIG_TimeoutValve2to1 = w;
		}

		w = UART_readword();
		if (0 !=w)
		{
			if (w > MAX_Timeout_Yield)
				w = MAX_Timeout_Yield;
			if (w < MIN_Timeout_Yield)
				w = MIN_Timeout_Yield;

			// yield must be no less than pump timeout
			if (w < CONFIG_TimeoutPump +20)
				w = CONFIG_TimeoutPump +20;

			CONFIG_TimeoutYield = w;
		}

		w = UART_readword();
		if (0 !=w)
		{
			if (w < MIN_Timeout_Idle)
				w = MIN_Timeout_Idle;
			CONFIG_TimeoutIdle = w;
		}

		// echo with CONFIG_TimeoutPump, CONFIG_TimeoutValve2to1, CONFIG_TimeoutYield, CONFIG_TimeoutIdle
		UART_485Dir(1);
		UART_writebyte(oc);

		UART_writeword(CONFIG_TimeoutPump);
		UART_writeword(CONFIG_TimeoutValve2to1);
		UART_writeword(CONFIG_TimeoutYield);
		UART_writeword(CONFIG_TimeoutIdle);

		break;

	case ocConfigTemperatures: // set the CONFIG_IntvOnTempDiff, CONFIG_LowestTemp, CONFIG_TempDiff2Loop
		w = UART_readword();
		if (w < MIN_IntvOnTempDiff)
			w = 0;
		CONFIG_TimeoutReadTemp = w;

		w = UART_readword();
		if (0 !=w)
		{
			if (w < MIN_TempDiff2Loop)
				w = MIN_TempDiff2Loop;
			CONFIG_TempDiff2Loop = w;
		}

		//		w = UART_readword();
		//		if (w > MAX_LowestTemp)
		//			w = MAX_LowestTemp;
		//		CONFIG_LowestTemp = w;

		w = UART_readword();
		if (w > MAX_TempToStop)
			w = MAX_TempToStop;
		CONFIG_TempToStop = w;

		// echo with CONFIG_IntvOnTempDiff
		UART_485Dir(1);
		UART_writebyte(oc);

		UART_writeword(CONFIG_TimeoutReadTemp);
		UART_writeword(CONFIG_TempDiff2Loop);
		//		UART_writeword(CONFIG_LowestTemp);
		UART_writeword(CONFIG_TempToStop);

		break;
	}

	UART_485Dir(0);
}
#endif 

void NetIf_transmitPacket(void* netIf, pbuf* packet)
{
	if (NULL == netIf|| NULL == packet)
		return;

	for (;packet; packet=packet->next)
	{
		if (netIf == &COM1)
		{
			USART_transmit(&COM1, packet->payload, packet->len);
			continue;
		}

		if (netIf == &si4432)
		{
			SI4432_transmit(&si4432, packet);
			continue;
		}
	}
}

/*
void TextMsg_OnRequest(void* netIf, uint16_t cseq, uint8_t* msg, uint16_t len)
{
	char obuf[64], *p=obuf;
	pbuf* pResp;
	uint8_t i;
	uint32_t w;
	do {
		// CMD>> query status: ws=?
		//          resp: ws=<statusByte>,<timerPump>,<timerValve2to1>,<timerYield>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(ws=?))) // list interfaces
		{
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "ws=%02x,p%d,v%d,y%d", byteStatus, timerPump, timerValve2to1, timerYield);
			break;
		}

		// CMD>> user force to state: ws=<statusByte>
		//          resp: ws=<statusByte>,<timerPump>,<timerValve2to1>,<timerYield>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(pt=)))
		{
			if (hex2byte((const char*)msg+3, &i)<2)
				return; // invalid
			byteStatus = i;
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "ws=%x,p%d,v%d,y%d\n", byteStatus, timerPump, timerValve2to1, timerYield);
			break;
		}

		// CMD>> temperature status: ts=?
		//          resp: ts=<temp1>,<temp2>,<temp3>...
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(ts=?))) // list interfaces
		{
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "ts=");
			for (i =0 ; i< cTemperatures; i++)
			{
				p += snprintf(p, obuf + sizeof(obuf) -p -2, "%d,", temperatures[i]);
			}
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "\n");
			break;
		}

		// CMD>> user request to pump: pt=<sec>[,0|1]
		//          resp: pt=<sec>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(pt=)))
		{
			w = atoi((const char*)msg+3);
			if (w > CONFIG_TimeoutPump *10)
				w = CONFIG_TimeoutPump *10;
			if (strstr((const char*) msg+3, ",1"))
			{
				// full loop
				timerValve2to1 = CONFIG_TimeoutValve2to1;
				byteStatus |= FLAG_FullLoop;
			}

			timerPump = w;
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "pt=%d\n", timerPump);
			break;
		}

		// CMD>> user config pump timeout: cpt=<sec>
		//          resp: cpt=<sec>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(cpt=)))
		{
			w = atoi((const char*)msg+CONST_STR_LEN(cpt=));
			ADJUST_BY_RANGE(w, MIN_Timeout_Pump, MAX_Timeout_Pump);
			CONFIG_TimeoutPump = w;

			p += snprintf(p, obuf + sizeof(obuf) -p -2, "cpt=%d\n", CONFIG_TimeoutPump);
			break;
		}

		// CMD>> user config valve timeout: cvt=<sec>
		//          resp: cvt=<sec>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(cvt=)))
		{
			w = atoi((const char*)msg+CONST_STR_LEN(cvt=));
			ADJUST_BY_RANGE(w, MIN_Timeout_Valve2to1, MAX_Timeout_Valve2to1);
			CONFIG_TimeoutValve2to1 = w;

			p += snprintf(p, obuf + sizeof(obuf) -p -2, "cvt=%d\n", CONFIG_TimeoutValve2to1);
			break;
		}

		// CMD>> user config valve timeout: cyt=<sec>
		//          resp: cyt=<sec>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(cyt=)))
		{
			w = atoi((const char*)msg+CONST_STR_LEN(cyt=));
			ADJUST_BY_RANGE(w, MIN_Timeout_Yield, MAX_Timeout_Yield);
			CONFIG_TimeoutYield = w;

			p += snprintf(p, obuf + sizeof(obuf) -p -2, "cyt=%d\n", CONFIG_TimeoutYield);
			break;
		}

		// CMD>> user config valve timeout: cit=<sec>
		//          resp: cit=<sec>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(cit=)))
		{
			w = atoi((const char*)msg+CONST_STR_LEN(cit=));
			ADJUST_BY_RANGE(w, MIN_Timeout_Idle, 36000);
			CONFIG_TimeoutIdle = w;

			p += snprintf(p, obuf + sizeof(obuf) -p -2, "cit=%d\n", CONFIG_TimeoutIdle);
			break;
		}

		// CMD>> user request to reboot: reboot=<nodeId>
		//          no resp if rebooted
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(reboot=)))
		{
			w =0;
			hex2int((const char*)msg+CONST_STR_LEN(reboot=), &w);
			if (w == nidThis)
				RNode_reset();
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "nid=%08x\n", nidThis);
			break;
		}

	} while(0);

	pResp = TextMsg_composeResponse(cseq, (uint8_t*)obuf, p - obuf);
	NetIf_transmitPacket(netIf, pResp);
	pbuf_free(pResp);
}

void TextMsg_OnResponse(void* netIf, uint8_t cseq, uint8_t* msg, uint16_t len)
{
	// do nothing
}

*/

NodeId_t nidThis = 0x123456;
// __IO__ uint32_t gClock_msec =0;


// -------------------------------------------------------------------------------
// Dictionary expose for HtNode
// -------------------------------------------------------------------------------
enum {
	ODID_status     = ODID_USER_MIN,      // the status byte
	ODID_timerPump, ODID_timerV2to1, ODID_timerYield,
	ODID_adc, ODID_cTemp, ODID_Temps,
	ODID_cnfTOPump, ODID_cnfTOV2to1, ODID_cnfTOYield, ODID_cnfTOTemps,
};

// OD declaration:
USR_OD_DECLARE_START()
  OD_DECLARE(status,     ODTID_BYTE, ODF_Readable|ODF_Writeable, &byteStatus, 1)
  OD_DECLARE(timerPump,  ODTID_BYTE, ODF_Readable|ODF_Writeable, &timerPump,   sizeof(timerPump))
  OD_DECLARE(timerV2to1, ODTID_BYTE, ODF_Readable|ODF_Writeable, &timerValve2to1,  sizeof(timerValve2to1))
  OD_DECLARE(timerYield, ODTID_BYTE, ODF_Readable|ODF_Writeable, &timerYield,  sizeof(timerYield))
  OD_DECLARE(adc,        ODTID_BYTE, ODF_Readable,               ADC_DMAbuf,   sizeof(uint16_t)*ADC_CHS)
  OD_DECLARE(cTemp,      ODTID_BYTE, ODF_Readable,               &cTemperatures,  sizeof(cTemperatures))
  OD_DECLARE(Temps,      ODTID_BYTE, ODF_Readable,               temperatures,  sizeof(temperatures))
  // configurations
  OD_DECLARE(cnfTOPump,  ODTID_BYTE, ODF_Readable|ODF_Writeable, &CONFIG_TimeoutPump,  sizeof(CONFIG_TimeoutPump))
  OD_DECLARE(cnfTOV2to1, ODTID_BYTE, ODF_Readable|ODF_Writeable, &CONFIG_TimeoutValve2to1,  sizeof(CONFIG_TimeoutValve2to1))
  OD_DECLARE(cnfTOYield, ODTID_BYTE, ODF_Readable|ODF_Writeable, &CONFIG_TimeoutYield,  sizeof(CONFIG_TimeoutYield))
  OD_DECLARE(cnfTOTemps, ODTID_BYTE, ODF_Readable|ODF_Writeable, &CONFIG_TimeoutReadTemp,  sizeof(CONFIG_TimeoutReadTemp))
USR_OD_DECLARE_END()

const uint8_t EVENTOID_OF_Heartbeat[] ={1,2,3};

enum {
	ODEvent_USER_Event1 = ODEvent_USER_MIN, 
};
//
static const uint8_t event_objIds[] = {ODID_data1, ODID_data2, ODID_NONE};
USR_ODEVENT_DECLARE_START()
 	ODEVENT_DECLARE(USER_Event1, event_objIds)
USR_ODEVENT_DECLARE_END()

HtNode   htnThis; // supposed to be in htcluster.c when full compile
