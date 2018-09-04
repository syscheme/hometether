// The command line or cgi program to interact with htd about the HomeTether components
#include "htc.h"
#include "getopt.h"
#include "htdomain.h"

#define FMT_PATTERN_HEXVAL     "%[0-9A-Za-z]"
#define FMT_PATTERN_DECVAL     "%[0-9]"

#define DEFAULT_TIMEOUT        (3000) // 3sec

// configurations
bool bCGI =false, bJSON =false;
std::string htd_server="localhost", htc_client="cmd", cgi_method, cgi_query;
int htd_port=HTD_SERVER_PORT, htd_timeout =DEFAULT_TIMEOUT;

typedef int (*fSubCmd_t) (int argc, char *argv[]);

typedef struct _subCmd
{
	const char* cmdName;
	fSubCmd_t   func;
} SubCmd;

#define SUBCMDS_BEGIN  SubCmd _SubCmds[] = {
#define ADD_SUBCMD(_subcmd)  { #_subcmd, SubCmd__##_subcmd },
#define SUBCMDS_END  {NULL, NULL} };

SUBCMDS_BEGIN
ADD_SUBCMD(hello)
ADD_SUBCMD(getADC)
ADD_SUBCMD(getTemp)
ADD_SUBCMD(getMotion)
ADD_SUBCMD(getRBuf)
ADD_SUBCMD(getState)
ADD_SUBCMD(setGState)
ADD_SUBCMD(getConfig)
ADD_SUBCMD(setConfig)
ADD_SUBCMD(reset)
ADD_SUBCMD(htcs)
ADD_SUBCMD(chstate)
SUBCMDS_END


/*
SubCmd _SubCmds[] = {
	{}
	{NULL, NULL} };

#define SUBCMDS_BEGIN  if (0) 
#define SUBCMDS_END  return -1;
#define ADD_SUBCMD(_subcmd) else if (0 == stricmp(cmd, #_subcmd)) return _subcmd(argc, argv)
*/
uint32_t _now()
{
#ifdef _WIN32
	FILETIME systemtimeasfiletime;
	GetSystemTimeAsFileTime(&systemtimeasfiletime);
	unsigned __int64 ltime;
	memcpy(&ltime, &systemtimeasfiletime, sizeof(ltime));
	ltime /= 10000;  //convert nsec to msec

	return (uint32_t) ltime;
#else
    struct timeval tmval;
    int ret = gettimeofday(&tmval, (struct timezone*)NULL);
    if(ret != 0)
        return 0;
    return (uint32_t) (tmval.tv_sec*1000LL + tmval.tv_usec/1000);
#endif
}

#ifdef _WIN32
class init_WSA
{
public:
	init_WSA()
	{
		//-initialize OS socket resources!
		WSADATA	wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData))
			abort();
	}

	~init_WSA() 
	{ 
		WSACleanup(); 
	}
} initWSA;

#endif // _WIN32


#ifdef _WIN32
#  define EXEC_NAME "htc.exe"
#else
#  define EXEC_NAME "htc"
#endif

// dummy definition in order to use class HomeTetherDomain
bool bQuit =false;
bool& HomeTetherDomain::_bQuit = bQuit;
int ecufd =2;

HTClient htc;

#define ARGS_MAX (10)

