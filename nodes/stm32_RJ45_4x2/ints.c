// =========================================================================
//                 STM32F10x Peripherals Interrupt Handlers                  
//  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the
//  available peripheral interrupt handler's name please refer to the startup
//  file (startup_stm32f10x_xx.s).
// =========================================================================

#include "bsp.h"
#include "../htod.h"
#include "secu.h"

// =========================================================================
// Customized ISRs for SECU
// =========================================================================
void ISR_RTC(void)
{
	uint32_t counter =0;
	if (RTC_GetITStatus(RTC_IT_SEC) == RESET)
		return;

	// clear the RTC Second interrupt
	RTC_ClearITPendingBit(RTC_IT_SEC);
	// wait until last write operation on RTC registers has finished
	RTC_WaitForLastTask();

	// enable time update
	// TimeDisplay = 1;

	// Reset RTC Counter every week
	counter = RTC_GetCounter();

	// format the counter: the highest byte means the day of week, and lower 3-byte means the second of the day
	if (counter & 0xffffff == 60*60*24)
	{
		counter >>=24;
		counter = ++counter %7;
		counter <<=24;

		RTC_SetCounter(counter);
		RTC_WaitForLastTask(); // wait until last write operation on RTC registers has finished
	}
}

#ifdef SECU_CAN
extern void canReceiveLoop(uint8_t);

void ISR_CAN1_RX(void)
{
	canReceiveLoop(MSG_CH_CAN1);
}
#endif // SECU_CAN

extern void RS232_OnMessage(char* msg);

void ISR_USART1(void)
{
	static uint8_t buf[10], i=0, buf_start=0;
	for (i=0; i < sizeof(buf) -1 && RESET != USART_GetFlagStatus(USART1, USART_FLAG_RXNE); i++)
		buf[i] = USART_ReceiveData(USART1);
		
	if (i>0)
		ThreadStep_doRecvTTY(MSG_CH_RS232_Received, (char*)buf, i, &buf_start);
//		MsgLine_recv(MSG_CH_RS232_Received, buf, i);

/*
	while (RESET != USART_GetFlagStatus(USART1, USART_FLAG_RXNE))
	{ 
		buf[buf_i] = USART_ReceiveData(USART1);
		if ('\r' != buf[buf_i] && '\n' != buf[buf_i])
		{
			buf_i = ++buf_i % (sizeof(buf)-2);
			continue;
		}

		// a line just received
		buf[buf_i++] = '\0'; // NULL terminate the string
		buf_i =0; // reset for next receiving

		if ('\0' == buf[0]) // empty line
			continue;

		// a valid line here
		processTxtMessage(fadm, (char*)buf);
	}
*/

	USART_ClearITPendingBit(USART1, USART_IT_RXNE);
}

extern void RS485_OnMessage(char* msg);

void ISR_RS485(void)
{
	static uint8_t buf[10], i=0;
	for (i=0; i < sizeof(buf) -1 && RESET != USART_GetFlagStatus(USART1, USART_FLAG_RXNE); i++)
		buf[i] = USART_ReceiveData(USART1);
		
	if (i>0)
		MsgLine_recv(MSG_CH_RS485_Received, buf, i);

/*
	static uint8_t buf[64], buf_i=0;
	while (RESET != USART_GetFlagStatus(USART3, USART_FLAG_RXNE))
	{ 
		buf[buf_i] = USART_ReceiveData(USART3);
		if ('\r' != buf[buf_i] && '\n' != buf[buf_i])
		{
			buf_i = ++buf_i % (sizeof(buf)-2);
			continue;
		}

		// a line just received
		buf[buf_i++] = '\0'; // NULL terminate the string
		buf_i =0; // reset for next receiving

		if ('\0' == buf[0]) // empty line
			continue;

		// a valid line here
		processTxtMessage(fext, (char*)buf);
	}
*/

	USART_ClearITPendingBit(USART3, USART_IT_RXNE);
}

void ISR_ADC1DMA(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC1))
       DMA_ClearITPendingBit(DMA1_IT_GL1);
}

void ISR_EXTI(void)
{
#ifdef EN28J60_INT
	if (EXTI_GetITStatus(EXTI_Line7) != RESET)	// handling of IR receiver as EXTi
	{
		GW_nicReceiveLoop(&nic);
		EXTI_ClearITPendingBit(EXTI_Line7);
	}
#endif // EN28J60_INT
}

// =========================================================================
// STM32F10x Peripherals Interrupt Handlers	that are necessary routed
// =========================================================================
#ifndef WITH_UCOSII
typedef  void (*CPU_FNCT_VOID)(void);
static int OSIntNesting =0;
#endif // CPU_FNCT_VOID

