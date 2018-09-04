#define OFS_FREQ    12000 // 12MHz
#define BOARD_SEQNO 0

#include "BaseBoard.h"
#include "LY51Utils.h"

BYTE latch_byte=0xFF;
WORD bin_states=0xFF;
BYTE xdata ADC_channels[ADC_CH_COUNT]; // the ADC channels
WORD xdata sigChData[SIG_CH_COUNT]; // the signal channels

// -----------------------------
// Implementations
// -----------------------------
void setLatch(BYTE newStates)
{
	P0 = latch_byte = newStates;
	LATCH_LE = 1;
	delayIOSet();
	LATCH_LE = 0;
}

// control the CD4067 to link the ANALOG_SIG to the Channel(chID)
void openAnalogSignal(BYTE chID)
{
	chID = (P2 & 0XF0) | (chID & 0X0F); // value to set to P2
	ANALOG_INH = 1; delayIOSet(); // disable CD4067 first, then update P2
	P2 = chID;
	ANALOG_INH = 0; delayIOSet(); // enable CD4067
}

// disable the CD4067 to on ANALOG_SIG
void closeAnalogSignals(void)
{
	ANALOG_INH = 1; delayIOSet();
}

// perform AD covertion on channel(chID), chID=[0, 7]
code BYTE MUXs[] = { 0x18, 0x1c, 0x19, 0x1d, 0x1a, 0x1e, 0x1b, 0x1f }; //channels at mono mode (1->sgl->odd->sel1->sel0):
//                   11000b, 11100b, 11001b, 11101b, 11010b, 11110b, 11011b, 11111b
#define ADC_MASK 0x10

// reference test result: Vref=Vdd, Vdd->LightRist->TestPoint->10K,1%->GND,
//     the range would be like: nature-night-light 0~4, lamp 130~180, sunlight >200
void refreshADC0838ch(BYTE chID)
{
	BYTE i, tmp;
	chID &= 0x7;

	ADC_channels[chID] = 0x00;

	// initialize the chip
	ADC_CS = 1;
	ADC_DI = 1;
	ADC_SARS = 1;
	ADC_CS = 0;
	ADC_CLK = 0;

	ADC_SARS =0;
    tmp = MUXs[chID];
	for (i =0; i < 5; i++)
	{
		if (tmp & ADC_MASK)
			ADC_DI =1;			
		else
			ADC_DI =0;

		tmp <<=1;
		
		delayIOSet();

		ADC_CLK = 1; delayIOSet();

		ADC_CLK = 0;
	}

	ADC_SARS =1;
	ADC_CLK =0; delayIOSet();
	ADC_CLK =1; delayIOSet();

	// read 8 bit from ADC via MSB-first
	for (i=8; i>0; i--)
	{
		ADC_CLK =0; delayIOSet();
	
		ADC_channels[chID]<<=1;
		if (ADC_DO)
			ADC_channels[chID] |=0x01;

		ADC_CLK =1; delayIOSet();
	}

	ADC_SARS =0;
	for (i=8; i>0; i--)
	{
		ADC_CLK =0; delayIOSet();
	
		ADC_CLK =1; delayIOSet();
	}
	ADC_SARS =1;
	ADC_CLK =0;

	ADC_CS = 1;
	
	delayIOSet();
}

// perform AD covertion on all the channels
void scanADC0838()
{
	BYTE i;
	for (i=0; i<ADC_CH_COUNT; i++)
		refreshADC0838ch(i);
}

void writeDS18B20byte(BYTE value)   
{
	BYTE j;

	for(j=0; j<8; j++)
	{
		if (value & 0x01)
		{ // write the bit=1
			ANALOG_SIG=0;
			delayIOSet(); // ensure L remain greater than 1us but must reset to H before 15us
			ANALOG_SIG=1;
			delayX10us(2);
		}
		else
		{
			// write the bit=0;
			ANALOG_SIG=0;
			delayX10us(6); // ensure L remain greater than 60usec
			ANALOG_SIG=1;
			delayIOSet();
		}

		value >>= 1;
	}
}

BYTE readDS18B20byte(void)
{
	BYTE i, j, value=0;

	for(i=0; i<8; i++)
	{
		value >>= 1;

		ANALOG_SIG=0;
		delayIOSet(); // delay a bit to ensure the L remains greater than 1us
		ANALOG_SIG=1; // reset the line to H
		delayX10us(1); // the DS18B20 becomes valid after 15usec since the L

		j= ANALOG_SIG;
		value |= (j<<7);

		delayX10us(1); // yield for the next bit
	}

	return value;
}

