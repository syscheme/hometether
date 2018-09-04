#include "textmsg.h"

// its encode takes pbuf and is impled in pbuf.c: pbuf* HtText_encode(const void* source, int8_t len)
uint16_t TextMsg_decode(uint8_t* source)
{
	uint16_t len =0;
	uint8_t *pSrc = source;
	uint8_t hi, lo;

	if (NULL == pSrc)
		return 0;

	while (*pSrc)
	{
		if ('%' == *pSrc)
		{
			hi =hexchval(*(++pSrc));
			lo =hexchval(*(++pSrc));
			if ((hi & 0xf0)|| (lo &0xf0))
				return len;

			source[len++] = (hi <<4) | lo;
			pSrc++;
			continue;
		}

		source[len++] = *pSrc++;
	}

	return len;
}

uint8_t TextMsg_queryStr2KLVs(char* qstr, KLV klvs[], uint8_t maxKlvs)
{
	char* p = qstr, *q, *t= p;
	uint8_t i; //, v;

	for (i=0; i <maxKlvs && p; i++)
	{
		memset(&klvs[i], 0x00, sizeof(KLV));
		q = strchr(p, '='), t = strchr(p, '&');
		klvs[i].key      = p;

		if (NULL == q) // no '=' found
		{
			klvs[i].val.addr = (uint8_t*)t;
			klvs[i].val.len  = 0;
		}
		else if (q <t || NULL ==t)
		{
			// '=' found
			*q++ ='\0';
			klvs[i].val.addr = (uint8_t*)q;
			klvs[i].val.len  = 0;
		}

		if (t >p)
		{
			*t++ = '\0';
			if (NULL != klvs[i].val.addr)
				klvs[i].val.len = (uint8_t) (((uint8_t*)t) - klvs[i].val.addr -1);
		}
		else
		{
			t = NULL;
			if (NULL != klvs[i].val.addr)
				klvs[i].val.len = (uint8_t) strlen((char*)klvs[i].val.addr);
		}

		p = t;

		// convert the value from %-encoded string to bin-value
		klvs[i].val.len = (uint8_t) TextMsg_decode(klvs[i].val.addr);

		//q = t =klvs[i].val.addr;
		//while (q < ((uint8_t*)klvs[i].val.addr) + klvs[i].val.len)
		//{
		//	if (2 != hex2byte(q, &v))
		//		break;
		//	*t++ = v;
		//	q +=2;
		//}
		//klvs[i].val.len = t - klvs[i].val.addr;
	}

	return i;
}

uint16_t TextMsg_KLVs2queryStr(char* qstr, uint16_t maxLen, uint8_t klvc, const KLV klvs[])
{
	char* p = qstr;
	uint8_t i, j, v;

	for (i=0; i <klvc && p<qstr+maxLen; i++)
	{
		if (NULL == klvs[i].key && p+strlen(klvs[i].key)+4 >=qstr+maxLen)
			break;

		strcpy(p, klvs[i].key), p +=strlen(p);
		*p++= '=';

		// p += TextMsg_encode(klvs[i].val.addr, klvs[i].val.len, p, maxLen -3);
		if (MRFLG_String & klvs[i].val.flags)
		{
			memcpy(p, klvs[i].val.addr, klvs[i].val.len);
			p += klvs[i].val.len;
		}
		else
		{
			for (j=0; j< klvs[i].val.len && p < qstr+maxLen -3; j++)
			{
				v = klvs[i].val.addr[j];
				*p++='%', *p++= hexchar(v>>4), *p++= hexchar(v & 0x0f);
			}
		}

		*p++= '&', *p= '\0';
	}

	if ('&' == *(p-1))
		* --p ='\0';

	return (uint16_t) (p - qstr);
}

