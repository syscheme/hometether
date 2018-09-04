#include "config.h"
#include "STC15Fxxxx.H"
// #include "USART1.h"
//#include <stdio.h>
#include <string.h>

sbit PIN_CCP0 = P1^1; // SOP16-pin16
sbit PIN_CCP1 = P1^0; // SOP16-pin15
sbit PIN_CCP2 = P3^7; // SOP16-pin14
#define READ_CCPs() (PIN_CCP0 ? 0x1:0) | (PIN_CCP1 ? 0x2:0) | (PIN_CCP2 ? 0x4:0)

sbit PIN_SENDP0 = P2^1;

#define PULSE_CH_STATE              (1<<0)
#define PULSE_CH_FIFO_OVERFLOW      (1<<4)
#define PULSE_CH_CAPTURED           (1<<5)

#define PULSE_CAP_INTV_USEC         (1000000L *12 *16 / MAIN_Fosc) // in usec: CMOD = 0x01 takes MAIN_Fosc/12, ISR_CAP takes higher 12bit as the counter
uint8_t value2eByte(uint16_t v);
void TX1_write(uint8_t dat);
void delayX10us(uint8 n);

void PulseChannel_execCmd();


// PULSE_CH_STATE
enum { PULSE_CH_STATE_IDLE, PULSE_CH_STATE_BITS, PULSE_CH_STATE_DATA, READY };

typedef struct _PulseChannel
{
	uint8_t  state;
	uint16_t fifo_buf[PULSE_CH_FIFO_MAXLEN];
	uint8_t  fifo_header, fifo_tail;
	uint8_t  pulses[PULSE_CH_LOOP_MAXBITS];
	uint8_t  pulses_len;
	uint16_t shortDur, maxWidth;
	uint8_t frame[PULSE_CH_FRAME_MAXLEN];
	uint8_t frame_len, frame_bits;
	uint8_t style_b[2];

	uint8_t offset_bitsLen;
} PulseChannel;

#define min(A,B) ((A<B)?A:B)
uint8_t calcCRC8(const uint8_t* buf, uint8_t len);

xdata PulseChannel pch0;
#if CCP_NUM >1
xdata PulseChannel pch1;
#endif // CCP_NUM

// #if 0
typedef void (*SetPulsePin_f) (uint8_t high);
#define DEFAULT_INTV_PulseFrame (0x10)

// #endif //0 =====================================================================
//===============================================================================

#ifndef USART1_TEXT_MSG
#  define TX1_writeByte(dat) TX1_write(dat)
#  define TX1_endm()         TX1_writeByte(0xff);
#else
   static code const char hexchars[] = "0123456789abcdef";
   static void puts(const char* str) { while(*str) TX1_write(*str++); }
#  define TX1_writeByte(dat)      { TX1_write(hexchars[dat>>4]); TX1_write(hexchars[dat&0x0f]); }  // printf("%02x", dat&0xff)
#  define TX1_endm()              puts("\r\n"); // printf("\r\n");
#endif // USART1_TEXT_MSG

#ifdef RECOGANIZE_HT_FRAME


// -----------------------------
// callback sample: PulseChannel_OnFrame()
// -----------------------------
void PulseChannel_OnHtFrame(PulseChannel* ch, uint8_t* msg, uint8_t len)
{
	TX1_writeByte(len +2); // the message len
	TX1_writeByte(0xf0 | ((ch == &pch0)?0:1)); // the channelId
	for (; len>0; len--)
		TX1_writeByte(*(msg++));
	TX1_endm();
}

#endif // RECOGANIZE_HT_FRAME

void PulseChannel_OnOtherFrame(PulseChannel* ch)
{
	uint8_t i=0;
//	TX1_writeByte(0x55);
	TX1_writeByte(ch->frame_len +2); // the message len
	TX1_writeByte((ch == &pch0)?0:1); // the channelId
	for (i=0; i<ch->frame_len; i++)
		TX1_writeByte(ch->frame[i]);
	TX1_endm();
}

#define SWAP(A, B, TMP) TMP = A; A = B; B = TMP

// -----------------------------
// PulseChannel_encodePulses()
// -----------------------------
// byte 0        1        2        3        4        5                   x    
//     +--------+--------+--------+--------+--------+--------...--------+--------+
//     |  0x55  | msgLen |shortDur|style-b0|style-b1| {pluse|len->data} |  crc8  |
//     +--------+--------+--------+--------+--------+--------...--------+--------+
// where
//     pluse's highest two bit could be 00,01,10
//     datalen's lighest two bit must be 11,

