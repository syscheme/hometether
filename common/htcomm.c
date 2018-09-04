#include "htcomm.h"

//
#define GCLOCK_MSEC_MAX   (604800000)	// 1week in msec, within int32
__IO__ uint32_t gClock_msec =0;

/*
int strncmp(const char *s1, const char *s2, size_t maxcount)
{
	while (maxcount-->0)
	{
		if (*s1 && *s1++ == *s2++)
			continue;

		return (*s1 - *s2);
	}

	return 0;
}
*/

#if 0
void memset(void* dest, char c, uint32_t len)
{
	char* d = (char*) dest;
	for (; len >0; len--)
		*(d++) =c;
}

//copy data from src to dest, between memory and per, 8 bit interface
int memcpy(void *dest, void *src, uint_t len)
{
	uint8_t* s = (uint8_t*)src;
	uint8_t* d = (uint8_t*)dest;

	if (NULL == src || NULL == dest || src == dest)
		return -1;//ERR_MEM_ADDR;

	while(len--)
		*d++ = *s++;

	return 0;
}

int memcmp(void *dst, void *src, uint_t len)
{
	int8_t* d = (int8_t*) dst;
	int8_t* s = (int8_t*) src;
	int r =0;

	while (len--)
	{
		r = *d - *s;
		if (r)
			return r;
	}
	return 0;
}

int strlen(char *s)
{
	int	len = 0;

	while (*s++ != '\0')
		len++;

	return len;
}

void strcpy(char *s1, char *s2)
{
	if (NULL == s1) return;
	while ((*s1++ = *s2++) != '\0');
}

int strncmp(char *s1, char *s2, uint16_t maxcount)
{
	while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2 && maxcount-->0)
		s1++, s2++;

	return *s1 - *s2;
}

int strcmp(char *s1, char *s2)
{
	return strncmp(char *s1, char *s2, 0xffff);
}

char* strcat(char *s1, char *s2)
{
	char *s = s1;
	if (NULL == s1) return NULL;

	while (*s++ != '\0');	s--;
	while ((*s++ = *s2++) != '\0');

	return s1; 
}

long atol(char *str)
{
	long sum = 0;
	int val, i = 0;
	if (NULL ==str) return 0;

	while (1)
	{
		val = str[i] - 0x30;
		if (val < 0 || val > 9)
			break;
		sum = sum * 10 + val;
		i++;
	}

	return sum;
}

char* strstr(char* src, char* dst)  
{     
	if (NULL == src || NULL == dst) return NULL;
      
    // calculate the each size    
    char *pSrc = src,*pDst = dst;  
    int nSrcSize = strlen(pSrc), nDstSize = strlen(pDst);  
    // check  
    if (nSrcSize < nDstSize)  
        return NULL;// error  

    // compare    
    int nCompareCount = nSrcSize-nDstSize+1;   
    int nNext = 0;  
    bool bFirst = false;  
    int i = 0, j = 0;  
    while (i < nSrcSize && j < nDstSize)  
    {  
        if (src[i] != dst[j])  
        {  
            if (--nCompareCount == 0)// save time    
            {    
                return NULL;// no substring    
            }    
            // backtrace  
            j = 0;  
            if (bFirst)  
            {  
                i = nNext;  
                bFirst = false;  
                continue;  
            }             
        }  
        else  
        {  
            ++j;  
            if (bFirst == false)  
            {  
                nNext = i+1;  
                bFirst = true;  
            }  
        }  
        ++i;  
    }  
    if (j == nDstSize)// absolute equal  
    {                     
        return &src[i-nDstSize];// have substring  
    }  
    return NULL;// no substring  
}  

#endif  // 0

char* trimleft(const char *s)
{
	while(*s && (' ' == *s || '\t' ==*s))
		s++;
	return (char*) s;
}

#ifdef _STM32F10X

