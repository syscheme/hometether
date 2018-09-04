#include "lwip_htddl.h"

eth_htddl_t eth_htddls[HTDDL_ETHS_MAX];

static err_t low_level_output(struct netif *netif, struct pbuf *p);

err_t lwip_htddl_init(struct netif *netif)
{
	eth_htddl_t *pEth = netif->state;
	return ERR_OK;
}

err_t eth_htddl_init(struct netif *netif, uint8_t idx, void* pDevice, htddl_lwip_init_f cbInit, htddl_lwip_tx_f cbTx, uint16_t szTx)
{
	eth_htddl_t *pEth;

	LWIP_ASSERT("netif != NULL", (netif != NULL));
	if (idx >= HTDDL_ETHS_MAX)
	{
		LWIP_DEBUGF(NETIF_DEBUG, ("eth_htddl_init: idx[%d] out of memory\n", idx));
		return ERR_MEM;
	}

	pEth = &eth_htddls[idx];
	netif->num = idx + 1;

#if LWIP_NETIF_HOSTNAME
	// Initialize interface hostname
	netif->hostname = "lwip";
# ifdef HTDDL_NETIF_HOSTNAME
	netif->hostname = HTDDL_NETIF_HOSTNAME;
# endif
#endif // LWIP_NETIF_HOSTNAME

	// Initialize the snmp variables and counters inside the struct netif.
	// The last argument should be replaced with your link speed, in units
	// of bits per second.
	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100000000);

	netif->state   = pEth;
	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;

	// We directly use etharp_output() here to save a function call.
	// You can instead declare your own function an call etharp_output()
	// from it if you have to do some checks before sending (e.g. if link
	// is available...)
	netif->output = etharp_output;
	netif->linkoutput = low_level_output;
#if LWIP_ARP
	netif->output = etharp_output;
#else
	netif->output = NULL; // not used for PPPoE
#endif // LWIP_ARP

	pEth->pNetIf = netif;
	pEth->pDev   = pDevice;
//	pEth->cbInit = cbInit;
	pEth->cbTx = cbTx;
	pEth->szTx = szTx;

	// initialize the hardware
	if (NULL != cbInit)
		cbInit(pEth);

	// copy MAC hardware address
	netif->hwaddr_len = 6;

	// maximum transfer unit
	netif->mtu = HTDDL_RECVBUF_LEN_MAX;

	// device capabilities
	// don't set NETIF_FLAG_ETHARP if this device is not an ethernet one
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	// TODO: the background thread
	//os_err = OSTaskCreate((void(*)(void *)) eth_enc28j60_input,
	//	(void *)0,
	//	(OS_STK *)&App_Task_eth_enc28j60_input_Stk[APP_TASK_eth_enc28j60_input_STK_SIZE - 1],
	//	(INT8U)APP_TASK_eth_enc28j60_input_PRIO);

	//#if OS_TASK_NAME_EN > 0
	//	OSTaskNameSet(APP_TASK_BLINK_PRIO, "Task eth_enc28j60_input", &os_err);
	//#endif

	return ERR_OK;
}

/*/// should be in htddl_lwip_nRF24L01.c
#include "../hw/nRF24L01.h"

// to call ddl like
// RF24L01* dev
// eth_htddl_init(netif, &dev, htddl_nRF24L01_init, htddl_nRF24L01_doTX, nRF24L01_MAX_PLOAD)

void htddl_nRF24L01_init(eth_htddl_t* pEth) // an implementation of htddl_lwip_init_f
{
	nRF24L01* pDev = (nRF24L01*)pEth->pDev;
	struct eth_addr* ethaddr = pEth->pNetIf->ethaddr;

	ethaddr->addr[0] = ethaddr->addr[1] = 0xbe;
	memcpy(ethaddr->addr, &pDev->nodeId, sizeof(pDev->nodeId));

	// call hw init
	nRF24L01_init(pDev, pDev->nodeId, ethaddr->addr, 1);
}

uint16_t htddl_nRF24L01_doTX(eth_htddl_t* pEth, uint8_t* data, uint16_t len) // an implementation of htddl_lwip_tx_f
{
	nRF24L01_TxPacket((nRF24L01*)pEth->pDev, data);
	return len;
}

//////////////*/

// portal of htddl to send a packet


// This function should do the actual transmission of the packet. The packet is
// contained in the pbuf that is passed to the function. This pbuf
// might be chained.
// @param netif the lwip network interface structure for this ethernetif
// @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
// @return ERR_OK if the packet could be sent
//         an err_t value if the packet couldn't be sent
// @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
//       strange results. You might consider waiting for space in the DMA queue
//       to become availale since the stack doesn't retry to send a packet
//       dropped because of memory failure (except for the TCP timers).
static char txbuf[HTDDL_RECVBUF_LEN_MAX];
#define OS_LOCK(_LK)
#define OS_UNLOCK(_LK)
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	struct pbuf* q;
	uint8_t* pBuf = txbuf;
	int len;
	eth_htddl_t* pEth = (eth_htddl_t*)netif->state;

	if (NULL == pEth || NULL == pEth->cbTx)
		return ERR_OK; // nothing to do

	OS_LOCK(lktxbuf);
	for (q = p, pBuf = txbuf; q != NULL; q = q->next)
	{
		len = txbuf + sizeof(txbuf)-pBuf;
		len = min(len, q->len);
		memcpy(pBuf, q->payload, len);
		pBuf += len;
	}

	len = pBuf - txbuf;
	len = htddl_send(pEth, txbuf, len, pEth->szTx);

	OS_UNLOCK(lktxbuf);

	return ERR_OK;
}