#define FRAME_OFFSET_SHORTDUR (0)
#define FRAME_OFFSET_STYLE_B0 (1)
#define FRAME_OFFSET_STYLE_B1 (2)

static void PulseChannel_encodePulses(PulseChannel* ch, uint8_t* pulses, uint8_t len, bool isEnd)
{
	idata uint8_t i=0, n=0;
	idata uint8_t styles[4], styles_c[4], tmp, idx;
	uint8_t* pFrame = ch->frame, *pFrame_bits= &(ch->frame_bits), *pFrame_len = &(ch->frame_len);
	if (*pFrame_len <=0)
	{
		*pFrame_len =0;

		// validate the frame start
		if (len <min(15, PULSE_CH_LOOP_MAXBITS) || pulses[0] <0x88 || pulses[1] <0x81)
			return;

		pFrame[(*pFrame_len)++] = value2eByte((uint16_t)ch->shortDur * PULSE_CAP_INTV_USEC);

		// copy the leading bits into frame by making it's highest two bit as 10b
		pFrame[FRAME_OFFSET_STYLE_B1+1] = (pulses[0] & 0x3f) | 0x80;
		pFrame[FRAME_OFFSET_STYLE_B1+2] = (pulses[1] & 0x3f) | 0x80;

		// determine style_b0 and style_b1
		memset(styles, 0x00, sizeof(styles));
		memset(styles_c, 0x00, sizeof(styles_c));
		for (i=2; i<len; i++)
		{
			if (pulses[i] & 0x80)
				continue;

			idx =0; tmp =0;
			for (n=0; n<4; n++)
			{
				if (0 == styles[n] || pulses[i] == styles[n])
				{
					idx = n;
					break;
				}

				if (tmp < styles_c[n])
				{
					tmp = styles_c[n];
					idx = n;
				}
			}

			if (pulses[i] != styles[idx])
			{
				styles_c[idx] = 0;
				styles[idx] = pulses[i];
			}

			tmp = ++styles_c[idx];
		}

		// sort by styles_c
		for (i=0; i<2; i++)
		{
			for (n = i +1; n<4; n++)
			{
				if (styles_c[i] < styles_c[n])
				{
					SWAP(styles[i], styles[n], tmp);
					SWAP(styles_c[i], styles_c[n], tmp);
				}
			}
		}

//		if (styles[0] + styles[1] <4) // two few recoganizd bit, look like noice
//			return;

		if (styles[0] < styles[1])
		{
			pFrame[FRAME_OFFSET_STYLE_B0] = styles[0];
			pFrame[FRAME_OFFSET_STYLE_B1] = styles[1];
		}
		else
		{
			pFrame[FRAME_OFFSET_STYLE_B0] = styles[1];
			pFrame[FRAME_OFFSET_STYLE_B1] = styles[0];
		}

		*pFrame_len = FRAME_OFFSET_STYLE_B1 +2+1;
		i =2;
	}
	else
	{
		styles[0] = pFrame[FRAME_OFFSET_STYLE_B0];
		styles[1] = pFrame[FRAME_OFFSET_STYLE_B1];
	}

	for (; i<len && *pFrame_len <PULSE_CH_FRAME_MAXLEN ; i++)
	{
		if (pulses[i] != styles[0] && pulses[i] != styles[1])
		{
			// not a recoganized bit

			if (*pFrame_bits >0 && *pFrame_bits <4)
				return; // two few continous bit, look like noice

			if (0 != ch->offset_bitsLen)
			{
				// quit the bits mode if currently in it
				pFrame[ch->offset_bitsLen] = (0x3f & (*pFrame_bits)) | 0xc0;
				ch->offset_bitsLen =0;
				if (0 != (*pFrame_bits) %8)
					pFrame_len++;

				*pFrame_bits =0;
			}

			// append this pulses[i]
			if (0 == (pulses[i] & 0x80))
				pFrame[(*pFrame_len)++] = pulses[i];
			else
			{
				pFrame[(*pFrame_len)++] = pulses[i++] | 0x80;
				pFrame[(*pFrame_len)++] = pulses[i++] | 0x80;
			}

			continue;
		}

		// a recoganized bit
		if (0 == ch->offset_bitsLen)
		{
			*pFrame_bits =0;
			ch->offset_bitsLen = (*pFrame_len)++;
		}
		else if ((*pFrame_bits) >=56)
		{
			pFrame[ch->offset_bitsLen] = (0x3f & (*pFrame_bits)) | 0xc0;
			*pFrame_bits =0;
			ch->offset_bitsLen = (*pFrame_len)++;
		}

		if (0 == ((*pFrame_bits)++) %8)
			pFrame[(*pFrame_len)++] =0;			

		pFrame[(*pFrame_len) -1] <<=1;
		if (pulses[i] == styles[1])
			pFrame[(*pFrame_len) -1] |=1;
	}

	if (isEnd)
	{
		if (0 != ch->offset_bitsLen)
		{
			pFrame[ch->offset_bitsLen] = (0x3f & (*pFrame_bits)) | 0xc0;
			ch->offset_bitsLen =0;
		}

		if (*pFrame_len >0)
		{
			// PulseChannel_printPulseSeq(ch);

#ifdef RECOGANIZE_HT_FRAME
			// PulseChannel_OnPulses(ch);
			// recognazing if this is a HT frame => tmp
			tmp = pFrame[FRAME_OFFSET_STYLE_B0] == 0x13 && pFrame[FRAME_OFFSET_STYLE_B1] == 0x31; // verify the bit 0/1 style
			tmp = tmp && pFrame[FRAME_OFFSET_STYLE_B1+1] ==0x9f && pFrame[FRAME_OFFSET_STYLE_B1+2] ==0x81; // verify the leading bit
			tmp = tmp && (pFrame[FRAME_OFFSET_STYLE_B1+3] >>6) == 0x03;  // must be bits
			tmp = tmp && 0x05 == (pFrame[FRAME_OFFSET_STYLE_B1+4] >>5) && 0xff == (pFrame[FRAME_OFFSET_STYLE_B1+4] ^ pFrame[FRAME_OFFSET_STYLE_B1+5] ); // the leading two bytes

			if (tmp)
			{
				// okay, i think this is it, decode it to the frame
				i = FRAME_OFFSET_STYLE_B1+3, len=0;
				n=0;
				while (i < ch->frame_len)
				{
					// test if there is enough bytes in our frame
					if (n >0 && n >= ((pFrame[0] &0x1f) +3))
						break;

					if (0x03 != (pFrame[i] >> 6))
					{
						tmp = FALSE;
						break;
					}

					len = pFrame[i++] & 0x3f;
					if (0 != len %8)
					{
						tmp = FALSE;
						break;
					}
					len /=8;

					while (len-- >0 && i < ch->frame_len)
						pFrame[n++] = pFrame[i++];
				}
			}

			if (0 == tmp)
				PulseChannel_OnOtherFrame(ch);
			else
			{
				if (n >= (pFrame[0] &0x1f) +3) // valid length
				{
					n = pFrame[0] &0x1f;
#ifdef PULSEFRAME_ENCRYPY
					idx = pFrame[1] ^ pFrame[n+2];
					for (i=0; i< n; i++)
						pFrame[i+2] ^= encrypt_table[idx^i];
#endif //PULSEFRAME_ENCRYPY
					if (pFrame[n+2] == calcCRC8(pFrame+2, n))
					{
//						printf("frame>> payload(%d), crc(%02x): ", n, pFrame[n+2]);
						*pFrame_len =n+3;
						PulseChannel_OnHtFrame(ch, pFrame+2, n);
//						printf("\n");
					}
				}
			}

#else
		PulseChannel_OnOtherFrame(ch);
#endif // RECOGANIZE_HT_FRAME
		}

		*pFrame_len =0;
	}
}

