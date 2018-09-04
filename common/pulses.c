#include "pulses.h"

hterr PulseFrame_send(PulsePin pinOut, uint8_t* frame, uint8_t frameLen, uint8_t repeats)
{
	uint16_t shortDurXusec=0;
	uint8_t i=0, j, len, tmp;
	if (NULL == frame || frameLen < 4)
		return ERR_INVALID_PARAMETER;

	shortDurXusec = eByte2value(frame[0]);

	do {
		for (i=3; i < frameLen; )
		{
			// check the data type
			switch (frame[i] >>6) // the high 2bits
			{
			case 3: // seq of '0','1' bits
				len = frame[i] & 0x3f; // this is a 0-1 bit len
				for (j=0; (j < len) && (i <frameLen); j++)
				{
					tmp = j%8;
					if (0== tmp)
						i++;

					// determine bit '0' or '1'
					if (frame[i] & (0x80>>tmp))
						tmp = frame[PULSEFRAME_OFFSET_STYLE_B1]; // bit '1'
					else tmp = frame[PULSEFRAME_OFFSET_STYLE_B0]; // bit '1'

					PulsePin_send(pinOut, ((tmp>>4)&0x0f)*shortDurXusec, (tmp&0x0f)*shortDurXusec);
				}
				i++;
				break;

			case 0:
			case 1: // a normal short pulse
				tmp = frame[i++];
				PulsePin_send(pinOut, ((tmp>>4)&0x0f)*shortDurXusec, (tmp&0x0f)*shortDurXusec);
				break;

			case 2: // a long pulse
				tmp = frame[i++] & 0x7f;
				if (i < frameLen)
					PulsePin_send(pinOut, tmp*shortDurXusec, (frame[i++] & 0x7f)*shortDurXusec);
				break;
			} // switch
		} // for i < frameLen

	} while (repeats--);

	return ERR_SUCCESS;
}

// ======================================================================
// About known pulse profiles
// ======================================================================
typedef hterr  (*PulseEncoder)(uint8_t* frame, uint8_t maxLen, uint8_t* codeval, uint8_t repeats);
typedef hterr (*PulseDecoder)(const uint8_t* frame, uint8_t* pCode, uint8_t maxSize);

static hterr PulseEncode_EV1527(uint8_t* frame, uint8_t maxLen, uint8_t* codeval, uint8_t repeats);
static hterr PulseDecode_EV1527(const uint8_t* frame, uint8_t* pCode, uint8_t maxSize);

typedef struct _PulseCodecNode
{
	uint8_t profId;
	const char* profName;
	PulseEncoder encoder;
	PulseDecoder decoder;
} PulseCodecNode;

static const PulseCodecNode __pulse_profiles__[] = {
	{ PulseProfId_EV1527,        "EV1527",        PulseEncode_EV1527, PulseDecode_EV1527 },
	{ PulseProfId_uPD6121,       "uPD6121",       NULL, NULL },  //TODO
	{ PulseProfId_TC9012F,       "TC9012F",       NULL, NULL },  //TODO
	{ PulseProfId_M7461M,        "M7461M",        NULL, NULL },  //TODO

	{ PulseProfId_UNKNOWN,       "UNKONWN", NULL, NULL },
	{ PulseProfId_MAX,           NULL, NULL, NULL },
};

uint8_t	Pulse_findProfId(const char* profname)
{
	uint8_t i;
	if (NULL == profname)
		return PulseProfId_UNKNOWN;

	for (i=0; i< PulseProfId_UNKNOWN && __pulse_profiles__[i].profName; i++)
	{
		if (NULL != strstr(__pulse_profiles__[i].profName, profname))
			return i;
	}

	return PulseProfId_UNKNOWN;
}

const char*	Pulse_profName(uint8_t profId)
{
	if (profId > PulseProfId_UNKNOWN)
		profId = PulseProfId_UNKNOWN;

	return __pulse_profiles__[profId].profName;
}

hterr PulseFrame_encodeByProf(uint8_t profileId, uint8_t* frame, uint8_t maxLen, uint8_t* codeval, uint8_t repeats)
{
	int i;
	if (NULL ==frame || maxLen <=3 || NULL == codeval)
		return ERR_INVALID_PARAMETER;

	for (i=0; __pulse_profiles__[i].profId < PulseProfId_UNKNOWN; i++)
	{
		if (profileId == __pulse_profiles__[i].profId)
			break;
	}

	if (__pulse_profiles__[i].profId >= PulseProfId_UNKNOWN)
		return ERR_INVALID_PARAMETER; // not found the apperiated profile

	if (NULL == __pulse_profiles__[i].encoder)
		return ERR_NOT_SUPPORTED; // encoder not available

	return __pulse_profiles__[i].encoder(frame, maxLen, codeval, repeats);
}

