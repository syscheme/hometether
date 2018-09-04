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

		        IMPORT	IntHandler_SysTick
		        IMPORT	IntHandler_PendSV

		        IMPORT	IntHandler_WWDG
		        IMPORT	IntHandler_PVD
		        IMPORT	IntHandler_TAMPER
		        IMPORT	IntHandler_RTC
		        IMPORT	IntHandler_FLASH
		        IMPORT	IntHandler_RCC
		        IMPORT	IntHandler_EXTI0
		        IMPORT	IntHandler_EXTI1
		        IMPORT	IntHandler_EXTI2
		        IMPORT	IntHandler_EXTI3
		        IMPORT	IntHandler_EXTI4
		        IMPORT	IntHandler_DMA1_CH1
		        IMPORT	IntHandler_DMA1_CH2
		        IMPORT	IntHandler_DMA1_CH3
		        IMPORT	IntHandler_DMA1_CH4
		        IMPORT	IntHandler_DMA1_CH5

		        IMPORT	IntHandler_DMA1_CH6
		        IMPORT	IntHandler_DMA1_CH7
		        IMPORT	IntHandler_ADC1_2
		        IMPORT	IntHandler_USB_HP_CAN_TX
		        IMPORT	IntHandler_USB_LP_CAN_RX0
		        IMPORT	IntHandler_CAN_RX1
		        IMPORT	IntHandler_CAN_SCE
		        IMPORT	IntHandler_EXTI9_5
		        IMPORT	IntHandler_TIM1_BRK
		        IMPORT	IntHandler_TIM1_UP
		        IMPORT	IntHandler_TIM1_TRG_COM
		        IMPORT	IntHandler_TIM1_CC
		        IMPORT	IntHandler_TIM2
		        IMPORT	IntHandler_TIM3
		        IMPORT	IntHandler_TIM4
		        IMPORT	IntHandler_I2C1_EV

		        IMPORT	IntHandler_I2C1_ER
		        IMPORT	IntHandler_I2C2_EV
		        IMPORT	IntHandler_I2C2_ER
		        IMPORT	IntHandler_SPI1
		        IMPORT	IntHandler_SPI2
		        IMPORT	IntHandler_USART1
		        IMPORT	IntHandler_USART2
		        IMPORT	IntHandler_USART3 
		        IMPORT	IntHandler_EXTI15_10
		        IMPORT	IntHandler_RTCAlarm
		        IMPORT	IntHandler_USBWakeUp

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
        DCD     IntHandler_PendSV                        ; 14, PendSV Handler                                          
        DCD     IntHandler_SysTick                       ; 15, uC/OS-II Tick ISR Handler
                                       
        DCD     IntHandler_WWDG                          ; 16, INTISR[  0]  Window Watchdog.                   
        DCD     IntHandler_PVD                           ; 17, INTISR[  1]  PVD through EXTI Line Detection.    
        DCD     IntHandler_TAMPER                        ; 18, INTISR[  2]  Tamper Interrupt.                   
        DCD     IntHandler_RTC                           ; 19, INTISR[  3]  RTC Global Interrupt.               
        DCD     IntHandler_FLASH                         ; 20, INTISR[  4]  FLASH Global Interrupt.             
        DCD     IntHandler_RCC                           ; 21, INTISR[  5]  RCC Global Interrupt.               
        DCD     IntHandler_EXTI0                         ; 22, INTISR[  6]  EXTI Line0 Interrupt.               
        DCD     IntHandler_EXTI1                         ; 23, INTISR[  7]  EXTI Line1 Interrupt.               
        DCD     IntHandler_EXTI2                         ; 24, INTISR[  8]  EXTI Line2 Interrupt.               
        DCD     IntHandler_EXTI3                         ; 25, INTISR[  9]  EXTI Line3 Interrupt.               
        DCD     IntHandler_EXTI4                         ; 26, INTISR[ 10]  EXTI Line4 Interrupt.               
        DCD     IntHandler_DMA1_CH1                      ; 27, INTISR[ 11]  DMA Channel1 Global Interrupt.      
        DCD     IntHandler_DMA1_CH2                      ; 28, INTISR[ 12]  DMA Channel2 Global Interrupt.      
        DCD     IntHandler_DMA1_CH3                      ; 29, INTISR[ 13]  DMA Channel3 Global Interrupt.      
        DCD     IntHandler_DMA1_CH4                      ; 30, INTISR[ 14]  DMA Channel4 Global Interrupt.      
        DCD     IntHandler_DMA1_CH5                      ; 31, INTISR[ 15]  DMA Channel5 Global Interrupt.      

        DCD     IntHandler_DMA1_CH6                      ; 32, INTISR[ 16]  DMA Channel6 Global Interrupt.      
        DCD     IntHandler_DMA1_CH7                      ; 33, INTISR[ 17]  DMA Channel7 Global Interrupt.      
        DCD     IntHandler_ADC1_2                        ; 34, INTISR[ 18]  ADC1 & ADC2 Global Interrupt.       
        DCD     IntHandler_USB_HP_CAN_TX                 ; 35, INTISR[ 19]  USB High Prio / CAN TX  Interrupts. 
        DCD     IntHandler_USB_LP_CAN_RX0                ; 36, INTISR[ 20]  USB Low  Prio / CAN RX0 Interrupts. 
        DCD     IntHandler_CAN_RX1                       ; 37, INTISR[ 21]  CAN RX1 Interrupt.                  
        DCD     IntHandler_CAN_SCE                       ; 38, INTISR[ 22]  CAN SCE Interrupt.                  
        DCD     IntHandler_EXTI9_5                       ; 39, INTISR[ 23]  EXTI Line[9:5] Interrupt.           
        DCD     IntHandler_TIM1_BRK                      ; 40, INTISR[ 24]  TIM1 Break  Interrupt.              
        DCD     IntHandler_TIM1_UP                       ; 41, INTISR[ 25]  TIM1 Update Interrupt.              
        DCD     IntHandler_TIM1_TRG_COM                  ; 42, INTISR[ 26]  TIM1 Trig & Commutation Interrupts. 
        DCD     IntHandler_TIM1_CC                       ; 43, INTISR[ 27]  TIM1 Capture Compare Interrupt.     
        DCD     IntHandler_TIM2                          ; 44, INTISR[ 28]  TIM2 Global Interrupt.              
        DCD     IntHandler_TIM3                          ; 45, INTISR[ 29]  TIM3 Global Interrupt.              
        DCD     IntHandler_TIM4                          ; 46, INTISR[ 30]  TIM4 Global Interrupt.              
        DCD     IntHandler_I2C1_EV                       ; 47, INTISR[ 31]  I2C1 Event  Interrupt.              

        DCD     IntHandler_I2C1_ER                       ; 48, INTISR[ 32]  I2C1 Error  Interrupt.              
        DCD     IntHandler_I2C2_EV                       ; 49, INTISR[ 33]  I2C2 Event  Interrupt.              
        DCD     IntHandler_I2C2_ER                       ; 50, INTISR[ 34]  I2C2 Error  Interrupt.             
        DCD     IntHandler_SPI1                          ; 51, INTISR[ 35]  SPI1 Global Interrupt.              
        DCD     IntHandler_SPI2                          ; 52, INTISR[ 36]  SPI2 Global Interrupt.              
        DCD     IntHandler_USART1                        ; 53, INTISR[ 37]  USART1 Global Interrupt.            
        DCD     IntHandler_USART2                        ; 54, INTISR[ 38]  USART2 Global Interrupt.            
        DCD     IntHandler_USART3                        ; 55, INTISR[ 39]  USART3 Global Interrupt.            
        DCD     IntHandler_EXTI15_10                     ; 56, INTISR[ 40]  EXTI Line [15:10] Interrupts.       
        DCD     IntHandler_RTCAlarm                      ; 57, INTISR[ 41]  RTC Alarm EXT Line Interrupt.       
        DCD     IntHandler_USBWakeUp                     ; 58, INTISR[ 42]  USB Wakeup from Suspend EXTI Int.   
        
        
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