uint16_t TextMsg_KLVs2Json(char* jstr, uint16_t maxLen, uint8_t klvc, const KLV klvs[])
{
	char* p = jstr;
	uint8_t v;
	int8_t i, j, k, itLen, itSize;

	for (i=0; i <klvc && p< jstr +maxLen -6; i++)
	{
		if (NULL == klvs[i].key && p+strlen(klvs[i].key)+6 >=jstr+maxLen)
			break;

		*p++= '"', strcpy(p, klvs[i].key), p +=strlen(p), *p++= '"', *p++= ':';  // "key":

		// p += TextMsg_encode(klvs[i].val.addr, klvs[i].val.len, p, maxLen -3);
		if (MRFLG_String & klvs[i].val.flags)
		{
			*p++= '"'; 
			memcpy(p, klvs[i].val.addr, klvs[i].val.len);
			p += klvs[i].val.len;
			*p++= '"';
		}
		else
		{
			itLen = MRFLG_IteratorSize(klvs[i].val);
			itSize = klvs[i].val.len / itLen; 

			if (itSize>1) // as array
				*p++= '[';

			if ((MRFLG_HexFmt & klvs[i].val.flags) || itLen>4)
			{
				for (j=0; j< itSize && p < jstr+maxLen -6 - itLen*2; j++)
				{
					*p++='0',*p++='x';
					for (k =itLen-1; k>=0; k--) // little endian
					{
						v = klvs[i].val.addr[j*itLen + k];
						*p++= hexchar(v>>4), *p++= hexchar(v & 0x0f);
					}
					*p++=',';
				}
			}
			else
			{
				switch(itLen)
				{
				case 1:
					p += snprintf(p, jstr+maxLen -p-2, "%d,", *((int8_t*)(klvs[i].val.addr +j*itLen)));
					break;
				case 2:
					p += snprintf(p, jstr+maxLen -p-2, "%d,", *((int16_t*)(klvs[i].val.addr +j*itLen)));
					break;
				case 4:
					p += snprintf(p, jstr+maxLen -p-2, "%d,", *((int32_t*)(klvs[i].val.addr +j*itLen)));
					break;
				default:
					break;
				}
			}

			if (',' == *(p-1))
				* (--p) ='\0';

			if (itSize>1) // as array
				*p++= ']';
		}

		*p++= ',', *p= '\0';
	}

	if (',' == *(p-1))
		* (--p) ='\0';

	return (uint16_t) (p - jstr);
}

void TextMsg_procLine(void* netIf, char* textMsg)
{
	KLV _klvs[TEXTMSG_ARGVS_MAX];
	uint8_t cseq =0, len=0, klvc=0;
	char     verb=0;
	// text requests are in format of: +<cseq><req-line>\0
	// where cseq is 2char hex byte
	// req-line is url-encoded
	if (TEXTMSG_TYPECH_REQ != *textMsg || TEXTMSG_TYPECH_REQ != *textMsg)
		return;

	hex2byte(textMsg+1, &cseq);
	len = (uint8_t) strlen(textMsg);
	if (len <=4)
		return;

	verb = textMsg[3];
	klvc = TextMsg_queryStr2KLVs(textMsg+4, _klvs, TEXTMSG_ARGVS_MAX);
	if (TEXTMSG_TYPECH_RESP == *textMsg)
	{
		TextMsg_OnResponse(netIf, cseq, klvc, _klvs);
		return;
	}

	switch(verb)
	{
	case TEXTMSG_VERBCH_SET:
		TextMsg_OnSetRequest(netIf, cseq, klvc, _klvs);
		return;

	case TEXTMSG_VERBCH_GET:
		TextMsg_OnGetRequest(netIf, cseq, klvc, _klvs);
		return;

	case TEXTMSG_VERBCH_OTH:
	default:
		TextMsg_OnPostRequest(netIf, cseq, klvc, _klvs);
		return;
	}

//	len = TextMsg_decode((uint8_t*) textMsg+3);
//	*(((char*) textMsg) +3 +len) = '\0';
//	if (TEXTMSG_TYPECH_REQ == *textMsg)
//	{
//		TextMsg_OnRequest(netIf, cseq, (char*) textMsg+3);
//		return;
//	}
//
//	if (TEXTMSG_TYPECH_RESP == *textMsg)
//	{
//		TextMsg_OnResponse(netIf, cseq, (char*) textMsg+3);
//		return;
//	}
}

#ifndef isprint
#define isprint(c)  (c>=0x20 && c<0x7f)
#endif // isprint

