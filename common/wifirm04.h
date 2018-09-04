#ifndef __WifiRM04_H__
#define __WifiRM04_H__

#include "htcomm.h"
#include "usart.h"
#include "textmsg.h"

typedef struct _WifiRM04
{
	USART    tty;
	TextLineCollector tlc;

	uint32_t localIp;
	__IO__ uint16_t flags;	 // combination of WifiRM04FLG_XXX

} WifiRM04;

#define WifiRM04FLG_OUT_TRANS    FLAG(0)

// ---------------------------------------------------------------------------
// method declarations
// ---------------------------------------------------------------------------
hterr WifiRM04_init(WifiRM04* chip);
hterr WifiRM04_sendLine(WifiRM04* chip, const char* line);

#define WifiRM04_udpConnect(chip, remoteIp, remotePort) WifiRM04_openUSARTMode(chip, FLAG(0)|FLAG(1), remoteIp, remotePort)
#define WifiRM04_udpListen(chip, serverPort)            WifiRM04_openUSARTMode(chip, FLAG(0), 0, serverPort)
#define WifiRM04_tcpConnect(chip, remoteIp, remotePort) WifiRM04_openUSARTMode(chip, FLAG(1), remoteIp, remotePort)
#define WifiRM04_tcpListen(chip, serverPort)            WifiRM04_openUSARTMode(chip, 0, 0, serverPort)

hterr WifiRM04_openUSARTMode(WifiRM04* chip, uint8_t mode, uint32_t remoteIp, uint16_t remotePort);

// portal entry
hterr WifiRM04_OnDataLineReceived(WifiRM04* chip, char* line);

// usage
// WifiRM04 rm04= { {USART1, {NULL,} , {NULL,}}, {NULL, 0,}, 0, 0};
// WifiRM04_init(&rm04);
// WifiRM04_tcpListen(&rm04, 8080)
// hterr WifiRM04_OnDataLineReceived(WifiRM04* chip, char* line)
// {
//     // parse the line and handling
//     WifiRM04_sendLine(chip, "response"); // send the response
//     return ERR_SUCCESS;
// }

#endif  // __WifiRM04_H__
