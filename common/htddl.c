#include "htddl.h"
#ifdef HT_CLUSTER
#  include "htcluster.h"
#endif // HT_CLUSTER

#ifndef SLOT_LOCK_TX
#  define SLOT_LOCK_TX()
#  define SLOT_UNLOCK_TX()
#endif // SLOT_LOCK_TX

#ifndef SLOT_LOCK_RX
#  define SLOT_LOCK_RX()
#  define SLOT_UNLOCK_RX()
#endif // SLOT_LOCK_RX

#define HTDDL_BMP_BYTES_MAX      (8)  // because the top two bits of transId are reserved for commands 

void HtNetIf_setHwAddr(HtNetIf* netif, const hwaddr_t hwaddr, uint8_t hwaddr_len)
{
	if (NULL == netif || NULL == hwaddr || hwaddr_len<=0)
		return;
	netif->hwaddr_len = min(HTDDL_HWADDR_LEN_MAX, hwaddr_len);
	memcpy(netif->hwaddr, hwaddr, netif->hwaddr_len);
}
				  
void HtNetIf_init(HtNetIf* netif, void* pDriverCtx, const char* name, const char*hostname,
				  uint16_t mtu, const hwaddr_t hwaddr, uint8_t hwaddr_len, uint16_t flags,
				  NetIf_cbReceived_ft cbReceived, NetIf_doTransmit_ft doTransmit, NetIf_cbLinkStatus_ft cbLinkStatus)
{
	if (NULL == netif)
		return;
	netif->name = name;
	netif->hostname = hostname;
	netif->pDriverCtx = pDriverCtx;
	netif->cbReceived = cbReceived;
	netif->doTransmit = doTransmit;
	netif->cbLinkStatus = cbLinkStatus;

	netif->flags = flags;
	netif->mtu = mtu;
	HtNetIf_setHwAddr(netif, hwaddr, hwaddr_len);
}

hterr NetIf_OnReceived_Default(HtNetIf *netif, pbuf* packet, uint8_t powerLevel)
{
	htddl_procRX(netif, packet->payload, packet->len, 0x0); 
	return ERR_SUCCESS;
}


typedef struct _htddl_slotRecv
{
	uint8_t  connId;
	hwaddr_t srcAddr;
	uint8_t  txSize;  // txSize is determined by the sender
//	uint16_t recvSize; // recvSize is determined by the receiver
	uint16_t timeout;
	uint16_t crc16;
	uint8_t  pendingBmp[HTDDL_BMP_BYTES_MAX];
	uint8_t  lastBmpIdx;

	// uint8_t  recvBuf[HTDDL_RECVBUF_LEN_MAX];
	pbuf*    rxdata; // recvBuf;
} htddl_slotRX;

static htddl_slotRX htddl_slots_RX[HTDDL_SLOTS_SIZE_RX];

typedef struct _htddl_slotTX
{
	uint8_t  connId;
	hwaddr_t dest;
	uint8_t  txSize;  // txSize is determined by the sender
	uint16_t recvSize; // recvSize is determined by the receiver
	uint16_t timeout;
	uint16_t crc16;
	uint8_t  pendingBmp[HTDDL_BMP_BYTES_MAX];
	uint8_t  lastBmpIdx;

	uint8_t  state;
} htddl_slotTX;

static htddl_slotTX htddl_slots_TX[HTDDL_SLOTS_SIZE_TX];

enum {
	HTTXD_STATE_CON, HTTXD_STATE_BMP, HTTXD_STATE_CRC_ERR
};

#define REQCON 	    HTDDL_REQ (HTCMD_CON)
#define RESPCON     HTDDL_RESP(HTCMD_CON)
#define REQBMP 	    HTDDL_REQ (HTCMD_BMP)
#define RESPBMP     HTDDL_RESP(HTCMD_BMP)
#define REQDISCON   HTDDL_REQ (HTCMD_DISCON)

