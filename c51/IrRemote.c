#include "BaseBoard.h"
#include "proto.h"
#define setIR() ANALOG_SIG =0;
#define resetIR() ANALOG_SIG =1;

BYTE* irData = InMsgData +2;
idata BYTE tmp, Ldelay, i, j;

void readIr_M50560()
{
//BYTE codebyte, BYTE databyte
//TODO:
}

void sendIr_M50560()
{
// BYTE codebyte, BYTE databyte
//TODO:
//	return 0;
}

void readIr_u6121(void)
{
// WORD* codeword, BYTE* databyte
//TODO:
}

void sendIr_u6121(void)
{
	// step 0 reset the IR and hold to ensure the clearnace
	resetIR();  delayXms(10);

	// step 1 sending the leading signal
	setIR();  delayXms(9); resetIR();  delayXms(4); 

	// step 2 sending the 4 bytes data
	for (i=0; i< 4; i++)
	{
		tmp = irData[i];
		for(j =0; j<8; j++, tmp<<=1)
		{
			Ldelay = (tmp & 0x1) ? 112 : 56;
			setIR(); delayX10us(55); resetIR(); delayX10us(Ldelay);
		}
	}

	// step 3 sending the ending signal
	setIR(); delayX10us(55); resetIR(); delayXms(2);
}

void readIr_tc9012(void)
{
// WORD* codeword, BYTE* databyte
//TODO:
}

void sendIr_tc9012(void)
{
//WORD codeword, BYTE databyte
//codeword &=0x1FFF; // 13bit user-code

//TODO:
}

void readIr_m3004(void)
{
// BYTE* codebyte, BYTE* databyte
//TODO:
}

void sendIr_m3004(void)
{
//BYTE codebyte, BYTE databyte
//TODO:
}

// ==================

IrIO code IrIOs[] = {
	{Ir_M50560,      readIr_M50560,   sendIr_M50560},
	{Ir_u6121,       readIr_u6121,    sendIr_u6121},
	{Ir_tc9012,      readIr_tc9012,   sendIr_tc9012},
	{Ir_m3004,       readIr_m3004,    sendIr_m3004},
	NULL};


