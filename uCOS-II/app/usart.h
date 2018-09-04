#ifndef __usart_h__
#define __usart_h__

void sendchar (int ch);
void myprintf(char *buf);
void USART1_InitConfig(uint32 BaudRate);

#endif // __usart_h__

