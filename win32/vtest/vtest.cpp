#define SCR_TRACE

// vtest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <process.h>

extern "C" {
#include "..\..\common\pbuf.h"
#include "..\..\common\htddl.h"
#include "..\..\common\pulses.h"
#include "..\..\common\nrf24l01.h"
#include <stdio.h>
}

#include<vector>
#include<string>

typedef unsigned char uint8;

class Buffer
{
protected:
	typedef struct _BD
	{
		uint8* data;
		size_t len, size;
	} BD;

	typedef std::vector < BD > BDList;

	BDList _bdlist;
	bool   _bReference;
	size_t _totalSize;

public:
	Buffer(bool bReference = false)
		: _bReference(bReference), _totalSize(0)
	{
	}

	virtual ~Buffer()
	{
		if (_bReference)
			return;

		// else free buffers in _bdlist that have been allocated locally
		for (BDList::iterator itBD = _bdlist.begin(); itBD < _bdlist.end(); itBD++)
		{
			if (NULL == itBD->data)
				continue;
			delete [] itBD->data;
			itBD->data=NULL;
		}

		_bdlist.clear();
	}

	size_t size() const
	{
		return _totalSize;
	}

	size_t length() const
	{
		size_t len =0;
		for (BDList::const_iterator itBD = _bdlist.begin(); itBD < _bdlist.end(); itBD++)
		{
			if (NULL == itBD->data)
				continue;
			len += itBD->len;
		}

		return len;
	}

	// join a new piece of buffer in, only allowed when this buffer is reference-only
	//@return the total size of buffer after join
	virtual size_t join(uint8* buf, size_t size)
	{
		if (!_bReference || NULL == buf || size <=0)
			return _totalSize;

		return doJoin(buf, size, size);
	}

#define PAGE_SIZE (4096)

	// fill data into this buffer. 
	//  - if this buffer is not refrerence only, it may grow if space is not enough to complete the filling
	//  - otherwise, the filling stops when the space in buffer is run out
	// return the bytes has been filled into this Buffer
	virtual size_t fill(uint8* buf, size_t offset, size_t len)
	{
		// step 1. seek to the right bd per offset
		size_t offsetInBuffer =0, offsetInBD =0;
		BDList::iterator itBD = _bdlist.begin();
		for (; itBD < _bdlist.end(); itBD++)
		{
			if ((offsetInBuffer+itBD->size) < offset)
			{
				offsetInBuffer += itBD->size;
				continue;
			}

			offsetInBD = offset - offsetInBuffer;
			break;
		}

		size_t nLeft = len;
		for (; nLeft >0 && itBD < _bdlist.end(); itBD++, offsetInBD=0)
		{
			size_t nBytes = itBD->size - offsetInBD;
			if (nBytes > nLeft)
				nBytes = nLeft;

			if (nBytes >0)
			{
				memcpy(itBD->data + offsetInBD, buf + len -nLeft, nBytes);
				nLeft -= nBytes;
				itBD->len = offsetInBD + nBytes;
			}
		}

		while (!_bReference && nLeft >0) // reached the BDList end but not yet filled completed
		{
			uint8* pNewBuf = new uint8[PAGE_SIZE];
			if (NULL == pNewBuf)
				break;

			size_t nBytes = (nLeft > PAGE_SIZE) ? PAGE_SIZE : nLeft;
			doJoin(pNewBuf, PAGE_SIZE, nBytes);

			memcpy(pNewBuf, buf + len -nLeft, nBytes);
			nLeft -= nBytes;
		}

		return (len -nLeft);
	}

	virtual size_t read(uint8* buf, size_t offset, size_t len)
	{
		// step 1. seek to the right bd per offset
		size_t offsetInBuffer =0, offsetInBD =0;
		BDList::iterator itBD = _bdlist.begin();
		for (; itBD < _bdlist.end(); itBD++)
		{
			if ((offsetInBuffer+itBD->len) < offset)
			{
				offsetInBuffer += itBD->len;
				continue;
			}

			offsetInBD = offset - offsetInBuffer;
			break;
		}

		// step 2. read the data from this Buffer to the output buf space
		size_t nLeft = len;
		for (; nLeft >0 && itBD < _bdlist.end(); itBD++, offsetInBD=0)
		{
			size_t nBytes = itBD->len - offsetInBD;
			if (nBytes > nLeft)
				nBytes = nLeft;

			if (nBytes >0)
			{
				memcpy(buf + len -nLeft, itBD->data + offsetInBD, nBytes);
				nLeft -= nBytes;
			}
		}

		return (len -nLeft);
	}

