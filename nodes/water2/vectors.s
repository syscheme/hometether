;******************************************************************************
;
;                             INTERRUPT VECTORS
;                                    ARM
;                             KEIL's uVision3 
;                   (RealView Microprocessor Developer Kit)
;
; Filename      : vectors.s
;******************************************************************************

                PRESERVE8
                AREA   VECT, CODE, READONLY                     ; Name this block of code                                   ;
                THUMB

                ENTRY

;******************************************************************************
;                                  IMPORTS
;******************************************************************************

		        IMPORT	SysTick_IRQHandler
		        IMPORT	PendSV_IRQHandler

				IMPORT WWDG_IRQHandler
				IMPORT PVD_IRQHandler
				IMPORT TAMPER_IRQHandler
				IMPORT RTC_IRQHandler
				IMPORT FLASH_IRQHandler
				IMPORT RCC_IRQHandler
				IMPORT EXTI0_IRQHandler
				IMPORT EXTI1_IRQHandler
				IMPORT EXTI2_IRQHandler
				IMPORT EXTI3_IRQHandler
				IMPORT EXTI4_IRQHandler
				IMPORT DMA1_CH1_IRQHandler
				IMPORT DMA1_CH2_IRQHandler
				IMPORT DMA1_CH3_IRQHandler
				IMPORT DMA1_CH4_IRQHandler
				IMPORT DMA1_CH5_IRQHandler
				IMPORT DMA1_CH6_IRQHandler
				IMPORT DMA1_CH7_IRQHandler
				IMPORT ADC_IRQHandler
				IMPORT USB_HP_CAN_TX_IRQHandler
				IMPORT USB_LP_CAN_RX0_IRQHandler
				IMPORT CAN_RX1_IRQHandler
				IMPORT CAN_SCE_IRQHandler
				IMPORT EXTI9_5_IRQHandler
				IMPORT TIM1_BRK_IRQHandler
				IMPORT TIM1_UP_IRQHandler
				IMPORT TIM1_TRG_COM_IRQHandler
				IMPORT TIM1_CC_IRQHandler
				IMPORT TIM2_IRQHandler
				IMPORT TIM3_IRQHandler
				IMPORT TIM4_IRQHandler
				IMPORT I2C1_EV_IRQHandler
				IMPORT I2C1_ER_IRQHandler
				IMPORT I2C2_EV_IRQHandler
				IMPORT I2C2_ER_IRQHandler
				IMPORT SPI1_IRQHandler
				IMPORT SPI2_IRQHandler
				IMPORT USART1_IRQHandler
				IMPORT USART2_IRQHandler
				IMPORT USART3_IRQHandler
				IMPORT EXTI15_10_IRQHandler
				IMPORT RTCAlarm_IRQHandler
				IMPORT USBWakeUp_IRQHandler

                IMPORT  ResetHndlr
                IMPORT  ||Image$$ARM_LIB_STACK$$ZI$$Limit||     ; Import stack limit from scatter-loading file              ;

;******************************************************************************
;                                  EXPORTS
;******************************************************************************


;******************************************************************************
;                                DEFINITIONS
;******************************************************************************


;******************************************************************************
;                      INITIALIZE EXCEPTION VECTORS
;******************************************************************************

