#include "hlkrm04.h"

#if 0
1) quit from out-trans mode, enter AT mode
USART_sendChar('+'); delayXmsec(100);
USART_sendChar('+'); delayXmsec(100);
USART_sendChar('+'); delayXmsec(100);
delayXmsec(500);
USART_sendChar('\0x1b'); delayXmsec(100);
USART_sendChar('\0x1b'); delayXmsec(100);
USART_sendChar('\0x1b'); delayXmsec(100);

2) read the local ip
at+net_ip=?
sample>>>192.168.11.254,255.255.255.0,192.168.11.1

3) start udp client
at+netmode=0
at+remotepro=udp
at+mode=client
at+remoteip=192.168.11.100 // www.google.com
at+remoteport=8000
at+CLport=8001
at+uartpacktimeout=100
at+uartpacklen=1200
at+net_commit=1

at+reconn=1 //start the client, and enter the out-trans mode

4) send a packet within 100msec

#endif

// ---------------------------------------------------------------------------
// native methods
// ---------------------------------------------------------------------------
#define CONST_STR(_STR)    _STR, sizeof(_STR)-1
char cmd[64];
static void HLKRM04_setAddr(HLKRM04* chip, uint32_t remoteIp, uint16_t serverPort)
{
	uint8_t* bs = (uint8_t*) &chip->bindIp;
	// set the bindaddr
	if (chip->bindIp != 0)
	{
		snprintf(cmd, sizeof(cmd)-1, "at+net_ip=%d.%d.%d.%d,255.255.255.0,%d.%d.%d.1\r\n", bs[0],bs[1],bs[2],bs[3], bs[0],bs[1],bs[2]);
		USART_transmit(chip->usart, (const uint8_t*)cmd, strlen(cmd));
	}

	// set bindPort
	snprintf(cmd, sizeof(cmd)-1, "at+CLport=%d\r\nat+remoteport=%d\r\n", chip->bindPort, chip->bindPort);
	USART_transmit(chip->usart, (const uint8_t*)cmd, strlen(cmd));

	if (0 == remoteIp) // skip if remoteIp is not specified
		return;

	// set the remoteAddr/port for client mode
	bs = (uint8_t*) &remoteIp;
	snprintf(cmd, sizeof(cmd)-1, "at+remoteip=%d.%d.%d.%d\r\n", bs[0],bs[1],bs[2],bs[3]);
	USART_transmit(chip->usart, (const uint8_t*)cmd, strlen(cmd));

	snprintf(cmd, sizeof(cmd)-1, "at+remoteport=%d\r\n", serverPort);
	USART_transmit(chip->usart, (const uint8_t*)cmd, strlen(cmd));
}

static void HLKRM04_enterAtMode(HLKRM04* chip)
{
	if (chip->flags & FLG_MODE_AT)
		return;

	USART_sendByte(chip->usart, '+'); delayXmsec(100);
	USART_sendByte(chip->usart, '+'); delayXmsec(100);
	USART_sendByte(chip->usart, '+'); delayXmsec(100);
	delayXmsec(500);
	USART_sendByte(chip->usart, '\27'); delayXmsec(100); // 0x1b
	USART_sendByte(chip->usart, '\27'); delayXmsec(100);
	USART_sendByte(chip->usart, '\27'); delayXmsec(100);

	chip->flags |= FLG_MODE_AT;
}

static void HLKRM04_enterOutTransMode(HLKRM04* chip)
{
	if (0 == (chip->flags & FLG_MODE_AT))
		return;

	USART_transmit(chip->usart, CONST_STR("at+reconn=1\r\n")); // the out-trans mode
	chip->flags &= ~FLG_MODE_AT;
}

void HLKRM04_setModeRX(HLKRM04* chip)
{
	USART_transmit(chip->usart, CONST_STR("at+remotepro=udp\r\nat+mode=server\r\n")); // the netmode
	HLKRM04_setAddr(chip, 0, 0);
}