void htddl_init(void)
{
	uint8_t i;

	SLOT_LOCK_RX();
	memset(htddl_slots_RX, 0x00, sizeof(htddl_slots_RX));
	for (i = 0; i < HTDDL_SLOTS_SIZE_RX; i++)
		htddl_slots_RX[i].connId = HTDDL_ID_INVALID;
	SLOT_UNLOCK_RX();

	SLOT_LOCK_TX();
	memset(htddl_slots_TX, 0x00, sizeof(htddl_slots_TX));
	for (i = 0; i < HTDDL_SLOTS_SIZE_TX; i++)
		htddl_slots_TX[i].connId = HTDDL_ID_INVALID;
	SLOT_UNLOCK_TX();
}

uint8_t htddl_locateBmpIdx(uint16_t offset, uint16_t blockSize, uint8_t* pBitNo)
{
	uint8_t idxBlk = offset / blockSize;
	if (NULL != pBitNo)
		*pBitNo = idxBlk % 8;
	return (idxBlk / 8);
}

static uint8_t htddl_allocSlot_RX(uint16_t seglen, uint8_t transSize)
{
	uint8_t idxRXD = 0, i, j;
	htddl_slotRX *pRXD = htddl_slots_RX;
	uint16_t recvSize=0;

	if (transSize <= 2 || seglen <= 0)
		return HTDDL_ID_INVALID;

	SLOT_LOCK_RX();

	// scan the segments to find out an idle one
	for (idxRXD = 0; idxRXD < HTDDL_SLOTS_SIZE_RX; idxRXD++, pRXD++)
	{
		if (pRXD->timeout <= 0)
		{
			pRXD->timeout = SLOT_TIMEOUT_CON;
			pRXD->txSize = transSize;
			break;
		}
	}

	if (idxRXD >= HTDDL_SLOTS_SIZE_RX) // no idle segment available
	{
		SLOT_UNLOCK_RX();
		return HTDDL_ID_INVALID;
	}

	// validate the approciated the segment length
	recvSize = (pRXD->txSize-2) * HTDDL_BMP_BYTES_MAX * 8;

	if (seglen < recvSize)
		recvSize = seglen;

	if (NULL == (pRXD->rxdata = pbuf_malloc(recvSize)))
	{
		SLOT_UNLOCK_RX();
		return HTDDL_ID_INVALID;
	}

	// prepare the pending BMP
	pRXD->lastBmpIdx = htddl_locateBmpIdx(recvSize -1, pRXD->txSize -2, &j);
	for (j++; j; j--)
	{
		pRXD->pendingBmp[pRXD->lastBmpIdx] <<= 1;
		pRXD->pendingBmp[pRXD->lastBmpIdx]++;
	}

	for (i = pRXD->lastBmpIdx; i > 0; i--)
		pRXD->pendingBmp[i-1] = 0xff;

	for (i = pRXD->lastBmpIdx + 1; i < HTDDL_BMP_BYTES_MAX; i++)
		pRXD->pendingBmp[i] = 0;

	SLOT_UNLOCK_RX();
	return idxRXD;
}

static void htddl_freeSlot_RX(uint8_t idxRXD)
{
	SLOT_LOCK_RX();
	
	if (idxRXD < HTDDL_SLOTS_SIZE_RX)
	{
		htddl_slots_RX[idxRXD].txSize = 0;
		htddl_slots_RX[idxRXD].connId = HTDDL_ID_INVALID;
		pbuf_free(htddl_slots_RX[idxRXD].rxdata);
	}

	SLOT_UNLOCK_RX();
}

static uint8_t htddl_allocSlot_TX(uint16_t seglen, uint8_t transSize)
{
	uint8_t idxTXD = 0;
	htddl_slotTX* pTXD = htddl_slots_TX;

	if (transSize <= 0 || seglen <= 0)
		return HTDDL_ID_INVALID;

	SLOT_LOCK_TX();

	// step 1. scan the TXDs to find out an idle one
	for (idxTXD = 0; idxTXD < HTDDL_SLOTS_SIZE_TX; idxTXD++, pTXD++)
	{
		if (pTXD->timeout <= 0)
		{
			pTXD->timeout = SLOT_TIMEOUT_CON;
			pTXD->txSize = transSize;
			break;
		}
	}

	SLOT_UNLOCK_TX();

	if (idxTXD >= HTDDL_SLOTS_SIZE_TX) // no idle segement available
		return HTDDL_ID_INVALID;

	// step 2. initialize some values
	pTXD->crc16 = 0x0000;
	pTXD->recvSize = 0;
	return idxTXD;
}