		// read data out from this buffer into a string 
	// return the bytes has been read from this BufferList
	size_t readToString(std::string& str, size_t offset=0, size_t len=-1)
	{
		// step 1. seek to the right bd per offset
		size_t offsetInBuffer =0, offsetInBD =0;
		BDList::iterator itBD = _bdlist.begin();
		for (; itBD < _bdlist.end(); itBD++)
		{
			if ((offsetInBuffer+itBD->len) < offset)
			{
				offsetInBuffer += itBD->len;
				continue;
			}

			offsetInBD = offset - offsetInBuffer;
			break;
		}

		// step 2. read the data from this BufferList to the output buf space
		size_t nLeft = len;
		for (; (len <0 || nLeft >0) && itBD < _bdlist.end(); itBD++, offsetInBD=0)
		{
			size_t nBytes = itBD->len - offsetInBD;
			if (nBytes > nLeft)
				nBytes = nLeft;

			if (nBytes >0)
			{
				str.append((char*)(itBD->data + offsetInBD), nBytes);
				nLeft -= nBytes;
			}
		}

		return (len -nLeft);
	}

private:
		// join a new piece of buffer in
	//@return the total size of buffer after join
	virtual size_t doJoin(uint8* buf, size_t size, size_t len)
	{
		BD bd;
		bd.data = buf;
		bd.size = size;
		bd.len =len;
		_bdlist.push_back(bd);
		_totalSize += size;
		return _totalSize;
	}

};

int testmain()
{
	std::vector<int>	 list;
	for (int i =0; i < 40; i++)
		list.push_back(i);

	list.erase(list.begin() + 50, list.end());
	for (int i =0; i < list.size(); i++)
		printf("%d, ", list[i] );

	return 0;
}

/*
uint16 sigs[] ={,0};
nextSig()
{
	static idx=0;

}
*/

const char* err="SessStateInService  	[T00003E6C] sess[5aZEw2gAv0mDEJpAo4xiVz:Provisioned(1)] failed to commit the stream[STREAM/59f3fcea-9e38-485e-8c21-39fce034a990 -t:tcp -h 192.168.81.112 -p 10800 -t 15000] due to TianShanIce::ServerError[doAction(SETUP) failed; error[503 ]";

/*
// -----------------------------
// callback sample: PulseChannel_printPulseSeq()
// -----------------------------
void PulseChannel_printPulseSeq(PulseChannel* ch)
{
// byte 0        1        2        3        4        5                   x    
//     +--------+--------+--------+--------+--------+--------...--------+--------+
//     |  0x55  | msgLen |shortDur|style-b0|style-b1| {pluse|len->data} |  crc8  |
//     +--------+--------+--------+--------+--------+--------...--------+--------+
	printf("55%02x", ch->frame_len);
	for (int i=0; i < ch->frame_len; i++)
		printf("%02x", ch->frame[i]);
}
*/
PulseCapture ch = { {&ch,0x00}, };

// -----------------------------
// callback sample: PulseChannel_OnFrame()
// -----------------------------
extern "C" void HtFrame_OnReceived(PulseCapture* ch, uint8_t* msg, uint8_t len)
{
	printf("htframe[%dB]: ", len);
	for (; len>0; len--)
		printf("%02x ", *(msg++));
	printf("\n");
}

extern "C" void PulseCapture_OnFrame(PulseCapture* ch)
{
	int i=0;
	printf("frame[%dB,%dusec,0-%02xH,1-%02xH]: ", PULSEFRAME_LENGTH(ch->frame), eByte2value(ch->frame[PULSEFRAME_OFFSET_SHORTDUR]), ch->frame[PULSEFRAME_OFFSET_STYLE_B0], ch->frame[PULSEFRAME_OFFSET_STYLE_B1]);
	for (i=PULSEFRAME_OFFSET_SIGDATA; i< PULSEFRAME_LENGTH(ch->frame); i++)
		printf("%02x ", ch->frame[i]);
	printf("\n");

	HtFrame_validateAndDispatch(ch);
}


