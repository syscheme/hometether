#ifndef __1Wire_H__
#define __1Wire_H__

#include "htcomm.h"

typedef uint64_t OneWireAddr_t;
typedef struct _OneWire
{
	IO_PIN pin;
} OneWire;

typedef hterr (*OW_cbDeviceDetected_f)(const OneWire* hostPin, OneWireAddr_t devAddr); // terminate scanning if non-ERR_SUCCESS returned by callback

// ---------------------------------------------------------------------------
// methods of OneWire
// ---------------------------------------------------------------------------
uint8_t OneWire_scanForDevices(const OneWire* hostPin, OW_cbDeviceDetected_f cbDetected);

// Verify the device with the ROM number in ROM_NO buffer is present.
// Return TRUE  : device verified present
//        FALSE : device not present
BOOL    OneWire_Verify(const OneWire* hostPin, OneWireAddr_t devAddr);

uint8_t OneWire_readByte(const OneWire* hostPin);
void    OneWire_writeByte(const OneWire* hostPin, uint8_t byte_value);

// Reset the 1-Wire bus and return the presence of any device
// Return ERR_SUCESS  : device present
//        otherwise : no device present
hterr   OneWire_resetBus(const OneWire* hostPin);

// ---------------------------------------------------------------------------
// portal OneWire* per bsp
// ---------------------------------------------------------------------------

#endif  // __1Wire_H__


/*
// ---------------------------------------------------------------------------
// an sample at IO layer portal
// ---------------------------------------------------------------------------
void OWPortal_initIO(OneWire* hostPin)
{
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
 	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	OWReset(hostPin);
}

void OWPortal_dqH(OneWire* hostPin)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_1);
}

void OWPortal_dqL(OneWire* hostPin)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);
}

uint8_t OWPortal_dqV(OneWire* hostPin)
{
	return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1);
}
*/
