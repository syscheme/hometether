#include "USART1.h"

COMx_Define	COM1;
u8	 TX1_Buffer[COM_TX1_Lenth];	//���ͻ���
u8 	xdata RX1_Buffer[COM_RX1_Lenth];	//���ջ���

u8 USART_Configuration(u8 UARTx, COMx_InitDefine *COMx)
{
	u8	i;
	u32	j;
	
	if(UARTx == USART1)
	{
		COM1.id = 1;
		COM1.TX_read    = 0;
		COM1.TX_write   = 0;
		COM1.B_Flags    = 0;
		COM1.RX_TimeOut = 0;
		for(i=0; i<COM_TX1_Lenth; i++)	TX1_Buffer[i] = 0;
		for(i=0; i<COM_RX1_Lenth; i++)	RX1_Buffer[i] = 0;

		if(COMx->UART_Mode > UART_9bit_BRTx)	return 1;	//ģʽ����
		if(COMx->UART_Polity == PolityHigh)		PS = 1;	//�����ȼ��ж�
		else									PS = 0;	//�����ȼ��ж�
		SCON = (SCON & 0x3f) | COMx->UART_Mode;
		if((COMx->UART_Mode == UART_9bit_BRTx) ||(COMx->UART_Mode == UART_8bit_BRTx))	//�ɱ䲨����
		{
			j = (MAIN_Fosc / 4) / COMx->UART_BaudRate;	//��1T����
			if(j >= 65536UL)	return 2;	//����
			j = 65536UL - j;
			if(COMx->UART_BRT_Use == BRT_Timer1)
			{
				TR1 = 0;
				AUXR &= ~0x01;		//S1 BRT Use Timer1;
				TMOD &= ~(1<<6);	//Timer1 set As Timer
				TMOD &= ~0x30;		//Timer1_16bitAutoReload;
				AUXR |=  (1<<6);	//Timer1 set as 1T mode
				TH1 = (u8)(j>>8);
				TL1 = (u8)j;
				ET1 = 0;	//��ֹ�ж�
				TMOD &= ~0x40;	//��ʱ
				INT_CLKO &= ~0x02;	//�����ʱ��
				TR1  = 1;
			}
			else if(COMx->UART_BRT_Use == BRT_Timer2)
			{
				AUXR &= ~(1<<4);	//Timer stop
				AUXR |= 0x01;		//S1 BRT Use Timer2;
				AUXR &= ~(1<<3);	//Timer2 set As Timer
				AUXR |=  (1<<2);	//Timer2 set as 1T mode
				TH2 = (u8)(j>>8);
				TL2 = (u8)j;
				IE2  &= ~(1<<2);	//��ֹ�ж�
				AUXR &= ~(1<<3);	//��ʱ
				AUXR |=  (1<<4);	//Timer run enable
			}
			else return 2;	//����
		}
		else if(COMx->UART_Mode == UART_ShiftRight)
		{
			if(COMx->BaudRateDouble == ENABLE)	AUXR |=  (1<<5);	//�̶�������SysClk/2
			else								AUXR &= ~(1<<5);	//�̶�������SysClk/12
		}
		else if(COMx->UART_Mode == UART_9bit)	//�̶�������SysClk*2^SMOD/64
		{
			if(COMx->BaudRateDouble == ENABLE)	PCON |=  (1<<7);	//�̶�������SysClk/32
			else								PCON &= ~(1<<7);	//�̶�������SysClk/64
		}
		if(COMx->UART_Interrupt == ENABLE)	ES = 1;	//�����ж�
		else								ES = 0;	//��ֹ�ж�
		if(COMx->UART_RxEnable == ENABLE)	REN = 1;	//��������
		else								REN = 0;	//��ֹ����
		P_SW1 = (P_SW1 & 0x3f) | (COMx->UART_P_SW & 0xc0);	//�л�IO
		if(COMx->UART_RXD_TXD_Short == ENABLE)	PCON2 |=  (1<<4);	//�ڲ���·RXD��TXD, ���м�, ENABLE,DISABLE
		else									PCON2 &= ~(1<<4);
		return	0;
	}

	return 3;	//��������
}


/*************** װ�ش��ڷ��ͻ��� *******************************/

void TX1_write2buff(u8 dat)	//д�뷢�ͻ��壬ָ��+1
{
	TX1_Buffer[COM1.TX_write] = dat;	//װ���ͻ���
	if(++COM1.TX_write >= COM_TX1_Lenth)	COM1.TX_write = 0;

	if(0 == (COMx_FLAG_TX_Busy & COM1.B_Flags))		//����
	{  
		COM1.B_Flags |= COMx_FLAG_TX_Busy;		//��־æ
		TI = 1;					//���������ж�
	}
}

/********************* UART1�жϺ���************************/
void ISR_UART1 (void) interrupt UART1_VECTOR
{
	uint8 ch=0, new_header=0;
	if(RI)
	{
		RI = 0;
		if (COM1.RX_TimeOut <=0)
		{
			ch = SBUF;
			new_header = (COM1.RX_header +1) % COM_RX1_Lenth;
			RX1_Buffer[COM1.RX_header] = ch;

			if ('\0' ==ch || '\r' ==ch || '\n' == ch)
			{
				RX1_Buffer[COM1.RX_header] = '\0';
				if (COM1.RX_header >0)
					COM1.RX_TimeOut = TimeOutSet1;
				COM1.RX_header = 0;
			}
			else COM1.RX_header = new_header;
		}
	}

	if(TI)
	{
		TI = 0;
		if(COM1.TX_read == COM1.TX_write)
			COM1.B_Flags &= ~COMx_FLAG_TX_Busy;
		else
		{
		 	SBUF = TX1_Buffer[COM1.TX_read];
			if(++COM1.TX_read >= COM_TX1_Lenth)
				COM1.TX_read = 0;
		}
	}
}

char putchar (char ch)
{
	TX1_write2buff(ch);
	return ch;
}



