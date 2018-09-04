#include "BaseBoard.h"
#include "LY51Utils.h"

// test codes start here
void testDelay(void)							
{
	// delay 0.5sec
	delayXms(250);
	delayXms(250);
}

void testLatch(void)
{
	int i=0;
	for (i =0; 1; i= ++i % 16)
	{
//		setLatch(i, 1);
		testDelay();
//		setLatch(i, 0);
		testDelay();
	}
}

void testAnalog()
{
	int i=0;
	ANALOG_SIG =0;
	for (i =0; 1; i= ++i % 8)
	{
		openAnalogSignal(i);
		ANALOG_SIG =1;
		testDelay();
		ANALOG_SIG =0;
		testDelay();
		ANALOG_SIG =1;
		testDelay();
		ANALOG_SIG =0;
		testDelay();
		closeAnalogSignals();
	}
}

void main(void)  
{
	BYTE i=0;
	initBoard();
	testAnalog();
}

