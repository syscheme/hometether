#ifndef __ETH_ENC28J60_H__
#define __ETH_ENC28J60_H__

#include "err.h"
#include "netif.h"

err_t eth_enc28j60_init(struct netif *netif);
err_t eth_enc28j60_input(struct netif *netif);
struct netif *eth_enc28j60_register(void);
int eth_enc28j60_poll(void);
void eth_enc28j60_setmac(unsigned char* macaddr);

#endif // __ETH_ENC28J60_H__