// -----------------------------
// PulseChannel_doParse()
// -----------------------------
uint8_t PulseChannel_doParse(PulseChannel * ch)
{
	data uint8_t p = ch->fifo_tail, tmp;
	data uint8_t cPulses =0;
	data uint16_t dur1, dur2, v;
	int i;

	if (p == ch->fifo_header)
		return 0;

	for ( ;p != ch->fifo_header; p%=PULSE_CH_FIFO_MAXLEN )
	{
		dur1 = ch->fifo_buf[p++ % PULSE_CH_FIFO_MAXLEN];

		if ((p%PULSE_CH_FIFO_MAXLEN) == ch->fifo_header)
			return 0;

		if (0 == (dur1 &0x8000)) // not start with a high v
		{
			ch->fifo_tail = p % PULSE_CH_FIFO_MAXLEN;
			ch->state = PULSE_CH_STATE_IDLE;
			break;
		}

		dur2 = ch->fifo_buf[p++ % PULSE_CH_FIFO_MAXLEN];
		if (0 != (dur2 &0x8000))
		{
			ch->fifo_tail = p % PULSE_CH_FIFO_MAXLEN;
			ch->state = PULSE_CH_STATE_IDLE;
			break;
		}

		i =0; cPulses++;
		switch (ch->state & 0x03)
		{
		case PULSE_CH_STATE_IDLE:
			if (cPulses <8)  // have not yet collected 5 pluses yet
				continue;

			// okay, we have 5 continuous pulses here
			// step IDLE.1 look for the three shortest in the seq, and determ the short-signal
			dur1 =min(dur1, dur2), ch->shortDur = dur2=0x7fff; // dur1 < shortDur <dur2 
			for (i =0, p = ch->fifo_tail; i<cPulses*2; i++)
			{
				v = ch->fifo_buf[(p+i) %PULSE_CH_FIFO_MAXLEN] & 0x7fff;
				if (dur2 > v && v >0)
				{
					if (ch->shortDur > v)
					{
						if (dur1 > v)
							dur1 = v;
						else
						{
							dur2 = ch->shortDur;
							ch->shortDur =v;
						}
						continue;
					}
					
					dur2 = v;
				}
			}

			// take the mid-shortest as the short pulse duration
			if (dur2 < (ch->shortDur *2))
				ch->shortDur = (dur1 + ch->shortDur *4 + dur2 *4 +5)/9;
			else ch->shortDur = (dur1 +ch->shortDur*4 +3)/5;

			// step IDLE.2 believe the leading bit should be at least 16 times longer than the shortDur
			//  IDLE.2.1 find the leading bit
			tmp = PULSE_CH_FIFO_MAXLEN +1;
			v = ch->shortDur <<4;
			for (p = ch->fifo_tail, i=0; i < cPulses; i++, p+=2)
			{
				dur1 = ch->fifo_buf[p%PULSE_CH_FIFO_MAXLEN] &0x7fff;
				dur2 = ch->fifo_buf[(p+1) %PULSE_CH_FIFO_MAXLEN] &0x7fff;
				if ((dur1 + dur2) >= v)
					break;
			}

			if (i >=cPulses)
			{
			    // no leading bit found, step the FIFO
				ch->fifo_tail = p %PULSE_CH_FIFO_MAXLEN;
				ch->state = PULSE_CH_STATE_IDLE;
				cPulses =0;
				continue;
			}
			
			// must found the leading bit, remember it
			tmp = p;

			// IDLE.2.2 find the first data bit
			for (; i < cPulses; i++, p +=2)
			{
				dur1 = ch->fifo_buf[p%PULSE_CH_FIFO_MAXLEN] &0x7fff;
				dur2 = ch->fifo_buf[(p+1) %PULSE_CH_FIFO_MAXLEN] &0x7fff;
				if ((dur1 + dur2) < v)
					break;
			}

			i++;
			if (tmp < p-2)
				tmp = p-2;

			// validate the scan result now
/*			if (tmp > PULSE_CH_FIFO_MAXLEN)
			{
				// no leading bit found, release the fifo and set to IDLE
				ch->fifo_tail = p %PULSE_CH_FIFO_MAXLEN;
				ch->state = PULSE_CH_STATE_IDLE;
				cPulses =0;
				continue;
			}
*/
			i = ((tmp + PULSE_CH_FIFO_MAXLEN - ch->fifo_tail) % PULSE_CH_FIFO_MAXLEN) /2;

			// addressed the leading bit and free the fifo previous
			if (cPulses - i < 5) // too few data bit collected, do it again in the next round
			{
				ch->fifo_tail = tmp %PULSE_CH_FIFO_MAXLEN;
				ch->state = PULSE_CH_STATE_IDLE;
				cPulses =0;
				continue;
			}

			// okay how we are good at capturing the start point of a pulses
			ch->pulses_len =0;
			v = ch->shortDur >>2;
//			ch->state = PULSE_CH_STATE_LEADING_BITS;

//		case PULSE_CH_STATE_LEADING_BITS:
			// the latest leading bits
			dur1 = ((ch->fifo_buf[tmp++ %PULSE_CH_FIFO_MAXLEN] &0x7fff) +v) /ch->shortDur;
			dur2=  ((ch->fifo_buf[tmp++ %PULSE_CH_FIFO_MAXLEN] &0x7fff) +v) /ch->shortDur;
			ch->maxWidth = dur1+dur2;  ch->maxWidth -= ch->maxWidth>>2; // make maxWidth= leading bit's 1.25 times long

//			if (dur1 + dur2)
//			{ // likely not a data 
//				ch->state = PULSE_CH_STATE_DATA;
//				continue;
//			}

			if (dur1 >0x08 || dur2 > 0x08)
			{
				ch->pulses[ch->pulses_len++] = 0x80|dur1;
				ch->pulses[ch->pulses_len++] = 0x80|dur2;
			}
			else ch->pulses[ch->pulses_len++] = dur1 <<4 | dur2;

			ch->fifo_tail = tmp %PULSE_CH_FIFO_MAXLEN;
			ch->state = PULSE_CH_STATE_BITS;

			// no break, no continue here

		case PULSE_CH_STATE_BITS:
		case PULSE_CH_STATE_DATA:
			v = ch->shortDur >>2;

			// the data bits
			for (p = ch->fifo_tail; i< cPulses && ch->pulses_len < PULSE_CH_LOOP_MAXBITS; i++)
			{
				dur1 = ((ch->fifo_buf[p++%PULSE_CH_FIFO_MAXLEN]&0x7fff) +v) /ch->shortDur;
				dur2=  ((ch->fifo_buf[p++%PULSE_CH_FIFO_MAXLEN]&0x7fff) +v) /ch->shortDur;

				if (dur1+dur2<10)
					ch->state = PULSE_CH_STATE_DATA;
					
				// if (dur1 >0x10 || dur2 > 0x10)
				if ((PULSE_CH_STATE_DATA == (ch->state & 0x03)) && (0 == dur1 || 0 == dur2 || (dur1+ dur2) > ch->maxWidth))
				{
					// rewind the p by 2, then set the tail
					p += PULSE_CH_FIFO_MAXLEN -2;
					ch->fifo_tail = p %PULSE_CH_FIFO_MAXLEN;
					PulseChannel_encodePulses(ch, ch->pulses, ch->pulses_len, TRUE); 
					ch->pulses_len = 0;
					ch->state = PULSE_CH_STATE_IDLE;
					return 1;
				}

				if (dur1 >0x08 || dur2 > 0x08)
				{
					ch->pulses[ch->pulses_len++] = 0x80|dur1;
					ch->pulses[ch->pulses_len++] = 0x80|dur2;
				}
				else ch->pulses[ch->pulses_len++] = dur1 <<4 | dur2;
			}

			// done with this round parse
			ch->fifo_tail = p %PULSE_CH_FIFO_MAXLEN;
			if (ch->pulses_len >= PULSE_CH_LOOP_MAXBITS)
			{
				PulseChannel_encodePulses(ch, ch->pulses, ch->pulses_len, FALSE);
				ch->pulses_len =0;
			}

			cPulses =0;
			continue;
		} // end of switch

		// break; // always break to quit the loop if reach here
	} // end of while

	if (PULSE_CH_STATE_DATA != (ch->state &0x3) && PULSE_CH_STATE_DATA != (ch->state &0x3) && ch->pulses_len >0)
	{
		PulseChannel_encodePulses(ch, ch->pulses, ch->pulses_len, TRUE);
		ch->pulses_len =0;
		return 1;
	}

	return 0;
}

