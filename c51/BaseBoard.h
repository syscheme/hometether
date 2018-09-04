#ifndef __BASEBOARD_H__
#define __BASEBOARD_H__

#include "defines.h"

#define ADC_CH_COUNT 8
#define SIG_CH_COUNT 16
#define SIG_CH_TYPES_COUNT 2

#define BOARD_SEQNO 0

#if BOARD_SEQNO == 0
sbit ANALOG_INH  = P2^4;	// the INH pin of Analog Switch CD4067, '0' to change the switch
sbit LATCH_LE    = P2^6;	// the LE pin of latch 74HS573, '1' to trigger latch
sbit ANALOG_SIG  = P1^0;	// the switched to the analog siginal thru CD4067

sbit ADC_CS      = P2^5;	// the CS pin of the ADC0838 convertor, '0' to select
sbit ADC_DI      = P1^2;	// the digital-in of ADC0838
sbit ADC_CLK     = P1^3;	// the CLK pin of the ADC0838
sbit ADC_SARS    = P1^4;	// the SARS pin of the ADC0838
sbit ADC_DO      = P1^5;	// the digital-out pin of the ADC0838

sbit IR_REV      = P3^3;	// pin to read IR receiver
sbit DE_485       = P3^6;   // <!!!!!!!!!!!!!!!!!!  be careful that DE_485 is a jump line after PCB

#endif // BOARD_SEQNO

// -----------------------------
// Declarition of global variables
// -----------------------------
extern BYTE latch_byte;
extern WORD bin_states;
extern xdata BYTE ADC_channels[ADC_CH_COUNT]; // the ADC channels
extern xdata WORD sigChData[SIG_CH_COUNT]; // the signal channels

// -----------------------------
// Declarition of major functions
// -----------------------------
void initBoard(void);

// 1) about the 8ch latch
void setLatch(BYTE newStates);

// 2) about the 16ch switch
// control the CD4067 to link the ANALOG_SIG to the Channel(chID)
void openAnalogSignal(BYTE chID);
// disable the CD4067 to on ANALOG_SIG
void closeAnalogSignals(void);

// 2.1) about the temperture sensor DS18B20
int readDS18B20(BYTE chID);

// 2.2) about the DHT-11 integrated temperture and humidity sensor
//@ return WORD: high-BYTE humidity in percent; low-byte tempture in Celsius degree
WORD readDHT11(BYTE chID);

// 2.3) about the IR sender
void sendIR(BYTE chID);

// 3) about the 8 ch AD converter
// perform AD covertion on channel(chID), chID=[0, 7]
void refreshADC0838ch(BYTE chID);
// perform AD covertion on all the channels
void scanADC0838();

// 4) about the RS485 communication
void set485Direction(bit recv);
BYTE readComByte(void);
void writeComByte(BYTE value);
WORD readComWord(void);
void writeComWord(WORD value);

#endif // __BaseBoard_H__
