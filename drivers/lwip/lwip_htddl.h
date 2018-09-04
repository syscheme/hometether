#ifndef __LWIP_HTDDL_H__
#define __LWIP_HTDDL_H__

// lwIP includes.
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include "netif/etharp.h"
#include "netif/ppp_oe.h"

#include "../../Common/htddl.h"

// Define those to better describe your network interface.
#define IFNAME0 'h'
#define IFNAME1 't'
#define HTDDL_ETHS_MAX (3)

// Helper struct to hold private data used to operate your ethernet interface.
// Keeping the ethernet address of the MAC in this struct is not necessary
// as it is already kept in the struct netif.
// But this is only an example, anyway...
struct _eth_htddl;
typedef void(*htddl_lwip_init_f) (struct _eth_htddl* pEth);
typedef uint16_t(*htddl_lwip_tx_f) (struct _eth_htddl* pEth, uint8_t* data, uint16_t len);

typedef struct _eth_htddl
{
	struct netif *pNetIf;
	void* pDev;

	htddl_lwip_tx_f   cbTx;
	uint16_t          szTx;
} eth_htddl_t;

#endif // __LWIP_HTDDL_H__