int main(int argc, char **argv)
{
	// htc.exec2("GET add\r\n", "POST %[0-9A-Za-z]/ddd?eee=%[0-9A-Za-z]&fff=%[0-9A-Za-z]", 3, 89);
	char *cmd = NULL;
	int argi =1;

	const char * q = getenv("SERVER_PROTOCOL");
	if (q && strstr(q, "HTTP/"))
	{
		// CGI mode
		bCGI = true;
		htc_client = "cgi[";
		if (q = getenv("REMOTE_USER"))
			htc_client += q;
		if (q = getenv("REMOTE_ADDR"))
			htc_client += std::string("@") + q;
		if (q = getenv("REQUEST_METHOD"))
		{
			cgi_method = q;
			htc_client += std::string(":") + cgi_method;
		}

		htc_client += "]";
		if (q = getenv("SCRIPT_NAME"))
		{
			static char scriptName[64]="";
			strncpy(scriptName, q, sizeof(scriptName)-2);
			if (argv[0] =strrchr(scriptName, '/'))
				argv[0]++;
			else argv[0] = scriptName;
		}

		if (q = getenv("QUERY_STRING"))
		{
			static char* cgiargv[ARGS_MAX*2+1];
			cgiargv[0] = argv[0];

			static char queryStr[256];
			strncpy(queryStr, q, sizeof(queryStr)-2);
			char* v =queryStr, *val;
			int i=0;
			for (i=0; i < ARGS_MAX && NULL !=v; i++)
			{
				cgiargv[i*2] = v; val=v;
				if (v = strchr(v, '&'))
					*v++ = '\0';

				cgiargv[i*2+1]=NULL;
				if (val = strchr(val, '='))
				{
					*val++='\0';
					cgiargv[i*2+1]=val;
				}
			}

			argc=1;
			for (int j=0; j <i*2; j++)
			{
				if (cgiargv[j] && *cgiargv[j])
					argv[argc++] = cgiargv[j];
			}
		} // QUERY_STRING
	}
	else // */
	{
		// command line mode
		for (; argi < argc; argi++)
		{
			if (0 == strcmp("-S", argv[argi]))
			{
				if (argi < argc -1)
					htd_server = argv[++argi];
				continue;
			}

			if (0 == strcmp("-P", argv[argi]))
			{
				if (argi < argc -1)
					htd_port = atoi(argv[++argi]);
				continue;
			}

			if (0 == strcmp("-T", argv[argi]))
			{
				if (argi < argc -1)
					htd_timeout = atoi(argv[++argi]);
				continue;
			}

			if (0 == strcmp("-W", argv[argi]))
			{
				bCGI = true;
				continue;
			}

			if (0 == strcmp("-J", argv[argi]))
			{
				bJSON = true;
				continue;
			}

			cmd = argv[argi];
			break;
		}

	char* p = strrchr(argv[0], FNSEPC);
	if (NULL == p)
		p = argv[0];
	else ++p;

	if (*p && 0 != stricmp(p, EXEC_NAME))
	{
		if (cmd) argi--; // reverse by 1
		cmd = p;
	}

	if (NULL == cmd || 0 == *cmd)
		return -2;

	// adjust the arguments for the sub-command
	argc -= argi;
	argv += argi;
	argv[0] = cmd;
	}

	htc.connect(htd_server.c_str(), htd_port);

	for (int i=0; _SubCmds[i].cmdName; i++)
	{
		if (0 != stricmp(_SubCmds[i].cmdName, cmd))
			continue;

		return _SubCmds[i].func(argc, argv);
	}

	printf("%s: unknown command\n", cmd);
	return -3;
}

// -----------------------------
// class HTClient
// -----------------------------
HTClient::HTClient(const char*bindIP)
:_so(-1), _bConnected(false)
{
	_so = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in soaddr;
	bzero(&soaddr, sizeof(soaddr));
	soaddr.sin_family = AF_INET;
	soaddr.sin_addr.s_addr = htons(INADDR_ANY);
	soaddr.sin_port = htons(0);
	if (NULL != bindIP)
	{
		struct hostent* hp = gethostbyname(bindIP);
		if (NULL != hp)
			soaddr.sin_addr.s_addr = (((struct in_addr **)hp->h_addr_list)[0])->s_addr;
		bind(_so, (struct sockaddr*)&soaddr, sizeof(soaddr));
	}
}

HTClient::~HTClient()
{
	if (_so>=0)
		close(_so);
	_so = -1;
}

const char* HTClient::readValueAt(uint8_t index)
{
	return (index < MAX_VALUES) ? _values[index] : NULL;
}

bool HTClient::connect(const char* server, int serverPort)
{
	if (_bConnected)
		return false;

	if (NULL == server)
		server = "localhost";

	struct sockaddr_in soaddr;
	bzero(&soaddr,sizeof(soaddr));
	soaddr.sin_family = AF_INET;
	soaddr.sin_port = htons(serverPort);

	struct hostent* hp = gethostbyname(server);
	if (NULL != hp)
		soaddr.sin_addr.s_addr = (((struct in_addr **)hp->h_addr_list)[0])->s_addr;

	socklen_t soaddr_length = sizeof(soaddr);
	// connect to the htd server
	if (::connect(_so, (struct sockaddr*)&soaddr, soaddr_length) < 0)
		return false;

	return _bConnected = true;
}

