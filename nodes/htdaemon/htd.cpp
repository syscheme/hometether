#include "ht_comm.h"
#include "htdomain.h"
#include "htdstate.h"

#include <string>
#include <map>

// http://blog.csdn.net/hevake_lcj/article/details/6639750

bool bQuit =false;
bool& HomeTetherDomain::_bQuit = bQuit;

int listenfd, ecufd;
#define BUF_SZ (200)
typedef struct _Conn
{
	int so;
	char buf[BUF_SZ+3];
	int offset;
} Conn;

Conn conns[MAX_CONN];

#define DUMMY_FD  (99)

#ifndef _WIN32
struct termios oldtio, newtio;
#endif

int connectECU()
{
#ifdef _WIN32
	ecufd =DUMMY_FD;
#else

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
#endif

	return ecufd;
}

void disconnectECU()
{
#ifndef _WIN32
	// restore the old port settings
	tcsetattr(ecufd,TCSANOW, &oldtio);
	close(ecufd);
#endif
}

typedef struct _MsgBuf
{
	char buf[1024]; // ="\0\r\nPOST hello\n\rPOST new\0\r1234567890";
	int  offset; // =40;

#ifdef _WIN32
#else
	pthread_mutex_t lock;
#endif
} MsgBuf;

class MsgBufGuard
{
public:
	MsgBufGuard(MsgBuf& mb) : _mb(mb)
	{
#ifdef _WIN32
#else
		pthread_mutex_lock(&_mb.lock);
#endif
	}

	~MsgBufGuard()
	{
#ifdef _WIN32
#else
		pthread_mutex_unlock(&_mb.lock);
#endif
	}

private:
	MsgBuf _mb;
};

MsgBuf MsgFromEcu;

// -----------------------------
// thread MessageProc
// -----------------------------
void* Thrd_MessageProc(void* args)
{
	trace("MessageProc() starts\n");
	strcpy(MsgFromEcu.buf, "\0\r\nPOST hello\n\rPOST new\0\r1234567890");
	MsgFromEcu.offset =40;

	while (!bQuit)
	{
		char lines[512];
		int i, j, lent=0, lens=0;

		{
			MsgBufGuard g(MsgFromEcu);

			for (i=0, j=0; (i < MsgFromEcu.offset) && (j <sizeof(lines)-2); j++)
			{
				lines[j] = MsgFromEcu.buf[i++]; 
				if ('\0' != lines[j] && '\r' != lines[j] && '\n' != lines[j])
					continue;

				lines[j] = '\0';
				lent = j;
				while ('\r' == MsgFromEcu.buf[i] || '\n' == MsgFromEcu.buf[i])
				{
					if (++i >= MsgFromEcu.offset)
						break;
				}

				lens =i;
			}

			// shift the MsgFromEcu.offset
			MsgFromEcu.offset -=lens;
			for (j=0; lens >0 && j <MsgFromEcu.offset; j++)
				MsgFromEcu.buf[j] = MsgFromEcu.buf[lens+j];

		} //unlock the MsgFromEcu

		if (lent <=0) // no valid line received yet
		{
			struct timeval timeout;
			timeout.tv_sec	= 0;
			timeout.tv_usec	= 50 *1000; // 50msec
			select(0, NULL, NULL, NULL, &timeout);
			continue;
		}

		for (char* p = lines; p < (lines + lent); p+= strlen(p)+1)
		{
			char URL[100];
			if (sscanf(p, "POST %s", URL) >0)
			{
				_htdomain.OnEcuPost(URL);
				continue;
			}
		}
	}

	trace("MessageProc() ends\n");
	
#ifndef WIN32
	pthread_exit(NULL);   //thread exit
#endif 
	return NULL;
}

