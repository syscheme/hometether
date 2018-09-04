#include "LY51Utils.h"

code BYTE CodeTube_digits[] = { 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71,0x00 }; // "0123456789AbCdEF<Empty>", 0x80=dot

void display(unsigned char *codebuf, unsigned char len)
{
	unsigned char i;
	len = min(len, 8);
	P2=0; // reset the output port
	P1=P1 & 0xF8; // reset the low 3bits of address to 138 to point to the first code tube
	for(i=0; i< len; i++)
	{		// display each code tube
			P2 = CodeTube_digits[codebuf[i] & 0x7F]; // translate the value to the display code from the table
			P2 |= codebuf[i] & 0x80; // turn on the dot if necessary
			delayIOSet(); delayIOSet(); delayIOSet();	// delay a bit to display the current cube
			P2=0;	// reset the code, then move to the next cube
			P1++;
	}
}


