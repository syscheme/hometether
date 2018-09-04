;******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
;* File Name          : startup_stm32f10x_hd.s
;* Author             : MCD Application Team
;* Version            : V3.0.0
;* Date               : 04/06/2009
;* Description        : STM32F10x High Density Devices vector table for RVMDK 
;*                      toolchain. 
;*                      This module performs:
;*                      - Set the initial SP
;*                      - Set the initial PC == Reset_Handler
;*                      - Set the vector table entries with the exceptions ISR address
;*                      - Configure external SRAM mounted on STM3210E-EVAL board
;*                        to be used as data memory (optional, to be enabled by user)
;*                      - Branches to __main in the C library (which eventually
;*                        calls main()).
;*                      After Reset the CortexM3 processor is in Thread mode,
;*                      priority is Privileged, and the Stack is set to Main.
;* <<< Use Configuration Wizard in Context Menu >>>   
;*******************************************************************************
; THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
; WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
; AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
; INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
; CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
; INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
;*******************************************************************************

; Amount of memory (in bytes) allocated for Stack
; Tailor this value to your application needs
; <h> Stack Configuration
;   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Stack_Size      EQU     0x00000400

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

__initial_spTop EQU    0x20000400                 ; stack used for SystemInit_ExtMemCtl
                                                  ; always internal RAM used 
                                                  
; <h> Heap Configuration
;   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Heap_Size       EQU     0x00000000		   ; i don't have dynamic malloc in program

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

; imports
		        IMPORT	OS_CPU_SysTickHandler
		        IMPORT	OS_CPU_PendSVHandler

		        IMPORT	BSP_IntHandlerWWDG
		        IMPORT	BSP_IntHandlerPVD
		        IMPORT	BSP_IntHandlerTAMPER
		        IMPORT	BSP_IntHandlerRTC
		        IMPORT	BSP_IntHandlerFLASH
		        IMPORT	BSP_IntHandlerRCC
		        IMPORT	BSP_IntHandlerEXTI0
		        IMPORT	BSP_IntHandlerEXTI1
		        IMPORT	BSP_IntHandlerEXTI2
		        IMPORT	BSP_IntHandlerEXTI3
		        IMPORT	BSP_IntHandlerEXTI4
		        IMPORT	BSP_IntHandlerDMA1_CH1
		        IMPORT	BSP_IntHandlerDMA1_CH2
		        IMPORT	BSP_IntHandlerDMA1_CH3
		        IMPORT	BSP_IntHandlerDMA1_CH4
		        IMPORT	BSP_IntHandlerDMA1_CH5

		        IMPORT	BSP_IntHandlerDMA1_CH6
		        IMPORT	BSP_IntHandlerDMA1_CH7
		        IMPORT	BSP_IntHandlerADC1_2
		        IMPORT	BSP_IntHandlerUSB_HP_CAN_TX
		        IMPORT	BSP_IntHandlerUSB_LP_CAN_RX0
		        IMPORT	BSP_IntHandlerCAN_RX1
		        IMPORT	BSP_IntHandlerCAN_SCE
		        IMPORT	BSP_IntHandlerEXTI9_5
		        IMPORT	BSP_IntHandlerTIM1_BRK
		        IMPORT	BSP_IntHandlerTIM1_UP
		        IMPORT	BSP_IntHandlerTIM1_TRG_COM
		        IMPORT	BSP_IntHandlerTIM1_CC
		        IMPORT	BSP_IntHandlerTIM2
		        IMPORT	BSP_IntHandlerTIM3
		        IMPORT	BSP_IntHandlerTIM4
		        IMPORT	BSP_IntHandlerI2C1_EV

		        IMPORT	BSP_IntHandlerI2C1_ER
		        IMPORT	BSP_IntHandlerI2C2_EV
		        IMPORT	BSP_IntHandlerI2C2_ER
		        IMPORT	BSP_IntHandlerSPI1
		        IMPORT	BSP_IntHandlerSPI2
		        IMPORT	BSP_IntHandlerUSART1
		        IMPORT	BSP_IntHandlerUSART2
		        IMPORT	BSP_IntHandlerUSART3 
		        IMPORT	BSP_IntHandlerEXTI15_10
		        IMPORT	BSP_IntHandlerRTCAlarm
		        IMPORT	BSP_IntHandlerUSBWakeUp


                PRESERVE8
                THUMB


