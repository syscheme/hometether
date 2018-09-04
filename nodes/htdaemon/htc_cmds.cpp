#include "htc.h"
#include "htdomain.h"
#include "../htcan.h"

#include <vector>

#define MAX_RETRY (3)
extern HTClient htc;
char req[100], respfmt[160];
typedef std::vector <std::string> StrList;

void declareVar(IntMap& valueMap, const char* varname, int val = INVALID_INTVAL)
{
	if (NULL == varname || !*varname)
		return;

	valueMap.insert(IntMap::value_type(varname, val));
}

#define DECLARE_PUT_INT(_VALMAP, _VAR) if (INVALID_INTVAL != _VAR) declareVar(_VALMAP, #_VAR, _VAR)
#define DECLARE_GET_INT(_VALMAP, _VAR) declareVar(_VALMAP, #_VAR)
void unencode(char *src, char *last, char *dest)
{
	for(; src != last; src++, dest++)
	{
		if (*src == '+')	*dest = ' ';
		else if(*src == '%')
		{
			int code;
			if(sscanf(src+1, "%2x", &code) != 1) code = '?';
			*dest = code;
			src +=2;
		}     
		else *dest = *src;

		*dest = '\n';
		*++dest = '\0';
	}
}

void outputIntMap(const IntMap& valueMap)
{
	std::string body;
	char buf[200];

	if (bJSON)
		body = IntMapToJSON(valueMap);
	else
	{

		for (IntMap::const_iterator it = valueMap.begin(); it!= valueMap.end(); it++)
		{
			snprintf(buf, sizeof(buf)-2, "%s=%02x, ", it->first.c_str(), it->second);
			body += buf;
		}

		if (',' == body[body.length()-2])
			body = body.substr(0, body.length()-2);
		body += "\n";	
	}

	if (bCGI)
		printf("Content-Type: %s\r\nContent-Length: %d\r\n\r\n", bJSON ? "application/json;charset=UTF-8" : "text/html;charset=iso-8859-1", body.length());

	printf(body.c_str());
}

int readIntValues(IntMap& valueMap, uint16_t nodeId, const char* uri)
{
	IntMap::iterator it = valueMap.begin();

	for (int retryNo=0; retryNo < MAX_RETRY; retryNo++)
	{
		for (it = valueMap.begin(); it != valueMap.end(); it++)
		{
			if (INVALID_INTVAL == it->second)
				break;
		}

		if (valueMap.end() ==it)
			break; // already read all the channels, quit the loop

		snprintf(req, sizeof(req)-2, "GET ht:%x/%s?%s=\r\n", nodeId, uri, it->first.c_str());
		snprintf(respfmt, sizeof(respfmt)-2, "POST ht:%x/%s?%%[^=]=%s&%%[^=]=%s&%%[^=]=%s&%%[^=]=%s", nodeId, uri,
			FMT_PATTERN_HEXVAL, FMT_PATTERN_HEXVAL, FMT_PATTERN_HEXVAL, FMT_PATTERN_HEXVAL);

		int ret = htc.exec(req, respfmt, 2, htd_timeout);
		for (int parseNo=0; parseNo < ret/2; parseNo++)
		{
			const char* valstr = htc.readValueAt(parseNo*2);
			std::string key = valstr?valstr:"";
			uint32_t val=0;
			hex2int(htc.readValueAt(parseNo*2 +1), &val);

			if (valueMap.end() != valueMap.find(key))
			{
				if (INVALID_INTVAL == valueMap[key])
					retryNo =0; // reset the retry counting if there is a expected value never seen earilier

				valueMap[key] = val;
			}
		}
	}

	StrList val2clean;
	for (it = valueMap.begin(); it != valueMap.end(); it++)
	{
		if (INVALID_INTVAL == it->second)
			val2clean.push_back(it->first);
	}

	for (StrList::iterator itC = val2clean.begin(); itC < val2clean.end(); itC++)
		valueMap.erase(*itC);

	return 0;
}

