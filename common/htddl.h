#ifndef __HT_DDL_H__
#define __HT_DDL_H__

// maximal to transfer 512Byte per transmission

#include "htcomm.h"
#include "pbuf.h"

#include <stdio.h>
#include <string.h>

// configurations
#define HTDDL_ID_INVALID         (0xff)
#define HTDDL_HEAD_LEN_MAX       (5)
#define SLOT_TIMEOUT_CON         (1000)
#define SLOT_TIMEOUT             (SLOT_TIMEOUT_CON<<2)
#define HTDDL_MSG_LEN_MAX        (16)

#define HTDDL_SLOTS_SIZE_RX      (4)
#define HTDDL_SLOTS_SIZE_TX      (10)

#define HTDDL_REQ(_CMD)  (_CMD | 0x80)
#define HTDDL_RESP(_CMD) (_CMD | 0xC0)

#define HTDDL_CMD_MASK     (0x3f)
#define IS_REQ(CTRLBYTE)   (0x80 == (CTRLBYTE & 0xC0))
#define IS_RESP(CTRLBYTE)  (0xC0 == (CTRLBYTE & 0xC0))

typedef enum {
	HTCMD_CON, HTCMD_DISCON, HTCMD_BMP,
} HTCMD;

// portal of OS funcs: locks, sleep, ticks
#ifdef FreeRTOS
#  include "task.h"
#  include "queue.h"
#  define SLOT_LOCK_TX()          vTaskSuspendAll()
#  define SLOT_UNLOCK_TX()        xTaskResumeAll()
#  define SLOT_LOCK_RX()          vTaskSuspendAll()
#  define SLOT_UNLOCK_RX()        xTaskResumeAll()
#endif //FreeRTOS

#ifndef min
#  define min(_X, _Y) ((_X)>(_Y)?(_Y):(_X))
#endif

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define HTDDL_DEFAULT_POWERLEVEL (0x08)

#define HTDDL_HWADDR_LEN_MAX     (6) // 6-byte hardware address len in max
typedef uint8_t hwaddr_t[HTDDL_HWADDR_LEN_MAX];

#define NETIF_FLAG_UP   FLAG(0)  // a software flag used to control whether is enabled and processes traffic

struct _HtNetIf;
// Callback prototype for netif->cbOnReceived functions. This function is saved as 'input'
// callback function in the netif struct. Call it when a packet has been received.
// @param netif The netif which received the packet
// @param packet The received packet, copied into a pbuf
// @param uint8_t the powerLevel of receiving such a packet, 0-lowest 0x0f-highest, HTDDL_DEFAULT_POWERLEVEL is recommented if not appliable
typedef hterr (*NetIf_cbReceived_ft)(struct _HtNetIf *netif, pbuf *packet, uint8_t powerLevel);

// Function prototype for netif->output functions. Called by the upper layer
// @param netif The netif which shall send a packet
// @param p The packet to send
// @param destAddr the destination address to which the packet shall be sent
typedef hterr (*NetIf_doTransmit_ft)(struct _HtNetIf *netif, pbuf *packet, hwaddr_t destAddr);

// Function prototype for netif->linkoutput functions. Only used for ethernet
// netifs. This function is called by ARP when a packet shall be sent.
// @param netif The netif which shall send a packet
// @param up  non-zero if the link is up
typedef hterr (*NetIf_cbLinkStatus_ft)(struct _HtNetIf *netif, uint8_t active);

// -----------------------------
// struct HtNetIf
// -----------------------------
// Generic data structure used for all lwIP network interfaces.
// The following fields should be filled in by the initialization
// function for the device driver: hwaddr_len, hwaddr[], mtu, flags
typedef struct _HtNetIf
{
  // struct _HtNetIf* next; // the NetIf list

  // Callback called by the network device driver to pass a received packet to the upper layer
  NetIf_cbReceived_ft cbReceived;
  // This function is called by the upperlayer hen it wants to send a packet on the interface.
  // This function typically first resolves the hardware address, then sends the packet.
  NetIf_doTransmit_ft doTransmit;
 
  // Callback called by the network device driver when the link status is changed
  NetIf_cbLinkStatus_ft cbLinkStatus;
  
  void *pDriverCtx; // Context to the extention per device driver

  const char*  name; // the name of the interface, such as "eth0"
  const char*  hostname; // the hostname for this netif, NULL is a valid value

  
  uint16_t mtu; // maximum transfer unit (in bytes)
  uint8_t  hwaddr_len; // number of bytes used in hwaddr
  hwaddr_t hwaddr; // link level hardware address of this interface
  uint16_t flags; // flags (see NETIF_FLAG_XXXX above)
} HtNetIf;

void HtNetIf_setHwAddr(HtNetIf* netif, const hwaddr_t hwaddr, uint8_t hwaddr_len);
void HtNetIf_init(HtNetIf* netif, void* pDriverCtx, const char* name, const char*hostname,
				  uint16_t mtu, const hwaddr_t hwaddr, uint8_t hwaddr_len, uint16_t flags,
				  NetIf_cbReceived_ft cbReceived, NetIf_doTransmit_ft doTransmit, NetIf_cbLinkStatus_ft cbLinkStatus);

#define NetIf_doTransmit(pNETIF, PACKET, DEST) ((pNETIF && pNETIF->doTransmit) ? pNETIF->doTransmit(pNETIF, PACKET, (uint8_t*)DEST) :ERR_NOT_SUPPORTED)
#define NetIf_cbReceived(pNETIF, PACKET) ((pNETIF && pNETIF->cbReceived) ? pNETIF->cbReceived(pNETIF, PACKET) :ERR_NOT_SUPPORTED)

// the default OnReceived() lead to htddl_procRX()
hterr NetIf_OnReceived_Default(HtNetIf *netif, pbuf* packet, uint8_t powerLevel);

// ---------------------------------------------------------------------------
// HtDDL
// ---------------------------------------------------------------------------
void htddl_init(void);
void htddl_doTimerScan(void); // this is expected to be excuted every 1 msec
uint8_t htddl_locateBmpIdx(uint16_t offset, uint16_t blockSize, uint8_t* pBitNo);

uint16_t htddl_send(HtNetIf* netif, hwaddr_t dest, pbuf* data, uint8_t txSize);
void htddl_procRX(HtNetIf* netif, uint8_t* rxbuf, uint16_t rxlen, uint8_t powerLevel);

// ---------------------------------------------------------------------------
// portal entries
// ---------------------------------------------------------------------------
void htddl_OnReceived(HtNetIf* netif, const hwaddr_t src, pbuf* data);
// uint16_t htddl_doTX(HtNetIf* netif, const hwaddr_t dest, const uint8_t* data, uint16_t len);
// uint8_t  htddl_getHwAddr(HtNetIf* netif, uint8_t** pBindaddr);

#endif // __HT_DDL_H__