//copy data from src to dest, between memory, 32 bit interface
int memcpy32(void *dest, void *src, uint32_t len)
{
	uint_t s, d, size;
	int	 i;

	if (NULL == src || NULL == dest || src == dest)
		return -1;//ERR_MEM_ADDR;

	if (dest < src)
	{	
		//from front to back
		s = (uint_t)src;
		d = (uint_t)dest;

		if ((s & 0x03) != (d & 0x03))
		{
			for (i = 0; i < len; i++)								//byte copy
				*(uint8_t *)d++ = *(uint8_t *)s++;	
		}
		else 
		{
			if (s & 0x03)
			{					//no align
				size = (len < 0x04 - (s & 0x03)) ? len : (0x04 - (s & 0x03));		//get byte copy len

				for (i = 0; i < size; i++)	//byte copy
					*(uint8_t *)d++ = *(uint8_t *)s++;

				len -= size;
			}

			for (i = 0; i < (len >> 2); i++, s += 4, d += 4, len -= 4)
				*(uint_t *)d = *(uint_t *)s;

			for (i = 0; i < len; i++)								//byte copy
				*(uint8_t *)d++ = *(uint8_t *)s++;
		}

	}
	else
	{							//from back to front
		s = (uint_t)src + len;
		d = (uint_t)dest + len;

		if ((s & 0x03) != (d & 0x03))
		{
			for (i = 0; i < len; i++, s--, d--)								//byte copy
				*(uint8_t *)(d - 1) = *(uint8_t *)(s - 1);	
		}
		else
		{
			if (d & 0x03)
			{
				size = (len < (s & 0x03)) ? len : (s & 0x03);

				for (i = 0; i < size; i++, s--, d--)
					*(uint8_t *)(d - 1) = *(uint8_t *)(s - 1);

				len -= size;
			}

			for (i = 0; i < (len >> 2); i++, s -= 4, d -= 4, len -= 4)			//long byte copy
				*(uint_t *)(d - 4) = *(uint_t *)(s - 4);

			for (i = 0; i < len; i++, s--, d--)								//byte copy
				*(uint8_t *)(d - 1) = *(uint8_t *)(s - 1);	
		}
	}

	return 0;
}

// -----------------------------
// string & characters
// -----------------------------
char* strchar(const char *s, char c)	 // ???????
{
	if (!s) return NULL;
	while(*s && c != *s)
		s++;
	return (char*)(*s ? s : NULL);
}

uint8_t ltoa(uint_t val, char *buffer)
{
	int	index = 0, data = val;
	uint8_t tmp = 0;
	if (NULL ==buffer) return 0;

	do 
	{
		index++;
		data /= 10;
	} while (data);

	tmp = index;
	buffer[index] = '\0';
	while (index--)
	{
		buffer[index] = (val % 10) + 0x30;
		val /= 10;
	}

	return tmp;
}

int is_ascii(char c)
{
	return !(c & 0x80);
}

#endif // _STM32F10X

void delayXmsecTick(int32_t msec)
{
	Timer_start(0, msec, NULL, NULL);
	while (Timer_isStepping(0));
	Timer_stop(0);
}

// -----------------------------
// timer
// -----------------------------
typedef struct _timer 						// TIMER DATA STRUCTURE
{                             
	uint32_t	    counter;                // Current value of timer (counting down)
	func_OnTimer	func;                  	// Function to execute when timer times out
	void*           pctx;
} timer;

timer timer_tbl[TIMER_MAX_NUM];          		 	// Table of timers managed by this module
uint16_t timer_bitmap_enable=0; // the bitmap indicates the mapped timer is in use
uint16_t timer_bitmap_inPending=0; // the bitmap indicates the mapped timer is in use

void Timer_OnTick(void)
{
	uint8_t i = 0;
	timer* p = &timer_tbl[0];
	uint32_t timer_flag =1;

	for (i =0; i < TIMER_MAX_NUM; i++, p++, timer_flag <<=1) 
	{
		if (!(timer_bitmap_enable & timer_flag))
			continue;

		if (--p->counter)
			continue;

		timer_bitmap_enable &= ~timer_flag;

		if (NULL != p->func)
			timer_bitmap_inPending |= timer_flag;
	}
}

void Timer_scan(void)
{
	uint8_t i = 0, timer_flag=1;
	timer* p = &timer_tbl[0];
	func_OnTimer func;

	for(i = 0; i < TIMER_MAX_NUM; i++, p++, timer_flag <<=1) 
	{	
		if (NULL == p->func || !(timer_bitmap_inPending & timer_flag))
			continue;

		timer_bitmap_inPending = ~timer_flag;
		func = p->func;
		p->func = NULL;
		func(p->pctx); // todo: should be a task in ucos ii
		p->pctx = NULL;
	}
}

bool _standAloneSysTick =FALSE;

void Timer_init(void)
{
	uint8_t i = 0;
	timer_bitmap_enable = timer_bitmap_inPending =0;
	// clear and disable all timers
	for (i = 0; i < TIMER_MAX_NUM; i++)
		memset((char*)&timer_tbl[i], 0x00, sizeof(timer));

	if (!_standAloneSysTick)
		return;
//#ifdef _STM32F10X
//	// Setup SysTick Timer for 1 msec interrupts
//	for (i=0; i <100 && SysTick_Config(SystemFrequency / 1000); i++);
//#endif // _STM32F10X
}