void UART_config(unsigned long baudrate);
#define  P_SW1_CCP (0x10 | 0x20)
uint8_t newheader;
void PulseChannel_Capfill(PulseChannel xdata* ch, uint16_t dur, uint8 isHigh)
{
	newheader = (ch->fifo_header +1) % PULSE_CH_FIFO_MAXLEN;

  if (newheader == ch->fifo_tail)
	{
		ch->state |= PULSE_CH_FIFO_OVERFLOW;
		return;
	}

	if (isHigh)
		dur |= 0x8000; 

	ch->fifo_buf[ch->fifo_header] = dur;
	ch->fifo_header = newheader;
}

#define DIV_BITS (8)
static uint16_t timer_cntH =0;

void ISR_PCA() interrupt 7 using 1
{
	static uint16_t cntLastCCP0=0;
#if CCP_NUM >1
	static uint16_t cntLastCCP1=0;
#endif // CCP_NUM

	uint32_t cnt;
	uint8_t flags  = READ_CCPs();

	if (CF)
		timer_cntH++;
	CF =0;

	cnt =timer_cntH; cnt <<=8;
	cnt |=CCAP0H; cnt <<=4;
	cnt |=CCAP0L >>4;


//	printf("CAP\r\n");
	if (CCF0)
	{
		PulseChannel_Capfill(&pch0, cnt - cntLastCCP0, flags & 0x1);
		cntLastCCP0 = cnt;
	}
	CCF0 =0;

#if CCP_NUM >1
	if (CCF1)
	{
		PulseChannel_Capfill(&pch1, cnt - cntLastCCP1, flags & 0x2);
		cntLastCCP1 = cnt;
	}
	CCF1 =0;
#endif // CCP_NUM

}

