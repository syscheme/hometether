// sensor & ctrl node
#include "bsp.h"

// -------------------------------------------------------------------------------
// The byte of state
// -------------------------------------------------------------------------------
uint8   byteStatus =0; // consist of the following flags
#define FLAG_DEBUG      (1<<7)
#define FLAG_TempSampl  (0x03)
#define FLAG_FullLoop   (1<<2)
#define FLAG_Yield      (1<<3)
#define FLAG_Init       (1<<4)

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

#define DEFAULT_Timeout_Pump      CONST(   10,  3*60)  // 3min
#define DEFAULT_Timeout_Valve2to1 CONST(   10, 20)
#define DEFAULT_Timeout_Yield     CONST(   10,  5*60)  // 5min
#define DEFAULT_Refresh_Yield     CONST(   5,   3*60)  // 3min
#define DEFAULT_Timeout_Idle      CONST(   20, 30*60)  // half hr
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

uint16 CONFIG_TimeoutPump           = 0;
uint16 CONFIG_TimeoutValve2to1      = 0;
uint16 CONFIG_TimeoutYield          = 0;
uint16 CONFIG_IntvOnTempDiff        = 0;
// uint16 CONFIG_LowestTemp            = 0;
uint16 CONFIG_TempToStop            = DEFAULT_TempToStop;  // if any temperature sensor reaches this value, the pump will be forced to stop
uint16 CONFIG_TempDiff2Loop         = 0;
uint16 CONFIG_TimeoutIdle           = 0;

// -------------------------------------------------------------------------------
// Declarations
// -------------------------------------------------------------------------------
#define enablePumper(_ENABLE) { RELAY_Pumper = _ENABLE?0:1; }
#define isPumping() (0==RELAY_Pumper)
#define ledOn(_ON)  { LED_sig = (_ON) ? 1:0; }
// #define verandasJoin(_ENABLE) { VALVE2to1 = _ENABLE; }

typedef enum _loopMode {
	LoopMode_IDLE0,
	LoopMode_FULL,
	LoopMode_NORMAL,
	LoopMode_IDLE =0x3
} LoopMode;

void setLoopMode(LoopMode mode)
{
	switch (mode)
	{
	case LoopMode_IDLE0:
		VALVE2to1A = 0;
		VALVE2to1B = 0;
		break;

	case LoopMode_FULL:
		VALVE2to1A = 1;
		VALVE2to1B = 0;
		break;

	case LoopMode_NORMAL:
		VALVE2to1A = 0;
		VALVE2to1B = 1;
		break;

	case LoopMode_IDLE:
	default:
		VALVE2to1A = 1;
		VALVE2to1B = 1;
		break;
	}
}

uint16 timerPump =0,  timerValve2to1 =0, timerIdle=0, timerYield=0;
uint16 timer_msec =0, timer_sec =0;

#define SIZE_TempSensors 3
uint16 TempSensors[SIZE_TempSensors];

uint8 maskDS18B20 =0;
uint8 blinkTick=0, blinkReload =0;

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

		// update the pumper's timeout
		if (timerPump)
			timerPump--;

		// update 2-1 valve's timeout
		if (timerValve2to1)
			timerValve2to1--;

		// update 2-1 yield timeout
		if (timerYield)
			timerYield--;

		// update the idle timeout if configured
		if (timerIdle && CONFIG_TimeoutIdle >=MIN_Timeout_Idle)
			timerIdle--;
	}

	if (!(timer_msec & 0x3f)) // approximately every 0.05sec
	{
		// approximately every 0.05sec
		if (0 ==(blinkReload & 0x7f)) 
		{ ledOn(0); } // no reload configured, so turn off simply
		else if (0 == (blinkTick-- & 0x7f)) 
		{
			// counter blinkTick reached 0
			blinkTick = (blinkReload & 0x7f) ^ (blinkTick & 0x80);
			if (blinkTick & 0x80)
			{
				if (blinkTick >0x8f)
					blinkTick = 0x8f;

				ledOn(1);
			}
			else ledOn(0);
		}
	}
}