void Timer_start(uint8_t n, uint32_t msec, func_OnTimer func, void* pCtx)
{
	timer* p;
	uint16_t flag =1; flag <<=n;
	if (n >= TIMER_MAX_NUM)
		return;
	p = &timer_tbl[n];

	p->func    = func;
	p->counter = msec;
	p->pctx    = pCtx;
	timer_bitmap_inPending &= ~flag;

	if (0 == timer_bitmap_enable)
	{
		// this is the first timer to enable: Enable the SysTick Counter
		//		SysTick_CounterCmd(SysTick_Counter_Enable);
	}

	timer_bitmap_enable |= flag;
}

void Timer_stop(uint8_t n)
{
	uint16_t flag =1; flag <<=n;
	if (n >= TIMER_MAX_NUM)
		return;
	timer_bitmap_enable    &= ~flag;
	timer_bitmap_inPending &= ~flag;
	timer_tbl[n].func = NULL;

	if (0 == timer_bitmap_enable)
	{
		// this is the last timer to stop, stop the systick counter
		//		SysTick_CounterCmd(SysTick_Counter_Disable);
		//		SysTick_CounterCmd(SysTick_Counter_Clear);
	}
}

uint32_t Timer_isStepping(uint8_t n)
{
	timer* p;
	if (n >= TIMER_MAX_NUM)
		return 0;
	p = &timer_tbl[n];

	if (!(timer_bitmap_enable & (((uint16_t) 1) <<n)))
		return 0;

	return p->counter;
}

#ifdef DEBUG

static int usprintf(char* s, char *fmt, va_list ap)
{
	char format_flag = 0;
	uint_t u_val, div_val, base;
	char * p = s;

	for(;;)
	{
		while((format_flag = *fmt++) != '%')
		{
			if (!format_flag)
			{
				va_end(ap);
				return 0;
			}

			*p++ = format_flag;
		}

		switch (format_flag = *fmt++)
		{
/*
		case 's': // string
                    s = va_arg(ap, char *);
                             printf("string %s/n", s);
                             break;
*/
		case 'c':	format_flag = va_arg(ap,int);
		default: 	*p++ = format_flag; continue;
		case 'd': 	base = 10; div_val = 10000; goto CONVERSION_LOOP;
			//case 'x': base = 16; div_val = 0x10;
		case 'x': 	base = 16; div_val = 0x1000;

CONVERSION_LOOP:
			u_val = va_arg(ap, int);
			if (format_flag == 'd')
			{
				if (((int)u_val) < 0)
				{
					u_val = - u_val;
					*p++ = '-';
				}
				while (div_val > 1 && div_val > u_val) div_val /= 10;
			}

			do
			{
				*p++ = hexchar(u_val/div_val);
				u_val %= div_val;
				div_val /= base;
			} while (div_val);
		}
	}
}

void MsgLine_log(uint8_t ifChId, char * fmt, ...)
{
	va_list	ap;
	char* p = NULL;
	volatile MsgLine* line = MsgLine_allocate(ifChId, 0xf0);
	if (NULL == line)
		return;

	p = (char*)line->msg + line->offset;
	va_start(ap, fmt);
	usprintf(p, fmt, ap);
	if (NULL == strchr(p, '\n'))
	{
		p += strlen(p);
		*p++ ='\r'; *p++ ='\n';	*p ='\0';
	}

	line->offset = 0;

//	uart_putstring(buff);
	va_end(ap);
}

#endif // DEBUG

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

char hexchar(const uint8_t n)
{
	static const char hextbl[]="0123456789abcdef";
	if (n > 0x0f)
		return 0;
	return hextbl[n];
}

const char* hex2int(const char* hexstr, uint32_t* pVal)
{
	uint8_t v =0;
	if (NULL == hexstr || NULL == pVal)
		return NULL;

	for(*pVal =0; *hexstr; hexstr++)
	{
		v = hexchval(*hexstr);
		if (v >0x0f) break;
		*pVal <<=4; *pVal += v;
	}

	return hexstr;
}