// -------------------------------------------------------------------------------
// UART_config
// -------------------------------------------------------------------------------
#define	UART_8bit_BRTx	(1<<6)	//8位数据,可变波特率
#define COMx_FLAG_TX_Busy      (1<<0)
#define	COMx_RX_Timeout		(5)

typedef struct
{ 
	uint8_t  B_Flags;
	uint8_t	TX_read;		//发送读指针
	uint8_t	TX_write;		//发送写指针
#ifndef USART1_NO_RX
	uint8_t	RX_TimeOut;		//接收超时
	uint8_t  RX_header;
#endif // USART1_NO_RX
} COMx_Define;

COMx_Define COM1;
 
uint8_t	idata TX1_Buffer[COM_TX1_Length];	//发送缓冲
#ifndef USART1_NO_RX
uint8_t idata RX1_Buffer[COM_RX1_Length];	//接收缓冲
#endif // USART1_NO_RX

void UART_config(unsigned long baudrate)
{
	uint32_t	i;
	memset(&COM1, 0x00, sizeof(COM1));
	memset(&TX1_Buffer, 0x00, sizeof(TX1_Buffer));
#ifndef USART1_NO_RX
	memset(&RX1_Buffer, 0x00, sizeof(RX1_Buffer));
#endif // USART1_NO_RX

	PS = 0;	//低优先级中断
	SCON = (SCON & 0x3f) | UART_8bit_BRTx;
	i = (MAIN_Fosc / 4) / baudrate;	//按1T计算
	i = 65536UL - i;

	// takes timer2
	AUXR &= ~(1<<4);	//Timer stop
	AUXR |= 0x01;		//S1 BRT Use Timer2;
	AUXR &= ~(1<<3);	//Timer2 set As Timer
	AUXR |=  (1<<2);	//Timer2 set as 1T mode
	TH2 = (uint8_t)(i>>8);
	TL2 = (uint8_t)i;
	IE2  &= ~(1<<2);	//禁止中断
	AUXR &= ~(1<<3);	//定时
	AUXR |=  (1<<4);	//Timer run enable

	ES = 1;	//允许中断
#ifndef USART1_NO_RX
	REN = 1;	//允许接收, TODO: REN = 0 禁止接收
#else
	REN = 0; // 禁止接收
#endif // USART1_NO_RX
	// P_SW1 = (P_SW1 & 0x3f) | (COMx->UART_P_SW & 0xc0); 切换IO
	PCON2 &= ~(1<<4);   //不做中继,不内部短路RXD与TXD
	puts("Test starts\r\n");	//SUART1发送一个字符串
}

