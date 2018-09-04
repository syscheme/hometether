#include "../htcomm.h"
#include "../usart.h"

#include  <stm32f10x_conf.h>
#include  <stm32f10x.h>

// read/write WORD thru SPI
// ---------------------------------------------------------------------------
// SPI read/write utility func
// ---------------------------------------------------------------------------
#ifndef SPI_GetFlagStatus
#  define SPI_GetFlagStatus      SPI_I2S_GetFlagStatus
#  define SPI_SendData           SPI_I2S_SendData
#  define SPI_ReceiveData        SPI_I2S_ReceiveData
#  define SPI_FLAG_RXNE          SPI_I2S_FLAG_RXNE
#  define SPI_FLAG_TXE           SPI_I2S_FLAG_TXE
#endif

uint8_t SPI_RW(SPI_TypeDef* spi, uint8_t data)
{
	// Loop while DR register in not emplty
	while(RESET == SPI_GetFlagStatus(spi, SPI_FLAG_TXE));

	// Send byte through the SPI peripheral
	SPI_SendData(spi, data);

	// Wait to receive a byte
	while(RESET == SPI_GetFlagStatus(spi, SPI_FLAG_RXNE));

	// Return the byte read from the SPI bus
	return SPI_ReceiveData(spi);
}

uint32_t CPU_id(void)
{                                                                              
	return ((*((vu32*)0x1ffff7e8)>>1) ^ (*((vu32*)0x1ffff7ec)>>2) ^ (*((vu32*)0x1ffff7f0)>>3));
}

uint32_t CPU_typeId(void)
{
	return *((const uint32_t*)0xE000ED00);
}

// =========================================================================
// USART read/write utility func
// =========================================================================
hterr USART_open(USART* USARTx, uint16_t baudRateX100)
{
	USART_InitTypeDef USART_InitStruct;
	USART_ClockInitTypeDef USART_ClockInitStruct;

	USARTx->opStatus = 0xff;

	USART_ClockInitStruct.USART_Clock = USART_Clock_Disable;     //禁止USART时钟
	USART_ClockInitStruct.USART_CPOL = USART_CPOL_Low;           //时钟低时数据有效
	USART_ClockInitStruct.USART_CPHA = USART_CPHA_2Edge;         //数据在第二个时钟边沿捕获
	USART_ClockInitStruct.USART_LastBit = USART_LastBit_Disable; //最后数据位的时钟不输出到SCLK
	USART_ClockInit(USARTx->port, &USART_ClockInitStruct);	         // initialize usart clock with the above parameters

	USART_InitStruct.USART_BaudRate = baudRateX100 *100;         //设置波特率
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;     //设置数据长度为8位
	USART_InitStruct.USART_StopBits = USART_StopBits_1;			 //设置一个停止位
	USART_InitStruct.USART_Parity = USART_Parity_No ;			 //无校验位
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //禁止硬件流控制模式
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; //使能串口的收发功能
	USART_Init(USARTx->port, &USART_InitStruct);						 // initialize usart with the above parameters

#if USART_PENDING_MAX_TX >0
	if (FIFO_init(&USARTx->txFIFO, USART_PENDING_MAX_TX, sizeof(uint8_t), FIFO_FLG_ALLOW_OVERWRITE) <=0)
		return ERR_INVALID_PARAMETER;
#endif // USART_PENDING_MAX_TX

	if (FIFO_init(&USARTx->rxFIFO, USART_PENDING_MAX_RX, sizeof(uint8_t), FIFO_FLG_ALLOW_OVERWRITE) <=0)
		return ERR_INVALID_PARAMETER;

	USART_ITConfig(USARTx->port, USART_IT_RXNE, ENABLE);  // | USART_IT_TXE
	// USART_ClearFlag(USARTx->port,USART_FLAG_TC); 
	USART_Cmd(USARTx->port, ENABLE);	// open the usart
	USARTx->opStatus = 0x00;

	return ERR_SUCCESS;
}

hterr USART_sendByte(USART* USARTx, uint8_t ch)
{
	if (!FIFO_ready(USARTx->txFIFO))
	{
		// if txFIFO is unavailable, take poll mode to send the byte
		while(RESET == USART_GetFlagStatus(USARTx->port, USART_FLAG_TXE));
		USART_SendData(USARTx->port, ch);
		return ERR_SUCCESS;
	}

	if (ERR_SUCCESS != FIFO_push(&USARTx->txFIFO, &ch))
		return ERR_OVERFLOW;

	if (USARTx->opStatus & USART_OPFLAG_SEND_IN_PROGRESS)
	{
		if (0 == --USARTx->txTimeout)
		{
			USARTx->opStatus &= ~USART_OPFLAG_SEND_IN_PROGRESS;
			USARTx->txTimeout =0;
			delayXmsec(1);
		}
	}

	if (0 == (USARTx->opStatus & USART_OPFLAG_SEND_IN_PROGRESS))
		USART_ITConfig(USARTx->port, USART_IT_TXE, ENABLE);

	return ERR_SUCCESS;
}