uint8_t hex2byte(const char* hexstr, uint8_t* pByte)
{
	uint8_t v =0;
	if (NULL == hexstr || NULL == pByte)
		return 0;
	
	*pByte =0;
	v = hexchval(*hexstr);
	if (v >0x0f)
		return 0;
	hexstr++; *pByte =v; 
	v = hexchval(*hexstr);
	if (v >0x0f)
		return 1;
	*pByte <<=4; *pByte |= v;
	return 2;
}

void gClock_step1msec()
{
	if (++gClock_msec >=GCLOCK_MSEC_MAX)
		gClock_msec =0;
	// gClock_msec&=0x7fffffff;

	if (0 == (gClock_msec% BLINK_INTERVAL_MSEC)) // equals %=32 => 32msec/blink-tick, almost 30fps
		BlinkList_doTick();
}

uint32_t gClock_calcExp(uint16_t timeout)
{
	return (gClock_msec +timeout) % GCLOCK_MSEC_MAX;
}

volatile MsgLine _pendingLines[MAX_PENDMSGS];
void MsgLine_init(void)
{
	memset((void*)_pendingLines, 0x00, sizeof(_pendingLines));
}

void MsgLine_free(volatile MsgLine* pLine)
{
	if (NULL == pLine)
		return;

	pLine->timeout = 0;
}

volatile MsgLine* MsgLine_allocate(uint8_t chId, uint8_t reservedTimeout)
{
	int i =0; //, j=-1;
	uint8_t minTo = 0xff;
	volatile MsgLine* thisMsg = NULL;
	if (reservedTimeout <=0)
		reservedTimeout = 0xff; // max(MAX_MSGLEN+1, (MAX_PENDMSGS<<4)|0x0f);

	for (i =0; i < MAX_PENDMSGS; i++)
	{
		if (0 != _pendingLines[i].timeout)
		{
			if (_pendingLines[i].timeout-- <minTo)
			{
				minTo = _pendingLines[i].timeout;
				// j=i;
			}

			if (_pendingLines[i].chId == chId && _pendingLines[i].offset >0)
				thisMsg = &_pendingLines[i]; // continue with the recent incomplete receving;
		}
		else if (NULL == thisMsg) // take the first idle pend msg
			thisMsg = &_pendingLines[i];
	}

//	if ((NULL == thisMsg) && j >=0)
//		thisMsg = &_pendingLines[j];

	if (NULL != thisMsg)
	{
		thisMsg->chId    = chId;
		thisMsg->timeout = reservedTimeout;
	}

	return thisMsg;
}

int MsgLine_recv(uint8_t chId, uint8_t* inBuf, uint8_t maxLen)
{
	int c =0;
	volatile MsgLine* thisMsg = MsgLine_allocate(chId, 0);  // if found, refill the timeout as 4 times of the buffer

	if (NULL == thisMsg) // failed to find an available idle pend buffer, simply give up this receiving
		return -1;

	for (c=0; c < maxLen && *inBuf; c++)
	{
		thisMsg->msg[thisMsg->offset] = *inBuf++;

		// a \0 is known as the data end of this received frame
		if ('\0' == thisMsg->msg[thisMsg->offset])
			break;

		// an EOL would be known as a message end, include ';' to ease those USART utility that hard to input \r\n
		if ('\r' != thisMsg->msg[thisMsg->offset] && '\n' != thisMsg->msg[thisMsg->offset]&& ';' != thisMsg->msg[thisMsg->offset])
		{
			thisMsg->offset = (++thisMsg->offset) % (MAX_MSGLEN-3);
			continue;
		}

		// a line just received
		if (thisMsg->offset < MAX_MSGLEN -3) // re-fill in the line-end if the buffer is enough
		{ thisMsg->msg[thisMsg->offset++] = '\r'; thisMsg->msg[thisMsg->offset++] = '\n'; }

		thisMsg->msg[thisMsg->offset] = '\0'; // NULL terminate the string
		thisMsg->offset =0; // reset for next receiving

		if ('\r' == thisMsg->msg[0] || '\n' == thisMsg->msg[0]|| ';' == thisMsg->msg[0]) // unify empty line to thisMsg->msg[0] = '\0' 
			thisMsg->msg[0] = '\0';

		if ('\0' == thisMsg->msg[0]) // empty line, ignore
			continue;

		// a valid line of message here
		break;
	}

	if ('\0' == thisMsg->msg[0]) // release this MsgLine instantly if empty line
		thisMsg->timeout = 0; 

	return c+1;
}

// -----------------------------
// BlinkPair
// -----------------------------
// about blink an output
static BlinkPair* bpList[BlinkList_MAX] = {NULL,};
#define BlinkPair_FLAG_AB          (1<<4)

