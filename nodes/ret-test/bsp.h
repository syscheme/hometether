#ifndef __BSP_H__
#define __BSP_H__

#define CCP_NUM   2

#define LUMIN_ADC         1
#define nRF24L01_ADAPTED
#define ADC_VAL_MAX      (4096) // 12bit ADC

#ifdef LUMIN_ADC
#  define ADC_CHS        (2)
#  define  luminVal      (*(((uint16_t*) ADC_DMAbuf)+1))
#else
#  define ADC_CHS        (1)
#endif

#include  "ds18b20.h"
#include  "nrf24l01.h"
#include  "pulses.h"
#include "si4432.h"

extern uint32_t BSP_nodeId;
extern nRF24L01 nrf24l01;
extern SI4432   si4432;

void  BSP_Init(void);
void  BSP_IntInit(void);
void  BSP_IntDisAll(void);
void  BSP_Start(void);

void RNode_reset(void);


#ifdef  DEBUG
void assert_failed(u8* file, u32 line);
void debug(void);
#endif

// onboard resources
extern DS18B20  ds18b20;
extern nRF24L01 nrf24l01;
extern PulseCapture pulsechs[CCP_NUM];

// extern PulseCapture pch0, pch1;

#define  chipTemperature  (*((uint16_t*) ADC_DMAbuf))

uint8_t motionState(void);
uint8_t irRecvBit(void);

enum
{
	CanBaud_10Kbps,
	CanBaud_20Kbps,
	CanBaud_50Kbps,
	CanBaud_100Kbps,
	CanBaud_125Kbps,
	CanBaud_250Kbps,
	CanBaud_500Kbps,
	CanBaud_800Kpbs,
	CanBaud_1Mbps
};

uint8_t CAN_setBaud(uint8_t baud);
void RNode_blink(uint8_t mode, uint8_t reloadA, uint8_t reloadB, uint8_t blinks);
uint8_t RNode_indexByExtiLine(uint32_t extiLine);
uint8_t RNode_readPlusePinStatus(void);

#endif // __BSP_H__