void PulsePin_send(PulsePin pin, uint16_t durH, uint16_t durL)
{
	PulseCapture_Capfill(&ch, durH, 1);
	PulseCapture_Capfill(&ch, durL, 0);
}

unsigned __stdcall _execute(void *thread)
{
	PulseCapture_init(&ch);
	uint8_t msg[] ={0x1f,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef}, msglen= 1; // sizeof(msg);
	while (1)
	{
		Sleep(1);
		++(*((volatile uint16_t*)msg));
		HtFrame_send(ch.capPin, msg, 5, 1, 0x10);
	}
	return 0;
}

uint16_t vs[] = { 123, 45, 23334, 56778, 68777, 98333, 73444, 9999, 0};


static const char idchars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static const int  nidchars = sizeof(idchars) -1;
typedef unsigned __int64 uint64;

int encodeCompactIdstr(uint64 id, char* buf, const int bufSize, bool bCaseSensitve)
{
	if (NULL == buf || bufSize<=0)
		return 0;

	int nchars = bCaseSensitve ? nidchars : (nidchars-26);

	char *p = buf + bufSize -1, *q= buf;
	for(*p-- ='\0'; id >0 && p>=buf; p--)
	{
		*p = idchars[(int) (id % nchars)];
		id = id / nchars;
	}

	for (p++; '\0' != *p; p++, q++)
		*q = *p;

	*q= '\0';

	return strlen(buf);
}

uint64 decoderCompactIdstr(char* str, bool bCaseSensitve)
{
	uint64 v = 0;
	uint8  base = nidchars, ch, i;
	if (!bCaseSensitve)
		base -=26;
	for (int j=0; str[j]; j++)
	{
		ch = bCaseSensitve?str[j]:toupper(str[j]);
		for (i=0; i < base; i++)
		{
			if (idchars[i] == ch)
				break;
		}
		if (i >=base)
			return v; // illegal ch reached
		v *= base;
		v += i;
	}

	return v;
}


int _tmain(int argc, _TCHAR* argv[])
{
	char idstr[100];
	uint64_t a=0x0123456789abcdef;
	encodeCompactIdstr(a, idstr, sizeof(idstr)-2, true);
	a=0;
	a=decoderCompactIdstr(idstr, true);

	uint16_t v0, v1, m;
	pbuf* p[10];
	for (int i =0; i < 10; i++)
		p[i] = pbuf_palloc(257);

	for (int i =0; i < 10; i++)
		pbuf_ref(p[i]);

	for (int i =0; i < 10; i++)
		pbuf_free(p[i]);

	for (int i =0; i < 10; i++)
		pbuf_free(p[i]);

	//	for (int vi=0; vs[vi]; vi++)
//	{ 
//		v0 = vs[vi];
	float mind=0;
	uint8_t e = value2eByte(0);
	v1 = eByte2value(0);
	for (v0=10; v0<100; v0++)
	{ 
		e = value2eByte(v0);
		v1 = eByte2value(e);
		float f= ((float)abs(v1-v0)) *100/v0;
		if (mind<f)
		{
			mind =f;
			m=v0;
		}

		printf("v[%d]->e[%02x]->v[%d]: d[%.2f%%]\n", v0, e, v1, f);
	}
//	printf("size[%d] max[%d]: d[%.2f%%] maxv[%ld]\n", sizeof(BaseNums), m, mind, eByte2value(0xff));
	// return testmain();
	const char* cmds[]= {
		"MV BD    123\r\n", "MV FL 100\r\n",
		NULL
	};

		char dir_P, dir_T;
		unsigned short dur;
		int c;
	for (int k =0; cmds[k]; k++)
	{
c =sscanf(cmds[k], "MV %c%c %d", &dir_P, &dir_T, &dur);
	}

	ch.fifo_tail =ch.fifo_header=0;
	uint8_t msg[] ={0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0}, msglen= 1; // sizeof(msg);
//	PulseFrame_send(msg, msglen, (SetPulsePin_f)1, 1, 0x10);
//	ch.fifo_buf[ch.fifo_header++]=0;
//	PulseFrame_send(msg+1, msglen, (SetPulsePin_f)1, 1, 0x10);
//	PulseFrame_send(msg+2, msglen, (SetPulsePin_f)1, 1, 0x10);
//	PulseFrame_send(msg+3, msglen, (SetPulsePin_f)1, 1, 0x10);
//	PulseFrame_send(msg, 8, (SetPulsePin_f)1, 1, 0x10);
//	ch.fifo_buf[ch.fifo_header++]=0;
	unsigned _thrdID;
	 HANDLE _hThread = (HANDLE)_beginthreadex(NULL, 0, _execute, (void *)0, CREATE_SUSPENDED, (unsigned*) &_thrdID);
	 ResumeThread(_hThread);

	while(1)
	{
		if (!PulseCapture_doParse(&ch))
;//			Sleep(1);
	}

	int d;
	char tmp[100];
	const char *pNGODErr=strstr(err, "doAction");
	if (pNGODErr)
	{
		if (sscanf(pNGODErr, "doAction(%[^)]) failed; error[%d", tmp, &d)>1 && d>770 && d<799)
				d =d;
	}

	Buffer buf;
	const char* aaa = "Kadfkjas$\r\n";
	size_t offset =0;
	size_t len=0;

	// for (int i=0; i<1000; i++)
		offset+=buf.fill((uint8*)aaa, offset, 16157); len= buf.length();
		offset+=buf.fill((uint8*)aaa, offset, 16384); len= buf.length();
/*
	size_t size = buf.size();
	size_t len = buf.length();

	uint8* pBuf = new uint8[len+1];
	memset(pBuf, 0, len+1);
	len = buf.read(pBuf, 0, len);
*/
	std::string str;
	buf.readToString(str);
	printf(str.c_str());

}