int  BlinkList_register(BlinkPair* bp)
{
	int i =0;
	if (NULL == bp)
		return -1;
	for (i =0; i <BlinkList_MAX; i++)
	{
		if (NULL == bpList[i])
			break;
	}

	if (i < BlinkList_MAX)
		bpList[i] = bp;
	if (i < BlinkList_MAX-1)
		bpList[i+1] = NULL;

	return i;
}

#define A_ON(p) 	pinSET(p->pinA, 0 != (p->ctrl & BlinkPair_FLAG_A_ON_VAL))
#define A_OFF(p)	pinSET(p->pinA, 0 == (p->ctrl & BlinkPair_FLAG_A_ON_VAL))
#define B_ON(p) 	pinSET(p->pinB, 0 != (p->ctrl & BlinkPair_FLAG_B_ON_VAL))
#define B_OFF(p)	pinSET(p->pinB, 0 == (p->ctrl & BlinkPair_FLAG_B_ON_VAL))

// count in 100msec
void BlinkPair_set(BlinkPair* bp, uint8_t mode, uint8_t reloadA, uint8_t reloadB, uint8_t blinks)
{
	uint8_t bChanged = ((bp->ctrl&0x0f) == (mode &0x0f)) ? 0:1; // 1 if changed
	bp->ctrl = mode & 0x0f;
	bp->blinks = (blinks>0) ? (blinks+1) :0;

	bChanged |= (bp->reloadA != reloadA || bp->reloadB != reloadB)?1:0; // 1 if style changed

	bp->reloadA = reloadA;
	bp->reloadB = reloadB;

	if (0 == bChanged) // return if style no changes
		return;

	bp->count   = 0; // to take effective instantly if style changed
	if (isPin(bp->pinA))
		A_OFF(bp); // GPIO_WriteBit(bp->pinA.port, bp->pinA.pin, (bp->ctrl & BlinkPair_FLAG_A_ON_VAL)?Bit_RESET:Bit_SET);
	else bp->ctrl &= ~BlinkPair_FLAG_A_ENABLE;

	if (isPin(bp->pinB))
		B_OFF(bp); // GPIO_WriteBit(bp->pinB.port, bp->pinB.pin, (bp->ctrl & BlinkPair_FLAG_B_ON_VAL)?Bit_RESET:Bit_SET);
	else bp->ctrl &= ~BlinkPair_FLAG_B_ENABLE;
}

void BlinkPair_OnTick(BlinkPair* p)
{
	uint8_t vA=0;
	if (NULL == p)
		return;

	if (0 == ((BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE) & p->ctrl))
		return;

	if (p->count--)
		return;

	p->count =0;
	p->ctrl ^= BlinkPair_FLAG_AB;

	if (p->ctrl & BlinkPair_FLAG_AB)
	{
		p->count = eByte2value(p->reloadA) /10;
		vA =1;
		if (p->blinks && 0 == --p->blinks) // test if need to turn off per blinks
		{
			if (p->ctrl & BlinkPair_FLAG_A_ENABLE)
				A_OFF(p); // GPIO_WriteBit(p->pinA.port, p->pinA.pin, (p->ctrl & BlinkPair_FLAG_A_ON_VAL)?Bit_RESET:Bit_SET);

			if (p->ctrl & BlinkPair_FLAG_B_ENABLE)
				B_OFF(p); //	GPIO_WriteBit(p->pinB.port, p->pinB.pin, (p->ctrl & BlinkPair_FLAG_A_ON_VAL)?Bit_RESET:Bit_SET);

			p->ctrl &= ~(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE); // turn off the blink
			return;
		}
	}
	else p->count = eByte2value(p->reloadB)/10;

	if (0 == p->count) // do nothing if this end-of-pair was set to reload=0
		return;

	// now drive the IO_PIN
	if (vA)
	{
		if (p->ctrl & BlinkPair_FLAG_A_ENABLE)
		{
			// adjust per FLAG_X_ON_VAL and set the pinA
			// GPIO_WriteBit(p->pinA.port, p->pinA.pin, (p->ctrl & BlinkPair_FLAG_A_ON_VAL)?Bit_SET:Bit_RESET);
			A_ON(p);
		}
		if (p->ctrl & BlinkPair_FLAG_B_ENABLE)
		{
			// adjust per FLAG_X_ON_VAL and set the pinA
			// GPIO_WriteBit(p->pinB.port, p->pinB.pin, (p->ctrl & BlinkPair_FLAG_B_ON_VAL)?Bit_RESET:Bit_SET);
			B_OFF(p);
		}
	}
	else
	{
		if (p->ctrl & BlinkPair_FLAG_A_ENABLE)
		{
			// adjust per FLAG_X_ON_VAL and set the pinA
			// GPIO_WriteBit(p->pinA.port, p->pinA.pin, (p->ctrl & BlinkPair_FLAG_A_ON_VAL)?Bit_RESET:Bit_SET);
			A_OFF(p);
		}

		if (p->ctrl & BlinkPair_FLAG_B_ENABLE)
		{
			// adjust per FLAG_X_ON_VAL and set the pinA
			B_ON(p); // GPIO_WriteBit(p->pinB.port, p->pinB.pin, (p->ctrl & BlinkPair_FLAG_B_ON_VAL)?Bit_SET:Bit_RESET);
		}
	}
}

