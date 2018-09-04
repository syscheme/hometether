#include "configures.h"

//=============
code int SigSampleIntervals[] = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000};
xdata  unsigned char SampleData[254];
int SampleLen =0;
BYTE SampleIntervalIndex =3;
sbit SIG_IN =P3^3;

//=============
void signal_capture(void) interrupt 2     //外部中断 1  做为红外线接收
{
	BYTE theByte;
	int j, interval, cOfFF=0;;
	EX1=0;
//	SampleIntervalIndex =3;
	interval = SigSampleIntervals[SampleIntervalIndex];

	for (SIG_IN =1, j=0; SIG_IN && j<100; j++) // filter out the noice
		_nop_();

	if (SIG_IN)
	{
		EX1=1;
		return;
	}
		
	for (SampleLen=0; SampleLen< sizeof(SampleData); SampleLen++)
	{
		theByte = 0x00;
		for (j=0; j<8; j++)
		{
			theByte <<= 1;
			theByte |= SIG_IN;
			delayX10usL(interval -1);
		}
		
		SampleData[SampleLen] = theByte;
		
		// terminate if there a punch of contignous 0xff
		if (0xff == theByte)
		{
			if (++cOfFF > 10)
				break;
		}
		else cOfFF =0;
	}
 
 	EX1=1;
}

void readCaptureResult(BYTE tmp)
{	// signal to analyzer
	int i;
	SampleIntervalIndex =tmp & 0x0f;
	tmp = 0x70 | (SampleIntervalIndex & 0x0f);
	SBUF= tmp;
	while(!TI);
	TI=0;

	tmp = SampleLen & 0xff;
	SBUF = tmp;
	while(!TI);
	TI=0;

	for (i=0; i< SampleLen; i++)
	{
		SBUF= SampleData[i];
		while(!TI);
		TI=0;
	}
	return;
}

///////////////////////////////////////////
/*
void delayX10us(BYTE n) // the test of 9/2 hints this should sleep 60% longer
{
	// at 12Mhz, a machine cycle equals to 1us, both LCALL and RET takes 2 cycle, so make _nop_() 6 times for 10us

	while(--n) // each loop takes 3 cycles, this loop makes (3*1 + 7*1) * (n-1) us
	{ _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); } //7 times
	
	_nop_(); _nop_(); _nop_(); _nop_(); _nop_(); // add additional 5 times of _nop_(), plus 4 (from LCALL and IRET) + 1 (from the parameter n),
	//so meet the 10*(n-1) +5 +4 +1 = 10*n us
// #elif 

// For the call nesting, be aware LCALL are counted for every nesting layer but ONLY one most inner IRET is counted
// for example, delay12us() {delay10us();} delay32us(){delay10us(); delay10us(); delay10us();} delay66us(){delay32us(); delay32us();} and so on
}

void delayX10usL(int n)
{
	data BYTE l = n & 0xff;
	data BYTE h = n >>8;

	if (h)
	{
		l -= 0x0f;
		if (l & 0xf0 == 0xf0)
			h--;
	}

	while (h--)
		delayX10us(0xff);
   
	delayX10us(l);
}
*/