//@ return the apperiated profileId
uint8_t PulseFrame_decodeByProf(const uint8_t* frame, uint8_t* pCode, uint8_t maxSize)
{
	uint8_t i;
	if (NULL ==frame || NULL == pCode || maxSize<=0)
		return PulseProfId_UNKNOWN;

	for (i=0; __pulse_profiles__[i].profId < PulseProfId_UNKNOWN; i++)
	{
		if (NULL == __pulse_profiles__[i].decoder)
			continue;

		if (ERR_SUCCESS == __pulse_profiles__[i].decoder(frame, pCode, maxSize))
			return i;
	}

	return PulseProfId_UNKNOWN;
}

// -----------------------------
// PulseEncode_EV1527()
// -----------------------------
#define EV1527_DEFAULT_SHORTDUR    (560) // 0.56msecsec
hterr PulseEncode_EV1527(uint8_t* frame, uint8_t maxLen, uint8_t* codeval, uint8_t repeats)	// recommend to repeat six times when sending
{
	int i =0;
	if (maxLen <9)
		return ERR_INVALID_PARAMETER;

	if (0x00 == frame[i])
		frame[i] = value2eByte(EV1527_DEFAULT_SHORTDUR);

	i =1; frame[i++]=0x13; frame[i++]=0x31; // the bit style
	frame[i++] = 0x8a; frame[i++] = 0x81;  // the leading bit
	frame[i++] = 0xC0 +24; // the data len
	frame[i++] = *(codeval); frame[i++] = *(codeval+1); frame[i++] = *(codeval+2); // the data bytes

	for (maxLen-=9; repeats-- && maxLen>4; maxLen-=4)
	{
		frame[i++] = 0x8a; frame[i++] = 0x81;  // the repeat code
		frame[i++] = 0xC0 +24; // the data len
		frame[i++] = *(codeval); frame[i++] = *(codeval+1); frame[i++] = *(codeval+2); // the data bytes
	}

	return ERR_SUCCESS;
}
 
hterr PulseDecode_EV1527(const uint8_t* frame, uint8_t* pCode, uint8_t maxSize)
{
	if (maxSize<3)
		return ERR_INVALID_PARAMETER;
	
	// verify the bit style
	if (frame[PULSEFRAME_OFFSET_STYLE_B0]!=0x13 || frame[PULSEFRAME_OFFSET_STYLE_B1]!=0x31)
		return ERR_INVALID_PARAMETER;

	// TODO: verify the leading bit
	// if (frame[PULSEFRAME_OFFSET_STYLE_B0]!=0x13 || frame[PULSEFRAME_OFFSET_STYLE_B1]!=0x31)
	//	return ERR_INVALID_PARAMETER;
	
	// the bit len
	if ((0xC0 +24) != frame[PULSEFRAME_OFFSET_SIGDATA])
		return ERR_INVALID_PARAMETER;

	memcpy(pCode, &frame[PULSEFRAME_OFFSET_SIGDATA+1], 3);
	if (maxSize>3)
		memset(pCode+3, 0x00, maxSize-3);

	return ERR_SUCCESS;
}

