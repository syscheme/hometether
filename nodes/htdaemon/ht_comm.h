#ifndef __HTD_COMMON_H__
#define __HTD_COMMON_H__

#include <string>
#include <map>

#include "../ht.h"

// extern "C" {
#ifdef _WIN32
#  pragma warning (disable: 4996)
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "wsock32.lib") // VC link wsock32
#  define bzero(_BUFF, _SIZE) memset(_BUFF, 0x00, _SIZE)
#  define close closesocket
#  define read(_SO, _BUF, _LEN)  ::recv(_SO, _BUF, _LEN, 0)
#  define write(_SO, _BUF, _LEN) ::send(_SO, _BUF, _LEN, 0)
#  define snprintf _snprintf
#  define FNSEPC '\\'
typedef char                int8_t;
typedef short               int16_t;
typedef __int32             int32_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned __int32    uint32_t;

BOOL WINAPI ConsoleHandler(DWORD CEvent);
#else
#               include <sys/types.h>
#               include <sys/socket.h>
#               include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netdb.h>
#include <net/if.h>
#include <fcntl.h>
#include <pthread.h>
#define FNSEPC '/'
#define stricmp strcasecmp
#define Sleep(msec) usleep(msec*1000)

/* already covered in <types.h>
typedef char                int8_t;
typedef short               int16_t;
typedef int                 int32_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
*/

void OnSignal(int sig);

#endif // _MSC_VER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// }

#define DEFAULT_TIMEOUT        (3000) // 3sec
#define HTD_SERVER_PORT        84
#define BAUDRATE               B38400
#define DEVICE_ECU             "/dev/ttyS0"
#define MAX_CONN               (5)

typedef std::map <std::string, uint32_t> IntMap;
#define INVALID_INTVAL  (0xfcfcfcfc)

#ifdef _WIN32
#  define trace printf
#  define info  printf
#  define notice  printf
#else
extern "C"
{
#include <syslog.h>
}

#  define info(...) syslog(LOG_INFO, __VA_ARGS__) // printf(__VA_ARGS__)
#  define trace(...) syslog(LOG_DEBUG, __VA_ARGS__) // printf(__VA_ARGS__)
#  define notice(...) syslog(LOG_NOTICE, __VA_ARGS__) // printf(__VA_ARGS__)
#endif

uint32_t now();
std::string IntMapToJSON(const IntMap& valueMap);
const char* hex2int(const char* hexstr, uint32_t* pVal);

#endif // __HTD_COMMON_H__
