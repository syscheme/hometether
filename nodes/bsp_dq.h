#ifndef __BSP_DQ_H__
#define __BSP_DQ_H__

#include "htcomm.h"
#include "usart.h"

// =========================================================================
// BSP funcs
// =========================================================================
#define RCC_APB2Periph_Devices ( RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM8 | RCC_APB2Periph_ADC1 \
	| RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE)

#define RCC_APB1Periph_Devices ( RCC_APB1Periph_USART3 | RCC_APB1Periph_CAN1 | RCC_APB1Periph_SPI2)

#define RCC_AHBPeriph_Devices  (RCC_AHBPeriph_DMA1)

// =========================================================================
// Onboard IO Resources
// =========================================================================
void  DQBoard_init(void);
void  DQBoard_reset(void);
void  DQBoard_Timer_config(void);

void DQBoard_beep(uint16_t reload, uint16_t msec);
void     DQBoard_setLed(uint8_t Id, uint8_t on); // i.e. DQBoard_setLed(0~3, 1) to turn on led 0~3, DQBoard_setLed(0~3, 0) to turn off the led
void     DQBoard_485sendMode(uint8_t sendMode);
uint32_t DQBoard_updateClock(uint32_t newTime);

extern USART RS232;
extern USART RS485;

// =========================================================================
// Interrupts Definition
// =========================================================================
enum {
	BSP_INT_ID_WWDG,                 // Window WatchDog Interrupt                           
	BSP_INT_ID_PVD,                  // PVD through EXTI Line detection Interrupt           
	BSP_INT_ID_TAMPER,               // Tamper Interrupt                                    
	BSP_INT_ID_RTC,                  // RTC global Interrupt                                
	BSP_INT_ID_FLASH,                // FLASH global Interrupt                              
	BSP_INT_ID_RCC,                  // RCC global Interrupt                                
	BSP_INT_ID_EXTI0,                // EXTI Line0 Interrupt                                
	BSP_INT_ID_EXTI1,                // EXTI Line1 Interrupt                                
	BSP_INT_ID_EXTI2,                // EXTI Line2 Interrupt                                
	BSP_INT_ID_EXTI3,                // EXTI Line3 Interrupt                                
	BSP_INT_ID_EXTI4,                // EXTI Line4 Interrupt                                
	BSP_INT_ID_DMA1_CH1,             // DMA1 Channel 1 global Interrupt                     
	BSP_INT_ID_DMA1_CH2,             // DMA1 Channel 2 global Interrupt                     
	BSP_INT_ID_DMA1_CH3,             // DMA1 Channel 3 global Interrupt                     
	BSP_INT_ID_DMA1_CH4,             // DMA1 Channel 4 global Interrupt                     
	BSP_INT_ID_DMA1_CH5,             // DMA1 Channel 5 global Interrupt                     
	BSP_INT_ID_DMA1_CH6,             // DMA1 Channel 6 global Interrupt                     
	BSP_INT_ID_DMA1_CH7,             // DMA1 Channel 7 global Interrupt                     
	BSP_INT_ID_ADC1_2,               // ADC1 et ADC2 global Interrupt                       
	BSP_INT_ID_USB_HP_CAN_TX,        // USB High Priority or CAN TX Interrupts              
	BSP_INT_ID_USB_LP_CAN_RX0,       // USB Low Priority or CAN RX0 Interrupts              
	BSP_INT_ID_CAN_RX1,              // CAN RX1 Interrupt                                   
	BSP_INT_ID_CAN_SCE,              // CAN SCE Interrupt                                   
	BSP_INT_ID_EXTI9_5,              // External Line[9:5] Interrupts                       
	BSP_INT_ID_TIM1_BRK,             // TIM1 Break Interrupt                                
	BSP_INT_ID_TIM1_UP,              // TIM1 Update Interrupt                               
	BSP_INT_ID_TIM1_TRG_COM,         // TIM1 Trigger and Commutation Interrupt              
	BSP_INT_ID_TIM1_CC,              // TIM1 Capture Compare Interrupt                      
	BSP_INT_ID_TIM2,                 // TIM2 global Interrupt                               
	BSP_INT_ID_TIM3,                 // TIM3 global Interrupt                               
	BSP_INT_ID_TIM4,                 // TIM4 global Interrupt                               
	BSP_INT_ID_I2C1_EV,              // I2C1 Event Interrupt                                
	BSP_INT_ID_I2C1_ER,              // I2C1 Error Interrupt                                
	BSP_INT_ID_I2C2_EV,              // I2C2 Event Interrupt                                
	BSP_INT_ID_I2C2_ER,              // I2C2 Error Interrupt                                
	BSP_INT_ID_SPI1,                 // SPI1 global Interrupt                               
	BSP_INT_ID_SPI2,                 // SPI2 global Interrupt                               
	BSP_INT_ID_USART1,               // USART1 global Interrupt                             
	BSP_INT_ID_USART2,               // USART2 global Interrupt                             
	BSP_INT_ID_USART3,               // USART3 global Interrupt                             
	BSP_INT_ID_EXTI15_10,            // External Line[15:10] Interrupts                     
	BSP_INT_ID_RTCAlarm,             // RTC Alarm through EXTI Line Interrupt               
	BSP_INT_ID_USBWakeUp,            // USB WakeUp from suspend through EXTI Line Interrupt 
	BSP_INT_ID_TIM8_BRK,             // TIM8 Break Interrupt                                
	BSP_INT_ID_TIM8_UP,              // TIM8 Update Interrupt                               
	BSP_INT_ID_TIM8_TRG_COM,         // TIM8 Trigger and Commutation Interrupt              
	BSP_INT_ID_TIM8_CC,              // TIM8 Capture Compare Interrupt                      
	BSP_INT_ID_ADC3,                 // ADC3 global Interrupt                               
	BSP_INT_ID_FSMC,                 // FSMC global Interrupt                               
	BSP_INT_ID_SDIO,                 // SDIO global Interrupt                               
	BSP_INT_ID_TIM5,                 // TIM5 global Interrupt                               
	BSP_INT_ID_SPI3,                 // SPI3 global Interrupt                               
	BSP_INT_ID_UART4,                // UART4 global Interrupt                              
	BSP_INT_ID_UART5,                // UART5 global Interrupt                              
	BSP_INT_ID_TIM6,                 // TIM6 global Interrupt                               
	BSP_INT_ID_TIM7,                 // TIM7 global Interrupt                               
	BSP_INT_ID_DMA2_CH1,             // DMA2 Channel 1 global Interrupt                     
	BSP_INT_ID_DMA2_CH2,             // DMA2 Channel 2 global Interrupt                     
	BSP_INT_ID_DMA2_CH3,             // DMA2 Channel 3 global Interrupt                     
	BSP_INT_ID_DMA2_CH4_5,           // DMA2 Channel 4 and DMA2 Channel 5 global Interrupt  

	BSP_INT_SRC_NBR
};                                       

// =========================================================================
// Onboard beeper via PWM
// =========================================================================
typedef enum {
	low_1 =55042, // 261.6Hz
	low_2 =49037,     
	low_3 =43687,     
	low_4 =41235,     
	low_5 =36735,     
	low_6 =32728,     
	low_7 =29157,
	mid_1 =27520, // 523.3Hz
	mid_2 =24519,     
	mid_3 =21843,     
	mid_4 =20617,     
	mid_5 =18368,     
	mid_6 =16364,     
	mid_7 =14579,
	high_1 =13761,   
	high_2 =12259,   
	high_3 =10922,   
	high_4 =10309,   
	high_5 =9184, 
	high_6 =8182,   
	high_7 =7246,
} PitchReload;

typedef struct _Syllable
{
	uint16_t pitchReload;
	uint8_t  lenByQuarter;
} Syllable;

extern const Syllable startSong[];

// =========================================================================
// Onboard CAN bus
// =========================================================================
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

#endif // __BSP_DQ_H__