//@note callback HtFrame_OnReceived() will be called if this is a qualified HtFrame
//@note the input frame has been overwitten if the func returns ERR_SUCCESS or ERR_CRC
hterr HtFrame_validateAndDispatch(PulseCapture* ch)
{
	uint8_t i=0, n=0, len;
	BOOL    succ = TRUE;
	uint8_t* frame =ch->frame;

	// recognazing if this is a HT frame => tmp
	succ = frame[PULSEFRAME_OFFSET_STYLE_B0] == 0x13 && frame[PULSEFRAME_OFFSET_STYLE_B1] == 0x31; // verify the bit 0/1 style
	succ = succ && frame[PULSEFRAME_OFFSET_SIGDATA] ==0x9f && frame[PULSEFRAME_OFFSET_STYLE_B1+2] ==0x81; // verify the leading bit
	succ = succ && (frame[PULSEFRAME_OFFSET_SIGDATA+2] >>6) == 0x03;  // must be bits
	succ = succ && 0x05 == (frame[PULSEFRAME_OFFSET_SIGDATA+3] >>5) && 0xff == (frame[PULSEFRAME_OFFSET_SIGDATA+3] ^ frame[PULSEFRAME_OFFSET_SIGDATA+4] ); // the leading two bytes

	if (!succ)
		return ERR_INVALID_PARAMETER;

	// okay, i think this is like it, decode it to the frame
	len = (frame[PULSEFRAME_OFFSET_SIGDATA+3] & 0x1f); // the payload len of HtFrame
	if (len + PULSEFRAME_OFFSET_SIGDATA +3 >= PULSEFRAME_LENGTH(frame))
		return ERR_UNDERFLOW;

	for (i = PULSEFRAME_OFFSET_SIGDATA+2, n=1; succ && i < PULSEFRAME_LENGTH(frame); ) // && n < ptmpdata->tot_len)
	{
		// the frame should consists of 01-bits
		if (0xC0 != (frame[i] & 0xC0))
		{
			succ = FALSE;
			break;
		}

		len = frame[i++] & 0x3f; // the bitlen
		if (0 != len %8) // test if it is a whole byte
		{
			succ = FALSE;
			break;
		}
		len /=8;

		// len = pbuf_write(ptmpdata, n, &frame[i], len);
		// i+= len, n+=len;
		while (len-- >0 && i < PULSEFRAME_LENGTH(frame))
			frame[n++] = frame[i++];
	}

	frame[0] =n;
	if (n < (frame[1] & 0x1f) +4)
		return ERR_UNDERFLOW;

	n = frame[1] &0x1f;
#ifdef PULSEFRAME_ENCRYPY
	idx = frame[1] ^ frame[n-1];
	for (i=0; i< n-3; i++)
		frame[i+2] ^= encrypt_table[idx^i];
#endif //PULSEFRAME_ENCRYPY

	if (frame[n+3] != calcCRC8(frame+3, n))
		return ERR_CRC;

	HtFrame_OnReceived(ch, frame+3, n);
	return ERR_SUCCESS;
}

/*
// -----------------------------
// The keyboard via Wiegand26
// -----------------------------
//@usecSample the interval of sample in usec
//@readDX     callback to read the signal pin D0 or D1
//@hasSig     callback to read if either D0 or D1	has signals

#define WGPINVAL(_DX) (_DX?1:0)	 // convert the value of DX to value bit
static bool Pulse_readWiegand26(volatile uint32_t* pCode, uint8_t usecSample, ReadPin_f readDX, ReadPin_f hasSig)
{
	int i=0, j;
	uint8_t tmp;

	if (usecSample <=10)
		usecSample =10;

	*pCode = 0;
	for (i=0; i < 26; i++)
	{
		// step 1. test if there is input signal
		for (j = (int) PULSE_1_PL_MAX_DUR_MSEC *1000/usecSample; j>0 && !hasSig(); j--)
		{
			delayXusec(usecSample);
		}

		if (j<=0)
			return FALSE;

		// step 2 read one pin
		tmp = readDX();

		// filter the noise
		delayXusec(usecSample*2);
		if (tmp != readDX())
			return FALSE;

		// test if it is a long signal
		for (j = (int) PULSE_1_PL_MAX_DUR_MSEC *1000/usecSample; j>0; j--)
		{
			if (tmp != readDX())
				break;

			delayXusec(usecSample);
		}

		if (j<=0)
			return FALSE;

		// valid signal here
		*pCode <<=1;
		*pCode |= WGPINVAL(tmp);
	}

	return TRUE;
}
*/
// ======================================================================
// Pulse codecs
// ======================================================================


#ifdef STM32
// the following will be cancelled by covered by PulseSend_byProfile
void IOSend_uPD6121(const IO_PIN* pin, bool HeqH, uint32_t codeval);
void IOSend_PT2262(const IO_PIN* pin, bool HeqH, uint32_t codeval);
void IOSend_TC9012F(const IO_PIN* pin, bool HeqH, uint32_t codeval);
void IOSend_M7461M(const IO_PIN* pin, bool HeqH, uint32_t codeval);
#endif // STM32


#if 0

#define IRRecvRead(_PORT, _PIN) (GPIO_ReadInputDataBit(_PORT, _PIN) ? 0:1)