// -----------------------------
// thread DomainScan
// -----------------------------
void* Thrd_DomainScan(void* args)
{
	trace("DomainScan() starts\n");

	while (!bQuit)
	{
		int nextSleep = _htdomain.doStateScan();
		if (bQuit)
			break;

		Sleep(nextSleep);

	// _htdomain.flushQueries();
	// _htdomain.logStatus();
	// usleep(500*1000);
	}

	trace("DomainScan() ends\n");
	
#ifndef WIN32
	pthread_exit(NULL);   //thread exit
#endif 
	return NULL;
}

void processUserReq(int connfd, char* message, int len)
{
	struct sockaddr_in addr;
	socklen_t solen = sizeof(addr);
	message[len]='\0';
	char str1[20], str2[20];
	// strcpy(message, "\r\n1234567890\r\n"); len=strlen(message)+1;
	getpeername(connfd, (struct sockaddr *)&addr, &solen);
	char *p =NULL, *q;
	if (NULL != (p = strstr(message, "PUT ht:00/")))
	{
		p += sizeof("PUT ht:00/") -1;
		uint8_t state = soc_Easy;
		if (NULL != (q = strstr(message, "gstate=")))
		{
			HtdState(_htdomain).moveToState((StateOrdinalCode) atoi(q+sizeof("gstate=")-1));
			return;
		}
	}

	if (DUMMY_FD != ecufd)
		write(ecufd, message, len);
	trace("from[%d@%s:%d] to[%s]: %s\n", connfd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), DEVICE_ECU, message);
}

void processEcuMsg(char* message, int len)
{
	std::string tolist;
	if (len<=0)
		return;

	message[len]='\0';
	struct sockaddr_in addr;
	socklen_t solen = sizeof(addr);
	char peerinfo[100];

	for (int i=0; i<MAX_CONN; i++)
	{
		if (conns[i].so<0)
			continue;

		::getpeername(conns[i].so, (struct sockaddr *)&addr, &solen);
		write(conns[i].so, message, len);
		snprintf(peerinfo, sizeof(peerinfo)-2, "%d@%s:%d, ", conns[i].so, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		tolist += peerinfo;
	}

	if (tolist.length() >2)
		tolist = tolist.substr(0, tolist.length()-2);

	trace("from[" DEVICE_ECU "] to[%s]: %s\n", tolist.c_str(), message);

		MsgBufGuard g(MsgFromEcu);
	
	if (len> sizeof(MsgFromEcu.buf) - MsgFromEcu.offset)
		len = sizeof(MsgFromEcu.buf) - MsgFromEcu.offset;
	
	if (len >0)
	{
		memcpy(MsgFromEcu.buf+MsgFromEcu.offset, message, len);
		MsgFromEcu.offset += len;
	}
}

#ifdef WIN32
void openlog(const char* svc, int flag, int flg2) {}
#endif 

// -----------------------------
// main()
// -----------------------------
int main(int argc, char **argv)
{
    // step 1. open log
	openlog("HomeTether", LOG_CONS | LOG_PID, 0);
	notice("=== HTD: %s starts", argv[0]);

	// step 2. load configuration
	_htdomain.loadConfig("/etc/htd.conf");
	_htdomain.validateConfig();

	// step 3. start Thrd_MessageProc(); 
	pthread_t thrd_MessageProc; 
	int thId_MessageProc = pthread_create(&thrd_MessageProc, NULL, Thrd_MessageProc, (void*)NULL);

	socklen_t solen;
	struct sockaddr_in peeraddr, servaddr;

	//values for select
	int i, maxi, maxfd;
	int nready;
	size_t n;
	fd_set rset, eset, allset;

	// step 4. open the serial port to ECU
	if (connectECU() <0)
	{
		trace("failed to open ECU[%s]: %s\n", DEVICE_ECU, (char*)strerror(errno)); 
		return 1;
	}

	// step 5. start Thrd_DomainScan() 
	pthread_t thrd_DomainScan; 
	int thId_DomainScan = pthread_create(&thrd_DomainScan, NULL, Thrd_DomainScan, (void*)NULL);

	// step 6. open the listener port
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(HTD_SERVER_PORT);

	if ((listenfd=socket(AF_INET, SOCK_STREAM, 0))<0)
	{
		trace("socket established error: %s\n", (char*)strerror(errno)); 
		return 2;
	}

	i=1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char*)&i, (socklen_t)sizeof(i));

	if (bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) <0)
	{
		trace("bind error: %s\n", strerror(errno));
		return 3;
	}

	listen(listenfd, SOMAXCONN);

	//initial "select" elements
	maxi=-1;
	for(i=0;i<MAX_CONN;i++)
	{
		conns[i].so = -1;
		conns[i].offset =0;
	}

	// step 7. listening loop
	maxfd=listenfd;
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