void TX1_write(uint8_t dat)	//写入发送缓冲，指针+1
{
	TX1_Buffer[COM1.TX_write] = dat;	//装发送缓冲
	if(++COM1.TX_write >= COM_TX1_Length) COM1.TX_write = 0;

	if(0 == (COMx_FLAG_TX_Busy & COM1.B_Flags))		//空闲
	{  
		COM1.B_Flags |= COMx_FLAG_TX_Busy;		//标志忙
		TI = 1;					//触发发送中断
	}
}

// ISR_UART1 interrupt service program
// ======================================
void ISR_UART1 (void) interrupt UART1_VECTOR
{
	uint8 ch=0, new_header=0;
	if(RI)
	{
		RI = 0;
#ifndef USART1_NO_RX
		if (COM1.RX_TimeOut >0)
			return;

		ch = SBUF;
		new_header = (COM1.RX_header +1) % COM_RX1_Length;
		RX1_Buffer[COM1.RX_header] = ch;

#ifdef USART1_TEXT_MSG
		if ('\0' ==ch || '\r' ==ch || '\n' == ch)
		{
			RX1_Buffer[COM1.RX_header] = '\0';
#else
		if (0xff ==ch) // take 0xff as end of message
		{
#endif
			if (COM1.RX_header >0)
				COM1.RX_TimeOut = COMx_RX_Timeout;
			COM1.RX_header = 0;
		}
		else COM1.RX_header = new_header;
#endif // USART1_NO_RX
	}

	if(TI)
	{
		TI = 0;
		if(COM1.TX_read == COM1.TX_write)
			COM1.B_Flags &= ~COMx_FLAG_TX_Busy;
		else
		{
		 	SBUF = TX1_Buffer[COM1.TX_read];
			if(++COM1.TX_read >= COM_TX1_Length)
				COM1.TX_read = 0;
		}
	}
}

/*
char putchar (char ch)
{
	TX1_write(ch);
	return ch;
}
*/

//////////////////////////////////
static code const uint8_t crc8_table[] = {
	0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
	35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
	87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};

static const uint8_t* encrypt_table = crc8_table;

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// global 'crc8' value. 
// Returns current global crc8 value
//
uint8_t calcCRC8(const uint8_t* buf, uint8_t len)
{
	uint8_t crc8=0, i;
	for(i=0; i<len; i++)
		crc8 = crc8_table[crc8 ^ buf[i]];
	return crc8;
}

/*
uint8_t encrypt(uint8_t* buf, uint8_t len, uint8_t seed)
{
	uint8_t i;
	for(i=0; i<len; i++)
	{
		seed = encrypt_table[buf[i] ^seed];
		buf[i] ^= seed;
	}

	return i;
}
*/

static code const uint8_t BaseNums[] = {
10,11,12,13,14,15,16,18, 20,22,24,25,27,29,
31,33,36,38, 40,43,47, 50,54,58, 62,66,
70,76, 82,89, 96};

uint8_t value2eByte(uint16_t v)
{
	uint8_t pwr=0, b=0;
	for (pwr=0; v>=100; pwr++)
		v = (v+5)/10;

	if (pwr >7)
		return 0xff;

	for (b=0; b < sizeof(BaseNums) && BaseNums[b] < v; b++);

	if (b>0 && BaseNums[b] > v)
	{
		if ((v-BaseNums[b-1]) < (BaseNums[b] -v))
			b -=1;
	}

	b = (b<<3) | (pwr &0x07);
	return b;
}

uint16_t eByte2value(uint8_t e)
{
	uint16_t v = (e>>3);
	v= (v>=sizeof(BaseNums)) ? 99 : BaseNums[v];
	for (e &=0x07; e>0; e--)
		v *=10;
	return v;
}

#define TIMER_RELOAD (65536 - MAIN_Fosc/12/1000)

/*
void Timer_init()
{
	AUXR |= 0x40;
	TMOD = 0x00;
	TL1  = TIMER_RELOAD;
	TH1  = TIMER_RELOAD>>8;
	TR1 =1;
	ET1 =1;
}
*/

void CCP_init()
{
	ACC = P_SW1;
	ACC &= ~P_SW1_CCP; // set CCP_S0=CCP_S1=0
	P_SW1 = ACC;

	CCON =0; // reset PCA ctrl
	CL = CH =0; // reset PCA register
	CCAP0L = CCAP0H = 0;
	 
	CMOD = 0x01; // take the MAIN_Fosc/12 as the source clock of PCA
	PCA0_capture_edge(); // PCA0 takes the both-edge, 16bit

	CR = 1; // start PCA
}

static uint16_t stampLastIdle =0;

void main()
{
	uint8_t secContIdle =0;
	uint8_t busy =0;
	UART_config(115200ul);
	// Timer_init();
	memset(&pch0, 0x00, sizeof(pch0));
#if CCP_NUM >1
	memset(&pch1, 0x00, sizeof(pch1));
#endif // CCP_NUM

	// refer to STC doc 11.9
	// step 1. function switch
	CCP_init();

	EA = 1; // enable interrupts

	while(1)
	{
		busy =0;
		PulseChannel_execCmd();

		if (0 != PulseChannel_doParse(&pch0))
			busy =1;

#if CCP_NUM >1
		if (0 != PulseChannel_doParse(&pch1))
			busy =1;
#endif // CCP_NUM

		if (busy) // has something captured
		{
			stampLastIdle =0;
			continue;
		}

		if (timer_cntH -stampLastIdle <20) // timer_cntH steps at around 1sec: PULSE_CAP_INTV_USEC*65536
			continue;
		if (0 == stampLastIdle)
			secContIdle =0;
		stampLastIdle = timer_cntH;

		if (secContIdle++)
		{
			// every 1 idle sec when reach here
			puts("IDLE\r\n");

			// step 1. reset the pulse channel ctx first
			memset(&pch0, 0x00, sizeof(pch0));
#if CCP_NUM >1
			memset(&pch1, 0x00, sizeof(pch1));
#endif // CCP_NUM
		}

		if (secContIdle <8)
			continue;
		secContIdle = 0;

		// after 10 continuous idle seconds when reach here
		// step 2. enter the sleep mode
		puts("going sleep\r\n");
		PCON |= 0x02;

		// after wake-up, the following _nop_() will be executed for the clock becomes stable 
		_nop_();  _nop_(); _nop_(); _nop_(); 
		_nop_();  _nop_(); _nop_(); _nop_(); 

		puts("waked up\r\n");
	}
}

void PulseFrame_sendPulse(uint8_t chId, uint16_t durHx10usec, uint16_t durLx10usec)
{
	if (chId & 0x80) P0 &= ~(1 << (chId &0x07));
	else P0 |= 1 << (chId &0x07);
	delayX10us(durHx10usec);

	if (chId & 0x80) P0 |= 1 << (chId &0x07);
	else P0 &= ~(1 << (chId &0x07));
	delayX10us(durLx10usec);
}

#define PulseFrame_sendByte(chId, B, durIntv)		for(i=0, tmp=B; i<8; i++, tmp<<=1) { \
												if (0x80 & tmp) { PulseFrame_sendPulse(chId, durIntv*3, durIntv); } \
												else { PulseFrame_sendPulse(chId, durIntv, durIntv*3); } }

