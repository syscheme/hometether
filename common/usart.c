#include "usart.h"

#ifdef __GNUC__ 
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf 
     set to 'Yes') calls __io_putchar() */ 
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch) 
#else 
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f) 
#endif /* __GNUC__ */ 

/*
// =========================================================================
// template fputc(): portal of printf() to USART1
// =========================================================================
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