#ifndef _WIN32
	if (maxfd< ecufd)
		ecufd = ecufd; 
	FD_SET(ecufd, &allset);
	signal(SIGINT, OnSignal);
#else
	if (::SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler,TRUE)==FALSE)
	{
		trace("Unable to install handler!\n");
		return -1;
	}
#endif

	struct timeval timeout;

	while (!bQuit)
	{
		timeout.tv_sec	= 0;
		timeout.tv_usec	= 200 *1000; // 200msec

		rset=allset; eset=allset;
		nready = select(maxfd+1, &rset, NULL, &eset, &timeout);

		if (nready <=0)
			continue;

		// step 1. processing the ecufd
		if (FD_ISSET(ecufd, &eset))
		{
			trace("quit per ECU comm error: %s\n",strerror(errno));
			bQuit = true;
			break;
		}
		else if (FD_ISSET(ecufd, &rset))
		{
			char buf[BUF_SZ+3];
			n = read(ecufd, buf, BUF_SZ);
			if (n>0)
				processEcuMsg(buf, n);
		}

		// step 2. processing the listenfd
		if (FD_ISSET(listenfd, &eset))
		{
			trace("quit per listener dead: %s\n", strerror(errno));
			close(listenfd);
			bQuit = true;
			break;
		}

		if (FD_ISSET(listenfd, &rset))
		{
			solen=sizeof(peeraddr);

			int connfd = accept(listenfd, (struct sockaddr*)&peeraddr, &solen);
			if (connfd < 0)
				trace("accept client error: %s\n",strerror(errno));
			else        
				trace("client[%d] connected\n", connfd);

			for (i=0; i<MAX_CONN; i++)
			{
				if (conns[i].so <0)
				{
					conns[i].so =connfd;
					break;
				}
			}

			if (i >=MAX_CONN)
			{
				trace("too many clients, reject the new one[%d]\n", connfd);
				close(connfd);
				break;
			}

			FD_SET(connfd, &allset);

			if(connfd >maxfd)
				maxfd =connfd;

			if (i >maxi)
				maxi=i;

			--nready;
		}

		if (nready <=0)
			continue;

		// step 3. process the message on each established connection
		for (i=0; i<=maxi; i++)
		{
			if (conns[i].so <0)
				continue;

			bool bToClose = FD_ISSET(conns[i].so, &eset) ? true:false;

			if (FD_ISSET(conns[i].so, &rset))
			{
				n = read(conns[i].so, conns[i].buf, BUF_SZ);
				if (n>0)
				{
					processUserReq(conns[i].so, conns[i].buf, n);
					continue;
				}

				bToClose = true;
			}

			if (bToClose)
			{
				trace("conn[%d] closed by peer\n", conns[i].so);
				close(conns[i].so);
				FD_CLR(conns[i].so, &allset);
				conns[i].so =-1;
				continue;
			}
		}
	}

	disconnectECU();
	if (listenfd >=0)
		close(listenfd);

	syslog(LOG_DEBUG, "%s quits", argv[0]);

	pthread_join(thId_MessageProc, NULL);
	pthread_join(thId_DomainScan, NULL);

	closelog(); 

	return 0;
}

#ifdef _WIN32
BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
	switch(CEvent)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		bQuit = true;
		break;

	}

	return TRUE;
}
#endif

void OnSignal(int sig)
{
	// function called when signal is received.
	bQuit = true;
}