int HTClient::exec(const char* req, const char* respFmt, int cValExpected, int timeout)
{
	char resp[512];
	connect();

	if (!_bConnected)
		return -3;

	if (send(_so, req, strlen(req), 0) <=0)
		return -1;

	Sleep(200);
	if (send(_so, req, strlen(req), 0) <=0)
		return -1;

	if (NULL == respFmt || cValExpected<=0) // no response is expected, return instantly after request is sent
		return 0;

	resetValueList();

	int ret =0;
	int stampExpire = _now() + timeout;

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(_so, &fdset);

	struct timeval to;
	char* pToRecv = resp;
	while (ret < cValExpected)
	{
		// test if it has reached timeout
		fd_set rset = fdset;
		int timeleft = stampExpire - _now();
		if (timeleft <=0)
			break;

		// wait for the response message to receive
		to.tv_sec = timeleft / 1000; timeleft %= 1000;
		to.tv_usec = timeleft * 1000; 

		int k = select(_so+1, &rset, NULL, &rset, &to);
		if (0 == k)
			continue;
		else if (k <0)
			break;

		// receive the response
		k = read(_so, pToRecv, resp + sizeof(resp) - pToRecv -2);
		if (k <=0)
			break;

		// parse the received lines if any matches the expected format
		bool bEOLReached =false;
		char* p = pToRecv;
		pToRecv += k;

		for (; k>0; k--, p++)
		{
			if (bEOLReached) // shift the received bytes if EOL ever reached
				*pToRecv++ = *p;

			if ('\r'!=*p && '\n'!=*p && '\0' !=*p)
				continue;

			// reached a EOL or EOS, terminate the string and reset the pToRecv
			bEOLReached = true;
			*pToRecv = '\0';
			pToRecv =resp;

			// reached a EOL or EOS, try to parse it if not yet comforted
			if (ret < cValExpected && resp[0] && resp[1])
			{
				ret = sscanf(resp, respFmt, _values[0], _values[1], _values[2], _values[3], _values[4],
					_values[5], _values[6], _values[7], _values[8], _values[9], _values[10],
					_values[11], _values[12], _values[13], _values[14], _values[15], _values[16],
					_values[17], _values[18], _values[19]);
			}
		}
	}

	if (ret < cValExpected)
		ret =0;

	return ret;
}

int HTClient::exec2(const char* req, const char* respFmt, int cValExpected, int timeout)
{
	char resp[512] = "";
	char in[512] = "POST 10/aaa?bbb=123&ccc=333\r\nPOST 10/ddd?eee=567&fff=890\n\r\0POST 10/aaa?bbb=123&ccc=333\r";

	resetValueList();

	int ret =0;
	char* pToRecv = resp;
	int m=0;
	while (ret < cValExpected)
	{
		int k = 20;
		memcpy(pToRecv, in+k*m++, k);

		// parse the received lines if any matches the expected format
		bool bEOLReached =false;
		char* p = pToRecv;
		pToRecv += k;

		for (; k>0; k--, p++)
		{
			if (bEOLReached) // shift the received bytes if EOL ever reached
				*pToRecv++ = *p;

			if ('\r'!=*p && '\n'!=*p && '\0' !=*p)
				continue;

			// reached a EOL or EOS, terminate the string and reset the pToRecv
			bEOLReached = true;
			*pToRecv = '\0';
			pToRecv =resp;

			// reached a EOL or EOS, try to parse it if not yet comforted
			if (ret < cValExpected && p > resp)
			{
				ret = sscanf(resp, respFmt, _values[0], _values[1], _values[2], _values[3], _values[4],
					_values[5], _values[6], _values[7], _values[8], _values[9], _values[10],
					_values[11], _values[12], _values[13], _values[14], _values[15], _values[16],
					_values[17], _values[18], _values[19]);
			}
		}
	}

	if (ret < cValExpected)
		ret =0;

	return ret;
}

void HTClient::resetValueList()
{
	for (int i =0; i < MAX_VALUES; i++)
	{
		_values[i] = _valuebuf + i*MAX_VALUE_LEN;
		*_values[i] = '\n';
	}
}