// this encode is copied from URL by enlarging the allowed characters to all printables except " and %
int16_t TextMsg_encode(const void* source, int16_t slen, char* deststr, int16_t maxdlen)
{
	//	pbuf* p=NULL, *q=NULL;
	uint16_t i;
	uint8_t hi, lo;
	const uint8_t* sptr=(const uint8_t *)source;
	char* p = deststr;
	static const uint8_t PERC ='%';

	if (NULL == sptr || NULL == p || maxdlen<=0)
		return 0;

	if (slen <=0)
		slen = (int16_t) strlen((char*) sptr);

	for (i=0; i <slen && p < deststr+maxdlen; i++)
	{
		// The ASCII characters digits or letters, and ".", "-", "*", "_"
		// remain the same
		if ('"'!=sptr[i] && '%'!=sptr[i] && isprint(sptr[i]))
		{
			*p++ = sptr[i];
			continue;
		}

		//All other characters are converted into the 3-character string "%xy",
		// where xy is the two-digit hexadecimal representation of the lower
		// 8-bits of the character

		if (p+3 > deststr+maxdlen)
			break;

		hi= (sptr[i] >>4) & 0x0f;
		lo= sptr[i] & 0x0f;
		hi+=(hi<10)? '0' : ('a' -10);
		lo+=(lo<10)? '0' : ('a' -10);

		*p++ = PERC, *p++ = hi, *p++= lo;
	}

	return (int16_t) (p -deststr);
}

pbuf* TextMsg_compose(char chReqResp, char chVerb, uint8_t cseq, uint8_t klvc, const KLV klvs[])
{
	uint8_t len=0;
	pbuf* p = NULL;
	static char msgbuf[TEXTMSG_MAX_LEN];

	if (klvc <=0)
		return NULL;
	
	CRITICAL_ENTER();
	msgbuf[len++] = chReqResp;
	msgbuf[len++] = hexchar(cseq>>4);
	msgbuf[len++] = hexchar(cseq&0x0f);
	msgbuf[len++] = chVerb;
	len += TextMsg_KLVs2queryStr(msgbuf+len, sizeof(msgbuf)-len, klvc, klvs);

	if (len <=5 || (sizeof(msgbuf) -len) < 5)
	{
		CRITICAL_LEAVE();
		return NULL;
	}

	msgbuf[len++] ='\n', msgbuf[len++] ='\0';
	p = pbuf_dup(msgbuf, len);

	CRITICAL_LEAVE();
	return p;
}

pbuf* TextMsg_composeRequest(char chVerb, uint8_t klvc, const KLV klvs[])
{
	static uint8_t _gcseq =0;
	if (_gcseq >253)
		_gcseq =0;
		
	return TextMsg_compose(TEXTMSG_TYPECH_REQ, chVerb, ++_gcseq, klvc, klvs);
}

pbuf* TextMsg_composeResponse(uint8_t cseq, uint8_t klvc, const KLV klvs[])
{
	return TextMsg_compose(TEXTMSG_TYPECH_RESP, TEXTMSG_VERBCH_OTH, cseq, klvc, klvs);
}

#ifdef heap_malloc
hterr TextLineCollector_init(TextLineCollector* tlc, cbOnLineReceived_f cbOnLine, void* netIf)
{
	if (NULL == tlc)
		return ERR_INVALID_PARAMETER;
	tlc->offset=0;
	tlc->cbOnLine = NULL;
	tlc->workbuf = heap_malloc(TEXTMSG_MAX_LEN);
	if (NULL == tlc->workbuf)
		return ERR_NOT_ENOUGH_MEMORY;

	tlc->netIf = netIf;
	tlc->cbOnLine = cbOnLine;
	return ERR_SUCCESS;
}

void TextLineCollector_push(TextLineCollector* tlc, char* str, uint16_t len)
{
	BOOL eol = FALSE;
	if (NULL == tlc)
		return;

	if (NULL == tlc->workbuf)
		return; 

	while(len--)
	{
		do {
			tlc->workbuf[tlc->offset] = *str++;

			// a \0 or \n is known as the data end of line
			if ('\0' == tlc->workbuf[tlc->offset] || '\n' == tlc->workbuf[tlc->offset])
			{
				eol = TRUE;
				break;
			}

			// a \r will be ignored by not stepping offset
			if ('\r' == tlc->workbuf[tlc->offset])
				break;

			// step the offset and check if about overflow
			if (++tlc->offset > TEXTMSG_MAX_LEN -3)
			{
				eol = TRUE;
				break;
			}
		} while (0);

		if (!eol)
			continue;

		tlc->workbuf[tlc->offset++] = '\0'; // terminate the string
		if (tlc->cbOnLine)
			tlc->cbOnLine(tlc->netIf, (char*) tlc->workbuf);

		tlc->offset =0; // reset the offset
	}
}
#endif // heap_malloc