void PulseChannel_decodeSend(uint8_t chId, uint16_t shortDur, uint8_t style0, uint8_t style1, uint8_t* pPulses, uint8_t pulses_len)
{
	uint8_t i,j,tmp;

	pPulses +=3; pulses_len-=3;
	shortDur /= 10; // convert duration from usec to 10usec

	EA = 0; // disable all interrupts

	while (pulses_len >0)
	{
		if ((*pPulses >> 6) ==0x03) // this is a bit sequence
		{
			i = *pPulses++ & 0x3f;
			pulses_len--;
			for (; i>0 && pulses_len>0; pPulses++, pulses_len--)
			{
				for(j=0, tmp=*pPulses; j<8 && j <i; j++, tmp<<=1)
				{ 
					if (0x80 & tmp) { PulseFrame_sendPulse(0, shortDur *(style1 >>4) , shortDur*(style1 & 0x0f)); }
					else { PulseFrame_sendPulse(0, shortDur *(style0>>4), shortDur*(style0 & 0x0f)); }
				}

				i -= j;
			}

			continue;
		}
		
		if (0x80 & *pPulses) // this is long pulse
		{
			PulseFrame_sendPulse(chId, shortDur *(0x7f & *pPulses++), shortDur *(0x7f & *pPulses++));
			pulses_len -=2;
			continue;
		}

		// this is normal pulse
		PulseFrame_sendPulse(chId, shortDur * (*pPulses >>4), shortDur * (0x0f & *pPulses));
		pPulses++, pulses_len--;
	}

	EA = 1; // enable interrupts
}