/*
// -------------------------------------------------------------------------------
// interrupt ISR_usr()
// -------------------------------------------------------------------------------
// the interrupt that user uses hot water
void ISR_usr() interrupt 2 // external interrupt 1
{
do {
//		if (timerPumping >0)
//			return; // no need to check if the pumper is currently working

if (ACT_ON_LOOPB)
{
// someone is using the hot water at outer loop, change 1-in-2-out valve to take full loop
if (!(byteStatus & FLAG_FullLoop))
{
// ignore FLAG_Yield but always turn on the pumper
byteStatus |= FLAG_FullLoop;
timerValve2to1 = CONFIG_TimeoutValve2to1;
break;
}
}

if (byteStatus & FLAG_Yield)
return; // no need to start pumping if it is at yield state

} while (0);

EX1 =0; // disable the interrupt
timerPumping = CONFIG_TimeoutPump;
}
*/

// -------------------------------------------------------------------------------
// isInUse()
// -------------------------------------------------------------------------------
//@param outerLoop 1 - if to test the activities on outer loop
//@return 1 if there are some activity
bit isInUse(bit veranda)
{
	bit tmp = InUse_any;
	do {
		tmp = InUse_any;
		delayXms(10);
	} while (tmp != InUse_any); // wait till the flow sensor gets stable

	if (veranda)
		return !InUse_veranda;

#ifdef DEBUG
	return !tmp || !InUse_veranda;
#else
	return !tmp;
#endif
}

