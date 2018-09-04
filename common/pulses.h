#ifndef __HomeTether_pulses_H__
#define __HomeTether_pulses_H__

#include "htcomm.h"

// -----------------------------
// PulsePin
// -----------------------------
#define PulseSenderFlag_BITNOT  (1<<0)
typedef struct _PulsePin
{
	IO_PIN        pin;
	uint8_t       flags; // combination of PulseSenderFlag_XXXX
} PulsePin;

#define PulsePin_set(_pin, vHigh)        pinSET(_pin.pin, (vHigh?1:0) ^(PulseSenderFlag_BITNOT& _pin.flags))
#define PulsePin_get(_pin)        		 ((pinGET(_pin.pin)?1:0) ^ (PulseSenderFlag_BITNOT& _pin.flags))
#ifdef PulsePin_LOOPBACK_TEST
void PulsePin_send(PulsePin pin, uint16_t durH, uint16_t durL);
#else
#  define PulsePin_send(_pin, durH, durL)	 PulsePin_set(_pin, 1); delayXusec(durH); PulsePin_set(_pin, 0); delayXusec(durL)
#endif // PulsePin_send
// -----------------------------
// Known Pulse Profiles
// -----------------------------
typedef enum _PulseProfId_e
{
	PulseProfId_EV1527 =0, PulseProfId_PT2262 = PulseProfId_EV1527,
	PulseProfId_uPD6121,
	PulseProfId_TC9012F,
	PulseProfId_M7461M,

	PulseProfId_UNKNOWN,
	PulseProfId_MAX = PulseProfId_UNKNOWN
} PulseProfId_e;

uint8_t	Pulse_findProfId(const char* profname);
const char*	Pulse_profName(uint8_t profId);

// -----------------------------
// PulseFrame
// -----------------------------
#define PULSEFRAME_OFFSET_LENGTH   (0)
#define PULSEFRAME_OFFSET_SHORTDUR (1)
#define PULSEFRAME_OFFSET_STYLE_B0 (2)
#define PULSEFRAME_OFFSET_STYLE_B1 (3)
#define PULSEFRAME_OFFSET_SIGDATA  (4)

#define PULSEFRAME_LENGTH(_FRAME)  (_FRAME[PULSEFRAME_OFFSET_LENGTH])
hterr   PulseFrame_send(PulsePin pinOut, uint8_t* frame, uint8_t frameLen, uint8_t repeats);

hterr   PulseFrame_encodeByProf(uint8_t profileId, uint8_t* frame, uint8_t maxLen, uint8_t* codeval, uint8_t repeats);
//@ return the apperiated profileId
uint8_t PulseFrame_decodeByProf(const uint8_t* frame, uint8_t* pCode, uint8_t maxSize);

// -----------------------------
// PulseCapture
// -----------------------------
#define PULSE_CAP_FRAME_MAXLEN	    (31) // must be less than 32
#define PULSE_CAP_FIFO_MAXLEN	    (20)
#define PULSE_CAP_LOOP_MAXBITS      (50)
#define PULSE_CAP_INTV_USEC         (10) // 10 usec

#define PULSE_CAP_STATE              (1<<0)
#define PULSE_CAP_FIFO_OVERFLOW      (1<<4)
#define PULSE_CAP_CAPTURED           (1<<5)

#define PULSE_CAP_CTRL_VAL_H         (1<<0)

typedef struct _PulseCapture
{
	PulsePin capPin;

	uint8_t  ctrl;
	uint8_t  __IO__ state;
	uint16_t fifo_buf[PULSE_CAP_FIFO_MAXLEN];
	uint8_t  __IO__ fifo_header, fifo_tail;
	uint8_t  pulses[PULSE_CAP_LOOP_MAXBITS];
	uint8_t  __IO__ pulses_len;
	uint16_t shortDur, maxWidth;
	uint8_t  frame[PULSE_CAP_FRAME_MAXLEN];
	uint8_t  __IO__ frame_bits;
	uint8_t  offset_bitsLen;
} PulseCapture;

bool PulseCapture_init(PulseCapture* pc);
bool PulseCapture_doParse(PulseCapture* ch);
void PulseCapture_Capfill(PulseCapture* ch, uint16_t dur, bool isHigh);

// callback sample: PulseCapture_OnFrame()
void PulseCapture_OnPulses(PulseCapture* ch);

void PulseCapture_OnFrame(PulseCapture* ch);

// -----------------------------
// About HtFrame
// -----------------------------
uint8_t HtFrame_send(PulsePin pinOut, uint8_t* msg, uint8_t len, uint8_t repeatTimes, uint8_t plusWideUsec);
void    HtFrame_OnReceived(PulseCapture* ch, uint8_t* msg, uint8_t len);

//@note callback HtFrame_OnReceived() will be called if this is a qualified HtFrame
//@note the input frame has been overwitten if the func returns ERR_SUCCESS or ERR_CRC
hterr HtFrame_validateAndDispatch(PulseCapture* ch);

#endif // __HomeTether_pulses_H__