; Vector Table Mapped to Address 0 at Reset
                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_spTop           ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler
                DCD     NMI_Handler               ; NMI Handler
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     MemManage_Handler         ; MPU Fault Handler
                DCD     BusFault_Handler          ; Bus Fault Handler
                DCD     UsageFault_Handler        ; Usage Fault Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; SVCall Handler
                DCD     DebugMon_Handler          ; Debug Monitor Handler
                DCD     0                         ; Reserved
                DCD     OS_CPU_PendSVHandler      ; PendSV Handler
                DCD     OS_CPU_SysTickHandler     ; SysTick Handler

                ; External Interrupts
        DCD     BSP_IntHandlerWWDG                          ; 16, INTISR[  0]  Window Watchdog.                   
        DCD     BSP_IntHandlerPVD                           ; 17, INTISR[  1]  PVD through EXTI Line Detection.    
        DCD     BSP_IntHandlerTAMPER                        ; 18, INTISR[  2]  Tamper Interrupt.                   
        DCD     BSP_IntHandlerRTC                           ; 19, INTISR[  3]  RTC Global Interrupt.               
        DCD     BSP_IntHandlerFLASH                         ; 20, INTISR[  4]  FLASH Global Interrupt.             
        DCD     BSP_IntHandlerRCC                           ; 21, INTISR[  5]  RCC Global Interrupt.               
        DCD     BSP_IntHandlerEXTI0                         ; 22, INTISR[  6]  EXTI Line0 Interrupt.               
        DCD     BSP_IntHandlerEXTI1                         ; 23, INTISR[  7]  EXTI Line1 Interrupt.               
        DCD     BSP_IntHandlerEXTI2                         ; 24, INTISR[  8]  EXTI Line2 Interrupt.               
        DCD     BSP_IntHandlerEXTI3                         ; 25, INTISR[  9]  EXTI Line3 Interrupt.               
        DCD     BSP_IntHandlerEXTI4                         ; 26, INTISR[ 10]  EXTI Line4 Interrupt.               
        DCD     BSP_IntHandlerDMA1_CH1                      ; 27, INTISR[ 11]  DMA Channel1 Global Interrupt.      
        DCD     BSP_IntHandlerDMA1_CH2                      ; 28, INTISR[ 12]  DMA Channel2 Global Interrupt.      
        DCD     BSP_IntHandlerDMA1_CH3                      ; 29, INTISR[ 13]  DMA Channel3 Global Interrupt.      
        DCD     BSP_IntHandlerDMA1_CH4                      ; 30, INTISR[ 14]  DMA Channel4 Global Interrupt.      
        DCD     BSP_IntHandlerDMA1_CH5                      ; 31, INTISR[ 15]  DMA Channel5 Global Interrupt.      

        DCD     BSP_IntHandlerDMA1_CH6                      ; 32, INTISR[ 16]  DMA Channel6 Global Interrupt.      
        DCD     BSP_IntHandlerDMA1_CH7                      ; 33, INTISR[ 17]  DMA Channel7 Global Interrupt.      
        DCD     BSP_IntHandlerADC1_2                        ; 34, INTISR[ 18]  ADC1 & ADC2 Global Interrupt.       
        DCD     BSP_IntHandlerUSB_HP_CAN_TX                 ; 35, INTISR[ 19]  USB High Prio / CAN TX  Interrupts. 
        DCD     BSP_IntHandlerUSB_LP_CAN_RX0                ; 36, INTISR[ 20]  USB Low  Prio / CAN RX0 Interrupts. 
        DCD     BSP_IntHandlerCAN_RX1                       ; 37, INTISR[ 21]  CAN RX1 Interrupt.                  
        DCD     BSP_IntHandlerCAN_SCE                       ; 38, INTISR[ 22]  CAN SCE Interrupt.                  
        DCD     BSP_IntHandlerEXTI9_5                       ; 39, INTISR[ 23]  EXTI Line[9:5] Interrupt.           
        DCD     BSP_IntHandlerTIM1_BRK                      ; 40, INTISR[ 24]  TIM1 Break  Interrupt.              
        DCD     BSP_IntHandlerTIM1_UP                       ; 41, INTISR[ 25]  TIM1 Update Interrupt.              
        DCD     BSP_IntHandlerTIM1_TRG_COM                  ; 42, INTISR[ 26]  TIM1 Trig & Commutation Interrupts. 
        DCD     BSP_IntHandlerTIM1_CC                       ; 43, INTISR[ 27]  TIM1 Capture Compare Interrupt.     
        DCD     BSP_IntHandlerTIM2                          ; 44, INTISR[ 28]  TIM2 Global Interrupt.              
        DCD     BSP_IntHandlerTIM3                          ; 45, INTISR[ 29]  TIM3 Global Interrupt.              
        DCD     BSP_IntHandlerTIM4                          ; 46, INTISR[ 30]  TIM4 Global Interrupt.              
        DCD     BSP_IntHandlerI2C1_EV                       ; 47, INTISR[ 31]  I2C1 Event  Interrupt.              

        DCD     BSP_IntHandlerI2C1_ER                       ; 48, INTISR[ 32]  I2C1 Error  Interrupt.              
        DCD     BSP_IntHandlerI2C2_EV                       ; 49, INTISR[ 33]  I2C2 Event  Interrupt.              
        DCD     BSP_IntHandlerI2C2_ER                       ; 50, INTISR[ 34]  I2C2 Error  Interrupt.             
        DCD     BSP_IntHandlerSPI1                          ; 51, INTISR[ 35]  SPI1 Global Interrupt.              
        DCD     BSP_IntHandlerSPI2                          ; 52, INTISR[ 36]  SPI2 Global Interrupt.              
        DCD     BSP_IntHandlerUSART1                        ; 53, INTISR[ 37]  USART1 Global Interrupt.            
        DCD     BSP_IntHandlerUSART2                        ; 54, INTISR[ 38]  USART2 Global Interrupt.            
        DCD     BSP_IntHandlerUSART3                        ; 55, INTISR[ 39]  USART3 Global Interrupt.            
        DCD     BSP_IntHandlerEXTI15_10                     ; 56, INTISR[ 40]  EXTI Line [15:10] Interrupts.       
        DCD     BSP_IntHandlerRTCAlarm                      ; 57, INTISR[ 41]  RTC Alarm EXT Line Interrupt.       
        DCD     BSP_IntHandlerUSBWakeUp                     ; 58, INTISR[ 42]  USB Wakeup from Suspend EXTI Int.   

                DCD     TIM8_BRK_IRQHandler       ; TIM8 Break
                DCD     TIM8_UP_IRQHandler        ; TIM8 Update
                DCD     TIM8_TRG_COM_IRQHandler   ; TIM8 Trigger and Commutation
                DCD     TIM8_CC_IRQHandler        ; TIM8 Capture Compare
                DCD     ADC3_IRQHandler           ; ADC3
                DCD     FSMC_IRQHandler           ; FSMC
                DCD     SDIO_IRQHandler           ; SDIO
                DCD     TIM5_IRQHandler           ; TIM5
                DCD     SPI3_IRQHandler           ; SPI3
                DCD     UART4_IRQHandler          ; UART4
                DCD     UART5_IRQHandler          ; UART5
                DCD     TIM6_IRQHandler           ; TIM6
                DCD     TIM7_IRQHandler           ; TIM7
                DCD     DMA2_Channel1_IRQHandler  ; DMA2 Channel1
                DCD     DMA2_Channel2_IRQHandler  ; DMA2 Channel2
                DCD     DMA2_Channel3_IRQHandler  ; DMA2 Channel3
                DCD     DMA2_Channel4_5_IRQHandler ; DMA2 Channel4 & Channel5
