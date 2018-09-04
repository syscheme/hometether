#include "ht_comm.h"
#ifdef _WIN32
#include "TimeUtil.h"
#endif

uint32_t now()
{
#ifdef _WIN32
	return (int32_t)(ZQ::common::now() & 0xffffffff);
#else
    struct timeval tmval;
    int ret = gettimeofday(&tmval,(struct timezone*)NULL);
    if(ret != 0)
        return 0;
    return (tmval.tv_sec*1000LL + tmval.tv_usec/1000) &0xffffffff;
#endif // _WIN32

}

std::string IntMapToJSON(const IntMap& valueMap)
{
	std::string ret = "{ ";
	char decbuf[100];
	for (IntMap::const_iterator it = valueMap.begin(); it != valueMap.end(); it++)
	{
		if (it->first.empty())
			continue;

		snprintf(decbuf, sizeof(decbuf)-2, "\"%s\": 0x%x, ", it->first.c_str(), it->second);
		ret += decbuf;
	}

	if (',' == ret[ret.length()-2])
		ret = ret.substr(0, ret.length()-2);

	ret += " }";
	return ret;
}

static uint8_t hexchval(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';

	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' +10;

	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' +10;

	return 0xff;
}

const char* hex2int(const char* hexstr, uint32_t* pVal)
{
	uint8_t v =0;
	if (NULL == hexstr || NULL == pVal)
		return NULL;

	for (*pVal =0; *hexstr; hexstr++)
	{
		v = hexchval(*hexstr);
		if (v >0x0f) break;
		*pVal <<=4; *pVal += v;
	}

	return hexstr;
}