// out[0] keeps the min duration of plus, out[1] keeps the number of plus in the sequence
int IRRecv(const IO_PIN* pin, int8_t out[], int8_t maxLen)
{
	int i, minDur;
	int16_t signals[PULSE_MAX*2];
	// int8_t  out[MAX_PLUS+2]; 

	minDur = min(maxLen -2, PULSE_MAX); // temporary save as the max length that can receive
	if (minDur <=0)
		return out[1]=0;

	// receive the plus into signals[]
	for (out[1]=0; out[1]<minDur; out[1]++)
	{
		for (i=0; i<PULSE_MAX_DURATION && 1 == IRRecvRead(pin->port, pin-pin); i++)
			delayXusec(PULSE_SAMPLE_INTV_USEC);
		signals[2*out[1]] = i;
		if (i >= PULSE_MAX_DURATION)
			break;

		for (i=0; i<PULSE_MAX_DURATION && 0 ==IRRecvRead(pin->port, pin-pin); i++)
			delayXusec(PULSE_SAMPLE_INTV_USEC);
		signals[2*out[1]+1] = i;
		if (i >= PULSE_MAX_DURATION)
			break;
	}

	// compress the plus sequence
	// step 1. find out the L with minimal duration, keep the value in minDur
	minDur = PULSE_MAX_DURATION;
	for (i=0; i< out[1]; i++)
	{
		if (signals[2*i] >2 && signals[2*i] < minDur)
			minDur = signals[2*i];
	}

	// step 2. divide each duration with the minimal duration and save it in out[2+]
	for (i=0; i< out[1]; i++)
		out[i+2] = (signals[2*i] + signals[2*i+1] + minDur/2) / minDur;

	// step 3. save minDur in out[0] by adjusting the unit to 0.1msec
	out[0] = minDur * PULSE_SAMPLE_INTV_USEC / 100;

	return out[1];
}

// ======================================================================
// about pulse sending
// ======================================================================
static int32_t _pulseusec =0;

static int pulsei, pulsej;
static uint32_t pulseWkcode;

#define clockWLx10usec   (10)

#define sendPulseF(_X_PIN, HeqH, _durH, _durL) \
     { if (HeqH) GPIO_SetBits(_X_PIN->port, _X_PIN->pin);   else GPIO_ResetBits(_X_PIN->port, _X_PIN->pin); delayXusec(_durH); \
	   if (HeqH) GPIO_ResetBits(_X_PIN->port, _X_PIN->pin); else GPIO_SetBits(_X_PIN->port, _X_PIN->pin);   delayXusec(_durL); \
	   _pulseusec +=_durH+_durL; }

#define beginFrame(_codeval)          { _pulseusec =0; pulseWkcode=_codeval; }

#define endFrame(_durFrame) \
     { _pulseusec = _durFrame -_pulseusec-100; if (_pulseusec>0) delayXusec(_pulseusec); _pulseusec=0; }

// -----------------------------
// PulseSend_bySeq()
// -----------------------------
void PulseSend_bySeq(const IO_PIN* pin, bool HeqH, PulseSeq* in)
{
	int i=0;
	if (NULL ==in || NULL == in->seq)
		return;

	for(; i< in->seqlen && in->seq[i] !=0xff; i++)
	{
		if (in->seq[i] >0x80)
		{
			//hidur  = (int32_t) in->baseIntvX10usec * in->short_dur * (0x7f & in->seq[i]);
			//lowdur = (int32_t) in->baseIntvX10usec * in->short_dur * (0x7f & in->seq[++i]);
			sendPulseF(pin, HeqH, 
				(int32_t) in->short_durXusec * (0x7f & in->seq[i]),
				(int32_t) in->short_durXusec * (0x7f & in->seq[++i]));

			if (0xff == in->seq[i])
				break;
		}
		else
		{
			//			hidur  = (int32_t) in->baseIntvX10usec * in->short_dur * (in->seq[i] >>4);
			//			lowdur = (int32_t) in->baseIntvX10usec * in->short_dur * (in->seq[i] & 0x0f);
			sendPulseF(pin, HeqH, 
				(int32_t) in->short_durXusec * (in->seq[i] >>4),
				(int32_t) in->short_durXusec * (in->seq[i] & 0x0f));
		}
	}
}

// ======================================================================
// About pulse sending
// ======================================================================
uint8_t PulseSeq_send(PulseSeq* in, SetPulsePin_f sender)
{
	int i=0;
	uint8_t hdur, ldur;
	if (NULL ==in || in->short_durXusec<=0 || NULL==sender)
		return 0;

	for (i=0; i < in->seqlen; i++)
	{
		if (0xff == in->seq[i])
			break;

		if (in->seq[i] & 0x80)
		{
			hdur = in->seq[i++] & 0x7f;
			ldur = in->seq[i] & 0x7f -hdur;
		}
		else
		{
			hdur = in->seq[i] >>4;
			ldur = in->seq[i] & 0xf - hdur;
		}

		if (hdur >0)
		{
			sender(1);
			delayXusec(in->short_durXusec * hdur);
		}

		if (ldur >0)
		{
			sender(0);
			delayXusec(in->short_durXusec * hdur);
		}

	}

	return i;
}