void htddl_OnReceived(void* nic, hwaddr_t src, pbuf* data)
{
	printf("htddl_OnReceived() len[%d]: ", data->tot_len);
	for (; data; data = data->next)
	{
		for(int i=0; i<data->len; i++)
			printf("%c", data->payload[i]);
	}
	printf("\r\n");
}

uint16_t htddl_doTX(void* nic, hwaddr_t to, uint8_t* data, uint16_t len)
{
	htddl_procRX(nic, data, len); 
	return len; 
}

uint8_t  htddl_getHwAddr(void* nic, uint8_t** pBindaddr)  { return 0; }


//////////////////////////////////////////

//////////////////////////////////////////
typedef     std::multiset<std::string> InProgressSet;

	static      InProgressSet _inProgressSet;

	bool isInProgress(const std::string& fileName, int64 startOffset, int size)
	{
		char buf[64];
		snprintf(buf, sizeof(buf)-2, "##%08x+", max(startOffset-1024*1024*20, 0)); // 20MB behind max
		std::string key = fileName + buf;
		snprintf(buf, sizeof(buf)-2, "##%08x+", startOffset+size);

		ZQ::common::MutexGuard g(_lkInProgressSet);
		InProgressSet::iterator itEnd = _inProgressSet.upper_bound(fileName + buf); // endKey
		InProgressSet::iterator it = _inProgressSet.lower_bound(key));

		for (; it < itEnd; it++)
		{
			key = (*it).substr(fileName.length());
			uint64 ipOffset;
			uint   ipSize;
			if (sscanf(key.str(), "##%08x+%x", &ipOffset, &ipSize) >=2)
			{
				if (startOffset < ipOffset)
					continue;

				if (startOffset > (ipOffset + ipSize))
					return false;
				
				size -= ipOffset + ipSize - startOffset;

				if (size <=0)
					return true;

				startOffset = ipOffset + ipSize; 
			}
		}

		return false;
	}

	static std::string _inProgessKey(const std::string& fileName, uint64 startOffset, uint size)
	{
		char buf[64];
		snprintf(buf, sizeof(buf)-2, "##%08x+%x", startOffset, size);
		return fileName + buf;
	}

int main()
{

		_inProgressKey = _inProgessKey(fileName, startOffset, size);
	ZQ::common::MutexGuard g(_lkInProgressSet);
	_inProgressSet.insert(_inProgressKey);

	char* msg = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		;

	pbuf* p =pbuf_mmap(msg, 522);
	hwaddr_t dest;
	uint16_t n = htddl_send(NULL, dest, p, HTDDL_TX_SIZE_MAX);

	return 0;
}


void nRF24L01_OnReceived(nRF24L01* chip, uint8_t pipeId, uint8_t* payload, uint8_t payloadLen) {}
void nRF24L01_OnSent(nRF24L01* chip, uint32_t destNodeId, uint8_t destPortNun, uint8_t* payload) {}
