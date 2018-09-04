#include "BaseBoard.h"

// -----------------------------
// The keyboard via Wiegand26
// -----------------------------
sbit WG26INT =P3^3; // the interrupt pin: WG DATA0 and DATA1 connect to this pin thru a D each, 
sbit WG26D0  =P1^0; // the yellow pin of the keyboard
void wg26_read(void)  // interrupt 2  external interrupt  
{
	BYTE wg26msg[4];
	BYTE i=0, j=0, k=0;
	bit theBit =WG26D0;
	wg26msg[0] = wg26msg[1] = wg26msg[2] = wg26msg[3] = 0;
	EX1=0;

	for (j =0; j <100;)
	{
		for (k=0; !WG26INT && k< 200; k++)
		{
			theBit = WG26D0;
			delayX10us(1);
		}

		if (k>100) // take it as a noise
			break;

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
		for (i=0; WG26INT && i< 254; i++) // wait till the next signal
			delayX10us(2);

		theBit = WG26D0;

		if (i>253)
			break;
	}

	WG26INT =1;
	EX1=1;
}