// impl of htddl portals
uint16_t htddl_doTX(void*pIF, uint8_t* data, uint16_t len)
{
	if (NULL == pIF || NULL == ((eth_htddl_t*)pIF)->cbTx)
		return len; // do nothing

	return ((eth_htddl_t*)pIF)->cbTx((eth_htddl_t*)pIF, data, len);
}

void htddl_OnReceived(void* pIF, uint8_t* data, uint16_t packet_len)
{
	struct pbuf *p;
//	int length = packet_len;
	struct eth_addr *dest = (struct eth_addr*) data;
	struct eth_addr *src = dest + 1;
	uint8_t unicast =1;
	const static uint8_t bcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	const static uint8_t ipv4mcast[] = { 0x01, 0x00, 0x5e };
	const static uint8_t ipv6mcast[] = { 0x33, 0x33 };

	// step 1. filter those non-of-business messages
#ifdef HTDDL_FILETER
	// Don't let feedback packets through
	if (!memcmp(src, ((eth_htddl_t*)pIF)->pNetIf->hwaddr, ETHARP_HWADDR_LEN))
	{
		// don't update counters here
		return;
	}

	// MAC filter: only let my MAC or non-unicast through (pcap receives loopback traffic, too)
	unicast = ((dest->addr[0] & 0x01) == 0);
	if (memcmp(dest, &((eth_htddl_t*)pIF)->pNetIf->hwaddr, ETHARP_HWADDR_LEN) &&
		(memcmp(dest, ipv4mcast, 3) || ((dest->addr[3] & 0x80) != 0)) &&
		memcmp(dest, ipv6mcast, 2) &&
		memcmp(dest, bcast, 6))
	{
		// don't update counters here!
		return;
	}
#endif // HTDDL_FILETER

#define HTDDL_RECVBUF_REF

	// step 2. allocate a pbuf chain of pbufs from the pool
	p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_REF);
	if (NULL == p)
		return;

	p->payload = (void*)data;
	p->len = p->tot_len = packet_len;

#if ETH_PAD_SIZE
	LWIP_ASSERT("packet_len >= ETH_PAD_SIZE", packet_len >= ETH_PAD_SIZE);

	p->payload = (void*)(data + ETH_PAD_SIZE);
	p->len = p->tot_len = (packet_len - ETH_PAD_SIZE);
#endif // ETH_PAD_SIZE

//	LINK_STATS_INC(link.recv);
	snmp_add_ifinoctets(netif, p->tot_len);
	if (unicast)
		snmp_inc_ifinucastpkts(netif);
	else
		snmp_inc_ifinnucastpkts(netif);

	if (NULL != ((eth_htddl_t*)pIF)->pNetIf->input && ERR_OK == ((eth_htddl_t*)pIF)->pNetIf->input(p, ((eth_htddl_t*)pIF)->pNetIf))
		return;

	// error occurs, free the lwif buf right the way
	LWIP_DEBUGF(NETIF_DEBUG, ("htddl_OnReceived(): IP input error\r\n"));
	pbuf_free(p);
	p = NULL;
}


// =====================================
//  test program
// =====================================
#define HTDDL_LWIP_LOOPBACK_TEST

#ifdef HTDDL_LWIP_LOOPBACK_TEST
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/api.h"

void htddl_lwip_loopback_init(eth_htddl_t* pEth) // an implementation of htddl_lwip_init_f
{
}

uint16_t htddl_lwip_loopback_doTX(eth_htddl_t* pEth, uint8_t* data, uint16_t len) // an implementation of htddl_lwip_tx_f
{
	htddl_procRX(pEth, data, len);
	return len;
}

struct netif netif; 
int main()
{
	char* msg = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0"
		;

//	lwip_init();
#define IPV4ADDR(B0,B1,B2,B3) (((uint32_t)B0) | (B1<<8) |(B2<<16) |(B3<<24))
	tcpip_init(NULL, NULL);

	ip_addr_t ipaddr, netmask, gw, peer;
	ipaddr.addr = IPV4ADDR(192,168,0,23);
	gw.addr = IPV4ADDR(192, 168, 0, 1);
	netmask.addr = IPV4ADDR(255, 255, 255, 0);
	peer.addr = IPV4ADDR(192, 168, 0, 24);
#if LWIP_DHCP
	ip_addr_set_zero(&gw);
	ip_addr_set_zero(&ipaddr);
	ip_addr_set_zero(&netmask);
#endif // LWIP_DHCP

	// initialize a HTDDL eth interface
	eth_htddl_init(&netif, 0, NULL, htddl_lwip_loopback_init, htddl_lwip_loopback_doTX, HTDDL_TX_SIZE_MAX);

	// initialize the lwip netif
	netif_add(&netif, &ipaddr, &netmask, &gw, netif.state, &lwip_htddl_init, &tcpip_input);
	netif_set_default(&netif);
	netif_set_up(&netif);

	// connection
	struct netconn *conn = netconn_new(NETCONN_UDP);
	struct netbuf * pbuf =  netbuf_new();
	// netbuf_alloc(pbuf, strlen(msg));
	netbuf_ref(pbuf, msg, 480);
	for (int i = 0; i < 100; i++)
		netconn_sendto(conn, pbuf, &peer, 200);

	sys_msleep(10000);

	netbuf_delete(pbuf);

	return 0;
}

#endif // HTDDL_LWIP_LOOPBACK_TEST