void writeDS18B20byte(uint8 value)   
{
	uint8 j;

	for(j=0; j<8; j++)
	{
		if (value & 0x01)
		{ // write the bit=1
			P_DS18B20 &= ~maskDS18B20; // ANALOG_SIG=0;
			delayIOSet(); // ensure L remain greater than 1us but must reset to H before 15us
			P_DS18B20 |=  maskDS18B20; // ANALOG_SIG=1;
			delayX10us(5);
		}
		else
		{
			// write the bit=0;
			P_DS18B20 &= ~maskDS18B20; // ANALOG_SIG=0;
			delayX10us(5); // ensure L remain greater than 60usec
			P_DS18B20 |=  maskDS18B20; // ANALOG_SIG=1;
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

		P_DS18B20 &= ~maskDS18B20; // ANALOG_SIG=0;
		delayIOSet(); // delay a bit to ensure the L remains greater than 1us
		P_DS18B20 |=  maskDS18B20; // ANALOG_SIG=1;
		delayIOSet(); delayIOSet();
		// delayX10us(1); // the DS18B20 becomes valid after 15usec since the L

		value |= (P_DS18B20 & maskDS18B20) ? 0x80 : 0x00; // j= ANALOG_SIG; value |= (j<<7);

		delayX10us(5); // yield for the next bit
		P_DS18B20 |=  maskDS18B20;   delayIOSet();  // ANALOG_SIG=1;
	}

	return value;
}

void resetDS18B20(void)
{
	int i =0;
	P_DS18B20 |=  maskDS18B20; // ANALOG_SIG=1;
	delayX10us(5);
	P_DS18B20 &= ~maskDS18B20; // ANALOG_SIG=0;
	delayX10us(60);
	P_DS18B20 |=  maskDS18B20; // ANALOG_SIG=1;
	delayX10us(4);

	for (i =1000; i && (P_DS18B20 & maskDS18B20); i--)
		delayIOSet();
	delayX10us(50);
	P_DS18B20 |=  maskDS18B20; // ANALOG_SIG=1;
}

int readDS18B20(uint8 chID) // unit = 0.1 degree
{
	uint8 a =0x00, b=0x00;
	int temp;

	// step 0, convert the channel Id to the bit of P_DS18B20
	maskDS18B20 = (1 << (chID &0x3)) << P_DS18B20_BASE_PIN; 

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

	// step 5. return the result
	return temp; // the return value is in 0.1 degree, for example, 238 mean 23.8C
}

// -------------------------------------------------------------------------------
// readTemperatures()
// -------------------------------------------------------------------------------
void readTemperatures()
{
	uint8 i;
	for (i=0; i < SIZE_TempSensors; i++)
		TempSensors[i] = readDS18B20(i);
}

// -------------------------------------------------------------------------------
// loopNeededPerTempertures()
// -------------------------------------------------------------------------------
// test if it is necessary to perform water loop up to the temperature differences
//@return: 1- to perform inner loop only, 2- to perform the full loop, otherwise no action
uint8 loopNeededPerTempertures()
{
	return 0;
}

uint testc=0;

// -------------------------------------------------------------------------------
// main()
// -------------------------------------------------------------------------------
// the main loop
void main(void)
{
	uint8 i;
	uint16 tmp=0;
	uint16 tempDiffIntv = 0;
	uint16 fullLoopAtIdleRounds = 0;

	CONFIG_TimeoutPump           = DEFAULT_Timeout_Pump;
	CONFIG_TimeoutValve2to1      = DEFAULT_Timeout_Valve2to1;
	CONFIG_TimeoutYield          = DEFAULT_Timeout_Yield;

	Timer0_init(1); // initialize timer0 w/ interval=1msec
	UART_init(96);  // initialize uart1 with 8n1,9600bps

	// set IO InUse_any and InUse_any to high-impedance state
	// 89C52 take PxM0[b]=0,PxM1[b]=1 as high-impedance import 
	P0M0 &= 0xf8;  
	P0M1 |= 0x07;

	readTemperatures();
	timerValve2to1 = 0;
	timerPump = MIN_Timeout_Pump; // pump for a while everytime when MCU restarts to confirm the event
	byteStatus |= FLAG_Init;

	while (1)
	{
		// step 1. read the tempture every 8 sec
		i = (timer_sec >>3) & FLAG_TempSampl;
		if (i != (byteStatus & FLAG_TempSampl))
		{	
			// a new min now
			byteStatus &= ~FLAG_TempSampl; 	byteStatus |= i; // copy the new min value
			readTemperatures();
		}

		// step 2. determin timerPump and FLAG_FullLoop
		if (!(byteStatus & FLAG_FullLoop) && isInUse(1)) // someone is using the hot water at verandas, change join the verandas to take full loop
				byteStatus |= FLAG_FullLoop;

		if (!timerPump)
		{
			if(!timerIdle && CONFIG_TimeoutIdle >=MIN_Timeout_Idle)
			{
				timerPump = CONFIG_TimeoutPump;

				if (fullLoopAtIdleRounds)
					fullLoopAtIdleRounds--;
				else
				{
					fullLoopAtIdleRounds = (2*60*60) /MIN_Timeout_Idle; // every 2hr
					byteStatus    |= FLAG_FullLoop;
					timerValve2to1 = CONFIG_TimeoutValve2to1;
					timerPump += (3*60); // pump longer because it is a full loop
				}
			}
			else if (!(byteStatus & FLAG_Yield) && isInUse(0))
				timerPump = CONFIG_TimeoutPump;
		}

		// force to turn off the pump if one of the temprature exceeds CONFIG_HighestTemp 
		if (CONFIG_TempToStop >0 && !timerPump)
		{
			for (i =0; i< SIZE_TempSensors; i++)
			{
				if (TempSensors[i] <0xff00 && TempSensors[i] > CONFIG_TempToStop)
					break;
			}

			if (i< SIZE_TempSensors)
			{
				// found one met the CONFIG_HighestTemp, refresh the temperature read to make
				// sure the values then determin if pump needs to be stopped
				readTemperatures();

				if (TempSensors[i] <0xff00 && TempSensors[i] > CONFIG_TempToStop)
					timerPump =0;
			}
		}

		// step 2.1 signal the LED per status of pump and valve
		if (timerPump)
		{
			blinkReload =8; // 1Hz if pump run at inner loop
			if (byteStatus & FLAG_FullLoop)
				blinkReload = 4; // 2Hz if pump run at outer loop
		}

		if (timerValve2to1)
			blinkReload =2; // 4Hz if valve is executing

		// step 3. drive the loop selection if needed
		if (timerValve2to1)
			setLoopMode((LoopMode)((byteStatus & FLAG_FullLoop) ? LoopMode_FULL : LoopMode_NORMAL));
		else setLoopMode(LoopMode_IDLE);

		// step 4. turn on if timerPump>0
		if (timerPump)
		{
			if (!isPumping())
			{
				enablePumper(1);

				// start yield as the pumper has been recently run
				byteStatus |= FLAG_Yield;
				timerYield  = CONFIG_TimeoutYield;
			}

			continue; // no need to follow if pumper is running
		}

		// step 5. process off state timerPump==0
 		if (isPumping()) // timerPump rearched 0 but pumper is still working, turn it off
		{
			enablePumper(0);

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

			continue;
		}

		// enter the idle state here: timerPump==0 && isPumping()==0
		// -------------------------------------------------------------

		// step 5.1 reset the yield when the yield timeout has been reached
		if (0 == timerYield && (byteStatus & FLAG_Yield))
			byteStatus &= ~FLAG_Yield;

		if (byteStatus & FLAG_Yield)
		{
			blinkReload =30; // blink every 3 seconds if yield

			// if water used during first 2min of yield period, refresh field timer to count again
			if (timerYield < CONFIG_TimeoutYield && (/* timerYield >(CONFIG_TimeoutYield /2) ||*/ timerYield > (CONFIG_TimeoutYield - DEFAULT_Refresh_Yield)) && isInUse(0))
				timerYield = CONFIG_TimeoutYield;

			delayXms(200);
			continue;
		}

		blinkReload =100; // blink @ 0.2Hz if at complete idle

		// step 5.2 diff the tempture to determin if it is necessary to launch
		if (CONFIG_IntvOnTempDiff < MIN_IntvOnTempDiff)
			continue; // no need to validate based on the temperture diff due to invalid configuration

		tempDiffIntv  = ~0;
		tempDiffIntv -= CONFIG_IntvOnTempDiff;
		if (timer_sec < tempDiffIntv)
		{
			timer_sec = ~0; // reset the timer_sec;
			switch (loopNeededPerTempertures())
			{
			case 2: // need take full loop
				timerValve2to1 = CONFIG_TimeoutValve2to1;
				byteStatus    |= FLAG_FullLoop;
				// no break; here

			case 1: // need to perform loop
				timerPump = CONFIG_TimeoutPump;
				break;

			default:
				// no need to perform loop, do nothing
				break;
			}
		}
	}
}

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
				    	 		
	UART_485Dir(0);
	// RI  =0;  tmp = SBUF; //
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

		UART_writebyte(SIZE_TempSensors);
		for (tmp =0 ; tmp< SIZE_TempSensors; tmp++)
			UART_writeword(TempSensors[tmp]);

		tmp = byteStatus & FLAG_TempSampl; 	byteStatus ^= tmp; // force to refresh the temperature reading in the next loop within main()
		// readTemperatures(); // force to refresh the temperature reading
		break;

	case ocQueryPorts: // read the IO ports
		UART_485Dir(1);
		UART_writebyte(oc);

		UART_writebyte(P0);
		UART_writebyte(P1);
		UART_writebyte(P2);
		UART_writebyte(P3);

		break;

	case ocReadTickCount: // read the tick count
		UART_485Dir(1);
		UART_writebyte(oc);

		UART_writeword(testc);
		UART_writeword(timer_msec);
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
		CONFIG_IntvOnTempDiff = w;

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

		UART_writeword(CONFIG_IntvOnTempDiff);
		UART_writeword(CONFIG_TempDiff2Loop);
//		UART_writeword(CONFIG_LowestTemp);
		UART_writeword(CONFIG_TempToStop);

		break;
	}

	UART_485Dir(0);
	ledOn(0);
	ET0 =1;
}

void UART_485Dir(uint8 sendMode)
{
	MAX485_DE = sendMode ?1:0;
}