hterr HLKRM04_init(HLKRM04* chip, USART* usart, uint32_t bindIp, uint16_t bindPort)
{
	chip->usart       = usart;
	chip->bindIp      = bindIp;
	chip->bindPort    = bindPort;

	HLKRM04_setModeRX(chip);

	return ERR_SUCCESS;
}

void HLKRM04_udpConnect(HLKRM04* chip, uint32_t destNodeId, uint16_t destPortNum)
{
	HLKRM04_enterAtMode(chip);
	USART_transmit(chip->usart, CONST_STR("at+remotepro=udp\r\nat+mode=client\r\n")); // the netmode
	HLKRM04_setAddr(chip, destNodeId, destPortNum);
	HLKRM04_enterOutTransMode(chip);
}

void HLKRM04_tcpConnect(HLKRM04* chip, uint32_t destNodeId, uint16_t destPortNum)
{
	HLKRM04_enterAtMode(chip);
	USART_transmit(chip->usart, CONST_STR("at+remotepro=tcp\r\nat+mode=client\r\n")); // the netmode
	HLKRM04_setAddr(chip, destNodeId, destPortNum);
	HLKRM04_enterOutTransMode(chip);
}

void HLKRM04_tcpListen(HLKRM04* chip, uint16_t servePortNum)
{
	HLKRM04_enterAtMode(chip);
	USART_transmit(chip->usart, CONST_STR("at+remotepro=tcp\r\nat+mode=server\r\n")); // the netmode
	chip->bindPort = servePortNum;
	HLKRM04_setAddr(chip, 0, servePortNum);
	HLKRM04_enterOutTransMode(chip);
}

void HLKRM04_status(HLKRM04* chip);

void HLKRM04_TxPacket(HLKRM04* chip, uint8_t* data, uint8_t len)
{
	USART_transmit(chip->usart, CONST_STR("at+usartpacktimeout=10\r\n"));
	snprintf(cmd, sizeof(cmd)-1, "at+usartpacklen=%d\r\n", len);
	USART_transmit(chip->usart, (const uint8_t*) cmd, strlen(cmd));
	USART_transmit(chip->usart, CONST_STR("at+net_commit=1\r\nat+reconn=1\r\n"));

	USART_transmit(chip->usart, data, len);
}

//@return the pipeId if receive successfully, otherwise 0xff as fail
uint8_t HLKRM04_RxPacket(HLKRM04* chip, uint8_t* bufRX);