static void htddl_freeSlot_TX(uint8_t idxTXD)
{
	if (idxTXD >= HTDDL_SLOTS_SIZE_TX) // no idle segement available
		return;

	SLOT_LOCK_TX();
	htddl_slots_TX[idxTXD].txSize = 0;
	htddl_slots_TX[idxTXD].recvSize = 0;
	htddl_slots_TX[idxTXD].timeout = 0;
	htddl_slots_TX[idxTXD].state = 0;
	SLOT_UNLOCK_TX();
}
	
void htddl_doTimerScan(void)
{
	static uint32_t lastTicks = 0;
	uint32_t i, ticks = getTickXmsec();
	if (ticks > lastTicks)
		i = ticks - lastTicks;
	else
		i = ~((uint32_t)1) + lastTicks - ticks;

	lastTicks = ticks;
	ticks = i;

	if (ticks == 0) // no changes between the two calls
		return;

	SLOT_LOCK_RX();
	for (i = 0; i < HTDDL_SLOTS_SIZE_RX; i++)
	{
		if (0 == htddl_slots_RX[i].timeout)
			continue;

		if (htddl_slots_RX[i].timeout > ticks)
		{
			htddl_slots_RX[i].timeout -= ticks;
			continue;
		}

		htddl_freeSlot_RX(i);
	}
	SLOT_UNLOCK_RX();

	SLOT_LOCK_TX();
	for (i = 0; i < HTDDL_SLOTS_SIZE_TX; i++)
	{
		if (0 == htddl_slots_TX[i].timeout)
			continue;
		else if (htddl_slots_TX[i].timeout > ticks)
		{
			htddl_slots_TX[i].timeout -= ticks;
			continue;
		}

		htddl_freeSlot_TX(i);
	}
	SLOT_UNLOCK_TX();

}