static void BSP_IntHandlerDummy(void);

static CPU_FNCT_VOID BSP_IntVectTbl[BSP_INT_SRC_NBR];

static void BSP_IntDispatch(int interruptId)
{
#ifdef WITH_UCOSII
#if (CPU_CFG_CRITICAL_METHOD == CPU_CRITICAL_METHOD_STATUS_LOCAL)
    CPU_SR   cpu_sr;
#endif
#endif // WITH_UCOSII
	CPU_FNCT_VOID  isr;

	// tell uC/OS-II that we are starting an ISR
	CPU_CRITICAL_ENTER();
	OSIntNesting++;
	CPU_CRITICAL_EXIT();

	if (interruptId >0 && interruptId < BSP_INT_SRC_NBR)
	{
		isr = BSP_IntVectTbl[interruptId];
		if (isr != (CPU_FNCT_VOID)0)
			isr();
	}

#ifdef WITH_UCOSII
	OSIntExit(); //  tell uC/OS-II that we are leaving the ISR 
#endif // WITH_UCOSII
}

void  BSP_IntHandlerWWDG          (void)  { BSP_IntDispatch(BSP_INT_ID_WWDG);            }
void  BSP_IntHandlerPVD           (void)  { BSP_IntDispatch(BSP_INT_ID_PVD);             }
void  BSP_IntHandlerTAMPER        (void)  { BSP_IntDispatch(BSP_INT_ID_TAMPER);          }
void  BSP_IntHandlerRTC           (void)  { BSP_IntDispatch(BSP_INT_ID_RTC);             }
void  BSP_IntHandlerFLASH         (void)  { BSP_IntDispatch(BSP_INT_ID_FLASH);           }
void  BSP_IntHandlerRCC           (void)  { BSP_IntDispatch(BSP_INT_ID_RCC);             }
void  BSP_IntHandlerEXTI0         (void)  { BSP_IntDispatch(BSP_INT_ID_EXTI0);           }
void  BSP_IntHandlerEXTI1         (void)  { BSP_IntDispatch(BSP_INT_ID_EXTI1);           }
void  BSP_IntHandlerEXTI2         (void)  { BSP_IntDispatch(BSP_INT_ID_EXTI2);           }
void  BSP_IntHandlerEXTI3         (void)  { BSP_IntDispatch(BSP_INT_ID_EXTI3);           }
void  BSP_IntHandlerEXTI4         (void)  { BSP_IntDispatch(BSP_INT_ID_EXTI4);           }
void  BSP_IntHandlerDMA1_CH1      (void)  { BSP_IntDispatch(BSP_INT_ID_DMA1_CH1);        }
void  BSP_IntHandlerDMA1_CH2      (void)  { BSP_IntDispatch(BSP_INT_ID_DMA1_CH2);        }
void  BSP_IntHandlerDMA1_CH3      (void)  { BSP_IntDispatch(BSP_INT_ID_DMA1_CH3);        }
void  BSP_IntHandlerDMA1_CH4      (void)  { BSP_IntDispatch(BSP_INT_ID_DMA1_CH4);        }
void  BSP_IntHandlerDMA1_CH5      (void)  { BSP_IntDispatch(BSP_INT_ID_DMA1_CH5);        }
void  BSP_IntHandlerDMA1_CH6      (void)  { BSP_IntDispatch(BSP_INT_ID_DMA1_CH6);        }
void  BSP_IntHandlerDMA1_CH7      (void)  { BSP_IntDispatch(BSP_INT_ID_DMA1_CH7);        }
void  BSP_IntHandlerADC1_2        (void)  { BSP_IntDispatch(BSP_INT_ID_ADC1_2);          }
void  BSP_IntHandlerUSB_HP_CAN_TX (void)  { BSP_IntDispatch(BSP_INT_ID_USB_HP_CAN_TX);   }
void  BSP_IntHandlerUSB_LP_CAN_RX0(void)  { BSP_IntDispatch(BSP_INT_ID_USB_LP_CAN_RX0);  }
void  BSP_IntHandlerCAN_RX1       (void)  { BSP_IntDispatch(BSP_INT_ID_CAN_RX1);         }
void  BSP_IntHandlerCAN_SCE       (void)  { BSP_IntDispatch(BSP_INT_ID_CAN_SCE);         }
void  BSP_IntHandlerEXTI9_5       (void)  { BSP_IntDispatch(BSP_INT_ID_EXTI9_5);         }
void  BSP_IntHandlerTIM1_BRK      (void)  { BSP_IntDispatch(BSP_INT_ID_TIM1_BRK);        }
void  BSP_IntHandlerTIM1_UP       (void)  { BSP_IntDispatch(BSP_INT_ID_TIM1_UP);         }
void  BSP_IntHandlerTIM1_TRG_COM  (void)  { BSP_IntDispatch(BSP_INT_ID_TIM1_TRG_COM);    }
void  BSP_IntHandlerTIM1_CC       (void)  { BSP_IntDispatch(BSP_INT_ID_TIM1_CC);         }
void  BSP_IntHandlerTIM2          (void)  { BSP_IntDispatch(BSP_INT_ID_TIM2);            }
void  BSP_IntHandlerTIM3          (void)  { BSP_IntDispatch(BSP_INT_ID_TIM3);            }
void  BSP_IntHandlerTIM4          (void)  { BSP_IntDispatch(BSP_INT_ID_TIM4);            }
void  BSP_IntHandlerI2C1_EV       (void)  { BSP_IntDispatch(BSP_INT_ID_I2C1_EV);         }
void  BSP_IntHandlerI2C1_ER       (void)  { BSP_IntDispatch(BSP_INT_ID_I2C1_ER);         }
void  BSP_IntHandlerI2C2_EV       (void)  { BSP_IntDispatch(BSP_INT_ID_I2C2_EV);         }
void  BSP_IntHandlerI2C2_ER       (void)  { BSP_IntDispatch(BSP_INT_ID_I2C2_ER);         }
void  BSP_IntHandlerSPI1          (void)  { BSP_IntDispatch(BSP_INT_ID_SPI1);            }
void  BSP_IntHandlerSPI2          (void)  { BSP_IntDispatch(BSP_INT_ID_SPI2);            }
void  BSP_IntHandlerUSART1        (void)  { BSP_IntDispatch(BSP_INT_ID_USART1);          }
void  BSP_IntHandlerUSART2        (void)  { BSP_IntDispatch(BSP_INT_ID_USART2);          }
void  BSP_IntHandlerUSART3        (void)  { BSP_IntDispatch(BSP_INT_ID_USART3);          }
void  BSP_IntHandlerEXTI15_10     (void)  { BSP_IntDispatch(BSP_INT_ID_EXTI15_10);       }
void  BSP_IntHandlerRTCAlarm      (void)  { BSP_IntDispatch(BSP_INT_ID_RTCAlarm);        }
void  BSP_IntHandlerUSBWakeUp     (void)  { BSP_IntDispatch(BSP_INT_ID_USBWakeUp);       }

