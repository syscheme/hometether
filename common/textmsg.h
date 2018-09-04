#ifndef __TEXT_MSG_H__
#define __TEXT_MSG_H__

#include "htcomm.h"
#include "pbuf.h"

#ifndef TEXTMSG_MAX_LEN
#  define TEXTMSG_MAX_LEN    (PBUF_MEMPOOL2_BSIZE/2) // take the larger pool block size as the default
#endif

#define TEXTMSG_ARGVS_MAX     (5)

// -----------------------------
// a URL-like decoding
// -----------------------------
// text message are in format of:
//   REQ>>  +<cseq><text-message>\0
//   RESP>> -<cseq><text-message>\0
// where cseq         is 2char hex byte
//       text-message is url-encoded text
// will dispatched to 
//       void TextMsg_OnRequest(uint16_t cseq, uint8_t* msg, uint16_t len)
//       void TextMsg_OnResponse(uint16_t cseq, uint8_t* msg, uint16_t len)
//@see-also TextMsg_composeXXX in pbuf.h
#define TEXTMSG_TYPECH_REQ   '+'
#define TEXTMSG_TYPECH_RESP  '-'

#define TEXTMSG_VERBCH_SET   '!'
#define TEXTMSG_VERBCH_GET   '?'
#define TEXTMSG_VERBCH_OTH   '>'

int16_t  TextMsg_encode(const void* source, int16_t slen, char* deststr, int16_t maxdlen);
uint16_t TextMsg_decode(uint8_t* source);
uint8_t  TextMsg_queryStr2KLVs(char* qstr, KLV klvs[], uint8_t maxKlvs);
uint16_t  TextMsg_KLVs2queryStr(char* qstr, uint16_t maxLen, uint8_t klvc, const KLV klvs[]);
uint16_t  TextMsg_KLVs2Json(char* jstr, uint16_t maxLen, uint8_t klvc, const KLV klvs[]);

pbuf* TextMsg_composeRequest(char chVerb, uint8_t klvc, const KLV klvs[]);
pbuf* TextMsg_composeResponse(uint8_t cseq, uint8_t klvc, const KLV klvs[]);

// portals
// void TextMsg_OnRequest(void* netIf, uint8_t cseq, char* msg);
void TextMsg_OnResponse(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[]);
void TextMsg_OnSetRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[]);
void TextMsg_OnGetRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[]);
void TextMsg_OnPostRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[]);

typedef void (*cbOnLineReceived_f)(void* netIf, char* line);
void     TextMsg_procLine(void* netIf, char* textMsg); // template impl of cbOnLineReceived_f
typedef struct _TextLineCollector
{
	uint8_t* workbuf;
	__IO__ uint16_t offset;
	cbOnLineReceived_f cbOnLine;
	void* netIf;
} TextLineCollector;

#define TLC_DEFAULT_LINES  (20);

hterr TextLineCollector_init(TextLineCollector* tlc, cbOnLineReceived_f cbOnLine, void* netIf);
// void TextLineCollector_pushc(TextLineCollector* tlc, uint8_t ch);
void TextLineCollector_push(TextLineCollector* tlc, char* str, uint16_t len);

#define CONST_STR_LEN2(_STR)            (sizeof(_STR) -1)
#define CONST_STR_AND_LEN2(_STR)        _STR, CONST_STR_LEN2(_STR)

#define CONST_STR_LEN(_STR)            CONST_STR_LEN2(#_STR)
#define CONST_STR_AND_LEN(_STR)        CONST_STR_AND_LEN2(#_STR)

#endif // __TEXT_MSG_H__