uint16_t htddl_send(HtNetIf* netif, hwaddr_t dest, pbuf* data, uint8_t txSize)
{
	uint8_t txbuf[HTDDL_MSG_LEN_MAX +1];
	uint8_t idxTXD = HTDDL_ID_INVALID, i, j, *p, connId = 0, nSend = 0, nSendLast, err = 0;
	uint16_t tmp16;
	htddl_slotTX* pTXD = NULL;
	pbuf* packet = NULL;
	// uint8_t* pNicAddr;
	// uint8_t lenNicAddr;

	pbuf_size_t offset =0;

	if (NULL == data || data->tot_len<=0 || NULL == netif)
		return 0;

	if (txSize > HTDDL_MSG_LEN_MAX) txSize = HTDDL_MSG_LEN_MAX; // TODO: should to call txbuf = malloc(txSize)
	
	// step 1. allocate a tx descriptor
	idxTXD = htddl_allocSlot_TX(data->tot_len, txSize);
	if (HTDDL_ID_INVALID == idxTXD)
		return 0;

	if (NULL != dest)
		memcpy(htddl_slots_TX[idxTXD].dest, dest, sizeof(hwaddr_t));
	else memset(htddl_slots_TX[idxTXD].dest, 0x00, sizeof(hwaddr_t));
	
	dest = htddl_slots_TX[idxTXD].dest;

	pTXD = &htddl_slots_TX[idxTXD];
	// lenNicAddr = htddl_getHwAddr(netif, &pNicAddr) & 0x0f;

	// step 2. send command  REQCON to establish the connection
	//  0          1           2           3          4          5          6          7          8          9          10              
	// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// |  REQCON  |TXID | RXID| TKL |S  req-seglen M | transLen | token(src-hwaddr plus optional code)
	// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	connId = (idxTXD & 0x0f) | 0xf0;
	
	p = txbuf;
	*p++ = REQCON;
	*p++ = connId;
	*p++ = ((data->tot_len & 0x0f) << 4) | netif->hwaddr_len; // token len = len-of-hwaddr here
	*p++ = (data->tot_len >> 4) & 0xff;
	*p++ = txSize;
	memcpy(p, netif->hwaddr, netif->hwaddr_len);  p += netif->hwaddr_len;// fill in this source hw addr as token
	for (; p < (txbuf + txSize); p++)
		*p = 0x00;

	// step 2.1 send and wait for the connection gets established
	// htddl_doTX(netif, dest, txbuf, txSize);
	packet = pbuf_mmap(txbuf, txSize);
	NetIf_doTransmit(netif, packet, dest);

	for (i = 0; i < 100; i++)
	{
		sleep(2 + (1<<min(2, i)));
		if (FLAG(HTTXD_STATE_CON) & pTXD->state)
			break;

		tmp16= NetIf_doTransmit(netif, packet, dest);

		if (ERR_SUCCESS != tmp16 && ERR_IN_PROGRESS != tmp16) // if (htddl_doTX(netif, dest, txbuf, txSize) <= 0)
			break;
	}
	pbuf_free(packet); packet=NULL;

	if (0 == (FLAG(HTTXD_STATE_CON) & pTXD->state))
	{
		// failed to establish the connection, quit
		htddl_freeSlot_TX(idxTXD);
		return 0;
	}

	SLOT_LOCK_TX();
	connId = pTXD->connId; // the established connection id
	pTXD->state &= ~FLAG(HTTXD_STATE_BMP);
	SLOT_UNLOCK_TX();

	nSend = pTXD->recvSize * 2 / (pTXD->txSize -2) + 1; // the quota for total sends

	// step 3 sending data
	do {
		// step 3.1 sending a round of data
		offset =0; // p = data; 
		nSendLast = nSend;
		
		for (i = 0; nSend>0 && i <= pTXD->lastBmpIdx; i++)
		{
			for (j = 0; j < 8; j++, offset+=pTXD->txSize -2) //, p += (pTXD->txSize -2))
			{
				if (0 == (FLAG(j) & pTXD->pendingBmp[i]))
					continue;

				// step 3.1.1 transmiss a chunk of data
				//  0          1          2          3          4          5          6          7          8          9          10              
				// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
				// |  TransId |TXID | RXID|  data bytes ...
				// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
				txbuf[0] = (i * 8 + j) & 0x3f;
				txbuf[1] = connId;

				// determine the size good to send in this chunk
				tmp16 = pTXD->recvSize - offset;
				if (tmp16 > (pTXD->txSize - 2))
					tmp16 = (pTXD->txSize - 2);
				tmp16 = pbuf_read(data, offset, txbuf +2, tmp16); // memcpy(txbuf + 2, p, tmp16);
				
				// htddl_doTX(netif, dest, txbuf, pTXD->txSize); // send
				packet = pbuf_mmap(txbuf, pTXD->txSize);
				NetIf_doTransmit(netif, packet, dest); 
				pbuf_free(packet); packet=NULL;
				
				if (0 == nSend--)
					break;
			}
		}

		if (nSendLast == nSend) // nothing had been sent, already done confirmed by the response BMP
			break;
		
		sleep(1);

		// step 3.2 read back the BMP of receiver
		//  0          1          2          3          4          5          6          7          8          9          10              
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		// |  REQBMP  |TXID | RXID|           |
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		txbuf[0] = REQBMP;
		txbuf[1] = connId;

		// send request and wait
		// htddl_doTX(netif, dest, txbuf, pTXD->txSize);
		packet = pbuf_mmap(txbuf, pTXD->txSize);
		NetIf_doTransmit(netif, packet, dest);
		for (i = 0; i < 5; i++)
		{
			sleep(1 + (1 << min(2, i)));
			if (FLAG(HTTXD_STATE_BMP) & pTXD->state)
				break;
			NetIf_doTransmit(netif, packet, dest);
		}
		pbuf_free(packet); packet=NULL;

		SLOT_LOCK_TX();
		// reset FLAG(HTTXD_STATE_BMP)
		pTXD->state &= ~FLAG(HTTXD_STATE_BMP);
		SLOT_UNLOCK_TX();

		sleep(5); // wait for a bit for the receiver to back to RX mode
	} while (nSend > 0 && nSendLast > nSend);

	for (i = 0; i <= pTXD->lastBmpIdx; i++)
	{
		if (0 != pTXD->pendingBmp[i])
		{
			err = 1;
			break;
		}
	}

	// step 4. TODO: double check with the CRC

	// step 5. commit and release the connection
	// step 5.1 send REQDISCON
	//  0          1          2          3          4          5          6          7          8          9          10              
	// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// |REQDISCON |TXID | RXID| 0-commit  |
	// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	txbuf[0] = REQDISCON;
	txbuf[1] = connId;
	txbuf[2] = err;
	
	// htddl_doTX(netif, dest, txbuf, pTXD->txSize); sleep(1); htddl_doTX(netif, dest, txbuf, pTXD->txSize); // send twice
	packet = pbuf_mmap(txbuf, pTXD->txSize);
	NetIf_doTransmit(netif, packet, dest);
	sleep(1);
	NetIf_doTransmit(netif, packet, dest);
	pbuf_free(packet); packet=NULL;

	// step 5.2 free the TXD descriptor
	offset = err ? 0 : pTXD->recvSize; // len = err ? 0 : pTXD->recvSize;
	htddl_freeSlot_TX(idxTXD);

	return offset;
}