static void BSP_IntHandlerDummy (void) {}

void  BSP_IntVectSet (int interruptId, CPU_FNCT_VOID isr)
{
#ifdef WITH_UCOSII
#if (CPU_CFG_CRITICAL_METHOD == CPU_CRITICAL_METHOD_STATUS_LOCAL)
    CPU_SR   cpu_sr;
#endif
#endif // WITH_UCOSII

	if (interruptId >=0 && interruptId < BSP_INT_SRC_NBR)
	{
		CPU_CRITICAL_ENTER();
		BSP_IntVectTbl[interruptId] = isr;
		CPU_CRITICAL_EXIT();
	}
}

void  BSP_IntInit(void)
{
	int  int_id;
	for (int_id = 0; int_id < BSP_INT_SRC_NBR; int_id++)
		BSP_IntVectSet(int_id, BSP_IntHandlerDummy);

	BSP_IntVectSet(BSP_INT_ID_RTC,    ISR_RTC);

	// TODO: map your own ISRs more
	BSP_IntVectSet(BSP_INT_ID_USART1, ISR_USART1); // the onboard RS232
	BSP_IntVectSet(BSP_INT_ID_DMA1_CH1, ISR_ADC1DMA);
}

#ifdef WITH_UCOSII
// clear a specified interrupt.
void  BSP_IntClr (CPU_DATA  int_id)
{
}

// enable a specified interrupt
void  BSP_IntEn (CPU_DATA  int_id)
{
    if (int_id < BSP_INT_SRC_NBR)
        CPU_IntSrcEn(int_id + 16);
}

// disable a specified interrupt
void  BSP_IntDis (CPU_DATA  int_id)
{
    if (int_id < BSP_INT_SRC_NBR)
        CPU_IntSrcDis(int_id + 16);
}

// disable all interrupts
void  BSP_IntDisAll (void)
{
    CPU_IntDis();
}

#endif // WITH_UCOSII


/**
* @brief  This function handles SDIO global interrupt request.
* @param  None
* @retval : None
*/
void SDIO_IRQHandler(void)
{
	/* Process All SDIO Interrupt Sources */
	//   SD_ProcessIRQSrc();
}

/**
* @brief  This function handles PPP interrupt request.
* @param  None
* @retval : None
*/
/*
void PPP_IRQHandler(void)
{
}*/