__Vectors_End

__Vectors_Size 	EQU 	__Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY

; Dummy SystemInit_ExtMemCtl function                
SystemInit_ExtMemCtl     PROC
                EXPORT  SystemInit_ExtMemCtl      [WEAK]
                BX      LR
                ENDP
                
; Reset handler routine
Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  __main

                LDR     R0, = SystemInit_ExtMemCtl ; initialize external memory controller
                BLX     R0

                LDR     R1, = __initial_sp        ; restore original stack pointer
                MSR     MSP, R1                   

                LDR     R0, =__main
                BX      R0
                ENDP
                
; Dummy Exception Handlers (infinite loops which can be modified)

NMI_Handler     PROC
                EXPORT  NMI_Handler                [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler          [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler          [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler           [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler         [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler                [WEAK]
                B       .
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler           [WEAK]
                B       .
                ENDP
OSPendSV        PROC
                EXPORT  OSPendSV                   [WEAK]
;PendSV_Handler  PROC
;                EXPORT  PendSV_Handler             [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler            [WEAK]
                B       .
                ENDP

Default_Handler PROC

                EXPORT  WWDG_IRQHandler            [WEAK]
                EXPORT  PVD_IRQHandler             [WEAK]
                EXPORT  TAMPER_IRQHandler          [WEAK]
                EXPORT  RTC_IRQHandler             [WEAK]
                EXPORT  FLASH_IRQHandler           [WEAK]
                EXPORT  RCC_IRQHandler             [WEAK]
                EXPORT  EXTI0_IRQHandler           [WEAK]
                EXPORT  EXTI1_IRQHandler           [WEAK]
                EXPORT  EXTI2_IRQHandler           [WEAK]
                EXPORT  EXTI3_IRQHandler           [WEAK]
                EXPORT  EXTI4_IRQHandler           [WEAK]
                EXPORT  DMA1_Channel1_IRQHandler   [WEAK]
                EXPORT  DMA1_Channel2_IRQHandler   [WEAK]
                EXPORT  DMA1_Channel3_IRQHandler   [WEAK]
                EXPORT  DMA1_Channel4_IRQHandler   [WEAK]
                EXPORT  DMA1_Channel5_IRQHandler   [WEAK]
                EXPORT  DMA1_Channel6_IRQHandler   [WEAK]
                EXPORT  DMA1_Channel7_IRQHandler   [WEAK]
                EXPORT  ADC1_2_IRQHandler          [WEAK]
                EXPORT  USB_HP_CAN1_TX_IRQHandler  [WEAK]
                EXPORT  USB_LP_CAN1_RX0_IRQHandler [WEAK]
                EXPORT  CAN1_RX1_IRQHandler        [WEAK]
                EXPORT  CAN1_SCE_IRQHandler        [WEAK]
                EXPORT  EXTI9_5_IRQHandler         [WEAK]
                EXPORT  TIM1_BRK_IRQHandler        [WEAK]
                EXPORT  TIM1_UP_IRQHandler         [WEAK]
                EXPORT  TIM1_TRG_COM_IRQHandler    [WEAK]
                EXPORT  TIM1_CC_IRQHandler         [WEAK]
                EXPORT  TIM2_IRQHandler            [WEAK]
                EXPORT  TIM3_IRQHandler            [WEAK]
                EXPORT  TIM4_IRQHandler            [WEAK]
                EXPORT  I2C1_EV_IRQHandler         [WEAK]
                EXPORT  I2C1_ER_IRQHandler         [WEAK]
                EXPORT  I2C2_EV_IRQHandler         [WEAK]
                EXPORT  I2C2_ER_IRQHandler         [WEAK]
                EXPORT  SPI1_IRQHandler            [WEAK]
                EXPORT  SPI2_IRQHandler            [WEAK]
                EXPORT  USART1_IRQHandler          [WEAK]
                EXPORT  USART2_IRQHandler          [WEAK]
                EXPORT  USART3_IRQHandler          [WEAK]
                EXPORT  EXTI15_10_IRQHandler       [WEAK]
                EXPORT  RTCAlarm_IRQHandler        [WEAK]
                EXPORT  USBWakeUp_IRQHandler       [WEAK]
                EXPORT  TIM8_BRK_IRQHandler        [WEAK]
                EXPORT  TIM8_UP_IRQHandler         [WEAK]
                EXPORT  TIM8_TRG_COM_IRQHandler    [WEAK]
                EXPORT  TIM8_CC_IRQHandler         [WEAK]
                EXPORT  ADC3_IRQHandler            [WEAK]
                EXPORT  FSMC_IRQHandler            [WEAK]
                EXPORT  SDIO_IRQHandler            [WEAK]
                EXPORT  TIM5_IRQHandler            [WEAK]
                EXPORT  SPI3_IRQHandler            [WEAK]
                EXPORT  UART4_IRQHandler           [WEAK]
                EXPORT  UART5_IRQHandler           [WEAK]
                EXPORT  TIM6_IRQHandler            [WEAK]
                EXPORT  TIM7_IRQHandler            [WEAK]
                EXPORT  DMA2_Channel1_IRQHandler   [WEAK]
                EXPORT  DMA2_Channel2_IRQHandler   [WEAK]
                EXPORT  DMA2_Channel3_IRQHandler   [WEAK]
                EXPORT  DMA2_Channel4_5_IRQHandler [WEAK]

WWDG_IRQHandler
PVD_IRQHandler
TAMPER_IRQHandler
RTC_IRQHandler
FLASH_IRQHandler
RCC_IRQHandler
EXTI0_IRQHandler
EXTI1_IRQHandler
EXTI2_IRQHandler
EXTI3_IRQHandler
EXTI4_IRQHandler
DMA1_Channel1_IRQHandler
DMA1_Channel2_IRQHandler
DMA1_Channel3_IRQHandler
DMA1_Channel4_IRQHandler
DMA1_Channel5_IRQHandler
DMA1_Channel6_IRQHandler
DMA1_Channel7_IRQHandler
ADC1_2_IRQHandler
USB_HP_CAN1_TX_IRQHandler
USB_LP_CAN1_RX0_IRQHandler
CAN1_RX1_IRQHandler
CAN1_SCE_IRQHandler
EXTI9_5_IRQHandler
TIM1_BRK_IRQHandler
TIM1_UP_IRQHandler
TIM1_TRG_COM_IRQHandler
TIM1_CC_IRQHandler
TIM2_IRQHandler
TIM3_IRQHandler
TIM4_IRQHandler
I2C1_EV_IRQHandler
I2C1_ER_IRQHandler
I2C2_EV_IRQHandler
I2C2_ER_IRQHandler
SPI1_IRQHandler
SPI2_IRQHandler
USART1_IRQHandler
USART2_IRQHandler
USART3_IRQHandler
EXTI15_10_IRQHandler
RTCAlarm_IRQHandler
USBWakeUp_IRQHandler
TIM8_BRK_IRQHandler
TIM8_UP_IRQHandler
TIM8_TRG_COM_IRQHandler
TIM8_CC_IRQHandler
ADC3_IRQHandler
FSMC_IRQHandler
SDIO_IRQHandler
TIM5_IRQHandler
SPI3_IRQHandler
UART4_IRQHandler
UART5_IRQHandler
TIM6_IRQHandler
TIM7_IRQHandler
DMA2_Channel1_IRQHandler
DMA2_Channel2_IRQHandler
DMA2_Channel3_IRQHandler
DMA2_Channel4_5_IRQHandler
                B       .

                ENDP

                ALIGN

;*******************************************************************************
; User Stack and Heap initialization
;*******************************************************************************
                 IF      :DEF:__MICROLIB
                
                 EXPORT  __initial_sp
                 EXPORT  __heap_base
                 EXPORT  __heap_limit
                
                 ELSE
                
                 IMPORT  __use_two_region_memory
                 EXPORT  __user_initial_stackheap
                 
__user_initial_stackheap

                 LDR     R0, =  Heap_Mem
                 LDR     R1, = (Stack_Mem + Stack_Size)
                 LDR     R2, = (Heap_Mem +  Heap_Size)
                 LDR     R3, = Stack_Mem
                 BX      LR

                 ALIGN

                 ENDIF

                 END

;******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE*****