int setIntValues(IntMap& valueMap, uint16_t nodeId, const char* uri)
{
	IntMap::iterator it = valueMap.begin();
	char* p = req;
	p += snprintf(p, req + sizeof(req)-2 -p, "PUT ht:%x/%s?", nodeId, uri);

	for (it = valueMap.begin(); it != valueMap.end(); it++)
	{
		p += snprintf(p, req + sizeof(req)-2 -p, "%s=%x&", it->first.c_str(), it->second);
		it->second = INVALID_INTVAL;
	}

	*--p = '\r'; *++p = '\n'; *++p = '\0'; // end of the line

	snprintf(respfmt, sizeof(respfmt)-2, "POST ht:%x/%s?%%[^=]=%s&%%[^=]=%s&%%[^=]=%s&%%[^=]=%s", nodeId, uri,
			FMT_PATTERN_HEXVAL, FMT_PATTERN_HEXVAL, FMT_PATTERN_HEXVAL, FMT_PATTERN_HEXVAL);

	int ret = htc.exec(req, respfmt, 2, htd_timeout);
	for (int parseNo=0; parseNo < ret/2; parseNo++)
	{
		const char* valstr = htc.readValueAt(parseNo*2);
		std::string key = valstr?valstr:"";
		uint32_t val=0;
		hex2int(htc.readValueAt(parseNo*2 +1), &val);

		if (valueMap.end() != valueMap.find(key))
		{
			valueMap[key] = val;
		}
	}

	StrList val2clean;
	for (it = valueMap.begin(); it != valueMap.end(); it++)
	{
		if (INVALID_INTVAL == it->second)
			val2clean.push_back(it->first);
	}

	for (StrList::iterator itC = val2clean.begin(); itC < val2clean.end(); itC++)
		valueMap.erase(*itC);

	return 0;
}

