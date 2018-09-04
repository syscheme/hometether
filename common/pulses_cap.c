#include "pulses.h"

// ======================================================================
// About ht pulse frame
// ======================================================================
// #define PULSEFRAME_ENCRYPY

#ifdef PULSEFRAME_ENCRYPY
static const uint8_t* encrypt_table = crc8_table;
#endif

#define HtFrame_sendByte(pin, B, durIntv)	for(i=0, tmp=B; i<8; i++, tmp<<=1) { \
												if (0x80 & tmp) { PulsePin_send(pin, durIntv*3, durIntv); } \
												else { PulsePin_send(pin, durIntv, durIntv*3); } }
#define DEFAULT_INTV_PulseFrame	 (200) // 200usec
// -----------------------------
// HtFrame_send()
// -----------------------------
uint8_t HtFrame_send(PulsePin pinOut, uint8_t* msg, uint8_t len, uint8_t repeatTimes, uint8_t plusWideUsec)
{
	int i =0, j=0;
	uint8_t crc8=0, leadingByte=0, tmp=0;
#ifdef PULSEFRAME_ENCRYPY
	uint8_t idxEncrypt =0;
#endif // PULSEFRAME_ENCRYPY

	if (NULL ==msg || len <=0)
		return 0;

	if (repeatTimes <=0)
		repeatTimes =1;

	if (len >PULSE_CAP_FRAME_MAXLEN)
		len = PULSE_CAP_FRAME_MAXLEN;

	if (plusWideUsec<=0)
		plusWideUsec = DEFAULT_INTV_PulseFrame;

	// step 1. assemble the leadingByte
	// bit  7    6    5    4    3    2    1    0    
	//     +----+----+----+----+----+----+----+----+
	//     |  1    0    1 |      dataLen           |
	//     +----+----+----+----+----+----+----+----+
	leadingByte = 0xa0 | len;

	// step 2. calculate the crc checksum
	crc8= calcCRC8(msg, len);

#ifdef PULSEFRAME_ENCRYPY
	// step 2.1 encrypt the data by taking the crc as the seed
	idxEncrypt = (~leadingByte) ^ crc8;
	for (j=0; j< len; j++)
		msg[j] ^= encrypt_table[idxEncrypt^j];
	
#endif // PULSEFRAME_ENCRYPY

	// step 3. send the frame
	while (repeatTimes--)
	{
		//step 3.1 the sync signal: H=(128-4)*a, L=4*a
		PulsePin_send(pinOut, plusWideUsec *31, plusWideUsec);

		//step 3.2 the leading byte
		HtFrame_sendByte(pinOut, leadingByte, plusWideUsec);
		HtFrame_sendByte(pinOut, ~leadingByte, plusWideUsec);

		//step 3.3 the frame payload
		for (j=0; j< len; j++)
			HtFrame_sendByte(pinOut, msg[j], plusWideUsec);

		//step 3.4 send the CRC byte
		HtFrame_sendByte(pinOut, crc8, plusWideUsec);
	}

	// the ending pulse
	PulsePin_send(pinOut, plusWideUsec*40, plusWideUsec*4); 

	return len;
}

// -----------------------------
// callback sample: PulseCapture_OnFrame()
// -----------------------------
void PulseCapture_OnHtFrame(PulseCapture* ch, uint8_t* msg, uint8_t len)
{
	for (; len>0; len--)
		printf("%02x ", *(msg++));
	printf("\n");
}

// -----------------------------
// callback sample: PulseCapture_printPulseSeq()
// -----------------------------
void PulseCapture_printPulseSeq(PulseCapture* ch)
{
	int i;
// byte 0        1        2        3        4        5                   x    
//     +--------+--------+--------+--------+--------+--------...--------+--------+
//     |  0x55  | msgLen |shortDur|style-b0|style-b1| {pluse|len->data} |  crc8  |
//     +--------+--------+--------+--------+--------+--------...--------+--------+
	printf("55%02x", ch->frame[PULSEFRAME_OFFSET_LENGTH]);
	for (i=0; i < ch->frame[PULSEFRAME_OFFSET_LENGTH]; i++)
		printf("%02x", ch->frame[i]);
}

// -----------------------------
// PulseCapture_encodePulses()
// -----------------------------
// byte 0        1        2        3        4        5                   x    
//     +--------+--------+--------+--------+--------+--------...--------+--------+
//     |  0x55  | msgLen |shortDur|style-b0|style-b1| {pluse|len->data} |  crc8  |
//     +--------+--------+--------+--------+--------+--------...--------+--------+
// where
//     pluse's highest two bit could be 00,01,10
//     datalen's lighest two bit must be 11,

