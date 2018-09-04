#ifndef __BEE_NET_H__
#define __BEE_NET_H__

#include <windows.h>
#include <string.h>

typedef unsigned char    uint8_t;
typedef char             int8_t;
typedef unsigned short   uint16_t;
typedef short            int16_t;
typedef unsigned __int32 uint32_t;

// #define BEEROUTER_ENABLED

typedef union _BeeAddress
{
	uint8_t  b[4];
	uint32_t dw;
} BeeAddress;

#define BEE_NET_ADDR_LEN      4

typedef uint8_t    BeeNodeId_t;

typedef struct _BeeIF
{
	BeeAddress  addr;
	BeeNodeId_t nodeId;
	BeeNodeId_t uplink;
} BeeIF;

//extern BeeNodeId_t thisBeeNodeId;
//extern BeeAddress  thisBeeAddr; // =b{BEE_NEST_ADDR_B0,'S','W','1'}
//extern BeeNodeId_t uplinkNodeId;

// reserved leading B0 of 4-byte address
#define DOMAIN_ADDR_B0_INVALID      (0x00)
#define DOMAIN_ADDR_B0_BEE          (0xBE)
#define DOMAIN_ADDR_B0_CAN          (0xCA)
#define DOMAIN_ADDR_B0_USART        (0x5A)

#define DOMAIN_ADDR_B0_IP_C         (192)
#define DOMAIN_ADDR_B0_IP_A         (10)
#define DOMAIN_ADDR_B0_IP_B         (172)

#define BEEROUTE_TIMEOUT_RELOAD     (0x3fff)
#define BEEROUTE_NEIGHBORHOOD_MAX   (16)
#define BEEROUTE_STACK_ADDRS_SZ     (10)
#define BEEROUTE_STACKS_MAX         (BEEROUTE_NEIGHBORHOOD_MAX >>2)
#define BEEROUTE_STACK_IDX_INVALID  (0xff)
#define BEEROUTE_NEIGHBOR_PENALTY_MAX (3)

#define UPLINK_DEST_IP                {8,8,8,8}


// #define BEE_NEST_ADDR				(DOMAIN_ADDR_B0_BEE <<24) | 0xFECD88 // 3-BYTE CLUSTER
#define BEE_NEST_BCAST_NODEID		0xff
#define BEE_NEST_INVALID_NODEID     0x00

/*
typedef struct _BeeRouteRow
{
	BeeAddress   dest;
	BeeNodeId_t  nextHop;
	uint16_t     timeout;
	uint8_t      hops;
} BeeRouteRow;

extern BeeRouteRow BeeRouteTable[BEE_ROUTE_TABLE_SIZE];
extern BeeRouteRow BeeRouteUpLinks[2];
*/

#define AIR_PAYLOAD      (20)

#define BEE_MAC_TTL_MAX  (3)

#define BEE_MAC_MSG_SIZE_MAX       (512)
#define BEE_MAC_TRANSMIT_MAX_NACK  (10)
#define BEE_MAC_TRANSMIT_CTX_COUNT (2)

#define BEE_MAC_FORWARD_BUF_COUNT  (2)
#define BEE_MAC_FORWARD_BUF_TIMEOUT_RELOAD  (0xff)

// BeeMac APIs
// ---------------------
void Bee_init(BeeIF* beeIf, BeeNodeId_t nodeId);
void Bee_doScan(BeeIF* beeIf);

uint8_t BeeMAC_sendShortMessage(BeeIF* beeIf, uint8_t* msgbuf, BeeAddress srcAddr, BeeAddress destAddr, BeeNodeId_t nextHop, uint8_t* data, uint16_t datalen);
uint8_t BeeMAC_transmit(BeeIF* beeIf, uint8_t* msgbuf, BeeAddress srcAddr, BeeAddress destAddr, uint8_t* data, uint16_t datalen);

// BeeMac callbacks
// ---------------------
//@return the 100msec that wish to linger the received message, 0 to hint the message has been processed in the NET layer
uint8_t BeeNet_OnDataArrived(BeeIF* beeIf, BeeAddress srcAddr, BeeAddress destAddr, uint8_t* data, uint16_t datalen);
void BeeNet_OnPong(BeeIF* beeIf, BeeAddress srcAddr, BeeNodeId_t nextHop, uint8_t hops);

//  BeeMac portal entries
// ------------------------------
uint8_t BeeMAC_doSend(BeeIF* beeIf, uint8_t nextHop, uint8_t* data, uint8_t datalen);

// BeeRoute
// ---------------------
void BeeRoute_add(BeeAddress dest, BeeNodeId_t nextHop, uint8_t hops);
BeeNodeId_t BeeRoute_lookfor(BeeIF* beeIf, BeeAddress dest);
void BeeRoute_delete(BeeAddress dest, BeeNodeId_t nextHop);

#endif // __BEE_NET_H__

