extern "C"
{
#include "esp8266.h"

#define atmsg atmsg1
extern const char* atmsg;

void ESP8266_OnReceived(ESP8266* chip, char* msg, int len, bool bEnd)
{
}

int readAT(char* buf, int maxLen)
{
	static const char* p =atmsg;
	int i=0;
	for (i=0; *p && i< maxLen-1; i++)
		buf[i] = *p++;
	return i;
}

}

ESP8266 chip;
void OnReceived(ESP8266* chip, uint8_t conn, char* msg, int len, bool bEnd)
{
}

int main()
{
	ESP8266_init(&chip, NULL);

	while (1)
		ESP8266_loopStep(&chip);
	return 0;
}

const char* atmsg0= "\n"
"OK\n"
"AT+RST\n"
"\n"
"OK\n"
"\n"
" ets Jan  8 2013,rst cause:4, boot mode:(3,7)\n"
"\n"
"wdt reset\n"
"load 0x40100000, len 24444, room 16 \n"
"tail 12\n"
"chksum 0xe0\n"
"ho 0 tail 12 room 4\n"
"load 0x3ffe8000, len 3168, room 12 \n"
"tail 4\n"
"chksum 0x93\n"
"load 0x3ffe8c60, len 4956, room 4 \n"
"tail 8\n"
"chksum 0xbd\n"
"csum 0xbd\n"
"\n"
"ready\n"
"AT\n"
"\n"
"\n"
"OK\n"
"AT+CWLIP\n"
"\n"
"\n"
"ERROR\n"
;

const char* atmsg1= "\n"
"ready\n"
"OK\n"
"AT+CWLAP\n"
"+CWLAP:(0,\"\",0)\n"
"+CWLAP:(4,\"syscheme@hotmail\",-45)\n"
"+CWLAP:(4,\"TP-LINK_CA98\",-79)\n"
"+CWLAP:(4,\"TP-LINK_501\",-74)\n"
"OK\n"
"AT+CWJIF\n"
"\n"
"\n"
"ERROR\n"
"AT+CWJIF?\n"
"\n"
"\n"
"ERROR\n"
"AT+CWLIF?\n"
"\n"
"no this fun\n"
"AT+CWJIF=?\n"
"OK\n"
"AT+CWJAP?\n"
"\n"
"+CWJAP:\"syscheme@hotmail\"\n"
"\n"
"OK\n"
"AT+CWJIF\n"
"\n"
"\n"
"ERROR\n"
"AT+CIFST\n"
"\n"
"\n"
"ERROR\n"
"AT+CIFSR\n"
"\n"
"192.168.199.205\n"
"AT+CIFSR=?\n"
"\n"
"\n"
"no this fun\n"
"\n"
"AT+CIPSEND=1,20sdfasfas\n"
"\n"
"> AT+CIPSEND=1,20sdfasfas\n"
"busy\n"
"\n"
"\n"
"SEND OK\n"
"AT+CIPSEND=1,8sdfasfas\n"
"\n"
"> \n"
"+IPD,1,40:http://www.cmsoft.cn QQ:10865600\n"
"OK\n"
"AT+CIPSEND=1,20sdfasfas\n"
"busy\n"
"\n"
"SEND OK\n"
"\n"
"AT+CIPSTATUS\n"
"STATUS:<stat>\n"
"+CIPSTATUS:0,udp,192.168.11.120,1234,11\n"
"+CIPSTATUS:1,tcp,192.168.11.123,1230,11\n"
"+IPD,1,40:A123456789012345678901234567890123456789\n"
"+IPD,1,20:B123456789012345678901234567890\n"
"+IPD,1,20:C12345678901234\n"
"+IPD,1,20:D12345678901234567890123456\n"
"+IPD,1,20:E12345678901234567890123456\n"
"\n"
;