#define SWAP(A, B, TMP) TMP = A; A = B; B = TMP

static uint8_t PulseCapture_encodePulses(PulseCapture* ch, bool isEnd)
{
	uint8_t i=0, n=0;
	uint8_t styles[4], styles_c[4], tmp, idx;
	uint8_t* pFrame = ch->frame, *pulses = ch->pulses;
	uint8_t volatile *pFrame_bits= &(ch->frame_bits); //, PULSEFRAME_LENGTH(pFrame) = &(ch->frame_len);
	if (PULSEFRAME_LENGTH(pFrame) <=1)
	{
		PULSEFRAME_LENGTH(pFrame) =1;

		// validate the frame start
		if (ch->pulses_len <min(15, PULSE_CAP_LOOP_MAXBITS) || pulses[0] <0x88 || pulses[1] <0x81)
			return 0;

		pFrame[(PULSEFRAME_LENGTH(pFrame))++] = value2eByte((uint16_t)ch->shortDur * PULSE_CAP_INTV_USEC);

		// copy the leading bits into frame by making it's highest two bit as 10b
		pFrame[PULSEFRAME_OFFSET_SIGDATA] = (pulses[0] & 0x3f) | 0x80;
		pFrame[PULSEFRAME_OFFSET_SIGDATA+1] = (pulses[1] & 0x3f) | 0x80;

		// determine style_b0 and style_b1
		memset(styles, 0x00, sizeof(styles));
		memset(styles_c, 0x00, sizeof(styles_c));
		for (i=2; i<ch->pulses_len; i++)
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
			pFrame[PULSEFRAME_OFFSET_STYLE_B0] = styles[0];
			pFrame[PULSEFRAME_OFFSET_STYLE_B1] = styles[1];
		}
		else
		{
			pFrame[PULSEFRAME_OFFSET_STYLE_B0] = styles[1];
			pFrame[PULSEFRAME_OFFSET_STYLE_B1] = styles[0];
			styles[0] = pFrame[PULSEFRAME_OFFSET_STYLE_B0];
			styles[1] = pFrame[PULSEFRAME_OFFSET_STYLE_B1];
		}

		PULSEFRAME_LENGTH(pFrame) = PULSEFRAME_OFFSET_STYLE_B1 +2+1;
		i =2;
	}
	else
	{
		styles[0] = pFrame[PULSEFRAME_OFFSET_STYLE_B0];
		styles[1] = pFrame[PULSEFRAME_OFFSET_STYLE_B1];
	}

	for (; i<ch->pulses_len && PULSEFRAME_LENGTH(pFrame) <PULSE_CAP_FRAME_MAXLEN ; i++)
	{
		if (pulses[i] != styles[0] && pulses[i] != styles[1])
		{
			// not a recoganized bit

			if (*pFrame_bits >0 && *pFrame_bits <4)
			{
				// two few continous bit, look like noice
				if (isEnd)
					PULSEFRAME_LENGTH(pFrame) =0;
				return 1;
			}

			if (0 != ch->offset_bitsLen)
			{
				// quit the bits mode if currently in it
				pFrame[ch->offset_bitsLen] = (0x3f & (*pFrame_bits)) | 0xc0;
				ch->offset_bitsLen =0;
				if (0 != (*pFrame_bits) %8)
					PULSEFRAME_LENGTH(pFrame)++;

				*pFrame_bits =0;
			}

			// append this pulses[i]
			if (0 == (pulses[i] & 0x80))
				pFrame[(PULSEFRAME_LENGTH(pFrame))++] = pulses[i];
			else
			{
				pFrame[(PULSEFRAME_LENGTH(pFrame))++] = pulses[i++] | 0x80;
				pFrame[(PULSEFRAME_LENGTH(pFrame))++] = pulses[i++] | 0x80;
			}

			continue;
		}

		// a recoganized bit
		if (0 == ch->offset_bitsLen)
		{
			*pFrame_bits =0;
			ch->offset_bitsLen = (PULSEFRAME_LENGTH(pFrame))++;
		}
		else if ((*pFrame_bits) >=56)
		{
			pFrame[ch->offset_bitsLen] = (0x3f & (*pFrame_bits)) | 0xc0;
			*pFrame_bits =0;
			ch->offset_bitsLen = (PULSEFRAME_LENGTH(pFrame))++;
		}

		if (0 == ((*pFrame_bits)++) %8)
			pFrame[(PULSEFRAME_LENGTH(pFrame))++] =0;			

		pFrame[(PULSEFRAME_LENGTH(pFrame)) -1] <<=1;
		if (pulses[i] == styles[1])
			pFrame[(PULSEFRAME_LENGTH(pFrame)) -1] |=1;
	}

	if (isEnd)
	{
		if (0 != ch->offset_bitsLen)
		{
			pFrame[ch->offset_bitsLen] = (0x3f & (*pFrame_bits)) | 0xc0;
			ch->offset_bitsLen =0;
		}

		if (PULSEFRAME_LENGTH(pFrame) >0)
			PulseCapture_OnFrame(ch);

		PULSEFRAME_LENGTH(pFrame) =0;
	}

	return 1;
}

