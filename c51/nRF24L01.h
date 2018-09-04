#ifndef __nRF24L01_H__
#define __nRF24L01_H__

#include "configures.h"


/*
// -------------------------------------------------------------------------------------------------
//                   FUNCTION's PROTOTYPES  //
// -------------------------------------------------------------------------------------------------
 void SPI_Init(BYTE Mode);     // Init HW or SW SPI
*/

// Function: TX_Mode();
// Description:
//   This function initializes one nRF24L01 device to TX mode, set TX address, set RX address for auto.ack,
//   fill TX payload, select RF channel, datarate & TX pwr. PWR_UP is set, CRC(2 bytes) is enabled, & PRIM:TX.
// ToDo:
//   One high pulse(>10us) on CE will now send this packet and expext an acknowledgment from the RX device.
void TX_Mode(void);

// Function: RX_Mode();
// Description:
//   This function initializes one nRF24L01 device to RX Mode, set RX address, writes RX payload width,
//   select RF channel, datarate & LNA HCURR. After init, CE is toggled high, which means that
//   this device is now ready to receive a datapacket.
void RX_Mode(void);

// Function: SPI_RW_Reg();
// Description:
//   Writes value 'value' to register 'reg'
BYTE SPI_RW(BYTE byte);                                // Single SPI read/write

// Function: SPI_Read();
// Description:
//   Read one byte from nRF24L01 register, 'reg'
BYTE SPI_Read(BYTE reg);                               // Read one byte from nRF24L01

// Function: SPI_RW_Reg();
// Description:
//   Writes value 'value' to register 'reg'
BYTE SPI_RW_Reg(BYTE reg, BYTE byte);                  // Write one byte to register 'reg'

// Function: SPI_Write_Buf();
// Description:
//   Writes contents of buffer '*pBuf' to nRF24L01. Typically used to write TX payload, Rx/Tx address
BYTE SPI_Write_Buf(BYTE reg, BYTE *pBuf, BYTE bufLen);  // Writes multiply bytes to one register

// Function: SPI_Read_Buf();
// Description:
//    Reads 'bytes' #of bytes from register 'reg'. Typically used to read RX payload, Rx/Tx address
BYTE SPI_Read_Buf(BYTE reg, BYTE *pBuf, BYTE bufLen);   // Read multiply bytes from one register

 #endif  // __nRF24L01_H__