// ============================================
// sub commands
// ============================================
int SubCmd__hello(int argc, char *argv[])
{
	// parse the command options
	if (argc <2)
	{
		usage(argv[0]);
		return -1;
	}

	int ch;
	while((ch = getopt(argc, argv, "hr:l:t:d:nm:v")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	printf("hello %s\n", argv[1]);

	return 0;
}

// -----------------------------
// sub-cmd readADC
// -----------------------------
#undef MAX_CH
#define MAX_CH (9)
const char* usage_getADC = 
"getADC [-n <nodeId>] [-c <channel>]\r\n"
"  options\r\n"
"     -n <nodeId>       to specify the nodeId to read\r\n"
"     -c <channelId>    to specify a ADC channel to read\r\n";

int SubCmd__getADC(int argc, char *argv[])
{
	// parse the command options
	int ch;
	int chId =-1, nodeId =0x10;
	while((ch = getopt(argc, argv, "n:c:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			nodeId = atoi(optarg);
			break;
		case 'c':
			chId = atoi(optarg);
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	IntMap valueMap;
	IntMap::iterator it;

	for (ch=0; ch < MAX_CH; ch++)
	{
		if (chId<0 || chId == ch)
		{
			char key[16];
			snprintf(key, sizeof(key)-2, "a%d", ch);
			declareVar(valueMap, key);
		}
	}

	readIntValues(valueMap, nodeId, "adc");

	if (chId <0)
	{
		IntMap confMap;
		DECLARE_GET_INT(confMap, lumineDiffMask);
		DECLARE_GET_INT(confMap, lumineDiffIntv);
		DECLARE_GET_INT(confMap, lumineDiffTshd);

		readIntValues(confMap, nodeId, "ecuConfig");

		for (it= confMap.begin(); it!= confMap.end(); it++)
			valueMap.insert(*it);
	}

	outputIntMap(valueMap);
	return 0;
}

// -----------------------------
// sub-cmd readTemp
// -----------------------------
#undef  MAX_CH
#define MAX_CH (8)
const char* usage_getTemp = 
"getTemp [-n <nodeId>] [-c <channel>]\r\n"
"  options\r\n"
"     -n <nodeId>       to specify the nodeId to read\r\n"
"     -c <channelId>    to specify a temp channel to read\r\n";

int SubCmd__getTemp(int argc, char *argv[])
{
	// parse the command options
	int ch;
	int chId =-1, nodeId =0x10;
	while((ch = getopt(argc, argv, "n:c:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			nodeId = atoi(optarg);
			break;
		case 'c':
			chId = atoi(optarg);
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	IntMap valueMap;
	IntMap::iterator it;

	for (ch=0; ch < MAX_CH; ch++)
	{
		if (chId<0 || chId == ch)
		{
			char key[16];
			snprintf(key, sizeof(key)-2, "t%d", ch);
			declareVar(valueMap, key);
		}
	}

	// test: outputIntMap(valueMap);

	readIntValues(valueMap, nodeId, "temp");

	if (chId <0)
	{
		IntMap confMap;
		DECLARE_GET_INT(confMap, tempMask);
		DECLARE_GET_INT(confMap, tempIntv);
		readIntValues(confMap, nodeId, "ecuConfig");

		for (it= confMap.begin(); it!= confMap.end(); it++)
			valueMap.insert(*it);
	}

	outputIntMap(valueMap);
	return 0;
}

// -----------------------------
// sub-cmd getMotion
// -----------------------------
int SubCmd__getMotion(int argc, char *argv[])
{
	// parse the command options
	int ch;
	int nodeId =0x10;
	while((ch = getopt(argc, argv, "n:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			nodeId = atoi(optarg);
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	IntMap valueMap;
	IntMap::iterator it;
	declareVar(valueMap, "mstate");
	readIntValues(valueMap, nodeId, "local");

	{
		IntMap confMap;
		DECLARE_GET_INT(confMap, motionDiffMask);
		DECLARE_GET_INT(confMap, motionOuterMask);
		DECLARE_GET_INT(confMap, motionBedMask);
		DECLARE_GET_INT(confMap, motionDiffIntv);

		readIntValues(confMap, nodeId, "ecuConfig");

		for (it= confMap.begin(); it!= confMap.end(); it++)
			valueMap.insert(*it);
	}

	outputIntMap(valueMap);
	return 0;
}

// -----------------------------
// sub-cmd getRBuf
// -----------------------------
int SubCmd__getRBuf(int argc, char *argv[])
{
	// parse the command options
	int ch;
	int nodeId =0x10;
	while((ch = getopt(argc, argv, "n:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			nodeId = atoi(optarg);
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	IntMap valueMap;
	IntMap::iterator it;

	for (int i=0; i < 0x20; i++)
	{
		char key[10];
		snprintf(key, sizeof(key)-2, "b%02x", i);
		declareVar(valueMap, key);
	}

	readIntValues(valueMap, nodeId, "rBuf");

	outputIntMap(valueMap);
	return 0;
}

// -----------------------------
// sub-cmd getState
// -----------------------------
int SubCmd__getState(int argc, char *argv[])
{
	// parse the command options
	int ch;
	int nodeId =0x10;
	while((ch = getopt(argc, argv, "n:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;

		case 'n':
			nodeId = atoi(optarg);
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	IntMap valueMap, tmpMap;
	DECLARE_GET_INT(tmpMap, gstate);
	DECLARE_GET_INT(tmpMap, masterId);
	readIntValues(tmpMap, nodeId, "global");

	DECLARE_GET_INT(valueMap, lstate);
	DECLARE_GET_INT(valueMap, mstate);
	readIntValues(valueMap, nodeId, "local");
	for(IntMap::iterator it =tmpMap.begin(); it!=tmpMap.end(); it++)
		valueMap.insert(*it);

	outputIntMap(valueMap);
	return 0;
}

// -----------------------------
// sub-cmd setGState
// -----------------------------
int SubCmd__setGState(int argc, char *argv[])
{
	// parse the command options
	char ch;
	int nodeId   =0x10;
	int gstate   = INVALID_INTVAL;
	int masterId = INVALID_INTVAL;

	while((ch = getopt(argc, argv, "hn:k:m:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;

		case 'n':
			nodeId = atoi(optarg);
			break;

		case 'k':
			gstate = atoi(optarg);
			break;

		case 'm':
			masterId = atoi(optarg);
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

/*
	if (gstate<0 || masterId<0)
	{
		fprintf(stderr, "gstate or masterId not specified: gstate[0x%02x] masterId[0x%02x]\n", gstate&0xff, masterId&0xff);
		return -1;
	}
*/
	IntMap valueMap;
	DECLARE_PUT_INT(valueMap, gstate);
	DECLARE_PUT_INT(valueMap, masterId);

	if (valueMap.size() <=0)
	{
		fprintf(stderr, "%s: not enough input\n", argv[0]);
		return -1;
	}

	setIntValues(valueMap, nodeId, "global");

	outputIntMap(valueMap);
	return 0;
}

// -----------------------------
// sub-cmd getConfig
// -----------------------------
int SubCmd__getConfig(int argc, char *argv[])
{
	// parse the command options
	int ch;
	int nodeId =0x10;
	while((ch = getopt(argc, argv, "n:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;

		case 'n':
			nodeId = atoi(optarg);
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	IntMap valueMap, tmpMap;
	DECLARE_GET_INT(valueMap, tempMask);
	DECLARE_GET_INT(valueMap, tempIntv);
	DECLARE_GET_INT(valueMap, motionDiffMask);
	DECLARE_GET_INT(valueMap, motionOuterMask);
	DECLARE_GET_INT(valueMap, motionBedMask);
	DECLARE_GET_INT(valueMap, motionDiffIntv);
	DECLARE_GET_INT(valueMap, lumineDiffMask);
	DECLARE_GET_INT(valueMap, lumineDiffIntv);
	DECLARE_GET_INT(valueMap, lumineDiffTshd);
	DECLARE_GET_INT(valueMap, fwdAdm);
	DECLARE_GET_INT(valueMap, fwdExt);

	readIntValues(valueMap, nodeId, "ecuConfig");

	outputIntMap(valueMap);
	return 0;
}

// -----------------------------
// sub-cmd setGState
// -----------------------------
int SubCmd__setConfig(int argc, char *argv[])
{
	// parse the command options
	char ch;
	int nodeId   =0x10;
	int tempMask=INVALID_INTVAL, tempIntv=INVALID_INTVAL,
		motionDiffMask=INVALID_INTVAL, motionOuterMask=INVALID_INTVAL, motionBedMask=INVALID_INTVAL, motionDiffIntv=INVALID_INTVAL,
		lumineDiffMask=INVALID_INTVAL, lumineDiffIntv=INVALID_INTVAL, lumineDiffTshd=INVALID_INTVAL,
		fwdAdm=INVALID_INTVAL, fwdExt=INVALID_INTVAL;

	while((ch = getopt(argc, argv, "hn:m:v:d:o:b:f:k:l:r:w:x:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;

		case 'n':
			nodeId = atoi(optarg);
			break;

		case 'm': tempMask =atoi(optarg); break;
		case 'v': tempIntv =atoi(optarg); break;
		case 'd': motionDiffMask =atoi(optarg); break;
		case 'o': motionOuterMask =atoi(optarg); break;
		case 'b': motionBedMask =atoi(optarg); break;
		case 'f': motionDiffIntv =atoi(optarg); break;
		case 'k': lumineDiffMask =atoi(optarg); break;
		case 'l': lumineDiffIntv =atoi(optarg); break;
		case 'r': lumineDiffTshd =atoi(optarg); break;
		case 'w': fwdAdm =atoi(optarg); break;
		case 'x': fwdExt =atoi(optarg); break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	IntMap valueMap;
	DECLARE_PUT_INT(valueMap, tempMask);
	DECLARE_PUT_INT(valueMap, tempIntv);
	DECLARE_PUT_INT(valueMap, motionDiffMask);
	DECLARE_PUT_INT(valueMap, motionOuterMask);
	DECLARE_PUT_INT(valueMap, motionBedMask);
	DECLARE_PUT_INT(valueMap, motionDiffIntv);
	DECLARE_PUT_INT(valueMap, lumineDiffMask);
	DECLARE_PUT_INT(valueMap, lumineDiffIntv);
	DECLARE_PUT_INT(valueMap, lumineDiffTshd);
	DECLARE_PUT_INT(valueMap, fwdAdm);
	DECLARE_PUT_INT(valueMap, fwdExt);

	if (valueMap.size() <=0)
	{
		fprintf(stderr, "%s: not enough input\n", argv[0]);
		return -1;
	}

	setIntValues(valueMap, nodeId, "ecuConfig");

	outputIntMap(valueMap);
	return 0;
}

int SubCmd__reset(int argc, char *argv[])
{
	// parse the command options
	int ch;
	int nodeId =-1;
	while((ch = getopt(argc, argv, "n:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;

		case 'n':
			nodeId = atoi(optarg);
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	if (nodeId<=0 ||nodeId >0x7f)
	{
		fprintf(stderr, "must specify a nodeId to reset\n");
		return -2;
	}

	char cmd[40]; // can:10R02V112233

	snprintf(cmd, sizeof(cmd)-2, "POST can:%XN02V%02X%02X\r\n", fcNMT<<7, nmt_resetNode, nodeId);
	htc.exec(cmd, NULL, 0, 0);
	return 0;
}

int SubCmd__htcs(int argc, char *argv[])
{
	// parse the command options
	int ch;
	char msg[256]="";
	while((ch = getopt(argc, argv, "m:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;

		case 'm':
			strncpy(msg, optarg, sizeof(msg)-8);
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	if (strlen(msg)<=0)
	{
		fprintf(stderr, "must specify URI to post\n");
		return -2;
	}

	strcat(msg, "\r\n\0"); 
	htc.exec(msg, NULL, 0, 0);
	return 0;
}

int SubCmd__chstate(int argc, char *argv[])
{
	// parse the command options
	int ch;
	uint8_t state = soc_Easy;
	while((ch = getopt(argc, argv, "a:")) != EOF)
	{
		switch (ch)
		{
		case '?':
		case 'h':
			usage(argv[0]);
			return 0;

		case 'a':
			state = atoi(optarg);
			break;

		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}

	char msg[100];
	snprintf(msg, sizeof(msg)-2, "PUT ht:00/global?gstate=%d\r\n\0", state);
	htc.exec(msg, NULL, 0, 0);
	return 0;
}

// -----------------------------
// usage screens
// -----------------------------
const char* usages[] = {
	usage_getADC, usage_getTemp,
	NULL
};

void usage(const char* cmd)
{
	char name[60];
	strncpy(name, cmd, sizeof(name)-2);
	int slen = strlen(name);
	char* p = name + slen; *p++=' '; *p='\0', slen++;

	for (int i =0; usages[i]; i++)
	{
		if (0 == strncmp(usages[i], name, slen))
		{
			printf("%s\r\n", usages[i]);
			return;
		}
	}
}

