#ifndef __BSP_H__
#define __BSP_H__

#include "stm32_RJ45_4x2.h"
#include "../bsp_dq.h"

#ifndef WITH_UCOSII
// make a dummy CPU_CRITICAL enter/leave
#define CPU_CRITICAL_ENTER() 
#define CPU_CRITICAL_EXIT() 
#endif // WITH_UCOSII

// =========================================================================
// BSP funcs
// =========================================================================
void  BSP_IntDisAll (void);
void BSP_Init(void);
uint32_t BSP_CPU_ClkFreq (void);

//INT32U  OS_CPU_SysTickClkFreq (void);

#ifdef  DEBUG
void assert_failed(u8* file, u32 line);
#endif

// #define CAN_LOOPBACK

#define RCC_APB2Periph_Devices ( RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM8 | RCC_APB2Periph_ADC1 \
	| RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE)

#define RCC_APB1Periph_Devices ( RCC_APB1Periph_USART3 | RCC_APB1Periph_CAN1 | RCC_APB1Periph_SPI2)

#define RCC_AHBPeriph_Devices  (RCC_AHBPeriph_DMA1)

#define	setIoPinH(_X_PIN) \
     GPIO_SetBits(_X_PIN->port, _X_PIN->pin)
#define	setIoPinL(_X_PIN) \
     GPIO_ResetBits(_X_PIN->port, _X_PIN->pin)
#define sendPulse(_X_PIN, _durH, _durL) \
     { setIoPinH(_X_PIN); delayXusec(_durH); setIoPinL(_X_PIN); delayXusec(_durL); }

// =========================================================================
// External IO Resources
// =========================================================================
#define CHANNEL_SZ_LUMIN       (8)
#define CHANNEL_SZ_DS18B20     (8)
#define CHANNEL_SZ_MOTION     (10)
#define CHANNEL_SZ_IrLED       (4)
#define CHANNEL_SZ_IrRecv      (4)
#define CHANNEL_SZ_Relay       (4)

extern DS18B20      ds18b20s[CHANNEL_SZ_DS18B20];
extern uint16_t*    adc_table;
const static OneWire OneWireBus[CHANNEL_SZ_DS18B20];
extern const IO_PIN IrRecvs[CHANNEL_SZ_IrRecv];

extern const uint8_t  MyIP[4];
extern const uint16_t MyServerPort;
extern const uint8_t  GroupIP[4];
extern const uint16_t GroupPort;

extern USART RS232;

uint16_t MotionState(void);
void ECU_setRelay(uint8_t id, uint8_t on);
void ECU_blinkLED(uint8_t id, uint8_t dsecEbOn, uint8_t dsecEbOff);

// per profile IrSend()s
void IrSend_PT2262(uint8_t chId, uint32_t code);
void IrSend_uPD6121G(uint8_t chId, uint16_t customCode, uint8_t data);

#endif // __BSP_H__