hterr USART_transmit(USART* USARTx, const uint8_t* data, uint16_t len)
{
	for (; data && len>0; len--)
	{
		if (ERR_SUCCESS != USART_sendByte(USARTx, *data++))
			break;
	}

	return (len <=0)? ERR_SUCCESS : ERR_OVERFLOW;
}

uint16_t USART_receive(USART* USARTx, uint8_t* data, uint16_t maxlen)
{
	uint16_t i=0;
	CRITICAL_ENTER();
	for (i=0; i<maxlen; i++)
	{
		if (ERR_SUCCESS != FIFO_pop(&USARTx->rxFIFO, data++))
			break;
	}

	CRITICAL_LEAVE();
	return i;
}

void USART_doISR(USART* USARTx)
{
	uint8_t ch =0;

	// do not use USART_GetITStatus() for ORE here: http://blog.csdn.net/zyboy2000/article/details/8677256
	if (USART_GetFlagStatus(USARTx->port, USART_FLAG_ORE) != RESET)
		IO_USART_getc(USARTx->port);

	if (USART_GetITStatus(USARTx->port, USART_IT_RXNE) != RESET)
	{
		ch = IO_USART_getc(USARTx->port);
		FIFO_push(&USARTx->rxFIFO, &ch);
		// USART_ClearITPendingBit(USARTx->port, USART_IT_RXNE);
	}

	if (USART_GetITStatus(USARTx->port, USART_IT_TXE) != RESET)
	{
		if (!FIFO_ready(USARTx->txFIFO))
			USART_ITConfig(USARTx->port, USART_IT_TXE, DISABLE);
		else
		{
			USARTx->opStatus &= ~USART_OPFLAG_SEND_IN_PROGRESS; // reset the flag SEND_IN_PROGRESS
			// Read one byte from the outgoing FIFO
			if (ERR_SUCCESS == FIFO_pop(&USARTx->txFIFO, &ch))
			{
				USARTx->opStatus |= USART_OPFLAG_SEND_IN_PROGRESS; // set the flag SEND_IN_PROGRESS
				USARTx->txTimeout = 0x20; // timeout
				IO_USART_putc(USARTx->port, ch);
			}
			else USART_ITConfig(USARTx->port, USART_IT_TXE, DISABLE); // will be turned on by USART_transmit()

			//if (FIFO_awaitSize(&USARTx->txFIFO) >0)
			//{
			//	if (ERR_SUCCESS == FIFO_pop(&USARTx->txFIFO, &ch))
			//		IO_USART_putc(USARTx->port, ch);
			//}
		}
	}

	USART_ClearITPendingBit(USARTx->port, USART_IT_ORE|USART_IT_RXNE);
}

uint16_t STM32TemperatureADC2dC(uint16_t tempADC)
{
	// temperature calcualatiion
	// ---------------------------
	// assume V25C(=1430mv), VDD(=3300v), Slope(=4300uv/C), ADCMAX=4095
	// Temp = 25 + (V25C - ADCtemp * VDD /ADCMAX) *1000/Slope
	// i.e. when ADCtemp = 1815, Temp = 25 + (1430 - 1815*3300/4095) *1000/4300 = 25 +(-32.64)*1000/4300 = 17.4C
	// so TempInDC= 250 + (V25C*100 - ADCtemp *VDD*100 /ADCMAX) /43 = 174 dC
	// or !!! TempInDC = 3576 - ADCtemp *15/8 !!!
	// [0C,100C] maps to [1907, 1374]
	tempADC *= 15;
	tempADC >>= 3;
	return 3576 - tempADC;
}

#ifdef __GNUC__ 
  // With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf 
  // set to 'Yes') calls __io_putchar()
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch) 
#else 
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f) 
#endif // __GNUC__

/*
// template fputc(): portal of printf() to USART1
// ----------------------------------------------
int fputc(int ch, FILE *f)
{
	// forward the printf() to USART1
	USART_SendData(USART1, (uint8_t) ch);
	while (!(USART1->SR & USART_FLAG_TXE)); // wait till sending finishes

	return (ch);
}

// GPIO CONFIG, i.e.  USART1(A9,10)
// ------------------------------------------------------
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Configure USART1 Tx (PA.09) as alternate function push-pull
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
*/