static void htddl_procRequest(HtNetIf* netif, uint8_t* req)
{
	uint8_t idxRXD;
	htddl_slotRX *pRXD = NULL;
	uint8_t txbuf[HTDDL_MSG_LEN_MAX+1];
	uint8_t i, j, k, l, txSize = HTDDL_MSG_LEN_MAX;
	uint16_t seglen;
//	hwaddr_t srcAddr;
	pbuf* packet=NULL;

	switch (req[0])
	{
	case REQCON:
		//  0          1          2          3          4          5          6          7          8          9          10              
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		// |  REQCON  |TXID | RXID| TKL S   req-seglen M |  txSize  | token(src-hwaddr plus optional code)
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		seglen = req[3]; seglen <<= 4; seglen |= req[2] >> 4; // read the requested segment length
		if (txSize > req[4])
			txSize = req[4];
		idxRXD = htddl_allocSlot_RX(seglen, txSize); // allocate a recv segment

		if (HTDDL_ID_INVALID == idxRXD)
			return;

		// compose the response
		//  0          1          2          3          4          5          6          7          8          9          10              
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		// | RESPCON  |TXID | RXID| TKL S   req-seglen M |  txSize  | token(src-hwaddr plus optional code)
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		i = 0;
		txbuf[i++] = RESPCON;
		txbuf[i++] = (req[1] & 0x0f) | (idxRXD << 4); // the connection id
		j = req[2] & 0x0f;
		// the approciated segment length
		txbuf[i++] = ((htddl_slots_RX[idxRXD].rxdata->tot_len & 0x0f) << 4) | j;
		txbuf[i++] = (htddl_slots_RX[idxRXD].rxdata->tot_len >> 4) & 0xff;
		txbuf[i++] = txSize; // the txSize
		// the token
		for (; j >0; j--)
		{
			txbuf[5+j -1] = req[5+j -1];
			if (j < HTDDL_HWADDR_LEN_MAX) // save the src hwaddr
				htddl_slots_RX[idxRXD].srcAddr[j] = req[5+j];
		}

		// send the response back
		// htddl_doTX(netif, htddl_slots_RX[idxRXD].srcAddr, txbuf, txSize); 
		packet = pbuf_mmap(txbuf, txSize);
		NetIf_doTransmit(netif, packet, htddl_slots_RX[idxRXD].srcAddr);
		pbuf_free(packet); packet=NULL;

		break;

	case REQBMP:
		//  0          1          2          3          4          5          6          7          8          9          10              
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		// |  REQBMP  |TXID | RXID| OFFS| LEN |
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		idxRXD = req[1] >> 4; // the idxRXD
		if (idxRXD >= HTDDL_SLOTS_SIZE_RX)
			return;

		pRXD = &htddl_slots_RX[idxRXD];
		if (pRXD->connId != req[1])
			return;

		// compose the response
		//  0          1          2          3          4          5          6          7          8          9          10              
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		// | RESPBMP  |TXID | RXID| OFFS| LEN |  bmp bytes
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		i = 0;
		txbuf[i++] = RESPBMP;
		txbuf[i++] = pRXD->connId; // the connection id
		for (i = 0; i < 2; i++) // two rounds
		{
			seglen = 0; // temporary used to count non-zero bmp bytes
			for (j = 0, k = 0; j <= pRXD->lastBmpIdx; j += k)
			{
				k = min(pRXD->txSize - 3, pRXD->lastBmpIdx + 1 - j);
				if (k <= 0)
					break;

				txbuf[2] = j | (k << 4);

				for (l=0; l<k; l++) // copy the BMP bytes
				{
					if (0 != (txbuf[3 + l] = pRXD->pendingBmp[j + l]))
						seglen++;
				}

				// htddl_doTX(netif, htddl_slots_RX[idxRXD].srcAddr, txbuf, HTDDL_MSG_LEN_MAX);
				packet = pbuf_mmap(txbuf, HTDDL_MSG_LEN_MAX);
				NetIf_doTransmit(netif, packet, htddl_slots_RX[idxRXD].srcAddr);
				pbuf_free(packet); packet=NULL;
			}

			sleep(2);
		}

		if (0 == seglen) // no pending BMP now, follow up with a CRC
		{
			//  0          1          2          3          4          5          6          7          8          9          10              
			// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
			// | RESPBMP  |TXID | RXID|   0xff    |        CRC16        |
			// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
			//TODO:
			// seglen = crc16(pRXD->recvBuf, pRXD->recvSize);
			// memcpy(&txbuf[3], &seglen, 2);
			
			// htddl_doTX(txbuf); htddl_doTX(txbuf); // send the response twice
		}

		return;

	case REQDISCON:
		idxRXD = req[1] >> 4; // the idxRXD
		if (idxRXD >= HTDDL_SLOTS_SIZE_RX)
			return;

		pRXD = &htddl_slots_RX[idxRXD];
		if (pRXD->connId != req[1])
			return;

		htddl_OnReceived(netif, pRXD->srcAddr, pRXD->rxdata);
		// no reponse for this request

		// free the allocation
		htddl_freeSlot_RX(idxRXD);

		return;

	default:
		break;
	}

}