/*
// ---------------------------------------------------------------------------
// USART methods
// ---------------------------------------------------------------------------
hterr USART_open(USART* USARTx, const char* fnCOM, uint16_t baudX100)
{
	DCB dcbCOM = {0}; 
	COMMTIMEOUTS tos = {0};

	if (FIFO_init(&USARTx->rxFIFO, USART_MAX_PLOAD*1.5, sizeof(uint8_t), 0) <=0)
		return ERR_INVALID_PARAMETER;

	// step 1. open the COM port, fnCOM=COM1, COM2, ... 
	USARTx->port = CreateFile(fnCOM, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (INVALID_HANDLE_VALUE == USARTx->port)
	{
		if (ERROR_FILE_NOT_FOUND == GetLastError())
			return ERR_NOT_FOUND;
		return ERR_IO;
	}

	// step 2. set the parameters
	//  2.1 8N1 + baudrate
	dcbCOM.DCBlength = sizeof(dcbCOM);
	if (!GetCommState(USARTx->port, &dcbCOM))
		return ERR_IO;

	dcbCOM.BaudRate = baudX100 *100; // CBR_19200;
	dcbCOM.ByteSize = 8;
	dcbCOM.StopBits = ONESTOPBIT;
	dcbCOM.Parity   = NOPARITY;

	if (!SetCommState(USARTx->port, &dcbCOM))
		return ERR_INVALID_PARAMETER;

	//  2.2 timeout
	tos.ReadIntervalTimeout         = 50;
	tos.ReadTotalTimeoutConstant    = 50;
	tos.ReadTotalTimeoutMultiplier  = 10;
	tos.WriteTotalTimeoutConstant   = 50;
	tos.WriteTotalTimeoutMultiplier = 10;
	if (!SetCommTimeouts(USARTx->port, &tos))
		return ERR_INVALID_PARAMETER;

	return ERR_SUCCESS;


#ifdef LINUX
	if ((ecufd = open(DEVICE_ECU, O_RDWR | O_NOCTTY |O_NONBLOCK | O_NDELAY)) <0)
		return -1;

	// step 1.2 apply the serial port settings
	memset(&newtio, 0, sizeof(newtio)); // clear struct for new port settings

	// BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
	// CRTSCTS : output hardware flow control (only used if the cable has
	//                all necessary lines. See sect. 7 of Serial-HOWTO)
	// CS8     : 8n1 (8bit,no parity,1 stopbit)
	// CLOCAL  : local connection, no modem contol
	// CREAD   : enable receiving characters
	newtio.c_cflag = BAUDRATE | CS8 | CSTOPB | CLOCAL | CREAD | HUPCL;

	// IGNPAR  : ignore bytes with parity errors
	// ICRNL   : map CR to NL (otherwise a CR input on the other computer
	//           will not terminate input) otherwise make device raw (no other input processing)
	newtio.c_iflag = IGNPAR; //| ICRNL;

	// Raw output.
	newtio.c_oflag = 0;

	// ICANON  : enable canonical input disable all echo functionality, and don't send signals to calling program
	// newtio.c_lflag = ICANON;

	// initialize all control characters 
	// default values can be found in /usr/include/termios.h, and are given
	// in the comments, but we don't need them here
	newtio.c_cc[VINTR]    = 0;     // Ctrl-c 
	newtio.c_cc[VQUIT]    = 0;     // Ctrl-\
	newtio.c_cc[VERASE]   = 0;     // del
	newtio.c_cc[VKILL]    = 0;     // @
	// newtio.c_cc[VEOF]     = 4;     // Ctrl-d
	newtio.c_cc[VTIME]    = 0;     // inter-character timer unused
	newtio.c_cc[VMIN]     = 1;     // blocking read until 1 character arrives
	newtio.c_cc[VSWTC]    = 0;     // '\0'
	newtio.c_cc[VSTART]   = 0;     // Ctrl-q 
	newtio.c_cc[VSTOP]    = 0;     // Ctrl-s
	newtio.c_cc[VSUSP]    = 0;     // Ctrl-z
	newtio.c_cc[VEOL]     = 0;     // '\0'
	newtio.c_cc[VREPRINT] = 0;     // Ctrl-r
	newtio.c_cc[VDISCARD] = 0;     // Ctrl-u
	newtio.c_cc[VWERASE]  = 0;     // Ctrl-w
	newtio.c_cc[VLNEXT]   = 0;     // Ctrl-v
	newtio.c_cc[VEOL2]    = 0;     // '\0'

	tcgetattr(ecufd, &oldtio); // save current serial port settings

	// now apply the settings for the port
	tcflush(ecufd, TCIFLUSH);
	tcsetattr(ecufd, TCSANOW, &newtio);
#endif // LINUX
}

void  USART_close(USART* USARTx)
{
	CloseHandle(USARTx->port);
	USARTx->port = INVALID_HANDLE_VALUE;

#ifdef LINUX
	// restore the old port settings
	tcsetattr(ecufd, TCSANOW, &oldtio);
	close(ecufd);
#endif // LINUX
}

hterr USART_transmit(USART* USARTx, uint8_t* data, uint16_t len)
{
	DWORD dwBytes = 0;
	while (len >0)
	{
		if (!WriteFile(USARTx->port, data, len, &dwBytes, NULL) || dwBytes < len) // LINUX: n = write(ecufd, data, len);
			return ERR_IO;
		len -= (uint16_t)dwBytes;
	}

	return ERR_SUCCESS;
}

uint16_t USART_receive(USART* USARTx, uint8_t* data, uint16_t maxlen)
{
	DWORD dwBytes = 0;
	if (!ReadFile(USARTx->port, data, maxlen, &dwBytes, NULL)) // LINUX: n = read(ecufd, data, maxlen); can use select() to wait
		return 0;
	return (uint16_t)dwBytes;
}


#ifdef HT_DDL
static hterr USARTNetIf_doTX(USART* USARTx, pbuf* packet, hwaddr_t destAddr)
{
	uint8_t buf[10]={0xaa,0x55};
	uint16_t offset=0;
	int8_t  n=2, ret;
	memcpy(buf+n, destAddr, sizeof(hwaddr_t)); n+= sizeof(hwaddr_t);
	*((uint16_t*)(buf+n)) = packet->tot_len; n+= sizeof(uint16_t);
	if (ERR_SUCCESS != (ret= USART_transmit(USARTx, buf, n)))
		return ret;

	while (offset <packet->tot_len)
	{
		n = (int8_t) pbuf_read(packet, offset, buf, sizeof(buf)-1);
		if (n <=0)
			return ERR_INVALID_PARAMETER;

		if (ERR_SUCCESS != (ret= USART_transmit(USARTx, buf, n)))
			return ret;

		offset += n;
	}

	buf[0] = '\r'; buf[1] = '\r';
	USART_transmit(USARTx, buf, 2);

	return ERR_SUCCESS;
}


hterr USARTNetIf_attach(USART* USARTx, HtNetIf* netif, const char* name)
{
	if (NULL == USARTx || NULL == netif)
		return ERR_INVALID_PARAMETER;

	HtNetIf_init(netif, USARTx, name, NULL, USART_MAX_PLOAD, (uint8_t*) "DUMMY", 4, 0,
				 NetIf_OnReceived_Default, USARTNetIf_doTX, NULL);
	USARTx->netif = netif;

	return ERR_SUCCESS;
}

void USART_OnReceived(USART* USARTx, uint8_t* data, uint16_t len)
{
	pbuf* packet = pbuf_mmap(data, len);
	if (NULL != USARTx && NULL != USARTx->netif && NULL != USARTx->netif->cbReceived && NULL != packet)
		USARTx->netif->cbReceived(USARTx->netif, packet, HTDDL_DEFAULT_POWERLEVEL);
	pbuf_free(packet);
}

void HtDDL_pollUSART(USART* USARTx)
{
	uint8_t buf[USART_MAX_PLOAD+2] ={0};
	int16_t n=0, i, j;

	i = FIFO_awaitSize(&USARTx->rxFIFO);
	n = USART_receive(USARTx, buf +i, sizeof(buf)-2 -i);
	if (n <=0)
		return; // do nothing

	// read the bytes left by the previous read
	for (j=0; j<i; j++)
		FIFO_pop(&USARTx->txFIFO, buf+j);

	n +=i; i =0;
	while (i < n-sizeof(hwaddr_t)-2)
	{
		if (0xaa != buf[i++] || 0x55 != buf[i++])
			continue; // not a leading header

		j = *((uint16_t*)(buf+ i +sizeof(hwaddr_t)));
		if (i + sizeof(hwaddr_t) + sizeof(uint16_t) +j >n)
		{
			// implemented packet
			i -=2;
			break;
		}

		if ('\r' == buf[i+2+sizeof(hwaddr_t) +sizeof(uint16_t) +j])
		{
			i++; // not a valid packet, move a bit ahead to detect the next packet
			continue;
		}

		i+= sizeof(hwaddr_t) + sizeof(uint16_t);

		USART_OnReceived(USARTx, buf+ i, j);
		i+= j;
		while (i<n && ('\r' == buf[i] || '\n' == buf[i]))
			i++;
	}

	// flush the unprocessed byte into the FIFO for next use
	for (; i<n; i++)
	{
		FIFO_push(&USARTx->rxFIFO, buf+i);
	}
}

#endif // HT_DDL

*/