uint8_t seq[50];
PulseSeq pulses; // = {50, 0, seq};

bool PulseCode_send(PulseCode* in, SetPulsePin_f sender, uint8_t repeatTimes)
{
	if (NULL ==in || NULL==sender)
		return FALSE;

	pulses.seqlen = sizeof(seq) -2;
	if (!Pulses_encodeEx(in, &pulses, 0))
		return FALSE;

	if (!PulseSeq_send(&pulses, sender))
		return FALSE;

	if (repeatTimes <=0)
		return TRUE;

	Pulses_encodeEx(in, &pulses, 1);
	for (; repeatTimes >0; repeatTimes--)
	{
		if (!PulseSeq_send(&pulses, sender))
			break;
	}
	return TRUE;
}


// -----------------------------
// struct PulseSend_Profile
// -----------------------------
typedef struct _PulseSend_Profile
{
	char*    name;
	uint8_t  baseIntvX10usec; // in 10 usec
	uint8_t  bitsCount;  // number of data bits in one message 
	uint8_t  sigLeading[2], sigZero[4], sigOne[4], sigRepeat[2]; // duration of H/L mutified with sampleInterval
	uint16_t flags; // b0-3 repeatCount, b4-pulse_firstL
} PulseSend_Profile;

// -----------------------------
// _PulseSend_Profiles: the known profiles
// -----------------------------
const static PulseSend_Profile _PulseSend_Profiles[] =
{
	{"uPD6121G", 56, 32, {16,8}, {1,1,0,0}, {1,3,0,0}, {40,1}, 0x07 },	  // D3 = /D2
	{"TC9012F",  56, 32, {8,8},  {1,1,0,0}, {1,3,0,0}, {40,1}, 0x07 },	  // D3 = /D2
	{"M50560",   50, 16, {8,8},  {1,1,0,0}, {1,3,0,0}, {40,1}, 0x47 },	
	{"LC7461M",  56, 32, {16,8}, {1,1,0,0}, {1,3,0,0}, {40,1}, 0x07 },	  // D1=/D0, D3=/D2
	{"PT2262",   56, 12, {1,31}, {1,3,1,3}, {3,1,3,1}, {0,140}, 0x06 },
	{"EV1527",   56, 24, {1,31}, {1,3,0,0}, {3,1,0,0}, {0,140}, 0x06 },
	{NULL,        0,  0, {0,0},  {0,0,0,0}, {0,0,0,0}, {0,0},  0x00 }	  // NULL terminator
};

// -----------------------------
// PulseSend_byProfileId()
// -----------------------------
void PulseSend_byProfileId(uint8_t profileId, uint32_t codeval, const IO_PIN* pin, uint8_t HeqH, uint32_t baseIntvXusec)
{
	int i=0, j=0;
	const PulseSend_Profile* profile = NULL;
	if (NULL == pin || profileId > (sizeof(_PulseSend_Profiles)/sizeof(PulseSend_Profile) -2))
		return;
	
	profile = &_PulseSend_Profiles[profileId];
	HeqH = HeqH ? 1:0;
	HeqH ^= (profile->flags >>4) & 0x01;

	if (0==baseIntvXusec)
		baseIntvXusec = ((uint32_t) profile->baseIntvX10usec) *10;

	// the repeat loop
	for (i=0; i <= (profile->flags & 0xf); i++)
	{
		if (0==i) // the leading signal
			{sendPulseF(pin, HeqH, baseIntvXusec * profile->sigLeading[0], baseIntvXusec * profile->sigLeading[1]);}
		else // the repeat signal
			{sendPulseF(pin, HeqH, baseIntvXusec * profile->sigRepeat[0], baseIntvXusec * profile->sigRepeat[1]);}

		// sending the bits, MSMF
		for (j=0; j < profile->bitsCount; j++, codeval>>=1)
		{
			if (codeval &1)
			{
				// send the "1"
				sendPulseF(pin, HeqH, baseIntvXusec * profile->sigOne[0], baseIntvXusec * profile->sigOne[1]);

				if (0 == profile->sigOne[2])
					continue;

				sendPulseF(pin, HeqH, baseIntvXusec * profile->sigOne[2], baseIntvXusec * profile->sigOne[3]);
			}
			else
			{
				// send the "0"
				sendPulseF(pin, HeqH, baseIntvXusec * profile->sigZero[0], baseIntvXusec * profile->sigZero[1]);

				if (0 == profile->sigOne[2])
					continue;

				sendPulseF(pin, HeqH, baseIntvXusec * profile->sigZero[2], baseIntvXusec * profile->sigZero[3]);
			}
		}

	}
}