uint8_t hexchval(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';

	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' +10;

	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' +10;

	return 0xff;
}

void PulseChannel_execCmd()
{
	uint8_t *p = RX1_Buffer;
	uint8_t len=0, chId;
	do {
		if (COM1.RX_TimeOut <=0)
			break;
		
#ifdef USART1_TEXT_MSG
		while(len <COM_RX1_Length)
		{
			chId = hexchval(RX1_Buffer[len++]);
			if (chId >0x0f)
				break;
			*p = chId <<4;
			
			chId = hexchval(RX1_Buffer[len++]);
			if (chId >0x0f)
				break;
			*p++ |= chId;
	}
#endif
		len = RX1_Buffer[0] % COM_RX1_Length;
		if (0xff != RX1_Buffer[len]) // validate the message len and EOM
			break;

		chId = RX1_Buffer[1]; // read the channelId
		PulseChannel_decodeSend(chId, eByte2value(RX1_Buffer[2]), RX1_Buffer[3], RX1_Buffer[4], RX1_Buffer +5, len-5);

	} while (0);

	COM1.RX_TimeOut =0;	// release the RX1_Buffer for new messages
}

void delayX10us(uint8 n)
{
	data uint8 i;

# define C_INLOOP()  i=8; while(--i) {_nop_(); _nop_(); _nop_(); _nop_();} // 10us -DEC(3/12)-MOV(1/12)-SJMP(3/12) -MOV(3/12)-JZ(3/12)-[DEC(3/12)+MOV(1/12)+SJMP(3/12) +5/12]*9
# define C_OUTLOOP() i=8; while(--i) {_nop_(); _nop_(); _nop_();} // 10us - MOV(3/12) -LCALL(6/12) -RET(4/12) -MOV(3/12) -[7/12 +3/12]*10

	while(--n) // MOV DPTR,#i?041;(24c,3c) MOV A,#0FFH;(12c,2c) MOV B,A;(12,1) LCALL ?C?IILDX;(24,6) ORL A,B;(12,2)	JZ ?C0003;(24,3)
	{ C_INLOOP(); }

	C_OUTLOOP();
}