void BlinkList_doTick()
{
	uint8_t i =0;
	for (i=0; i < BlinkList_MAX; i++)
		BlinkPair_OnTick(bpList[i]);
}

/*
bool Blink_tick(BlinkTick* pBT)
{ 
	if (NULL == pBT || 0 == pBT->reloadOn)
		return FALSE;
		
	if (0 == (pBT->tick-- & 0x7f))
	{
		pBT->tick = (pBT->reloadOn & 0x7f) ^ (pBT->tick & 0x80);
		if (0 == (pBT->tick & 0x80))
			pBT->tick = pBT->reloadOff & 0x7f;
	}

	return (pBT->tick & 0x80) ? TRUE:FALSE;
}

void Blink_config(BlinkTick* pPT, uint8_t reloadOn, uint8_t reloadOff)
{
	if (NULL ==pPT)
		return;

	pPT->reloadOn  = reloadOn;
	pPT->reloadOff = reloadOff;
	pPT->tick      = 0;
}

void Blink_configByUri(BlinkTick* pPT, char* msg)
{
	uint32_t val;
	uint8_t  reloadOn =0;
	char* q = strstr(msg, "ond=");
	if (NULL == msg || NULL ==pPT)
		return;

	if (NULL != q)
	{
		q+= sizeof("ond=")-1;
		hex2int(q, &val);  //RF_masterId = val;
		reloadOn = val & 0x7f; val =0;
	}

	q = strstr(msg, "offd=");
	if (NULL != q)
	{
		q+= sizeof("offd=") -1;
		hex2int(q, &val);  //RF_masterPort = val %5;
	}

	Blink_config(pPT, reloadOn, val & 0x7f);
}

bool Blink_isActive(BlinkTick* pPT)
{
	if (NULL == pPT)
		return FALSE;
	
	if(0 == (pPT->reloadOn&0x7f) && 0 == (pPT->reloadOff&0x7f))
		return FALSE;

	return TRUE;
}

*/

static const uint8_t crc8_table[] = {
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

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// global 'crc8' value. 
// Returns current global crc8 value
//
uint8_t calcCRC8(const uint8_t* databuf, uint8_t len)
{
	uint8_t crc8=0, i;
	for(i=0; i<len; i++)
		crc8 = crc8_table[crc8 ^ databuf[i]];
	return crc8;
}

// 31nums to cover accurateRate<=7%
static const uint8_t BaseNums[] = {0,
	10,11,12,13,14,15,16,18, 20,22,24,25,27,29,
	31,33,36,38, 40,43,47, 50,54,58,
	62,66, 70,76, 82,89, 96};

uint8_t value2eByte(uint16_t v)
{
	uint8_t pwr=0, b=0;
	for (pwr=0; v>=100; pwr++)
		v = (v+5)/10;

	if (pwr >7)
		return EBYTE_INVALID_VALUE;

	for (b=0; b < sizeof(BaseNums) && BaseNums[b] < v; b++);

	if (b>0 && BaseNums[b] > v)
	{
		if ((v-BaseNums[b-1]) < (BaseNums[b] -v))
			b -=1;
	}

	b |= (pwr &0x07)<<5;
	return b;
}

uint16_t eByte2value(uint8_t e)
{
	uint16_t v = e &0x1f; // the low 5bits
	if (EBYTE_INVALID_VALUE == e)
		return 0;

	v= (v>=sizeof(BaseNums)) ? 99 : BaseNums[v];
	for (e >>=5; e>0; e--)
		v *=10;

	return v;
}