// -----------------------------
// PulseCapture_Capfill()
// -----------------------------
void PulseCapture_Capfill(PulseCapture* ch, uint16_t dur, bool isHigh)
{
	uint8_t newheader = (ch->fifo_header +1) % PULSE_CAP_FIFO_MAXLEN;

#ifdef PulsePin_LOOPBACK_TEST
 	while (newheader == ch->fifo_tail)
		Sleep(1);
#else
 	if (newheader == ch->fifo_tail)
	{
		ch->state |= PULSE_CAP_FIFO_OVERFLOW;
		return;
	}
#endif // PulsePin_LOOPBACK_TEST

	if (isHigh)
		dur |= 0x8000; 

	ch->fifo_buf[ch->fifo_header] = dur;
	ch->fifo_header = newheader;
}

// PULSE_CAP_STATE
enum { PULSE_CAP_STATE_IDLE, PULSE_CAP_LEADING_BITS, PULSE_CAP_STATE_DATA, READY };

bool PulseCapture_init(PulseCapture* pc)
{
	if (NULL == pc)
		return FALSE;

	memset(&(pc->ctrl), 0x00, ((uint8_t*) pc) +sizeof(PulseCapture) -((uint8_t*) &(pc->ctrl)));
	return TRUE;
}

// -----------------------------
// PulseCapture_doParse()
// -----------------------------
uint8_t PulseCapture_doParse(PulseCapture* ch)
{
	uint16_t p = ch->fifo_tail, tmp;
	uint8_t cPulses =0;
	uint16_t dur1, dur2, v;
	int i;

	if (p == ch->fifo_header)
		return 0;

	for ( ;p != ch->fifo_header; p%=PULSE_CAP_FIFO_MAXLEN )
	{
		dur1 = ch->fifo_buf[p++ % PULSE_CAP_FIFO_MAXLEN];

		if ((p%PULSE_CAP_FIFO_MAXLEN) == ch->fifo_header)
			return 0;

		if (0 == (dur1 &0x8000)) // not start with a high v
		{
			ch->fifo_tail = p % PULSE_CAP_FIFO_MAXLEN;
			ch->state = PULSE_CAP_STATE_IDLE;
			break;
		}

		dur2 = ch->fifo_buf[p++ % PULSE_CAP_FIFO_MAXLEN];
		if (0 != (dur2 &0x8000))
		{
			ch->fifo_tail = p % PULSE_CAP_FIFO_MAXLEN;
			ch->state = PULSE_CAP_STATE_IDLE;
			break;
		}

		i =0; cPulses++;
		switch (ch->state & 0x03)
		{
		case PULSE_CAP_STATE_IDLE:
			if (cPulses <8)  // have not yet collected 5 pluses yet
				continue;

			// okay, we have 5 continuous pulses here
			// step IDLE.1 look for the three shortest in the seq, and determ the short-signal
			dur1 =min(dur1, dur2), ch->shortDur = dur2=0x7fff; // dur1 < shortDur <dur2 
			for (i =0, p = ch->fifo_tail; i<cPulses*2; i++)
			{
				v = ch->fifo_buf[(p+i) %PULSE_CAP_FIFO_MAXLEN] & 0x7fff;
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
			tmp = PULSE_CAP_FIFO_MAXLEN +1;
			v = ch->shortDur <<4;
			for (p = ch->fifo_tail, i=0; i < cPulses; i++, p+=2)
			{
				dur1 = ch->fifo_buf[p%PULSE_CAP_FIFO_MAXLEN] &0x7fff;
				dur2 = ch->fifo_buf[(p+1) %PULSE_CAP_FIFO_MAXLEN] &0x7fff;
				if ((dur1 + dur2) >= v)
					break;
			}

			if (i >=cPulses)
			{
			    // no leading bit found, step the FIFO
				ch->fifo_tail = p %PULSE_CAP_FIFO_MAXLEN;
				ch->state = PULSE_CAP_STATE_IDLE;
				cPulses =0;
				continue;
			}
			
			// must found the leading bit, remember it
			tmp = p;

			// IDLE.2.2 find the first data bit
			for (; i < cPulses; i++, p +=2)
			{
				dur1 = ch->fifo_buf[p%PULSE_CAP_FIFO_MAXLEN] &0x7fff;
				dur2 = ch->fifo_buf[(p+1) %PULSE_CAP_FIFO_MAXLEN] &0x7fff;
				if ((dur1 + dur2) < v)
					break;
			}

			i++;
			if (tmp < p-2)
				tmp = p-2;

			// validate the scan result now
/*			if (tmp > PULSE_CAP_FIFO_MAXLEN)
			{
				// no leading bit found, release the fifo and set to IDLE
				ch->fifo_tail = p %PULSE_CAP_FIFO_MAXLEN;
				ch->state = PULSE_CAP_STATE_IDLE;
				cPulses =0;
				continue;
			}
*/
			i = ((tmp + PULSE_CAP_FIFO_MAXLEN - ch->fifo_tail) % PULSE_CAP_FIFO_MAXLEN) /2;

			// addressed the leading bit and free the fifo previous
			if (cPulses - i < 5) // too few data bit collected, do it again in the next round
			{
				ch->fifo_tail = tmp %PULSE_CAP_FIFO_MAXLEN;
				ch->state = PULSE_CAP_STATE_IDLE;
				cPulses =0;
				continue;
			}

			// okay how we are good at capturing the start point of a pulses
			ch->pulses_len =0;
			v = ch->shortDur >>2;
//			ch->state = PULSE_CAP_STATE_LEADING_BITS;

//		case PULSE_CAP_STATE_LEADING_BITS:
			// the latest leading bits
			dur1 = ((ch->fifo_buf[tmp++ %PULSE_CAP_FIFO_MAXLEN] &0x7fff) +v) /ch->shortDur;
			dur2=  ((ch->fifo_buf[tmp++ %PULSE_CAP_FIFO_MAXLEN] &0x7fff) +v) /ch->shortDur;
			ch->maxWidth = dur1+dur2;  ch->maxWidth -= ch->maxWidth>>2; // make maxWidth= leading bit's 1.25 times long

//			if (dur1 + dur2)
//			{ // likely not a data 
//				ch->state = PULSE_CAP_STATE_DATA;
//				continue;
//			}

			if (dur1 >0x08 || dur2 > 0x08)
			{
				ch->pulses[ch->pulses_len++] = 0x80|dur1;
				ch->pulses[ch->pulses_len++] = 0x80|dur2;
			}
			else ch->pulses[ch->pulses_len++] = dur1 <<4 | dur2;

			ch->fifo_tail = tmp %PULSE_CAP_FIFO_MAXLEN;
			ch->state = PULSE_CAP_LEADING_BITS;

			// no break, no continue here

		case PULSE_CAP_LEADING_BITS:
		case PULSE_CAP_STATE_DATA:
			v = ch->shortDur >>2;

			// the data bits
 			for (p = ch->fifo_tail; i< cPulses && ch->pulses_len < PULSE_CAP_LOOP_MAXBITS; i++)
			{
				dur1 = ((ch->fifo_buf[p++%PULSE_CAP_FIFO_MAXLEN]&0x7fff) +v) /ch->shortDur;
				dur2=  ((ch->fifo_buf[p++%PULSE_CAP_FIFO_MAXLEN]&0x7fff) +v) /ch->shortDur;

				if (dur1+dur2<10)
					ch->state = PULSE_CAP_STATE_DATA;
					
				// if (dur1 >0x10 || dur2 > 0x10)
				if ((PULSE_CAP_STATE_DATA == (ch->state & 0x03)) && (0 == dur1 || 0 == dur2 || (dur1+ dur2) > ch->maxWidth))
				{
					// rewind the p by 2, then set the tail
					p += PULSE_CAP_FIFO_MAXLEN -2;
					ch->fifo_tail = p %PULSE_CAP_FIFO_MAXLEN;
					PulseCapture_encodePulses(ch, TRUE); 
					ch->pulses_len = 0;
					ch->state = PULSE_CAP_STATE_IDLE;
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
			ch->fifo_tail = p %PULSE_CAP_FIFO_MAXLEN;
			if (ch->pulses_len >= PULSE_CAP_LOOP_MAXBITS)
			{
				PulseCapture_encodePulses(ch, FALSE);
				ch->pulses_len =0;
			}

			cPulses =0;
			continue;
		} // end of switch

		// break; // always break to quit the loop if reach here
	} // end of while

	if (PULSE_CAP_STATE_DATA != (ch->state &0x3) && ch->pulses_len >0)
	{
		PulseCapture_encodePulses(ch, TRUE);
		ch->pulses_len =0;
		return 1;
	}

	return 0;
}