Vectors
        DCD     ||Image$$ARM_LIB_STACK$$ZI$$Limit||         ;  0, SP start value.                                         
        DCD     ResetHndlr                                  ;  1, PC start value.                                         
        DCD     App_NMI_ISR                                 ;  2, NMI                                                     
        DCD     App_Fault_ISR                               ;  3, Hard Fault                                              
        DCD     App_MemFault_ISR                            ;  4, Memory Management                                      
        DCD     App_BusFault_ISR                            ;  5, Bus Fault                                               
        DCD     App_UsageFault_ISR                          ;  6, Usage Fault                                             
        DCD     0                                           ;  7, Reserved                                                
        DCD     0                                           ;  8, Reserved                                                
        DCD     0                                           ;  9, Reserved                                                
        DCD     0                                           ; 10, Reserved                                                
        DCD     App_Spurious_ISR                            ; 11, SVCall                                                  
        DCD     App_Spurious_ISR                            ; 12, Debug Monitor                                           
        DCD     App_Spurious_ISR                            ; 13, Reserved                                                
        DCD     PendSV_IRQHandler                        ; 14, PendSV Handler                                          
        DCD     SysTick_IRQHandler                       ; 15, uC/OS-II Tick ISR Handler
                                       
        DCD     WWDG_IRQHandler                          ; 16, INTISR[  0]  Window Watchdog.                   
        DCD     PVD_IRQHandler                           ; 17, INTISR[  1]  PVD through EXTI Line Detection.    
        DCD     TAMPER_IRQHandler                        ; 18, INTISR[  2]  Tamper Interrupt.                   
        DCD     RTC_IRQHandler                           ; 19, INTISR[  3]  RTC Global Interrupt.               
        DCD     FLASH_IRQHandler                         ; 20, INTISR[  4]  FLASH Global Interrupt.             
        DCD     RCC_IRQHandler                           ; 21, INTISR[  5]  RCC Global Interrupt.               
        DCD     EXTI0_IRQHandler                         ; 22, INTISR[  6]  EXTI Line0 Interrupt.               
        DCD     EXTI1_IRQHandler                         ; 23, INTISR[  7]  EXTI Line1 Interrupt.               
        DCD     EXTI2_IRQHandler                         ; 24, INTISR[  8]  EXTI Line2 Interrupt.               
        DCD     EXTI3_IRQHandler                         ; 25, INTISR[  9]  EXTI Line3 Interrupt.               
        DCD     EXTI4_IRQHandler                         ; 26, INTISR[ 10]  EXTI Line4 Interrupt.               
        DCD     DMA1_CH1_IRQHandler                      ; 27, INTISR[ 11]  DMA Channel1 Global Interrupt.      
        DCD     DMA1_CH2_IRQHandler                      ; 28, INTISR[ 12]  DMA Channel2 Global Interrupt.      
        DCD     DMA1_CH3_IRQHandler                      ; 29, INTISR[ 13]  DMA Channel3 Global Interrupt.      
        DCD     DMA1_CH4_IRQHandler                      ; 30, INTISR[ 14]  DMA Channel4 Global Interrupt.      
        DCD     DMA1_CH5_IRQHandler                      ; 31, INTISR[ 15]  DMA Channel5 Global Interrupt.      
        DCD     DMA1_CH6_IRQHandler                      ; 32, INTISR[ 16]  DMA Channel6 Global Interrupt.      
        DCD     DMA1_CH7_IRQHandler                      ; 33, INTISR[ 17]  DMA Channel7 Global Interrupt.      
        DCD     ADC_IRQHandler                           ; 34, INTISR[ 18]  ADC1 & ADC2 Global Interrupt.       
        DCD     USB_HP_CAN_TX_IRQHandler                 ; 35, INTISR[ 19]  USB High Prio / CAN TX  Interrupts. 
        DCD     USB_LP_CAN_RX0_IRQHandler                ; 36, INTISR[ 20]  USB Low  Prio / CAN RX0 Interrupts. 
        DCD     CAN_RX1_IRQHandler                       ; 37, INTISR[ 21]  CAN RX1 Interrupt.                  
        DCD     CAN_SCE_IRQHandler                       ; 38, INTISR[ 22]  CAN SCE Interrupt.                  
        DCD     EXTI9_5_IRQHandler                       ; 39, INTISR[ 23]  EXTI Line[9:5] Interrupt.           
        DCD     TIM1_BRK_IRQHandler                      ; 40, INTISR[ 24]  TIM1 Break  Interrupt.              
        DCD     TIM1_UP_IRQHandler                       ; 41, INTISR[ 25]  TIM1 Update Interrupt.              
        DCD     TIM1_TRG_COM_IRQHandler                  ; 42, INTISR[ 26]  TIM1 Trig & Commutation Interrupts. 
        DCD     TIM1_CC_IRQHandler                       ; 43, INTISR[ 27]  TIM1 Capture Compare Interrupt.     
        DCD     TIM2_IRQHandler                          ; 44, INTISR[ 28]  TIM2 Global Interrupt.              
        DCD     TIM3_IRQHandler                          ; 45, INTISR[ 29]  TIM3 Global Interrupt.              
        DCD     TIM4_IRQHandler                          ; 46, INTISR[ 30]  TIM4 Global Interrupt.              
        DCD     I2C1_EV_IRQHandler                       ; 47, INTISR[ 31]  I2C1 Event  Interrupt.              

        DCD     I2C1_ER_IRQHandler                       ; 48, INTISR[ 32]  I2C1 Error  Interrupt.              
        DCD     I2C2_EV_IRQHandler                       ; 49, INTISR[ 33]  I2C2 Event  Interrupt.              
        DCD     I2C2_ER_IRQHandler                       ; 50, INTISR[ 34]  I2C2 Error  Interrupt.             
        DCD     SPI1_IRQHandler                          ; 51, INTISR[ 35]  SPI1 Global Interrupt.              
        DCD     SPI2_IRQHandler                          ; 52, INTISR[ 36]  SPI2 Global Interrupt.              
        DCD     USART1_IRQHandler                        ; 53, INTISR[ 37]  USART1 Global Interrupt.            
        DCD     USART2_IRQHandler                        ; 54, INTISR[ 38]  USART2 Global Interrupt.            
        DCD     USART3_IRQHandler                        ; 55, INTISR[ 39]  USART3 Global Interrupt.            
        DCD     EXTI15_10_IRQHandler                     ; 56, INTISR[ 40]  EXTI Line [15:10] Interrupts.       
        DCD     RTCAlarm_IRQHandler                      ; 57, INTISR[ 41]  RTC Alarm EXT Line Interrupt.       
        DCD     USBWakeUp_IRQHandler                     ; 58, INTISR[ 42]  USB Wakeup from Suspend EXTI Int.   
        
        
;******************************************************************************
;                          DEFAULT HANDLERS
;******************************************************************************

App_NMI_ISR         B       App_NMI_ISR

App_Fault_ISR       B       App_Fault_ISR

App_MemFault_ISR    B       App_MemFault_ISR

App_BusFault_ISR    B       App_BusFault_ISR

App_UsageFault_ISR  B       App_UsageFault_ISR

App_Spurious_ISR    B       App_Spurious_ISR


                ALIGN
                END