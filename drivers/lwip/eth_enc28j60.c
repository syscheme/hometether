/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

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
#include "err.h"
#include "eth_enc28j60.h"
#include "../hw/enc28j60.h"

#include <includes.h>

/* Define those to better describe your network interface. */
#define IFNAME0 's'
#define IFNAME1 't'

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */

struct ethernetif
{
	struct eth_addr *ethaddr;

	/* Add whatever per-interface state that is needed here. */
	ENC28J60* pDev;
};

extern ENC28J60 theENC28J60;

static uint8_t eth_enc28j60_sendPacket(ENC28J60* nic, struct pbuf* p);
static struct pbuf * eth_enc28j60_recvPacket(ENC28J60* nic);


// Private variables 
static  OS_STK   App_Task_eth_enc28j60_input_Stk[APP_TASK_eth_enc28j60_input_STK_SIZE];

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif)
{
	CPU_INT08U  os_err;

	os_err = os_err; // prevent warning... 

	SYS_ARCH_DECL_PROTECT(sr);

	// set MAC hardware address length
	netif->hwaddr_len = 6;

	// set MAC hardware address
	netif->hwaddr[0] = emacETHADDR0;
	netif->hwaddr[1] = emacETHADDR1;
	netif->hwaddr[2] = emacETHADDR2;
	netif->hwaddr[3] = emacETHADDR3;
	netif->hwaddr[4] = emacETHADDR4;
	netif->hwaddr[5] = emacETHADDR5;

	// maximum transfer unit
	netif->mtu = 1500;

	// device capabilities
	// don't set NETIF_FLAG_ETHARP if this device is not an ethernet one
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	SYS_ARCH_PROTECT(sr);

	ENC28J60_init(&theENC28J60);

	SYS_ARCH_UNPROTECT(sr);

	os_err = OSTaskCreate((void(*)(void *)) eth_enc28j60_input,
		(void *)0,
		(OS_STK *)&App_Task_eth_enc28j60_input_Stk[APP_TASK_eth_enc28j60_input_STK_SIZE - 1],
		(INT8U)APP_TASK_eth_enc28j60_input_PRIO);

#if OS_TASK_NAME_EN > 0
	OSTaskNameSet(APP_TASK_BLINK_PRIO, "Task eth_enc28j60_input", &os_err);
#endif
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{

	SYS_ARCH_DECL_PROTECT(sr);

	// Interrupts are disabled through this whole thing to support multi-threading
	// transmit calls. Also this function might be called from an ISR.
	SYS_ARCH_PROTECT(sr);
	// Access to the EMAC is guarded using a semaphore.
	eth_enc28j60_sendPacket(&theENC28J60, p);

	SYS_ARCH_UNPROTECT(sr);

	return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf * low_level_input(struct netif *netif)
{
	struct pbuf *p = eth_enc28j60_recvPacket(&theENC28J60);
	return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
err_t eth_enc28j60_input(struct netif *netif)
{
	struct pbuf *p;
	if (NULL == netif)
		return -1;

	for (;;)
	{
		p = low_level_input(netif);
		if (NULL == p)
		{
			OSTimeDlyHMSM(0, 0, 0, 10);
			continue;
		}

		err_t err;
		// move received packet into a new pbuf
		err = netif->input(p, netif);
		if (err != ERR_OK)
		{
			LWIP_DEBUGF(NETIF_DEBUG, ("eth_enc28j60_input: IP input error\n\r"));
			pbuf_free(p);
			p = NULL;
		}
	}
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t eth_enc28j60_init(struct netif *netif)
{
	struct ethernetif *ethernetif;

	LWIP_ASSERT("netif != NULL", (netif != NULL));

	ethernetif = mem_malloc(sizeof(struct ethernetif));
	if (ethernetif == NULL)
	{
		LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
		return ERR_MEM;
	}

#if LWIP_NETIF_HOSTNAME
	// Initialize interface hostname/
	netif->hostname = "lwip";
#endif // LWIP_NETIF_HOSTNAME

	// Initialize the snmp variables and counters inside the struct netif.
	// The last argument should be replaced with your link speed, in units
	// of bits per second.
	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100000000);

	netif->state   = ethernetif;
	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;

	// We directly use etharp_output() here to save a function call.
	// You can instead declare your own function an call etharp_output()
	// from it if you have to do some checks before sending (e.g. if link
	// is available...)
	netif->output = etharp_output;
	netif->linkoutput = low_level_output;

	ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

	// initialize the hardware 
	low_level_init(netif);

	return ERR_OK;
}

struct pbuf * eth_enc28j60_recvPacket(ENC28J60* nic)
{
	INT8U err;
	struct pbuf* p;
	unsigned int len;
	unsigned short rxstat;
	unsigned int pk_counter;

	p = NULL;

	// lock enc28j60
	OSSemPend(enc28j60_sem_lock, 0, &err);
	
	//TODO: disable enc28j60 interrupt

	do {

		if (0 == ENC28J60_read(nic, EPKTCNT))
		{
			// switch to bank 0
			ENC28J60_setBank(nic, ECON1);
			// enable packet reception
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

			break;
		}

		//step 1. Set the read pointer to the start of the received packet
		ENC28J60_write(nic, ERDPTL, (nic->ptrNextPacket & 0xff));
		ENC28J60_write(nic, ERDPTH, (nic->ptrNextPacket >> 8));

		//step 2. save the next packet pointer
		nic->ptrNextPacket = ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0);
		nic->ptrNextPacket |= ShiftToWordH(ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0));

		//step 3. read the packet length (see datasheet page 43)
		len = ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0);
		len |= ShiftToWordH(ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0));
		len -= 4; //remove the CRC count

		// step 4. read the receive status (see datasheet page 43)
		rxstat = ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0);
		rxstat |= ShiftToWordH(ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0));

		// check CRC and symbol errors (see datasheet page 44, table 7-3):
		// The ERXFCON.CRCEN is set by default. Normally we should not
		// need to check this.
		if ((rxstat & 0x80) == 0)
		{
			len = 0; // invalid
		}
		else
		{
			// allocation pbuf
			p = pbuf_alloc(PBUF_LINK, len, PBUF_RAM);
			if (p != NULL)
			{
				unsigned char* data;
				struct pbuf* q;

				// copy the packet from the receive buffer
				for (q = p; q != NULL; q = q->next)
				{
					data = q->payload;
					len = q->len;
					ENC28J60_readBuf(nic, len, data); 
				}
			}
		}

		// Move the RX read pointer to the start of the next received packet
		// This frees the memory we just read out
		ENC28J60_write(nic, ERXRDPTL, nic->ptrNextPacket & 0xff);
		ENC28J60_write(nic, ERXRDPTH, nic->ptrNextPacket >> 8);

		// decrement the packet counter indicate we are done with this packet
		ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
	} while (0);

	/* unlock enc28j60 */
	OSSemPost(enc28j60_sem_lock);	  // 发送信号量

	return p;
}

// Transmit a packet.
uint8_t eth_enc28j60_sendPacket(ENC28J60* nic, struct pbuf* p)
{
	static uint8_t cFail = MAX_SEND_FAIL;

	INT8U err;
	struct pbuf* q;
	unsigned int len;
	unsigned char* ptr;

	// lock enc28j60
	OSSemPend(enc28j60_sem_lock, 0, &err);

	//TODO: disable enc28j60 interrupt

	do {
		ENC28J60_startPacketToSend(nic, p->tot_len);
		for (q = p; q != NULL; q = q->next)
		{
			len = q->len;
			ptr = q->payload;
			ENC28J60_fillPacketToSend(nic, q->len, q->payload);
		}

		if (ENC28J60_submitPacketToSend(nic))
		{
			cFail = MAX_SEND_FAIL;
			break;
		}

		if (--cFail)
			break;

		// continuous send fail, reset the NIC
		ENC28J60_init(nic);

	} while (0);

	// TODO: enable enc28j60 interrupt

	// unlock enc28j60
	OSSemPost(enc28j60_sem_lock); // 发送信号量

	return 0;
}