static void htddl_procResponse(HtNetIf* netif, uint8_t* resp)
{
	uint8_t idxTXD = resp[1] & 0x0f; // the idxTXD
	uint8_t i, j;
	htddl_slotTX *pTXD = NULL;
	if (idxTXD < HTDDL_SLOTS_SIZE_TX)
		pTXD = &htddl_slots_TX[idxTXD];

	switch (resp[0])
	{
	case RESPCON:
		//  0          1          2          3          4          5          6          7          8          9          10              
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		// | RESPCON  |TXID | RXID| TKL S   req-seglen M | transLen | token(optional)
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		if (NULL == pTXD || 0xf0 == (resp[1] & 0xf0))
			return;

		SLOT_LOCK_TX();
		if (FLAG(HTTXD_STATE_CON) & pTXD->state) // this segment has already been taken
		{
			SLOT_UNLOCK_TX();
			return;
		}

		pTXD->recvSize = resp[3]; pTXD->recvSize <<= 4; pTXD->recvSize |= resp[2] >> 4; // read the approciated segment length

		pTXD->connId = resp[1];
		pTXD->state |= FLAG(HTTXD_STATE_CON);
		pTXD->timeout = SLOT_TIMEOUT;

		// prepare the pending BMP
		pTXD->lastBmpIdx = htddl_locateBmpIdx(pTXD->recvSize -1, pTXD->txSize -2, &j);
		for (j++; j; j--)
		{
			pTXD->pendingBmp[pTXD->lastBmpIdx] <<= 1;
			pTXD->pendingBmp[pTXD->lastBmpIdx]++;
		}

		for (i = pTXD->lastBmpIdx; i > 0; i--)
			pTXD->pendingBmp[i - 1] = 0xff;

		for (i = pTXD->lastBmpIdx + 1; i < HTDDL_BMP_BYTES_MAX; i++)
			pTXD->pendingBmp[i] = 0;

		SLOT_UNLOCK_TX();
		break;

	case RESPBMP:
		//  0          1          2          3          4          5          6          7          8          9          10              
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		// | RESPBMP  |TXID | RXID| OFFS| LEN |  bmp bytes
		// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
		if (NULL == pTXD || pTXD->connId != resp[1])
			return;

		if (0xff == resp[2]) // the incoming is a CRC16
		{
			SLOT_LOCK_TX();
			if (memcmp(&pTXD->crc16, resp + 3, sizeof(uint16_t)))
				pTXD->state |= FLAG(HTTXD_STATE_CRC_ERR);
			SLOT_UNLOCK_TX();
			return;
		}

		i = resp[2] & 0x0f; // bmp byte offset
		j = resp[2] >> 4; // bmp bytes

		if ((i + j) > HTDDL_BMP_BYTES_MAX || j > (pTXD->txSize - 3)) // illegal input
			return;

		SLOT_LOCK_TX();
		memcpy(&pTXD->pendingBmp[i], &resp[3], j);
		pTXD->state |= FLAG(HTTXD_STATE_BMP);
		SLOT_UNLOCK_TX();

		return;

	default:
		break;
	}

}