// -----------------------------
// PulseSend_byProfile()
// -----------------------------
void PulseSend_byProfile(char* profileName, uint32_t codeval, const IO_PIN* pin, uint8_t HeqH, uint32_t baseIntvXusec)
{
	uint8_t profId=0;
	for (profId=0; NULL != _PulseSend_Profiles[profId].name && 0 != strcmp(_PulseSend_Profiles[profId].name, profileName); profId++);
	PulseSend_byProfileId(profId, codeval, pin, HeqH, baseIntvXusec);
}

void IOSend_uPD6121(const IO_PIN* pin, bool HeqH, uint32_t codeval)
{
	for (pulsej=0; pulsej <6; pulsej++) // repeat six times by default
	{
		beginFrame(codeval);

		if (pulsej>0)
		{
			// send the repeat signal
			sendPulseF(pin, HeqH, 9000, 2250);	
		}
		else  sendPulseF(pin, HeqH, 9000, 4500); // send the lead signal

		// 8bit address + 4bit data codeval, every bit takes 2bit of codeval: 0=00, 1=01, f=11
		for (pulsei=32; pulsei>0; pulsei--)
		{
			if (pulseWkcode & 0x1)
			{ sendPulseF(pin, HeqH, 560, 2250-560); } // send the '1': H=0.56ms, L=2.250-0.56msec
			else { sendPulseF(pin, HeqH, 560, 1125-560); } // send the '0': H=0.56ms, L=1.125-0.56msec

			pulseWkcode <<=1;
		}

		endFrame(108000); // time of frame =108msec
	}

}

void IOSend_TC9012F(const IO_PIN* pin, bool HeqH, uint32_t codeval)
{
	for (pulsej=0; pulsej <6; pulsej++) // repeat six times by default
	{
		beginFrame(codeval);

		if (pulsej>0)
		{
			// send the repeat codeval
			_pulseusec =0;
			sendPulseF(pin, HeqH, 4500, 4500);	
			if (pulseWkcode & 0x1)
				{ sendPulseF(pin, HeqH, 560, 1125-560); }
			else { sendPulseF(pin, HeqH, 560, 2250-560); }
		}
		else sendPulseF(pin, HeqH, 4500, 4500); // send the lead signal: H=4.5ms, L=4.5ms

		// 8bit address + 4bit data codeval, every bit takes 2bit of codeval: 0=00, 1=01, f=11
		for (pulsei=32; pulsei>0; pulsei--)
		{
			if (pulseWkcode & 0x1)
			{ sendPulseF(pin, HeqH, 560, 2250-560); } // send the '1': H=0.56ms, L=2.250-0.56msec
			else { sendPulseF(pin, HeqH, 560, 1125-560); } // send the '0': H=0.56ms, L=1.125-0.56msec

			pulseWkcode <<=1;
		}

		endFrame(108000); // time of frame =108msec
	}
}

void IOSend_M7461M(const IO_PIN* pin, bool HeqH, uint32_t codeval)
{
	for (pulsej=0; pulsej <6; pulsej++) // repeat six times by default
	{
		beginFrame(codeval);

		if (pulsej>0)
		{
			// send the repeat signal H=9ms, L=4.5ms
			_pulseusec =0;
			sendPulseF(pin, HeqH, 9000, 4500);
		}
		else sendPulseF(pin, HeqH, 9000, 4500); // send the lead signal: H=9ms, L=4.5ms

		// 8bit address + 4bit data codeval, every bit takes 2bit of codeval: 0=00, 1=01, f=11
		for (pulsei=32; pulsei>0; pulsei--)
		{
			if (pulseWkcode & 0x1)
			{ sendPulseF(pin, HeqH, 560, 2250-560); } // send the '1': H=0.56ms, L=2.250-0.56msec
			else { sendPulseF(pin, HeqH, 560, 1125-560); } // send the '0': H=0.56ms, L=1.125-0.56msec

			pulseWkcode <<=1;
		}

		endFrame(108000); // time of frame =108msec
	}
}

#endif // STM32
