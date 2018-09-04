#ifndef __BSP_H__
#define __BSP_H__

//------------------------------
// RJ45 connectivities
//------------------------------
// Pin1 -TX	       Cable201503- OrangeWhite
// Pin2 -RX		   Cable201503- Orange
// Pin3 -3v3	   Cable201503- GreenWhite
// Pin4 -GND	   Cable201503- Blue
// Pin5 -DS18B20   Cable201503- BlueWhite
// Pin6 -ADC, 22K any-to-GND, 5K veranda-to-GND	Cable201503- Green
// Pin7,8 - Valve 12v  Cable201503- Brown/BwW

#define CCP_NUM   1

#include  "ds18b20.h"
#include  "pulses.h"
#include  "si4432.h"
#include  "usart.h"

#define LUMIN_ADC          1
#define ADC_VALUE_MAX      (4096) // 12bit ADC
#define FIFO_LEN_USART     (32)

#define DS18B20_MAX        (4)

// onboard resources
extern uint32_t BSP_nodeId;
extern PulseCapture pulsechs[CCP_NUM];
extern SI4432   si4432;
extern DS18B20  ds18b20s[DS18B20_MAX];
extern int16_t  temperatures[DS18B20_MAX];
extern uint8_t  cTemperatures;

extern USART COM1;
extern FIFO rxRF;

#define ADC_CHS            (2)
#define ADCVAL_WaterFlow   (*(((uint16_t*) ADC_DMAbuf)+1))
#define ADCVAL_innerTemp   (*((uint16_t*) ADC_DMAbuf))
extern uint16_t ADC_DMAbuf[ADC_CHS*2];

void  BSP_Init(void);
void  BSP_IntInit(void);
void  BSP_IntDisAll(void);
void  BSP_Start(void);

void  RNode_reset(void);

#ifdef  DEBUG
#define USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line);
void debug(void);
#endif


// extern PulseCapture pch0, pch1;

void    RNode_blink(uint8_t mode, uint8_t reloadA, uint8_t reloadB, uint8_t blinks);
uint8_t RNode_indexByExtiLine(uint32_t extiLine);
uint8_t RNode_readPlusePinStatus(void);



#endif // __BSP_H__
