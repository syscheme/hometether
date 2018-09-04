#include "wifirm04.h"
#include "textmsg.h"

#define  WifiRM04_PacketTimeout (100) // 100 msec

#ifdef sleep
#  define WifiRM04_sleep(_msec) sleep(_msec)
#else
#  define WifiRM04_sleep(_msec) delayXmsec(_msec)
#endif // WifiRM04_sleep

hterr WifiRM04_enterAtMode(WifiRM04* chip)
{
	uint8_t cRetries = 5;
	chip->flags |= WifiRM04FLG_OUT_TRANS; // assume currently is in the mode of out-trans

	while (cRetries-- && (chip->flags & WifiRM04FLG_OUT_TRANS))
	{
		USART_transmit(&chip->tty, "+", 1);     WifiRM04_sleep(WifiRM04_PacketTimeout>>1);
		USART_transmit(&chip->tty, "+", 1);     WifiRM04_sleep(WifiRM04_PacketTimeout>>1);
		USART_transmit(&chip->tty, "+", 1);     WifiRM04_sleep(WifiRM04_PacketTimeout>>1);
		WifiRM04_sleep(500);
		USART_transmit(&chip->tty, "\0x1b", 1); WifiRM04_sleep(WifiRM04_PacketTimeout>>1);
		USART_transmit(&chip->tty, "\0x1b", 1); WifiRM04_sleep(WifiRM04_PacketTimeout>>1);
		USART_transmit(&chip->tty, "\0x1b", 1); WifiRM04_sleep(WifiRM04_PacketTimeout>>1);
		
		// TODO: send the at comand to read the version
		USART_transmit(&chip->tty, CONST_STR_AND_LEN(at+ver=?));
		WifiRM04_sleep(100);
	}

	if (0 == (chip->flags & WifiRM04FLG_OUT_TRANS))
	{
		// successfully entered the AT mode, refresh the lcoal ip as well
	   	USART_transmit(&chip->tty, CONST_STR_AND_LEN(at+net_ip=?\n));
		return ERR_SUCCESS;
	}

	// TODO test if the RM04 responses "at"
	return ERR_IO;
}

hterr WifiRM04_init(WifiRM04* chip)
{
	hterr ret;

	TextLineCollector_init(&chip->tlc, TextMsg_procLine, &chip);
	ret = WifiRM04_enterAtMode(chip);
	// if (ERR_SUCCESS != ret)
		return ret;
}

// portal of texMsg's void (*cbOnLineReceived_f)(void* netIf, char* line)
hterr WifiRM04_OnAtLine(WifiRM04* chip, char* line)
{
	char *p = NULL;
	uint8_t i;

	// case 0, in non-AT mode, this only listens the version response
	if (chip->flags & WifiRM04FLG_OUT_TRANS)
	{
		// the sample at+ver=? response: V1.78(Jul 23 2013)
		if ('V' == line[0] && '.' == line[2] && strchr(line, '(') && strchr(line, ')'))
		{
			chip->flags &= ~WifiRM04FLG_OUT_TRANS; // reset the flag
			return ERR_SUCCESS;
		}
		
		// all other cases will be ignored
		return ERR_OPERATION_ABORTED;
	}

	//case 1. sample response of at+net_ip=? >192.168.11.254,255.255.255.0,192.168.11.1
	p = strstr(line, ",255.255.");
	if (p>line)
	{
		for (i =0, p=line; p >line && i<4; i++)
		{
			*(((uint8_t*)&chip->localIp) +i) = atoi(p);
			p = strchr(p, '.');
		}

		return ERR_SUCCESS;
	}

	// all unknown messages in AT mode would be treated finished
	return ERR_SUCCESS;
}

void WifiRM04_OnLineReceived(void* netIf, char* line)
{
	WifiRM04* chip = (WifiRM04*) netIf;
	if (ERR_SUCCESS == WifiRM04_OnAtLine(chip, line))
		return;

	WifiRM04_OnDataLineReceived(chip, line);
}

static const char* basecmds =
"at+CLport=8001\n"
"at+uartpacktimeout=100\n" "at+uartpacklen=1200\n"
"at+net_commit=1\n"	"at+reconn=1\n";

#define WifiRM04_udpConnect(chip, remoteIp, remotePort) WifiRM04_openUSARTMode(chip, FLAG(0)|FLAG(1), remoteIp, remotePort)
#define WifiRM04_udpListen(chip, serverPort)            WifiRM04_openUSARTMode(chip, FLAG(0), 0, serverPort)
#define WifiRM04_tcpConnect(chip, remoteIp, remotePort) WifiRM04_openUSARTMode(chip, FLAG(1), remoteIp, remotePort)
#define WifiRM04_tcpListen(chip, serverPort)            WifiRM04_openUSARTMode(chip, 0, 0, serverPort)

hterr WifiRM04_openUSARTMode(WifiRM04* chip, uint8_t mode, uint32_t remoteIp, uint16_t remotePort)
{
	uint8_t tmp = 0;

	char buf[20], *p;

	if (chip->flags & WifiRM04FLG_OUT_TRANS)
	{
		tmp = WifiRM04_enterAtMode(chip);
		if (ERR_SUCCESS != tmp)
			return tmp;
	}

	p = (mode & FLAG(0)) ? "udp\n" : "tcp\n";
   	USART_transmit(&chip->tty, CONST_STR_AND_LEN(at+remotepro=));
   	USART_transmit(&chip->tty, (const uint8_t*)p, strlen(p));

	p = (mode & FLAG(1)) ? "client\n" : "server\n";
   	USART_transmit(&chip->tty, CONST_STR_AND_LEN(\nat+mode=));
   	USART_transmit(&chip->tty, (const uint8_t*)p, strlen(p));
		
	if (remoteIp >0)
	{
		snprintf(buf, sizeof(buf)-1, "%d.%d.%d.%d\n", 
			*(((uint8_t*)&remoteIp) +0), *(((uint8_t*)&remoteIp) +1),
			*(((uint8_t*)&remoteIp) +0), *(((uint8_t*)&remoteIp) +3));
   		USART_transmit(&chip->tty, CONST_STR_AND_LEN(at+remoteip=));
		USART_transmit(&chip->tty, (const uint8_t*)buf, strlen(buf));
	}

	if (remotePort >0)
	{
		snprintf(buf, sizeof(buf)-1, "%d\n", remotePort);
   		USART_transmit(&chip->tty, CONST_STR_AND_LEN(at+remoteport=));
		USART_transmit(&chip->tty, (const uint8_t*)buf, strlen(buf));
	}

	USART_transmit(&chip->tty, (const uint8_t*)basecmds, strlen(basecmds));
	chip->flags |= WifiRM04FLG_OUT_TRANS;

	return ERR_SUCCESS;
}

hterr WifiRM04_doReceive(WifiRM04* chip)
{
	uint8_t tmp = 0;
	uint8_t buf[20];

	tmp = USART_receive(&chip->tty, (uint8_t*)buf, sizeof(buf));
	if (tmp >0)
		TextLineCollector_push(&chip->tlc, (char*)buf, tmp);

	return tmp;
}

hterr WifiRM04_sendLine(WifiRM04* chip, const char* line)
{
	USART_transmit(&chip->tty, (const uint8_t*)line, strlen(line));
	USART_transmit(&chip->tty, "\n", 1);
	WifiRM04_sleep(1);
	return ERR_SUCCESS;
}