int readDS18B20(BYTE chID) // unit = 0.1 degree
{
	BYTE a =0x00, b=0x00;
	int wTemp;
//	float idata temperature =0.0;

	openAnalogSignal(chID);

	// step 1. reset DS18B20
	ANALOG_SIG=0;
	delayX10us(50);
	ANALOG_SIG=1;
	delayX10us(50);
	
	// step 2. send the byte 0xcc and 0xbe
	writeDS18B20byte(0xcc);
	writeDS18B20byte(0xbe);

	// step 3 read the value
	a = readDS18B20byte(); // the low byte
	b = readDS18B20byte(); // the high byte

	// step 4. calcuate the true temperature value
	// step 4.1 comb into a 16bit integter
	wTemp = b;
	wTemp <<=8;
	wTemp |= a;

	// step 4.2 every 1 means 1/16 degree in C, convert it to dec via wTemp = Round(wTemp/16 *10 +0.5)
	wTemp = wTemp *10 +8;
	wTemp >>=4;

	// step 5. return the result
	return wTemp; // the return value is in 0.1 degree, for example, 238 mean 23.8C
}

//read the DHT-11 integrated temperture and humidity sensor
//@ return WORD: high-BYTE humidity in percent; low-byte tempture in Celsius degree
WORD readDHT11(BYTE chID)
{
	WORD ret;
	BYTE i, j, n, v[5];
	v[0] = v[1] = v[2] = v[3] =0;

	openAnalogSignal(chID);

	//step 1. the start signal:
	ANALOG_SIG =0; delayXms(18); // keep L no less than 18msec
	ANALOG_SIG =1; // reset to H
	delayX10us(4);

	//step 2. test the output of DHT11
	ANALOG_SIG =1; // test if DHT11 is outputing
    if (ANALOG_SIG)
		return 0xFFFF;

	// test the 80us DHT11's L output
	for (i=2; !ANALOG_SIG && i; i++) delayIOSet();
	if (!i)
		return 0xFFFF;

	// test the 80us DHT11's H output
	for (i=2; ANALOG_SIG && i; i++) delayIOSet();
	if (!i)
		return 0xFFFF;

	// step3. start reading the five bytes: 0-humidity, 1-humidity decimals, 2-temperture, 3-temperture decimals, 4-checksum 
	for (n=0; n<5; n++)
	{
		for (j=0; j<8; j++)
		{
			v[n] <<=1;
			for (i=3; !ANALOG_SIG && i; i++) delayIOSet();
			delayX10us(3);
			if (ANALOG_SIG)
			{
				v[n] |= 1;
				for (i=1; ANALOG_SIG && i; i++) delayIOSet();
			}
		}
	}

	// step4. validate the checksum
	i = v[0] + v[1] + v[2] + v[3];
	if (v[4] != i)
		return 0xFFFF;
	
	// step5. compose the return WORD
	ret = v[0];
	ret <<=8;
	ret |= v[2];

	// yield and reset
	delayXms(1);
	ANALOG_SIG =1; delayIOSet(); // reset to H
	return ret;
}

void sendIR(BYTE chID)
{
	openAnalogSignal(chID);
	// TODO: impl the IR sending
	closeAnalogSignals();
}

void set485Direction(bit recv)
{
	if (DE_485 != recv)
	{
		DE_485 = recv;
		delayIOSet();
	}
}

BYTE readComByte(void)
{
	while(!RI);
	RI=0;
	return SBUF;	
}

void writeComByte(BYTE value)
{
	SBUF = value;
	while(!TI); // wait for the sending gets completed
	TI = 0; // reset TI
}

WORD readComWord(void)
{
	idata WORD value=0;
	idata BYTE *p = (BYTE*) &value;

	*p++ = readComByte();
	*p = readComByte();

	return value;
}

void writeComWord(WORD value)
{
	idata BYTE *p = (BYTE*) &value;

	writeComByte(*p++);
	writeComByte(*p);
}


void initBoard(void)
{
	BYTE i;
	// step 1. reset the address or chip selections
	P2         = 0XF0;
	ADC_CS     = 1; // unselect ADC converter
	ANALOG_INH = 1; // lock the switch

	// step 2. reset the latch to 0xFF
	P0 = 0XFF;
	LATCH_LE = 1;
	LATCH_LE = 0;

	// step 3. reset the 
	P1 = 0x7F;

	// step 4. reset the data
	for (i=0; i<ADC_CH_COUNT; i++)
		ADC_channels[i] = 0x00;

	set485Direction(1);
}