static void htddl_procData(HtNetIf* netif, uint8_t* msg)
{
	uint8_t idxRXD = msg[1] >> 4; // the idxRXD
	uint32_t offset, len;
	uint8_t idxBmp, j;
	htddl_slotRX *pRXD = NULL;
	if (idxRXD >= HTDDL_SLOTS_SIZE_RX)
		return;

	pRXD = &htddl_slots_RX[idxRXD];
	if (pRXD->connId != msg[1])
		return;

	//  0          1          2          3          4          5          6          7          8          9          10              
	// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// |  TransId |TXID | RXID|  data bytes ...
	// +----------+-----+-----+-----+-----+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	offset = msg[0] * (pRXD->txSize -2);
	len = pRXD->rxdata->tot_len - offset;
	if ((int)len > (pRXD->txSize -2))
		len = pRXD->txSize -2;

	pbuf_write(pRXD->rxdata, offset, msg+2, len); // memcpy(&pRXD->recvBuf[offset], &msg[2], len);

	// reset this flag
	idxBmp = htddl_locateBmpIdx(offset, pRXD->txSize -2, &j);
	j = FLAG(j);
	pRXD->pendingBmp[idxBmp] &= ~j;
}
	
// entry
void htddl_procRX(HtNetIf* netif, uint8_t* rxbuf, uint16_t rxlen, uint8_t powerLevel)
{
	if (NULL == rxbuf)
		return;

	if (IS_REQ(rxbuf[0]))
	{
#ifdef HT_CLUSTER
		if ((HTCLU_CMD_MASK & rxbuf[0]) == (HTCLU_CMD_MASK & HTCLU_GETOBJ))
		{
			HtNode_procMsg(netif, rxbuf, rxlen, powerLevel);
			return;
		}
#endif // HT_CLUSTER
		htddl_procRequest(netif, rxbuf);
		return;
	}
	
	if (IS_RESP(rxbuf[0]))
	{
		htddl_procResponse(netif, rxbuf);
		return;
	}

	htddl_procData(netif, rxbuf);
}


// =====================================
//  test program
// =====================================
// #define HTDDL_LOOPBACK_TEST

#ifdef HTDDL_LOOPBACK_TEST
void htddl_OnReceived(void* pIF, uint8_t* data, uint16_t len) {}
uint16_t htddl_doTX(void*pIF, uint8_t* data, uint16_t len)
{
	htddl_procRX(pIF, data, len, 0x0f); 
	return len; 
}

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
	uint16_t n = htddl_send(NULL, msg, 522, HTDDL_MSG_LEN_MAX);

	return 0;
}

#endif // HTDDL_LOOPBACK_TEST
