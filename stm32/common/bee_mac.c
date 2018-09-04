#include "bee_mac.h"

#define INVAILD_OFFSET_LEN         (0xffff)
#define MAC_DATA_PAYLOAD           (WPAYLOAD -3)
#define BEE_BUF_BITMAP_MAX         (BEE_MAC_MSG_SIZE_MAX/MAC_DATA_PAYLOAD/8+1)

BeeIF      thisBeeIF;

void BeeRoute_reset();

/*
BeeNodeId_t thisBeeNodeId;
BeeAddress  thisBeeAddr; // =b{BEE_NEST_ADDR_B0,'S','W','1'}
BeeNodeId_t uplinkNodeId;
*/
const uint8_t NonIPAddrsB0[] =
{
	DOMAIN_ADDR_B0_BEE, DOMAIN_ADDR_B0_CAN, DOMAIN_ADDR_B0_USART, 0
};

/*
BeeRouteRow BeeRouteTable[BEE_ROUTE_TABLE_SIZE];

void Bee_addRoute(BeeAddress dest, BeeNodeId_t nextHop, uint8_t hops)
{
	uint16_t i = dest.b[0] + dest.b[1] + dest.b[2] + dest.b[3];
	BeeRouteRow* pRow = &BeeRouteTable[i], *firstIdle=NULL;
	i %= BEE_ROUTE_TABLE_SIZE;

	// scan the route table to address that of the given dest
	for (i=0; i< BEE_ROUTE_TABLE_SIZE; i++, pRow++)
	{
		if (NULL ==firstIdle && pRow->timeout <=0)
			firstIdle = pRow;
		
		if (0 == memcmp(&pRow->dest, &dest, sizeof(dest)))
			break;
	}

	// check if it is necessary to take the first idle row
	if (NULL != firstIdle && (i>=BEE_ROUTE_TABLE_SIZE || pRow->timeout <=0))
		pRow = firstIdle;

	memcpy(&pRow->dest, &dest, sizeof(dest));
	pRow->nextHop = nextHop;
	pRow->hops    = hops;
	pRow->timeout = BEE_ROUTE_TIMEOUT_RELOAD;
}

BeeNodeId_t Bee_findRoute(BeeAddress dest)
{
	uint16_t i = dest.b[0] + dest.b[1] + dest.b[2] + dest.b[3];
	BeeRouteRow* pRow = &BeeRouteTable[i];
	i %= BEE_ROUTE_TABLE_SIZE;

	// scan the route table to address that of the given dest
	for (i=0; i< BEE_ROUTE_TABLE_SIZE; i++, pRow++)
	{
		if (0 == memcmp(&pRow->dest, &dest, sizeof(dest)))
			return pRow->nextHop;
	}

	return BEE_NEST_BCAST_NODEID;
}

*/

// -------------------------
//  BeeMac Messages
// -------------------------

enum {
	BeeCtrl_ShortMessage = 0,  // "msg"
	BeeCtrl_OpenTransmit   =1,   // S->R, "OpenTranmit" command is to start a transmit tx
	BeeCtrl_WithdrawTransmit,
	BeeCtrl_Transmit,
	BeeCtrl_TransmitAck,
	BeeCtrl_TransmitNack
};

#define CTRL_CODE(CMD, TTL, DLEN) ((CMD << 12) | (TTL&0x3) <<10 | (DLEN & 0x3ff))
#define CTRL_CODE_CMD(CTRLCODE)   (CTRLCODE >> 12)
#define CTRL_CODE_TTL(CTRLCODE)   ((CTRLCODE >> 10) & 0x3)
#define CTRL_CODE_DLEN(CTRLCODE)  (CTRLCODE & 0x3ff)
#define MSG_CMD(_MSG)             CTRL_CODE_CMD(*((uint16_t*) (&_MSG[0])))
#define MSG_TTL(_MSG)             CTRL_CODE_TTL(*((uint16_t*) (&_MSG[0])))
#define MSG_DLEN(_MSG)            CTRL_CODE_DLEN(*((uint16_t*) (&_MSG[0])))

static uint8_t BeePacket_composeOpenTranmit(BeeIF* beeIf, uint8_t* outmsg, uint8_t ttl, BeeAddress srcAddr, BeeAddress destAddr, uint16_t datalen)
{
	//  0          1          2          3          4          5          6          7          8          9          10              
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// |      ctrl_code      |   from   |                  srcAddr                  |                 destAddr                  |
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// ctrl_code = MLB: 4b cmd + 2b ttl +10b datalen

	if (NULL ==outmsg)
		return 0;

	*((uint16_t*) (&outmsg[0])) = CTRL_CODE(BeeCtrl_OpenTransmit, ttl, datalen);
	outmsg[2] = beeIf->nodeId;
	memcpy(&outmsg[3], srcAddr.b,  4);
	memcpy(&outmsg[7], destAddr.b, 4);

	return 11;
}

static uint8_t BeePacket_composeWithdrawTransmit(BeeIF* beeIf, uint8_t* outmsg, uint8_t ttl, BeeAddress srcAddr, BeeAddress destAddr, uint16_t datalen)
{
	//  0          1          2          3          4          5          6          7          8          9          10              
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// |      ctrl_code      |   from   |                  srcAddr                  |                 destAddr                  |
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// ctrl_code = MLB: 4b cmd + 2b ttl +10b datalen

	if (NULL ==outmsg)
		return 0;

	*((uint16_t*) &outmsg[0]) = CTRL_CODE(BeeCtrl_WithdrawTransmit, ttl, datalen);
	outmsg[2] = beeIf->nodeId;
	memcpy(&outmsg[3], srcAddr.b,  4);
	memcpy(&outmsg[7], destAddr.b, 4);

	return 11;
}

static uint8_t BeePacket_composeTransmit(BeeIF* beeIf, uint8_t* outmsg, uint8_t ttl, uint8_t* data, uint16_t offset, uint8_t* datalen)
{
	//  0          1          2          3          4            WPAYLOAD
	// +----------+----------+----------+----------+---...----+----------+
	// |      ctrl_code      |   from   |             Data                |
	// +----------+----------+----------+----------+---...----+----------+
	// ctrl_code = MLB: 4b cmd + 2b ttl +10b datalen

	if (NULL ==outmsg || NULL == datalen || NULL == data)
		return 0;

	if (*datalen > MAC_DATA_PAYLOAD)
		*datalen = MAC_DATA_PAYLOAD;

	*((uint16_t*) &outmsg[0]) = CTRL_CODE(BeeCtrl_Transmit, ttl, offset);
	outmsg[2] = beeIf->nodeId;
	memcpy(&outmsg[3], data+offset, *datalen);

	return *datalen+3;
}

static uint8_t BeePacket_composeTransmitAck(BeeIF* beeIf, uint8_t* outmsg, BeeAddress srcAddr, BeeAddress destAddr, uint16_t datalen)
{
	//  0          1          2          3          4          5          6          7          8          9          10              
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// |      ctrl_code      |   from   |                  srcAddr                  |                 destAddr                  |
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// ctrl_code = MLB: 4b cmd + 2b ttl +10b datalen

	if (NULL ==outmsg)
		return 0;

	*((uint16_t*) (&outmsg[0])) = CTRL_CODE(BeeCtrl_TransmitAck, 1, datalen);
	outmsg[2] = beeIf->nodeId;
	memcpy(&outmsg[3], srcAddr.b,  4);
	memcpy(&outmsg[7], destAddr.b, 4);

	return 11;
}

static uint8_t BeePacket_composeTransmitNack(BeeIF* beeIf, uint8_t* outmsg, BeeAddress srcAddr, BeeAddress destAddr, uint16_t offset)
{
	//  0          1          2          3          4          5          6          7          8          9          10              
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// |      ctrl_code      |   from   |                  srcAddr                  |                 destAddr                  |
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// ctrl_code = MLB: 4b cmd + 2b ttl +10b datalen

	if (NULL ==outmsg)
		return 0;

	*((uint16_t*) (&outmsg[0])) = CTRL_CODE(BeeCtrl_TransmitNack, 1, 0);
	outmsg[2] = beeIf->nodeId;
	memcpy(&outmsg[3], srcAddr.b,  4);
	memcpy(&outmsg[7], destAddr.b, 4);

	return 11;
}

static uint8_t BeeMAC_sendShortMessage(BeeIF* beeIf, uint8_t* msgbuf, BeeAddress srcAddr, BeeAddress destAddr, uint8_t* data, uint16_t datalen)
{
	BeeNodeId_t nextHop = 0;
	// step 1. compose the packet
	//  0          1          2          3          4          5          6          7          8          9          10              
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// |      ctrl_code      |   from   |                  srcAddr                  |                 destAddr                  |
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// ctrl_code = MLB: 4b cmd + 2b ttl +10b datalen

	if (NULL ==msgbuf)
		return 0;

	*((uint16_t*) &msgbuf[0]) = CTRL_CODE(BeeCtrl_Transmit, BeeMAC_TTL_MAX, datalen);
	msgbuf[2] = beeIf->nodeId;
	memcpy(&msgbuf[3], srcAddr.b,  4);
	memcpy(&msgbuf[7], destAddr.b, 4);

	if (NULL == data)
		datalen=0;
	else
	{
		if (datalen > (WPAYLOAD-11))
			datalen = (WPAYLOAD-11);
		
		memcpy(&msgbuf[11], data, datalen);
	}

	datalen += 11; // take the datalen as the packet len
	
	// step 2. find the dest in the route table
	nextHop = BeeRoute_lookfor(beeIf, destAddr);
	return BeeMAC_doSend(beeIf, nextHop, msgbuf, (uint8_t)datalen);
}

// -------------------------
//  BeeMac Txn
// -------------------------

typedef struct _TransmitCtx
{
	BeeNodeId_t nextHop;
	uint8_t     bitmapNACKs[BEE_BUF_BITMAP_MAX]; // bitmap identifies the receivd NACKs
//	uint16_t NACKs[BEE_MAC_TRANSMIT_MAX_NACK];
	uint16_t    lenOfAck;
} TransmitCtx;

TransmitCtx transmitCtxs[BEE_MAC_TRANSMIT_CTX_COUNT];

static uint8_t calcBitmapIdx(uint16_t offset, uint8_t* idx, uint8_t* flag)
{
	*idx = (offset +MAC_DATA_PAYLOAD -1) /MAC_DATA_PAYLOAD; // packageIdx here

	*flag = 1 <<(*idx %8);
	*idx /= 8;
	return *idx;
}

static uint16_t calcBitmapOffset(uint16_t size, uint8_t idx, uint8_t flag)
{
	uint16_t offset = 1;
	if (0 == flag || (0 == idx && 1 == flag))
		return 0;

	for (offset=0; (offset <8) && (0 == (flag & 0x1)); offset++, flag >>=1);

	offset += 8*idx;
	if (size % MAC_DATA_PAYLOAD)
		offset = size % MAC_DATA_PAYLOAD + --offset * MAC_DATA_PAYLOAD;
	else offset *= MAC_DATA_PAYLOAD;

	return offset;
}

static uint8_t BeeMAC_transmitEx(BeeIF* beeIf, uint8_t* msgbuf, uint8_t ttl, BeeAddress srcAddr, BeeAddress destAddr, uint8_t* data, uint16_t datalen)
{
	int i;
	uint8_t packetSz =0;
	TransmitCtx* ctx = NULL;
	int16_t transmitQuota =0, offset, flag;
	uint8_t sendlen = MAC_DATA_PAYLOAD;

	if (NULL == msgbuf)
		return 0;

	if (datalen >BEE_MAC_MSG_SIZE_MAX)
		datalen = BEE_MAC_MSG_SIZE_MAX;

	// step 0. find an idle context
	for (i=0; i <BEE_MAC_TRANSMIT_CTX_COUNT; i++)
	{
		if (BEE_NEST_INVALID_NODEID == transmitCtxs[i].nextHop)
		{
			ctx = &transmitCtxs[i];
			break;
		}
	}

	if (NULL == ctx)
		return 0; // failed to find an available transmitCtxs

	memset(ctx->bitmapNACKs, 0x00, sizeof(ctx->bitmapNACKs));
	ctx->lenOfAck = INVAILD_OFFSET_LEN;
//	ctx->srcAddr  = srcAddr;
//	ctx->destAddr = destAddr;
//	ctx->data     = data;

	// step 1. find the nextHop to the destAddr in the route table
	ctx->nextHop = BeeRoute_lookfor(beeIf, destAddr);

	// step 2. compose and send the open packet
	packetSz = BeePacket_composeOpenTranmit(beeIf, msgbuf, ttl, srcAddr, destAddr, datalen);
	if (BeeMAC_doSend(beeIf, ctx->nextHop, msgbuf, packetSz)<=0)
		return 0;

	// 2.1 wait for the TransmitAck(datalen=0)
#ifdef _DEBUG
	ctx->lenOfAck =0;
#else
	// TODO: set to receive mode
	for (i=0; INVAILD_OFFSET_LEN == ctx->lenOfAck && i < 100; i++)
		Sleep(100);

#endif // _DEBUG

	if (i>=100 || 0 !=ctx->lenOfAck)
		return 0; // either no accept ACK or rejected

	// step 3. perform transmitting
	//  3.1 determin max transmit packets
	transmitQuota= (uint16_t) ((datalen / MAC_DATA_PAYLOAD +3) *1.3); // maximal 30% retransmit
	transmitQuota += (transmitQuota>>4) <<1; // added the doubled number of rounds of the batch-send, each batch 16 packets
	offset = datalen;
	
	while (transmitQuota-- >0 && ctx->lenOfAck <=0)
	{
		if (0 == offset && transmitQuota <3)
		{
			// the offset=0 packet is special if lost its NACK, just retransmit it for more times to avoid
			sendlen = MAC_DATA_PAYLOAD;
			packetSz = BeePacket_composeTransmit(beeIf, msgbuf, ttl, data, offset, &sendlen);
			BeeMAC_doSend(beeIf, ctx->nextHop, msgbuf, packetSz);
		}

		// the regular loop to send packet from the end one by one
		for (i=0; !ctx->lenOfAck && offset >0 && transmitQuota >0 && i <16; i++)
		{
			offset -= sendlen;
			if (offset < 0) // the last packet with offset =0
				offset =0;

			sendlen = MAC_DATA_PAYLOAD;
			packetSz = BeePacket_composeTransmit(beeIf, msgbuf, ttl, data, offset, &sendlen); // the in completed packet
			BeeMAC_doSend(beeIf, ctx->nextHop, msgbuf, packetSz);
			transmitQuota--;
		}

		// TODO: sleep at receive mode
		// TODO: mutex here

		// quit loop if TransmitAck has ever received
		if (ctx->lenOfAck)
			break;

		// scan if there is any NACK needs to re-transmit
		for (i=0; i < BEE_BUF_BITMAP_MAX; i++)
		{
			if (0 ==ctx->bitmapNACKs[i])
				continue;

			for (flag=1; flag <0x100; flag<<=1)
			{
				if (0 == (flag & ctx->bitmapNACKs[i]))
					continue;
				offset = calcBitmapOffset(datalen, i, flag & 0xff);
				if (offset >=datalen)
					break;

				// a valid NACK here, retransmit it
				sendlen = MAC_DATA_PAYLOAD;
				packetSz = BeePacket_composeTransmit(beeIf, msgbuf, ttl, data, offset, &sendlen);
				BeeMAC_doSend(beeIf, ctx->nextHop, msgbuf, packetSz);
				transmitQuota--;
				ctx->bitmapNACKs[i] &= ~flag;
			}

		}
/*
		for (i=0; !ctx->lenOfAck && transmitQuota >0 && i <BEE_MAC_TRANSMIT_MAX_NACK; i++)
		{
			if (ctx->NACKs[i] >=datalen || ctx->NACKs[i] < offset)
				ctx->NACKs[i] = INVAILD_OFFSET_LEN;

			if (INVAILD_OFFSET_LEN == ctx->NACKs[i])
				continue;

			// retransmit the NACK packet
			sendlen = MAC_DATA_PAYLOAD;
			packetSz = BeePacket_composeTransmit(beeIf, msgbuf, ttl, data, ctx->NACKs[i], &sendlen);
			BeeMAC_doSend(beeIf, ctx->nextHop, msgbuf, packetSz);
			transmitQuota--;
			ctx->NACKs[i] =INVAILD_OFFSET_LEN;
		}
*/
	}

/* no need to post close
	// step 4. close the transmit
	packetSz = BeePacket_composeCloseTranmit(msgbuf, ttl, ctx->srcAddr, ctx->destAddr, datalen); // the in completed packet
	BeeMAC_doSend(BEE_NEST_ADDR, ctx->thruNode, msgbuf, packetSz);
*/
	offset = ctx->lenOfAck;
	
	// free the ctx
	ctx->nextHop = BEE_NEST_INVALID_NODEID;

	if (offset == datalen)
		return 1;

	packetSz = BeePacket_composeWithdrawTransmit(beeIf, msgbuf, ttl, srcAddr, destAddr, datalen);
	BeeMAC_doSend(beeIf, ctx->nextHop, msgbuf, packetSz);
	return 0;
}

uint8_t BeeMAC_transmit(BeeIF* beeIf, uint8_t* msgbuf, BeeAddress srcAddr, BeeAddress destAddr, uint8_t* data, uint16_t datalen)
{
	return BeeMAC_transmitEx(beeIf, msgbuf, BeeMAC_TTL_MAX, srcAddr, destAddr, data, datalen);
}

typedef struct _RecvMsgBuf
{
	uint8_t    buf[BEE_MAC_MSG_SIZE_MAX];
	uint8_t    from; 
	BeeAddress srcAddr, destAddr;
	uint16_t   datalen; // the total bytes of the message. 0 means this buffer is idle
	uint8_t    bitmap[BEE_BUF_BITMAP_MAX]; // bitmap identifies the good data received (in MAC_DATA_PAYLOAD) 
	uint8_t    timeout; // the down-counting timeout, in 100msec. 0 means idle
	uint16_t   minOffset;
} RecvMsgBuf;

RecvMsgBuf  recvMB[BEE_MAC_FORWARD_BUF_COUNT];

#define OFFSET_TO_BITMAP_POS(_OFFSET, _IDXBMP, _FLAG)  { _IDXBMP = (_OFFSET +MAC_DATA_PAYLOAD -1) /MAC_DATA_PAYLOAD; _FLAG = 1 <<(_IDXBMP %8); _IDXBMP /= 8; }

void BeeMAC_procPacketReceived(BeeIF* beeIf, uint8_t* msgbuf, uint16_t msglen, uint8_t heardfrom) // heardfrom is the local bound interface thru which the message is received
{
	int k, c;
	uint8_t packetSz=0, nextHop, bit;
	int8_t  idxBmp;
	uint16_t datalen = MSG_DLEN(msgbuf);
	BeeAddress srcAddr, destAddr;
	uint8_t from = msgbuf[2];
	uint8_t tmp, tmp2;
	RecvMsgBuf* pFwdBuf = NULL;
	TransmitCtx* pTransmitCtx = NULL;

	// step 1. about the addresses, add it into routeTable if necessary
	// the addrs may not be valid for some commands
	memcpy(srcAddr.b,  &msgbuf[3], 4);
	memcpy(destAddr.b, &msgbuf[7], 4);

	switch(MSG_CMD(msgbuf))
	{
	case BeeCtrl_ShortMessage:
	case BeeCtrl_OpenTransmit:
	case BeeCtrl_WithdrawTransmit:
		if (BEE_NEST_BCAST_NODEID != heardfrom && BEE_NEST_BCAST_NODEID != from)
		{
			// calculate the hops from TTL
			k = BeeMAC_TTL_MAX - MSG_TTL(msgbuf);
			if (k<0)
				k =0;
			BeeRoute_add(srcAddr, from, k); 
		}
		break;

	default:
		break;
	}

	// step 2. dispatch per the command type
	switch(MSG_CMD(msgbuf))
	{
	case BeeCtrl_ShortMessage:
		tmp = MSG_TTL(msgbuf); // the ttl
		if (0 == memcmp(&beeIf->addr, &destAddr, 4))
			BeeMAC_OnShortMessage(beeIf, srcAddr, msgbuf+11, datalen-11);
		else if (tmp>0) // ttl >0
		{
			// forward this notice to the other
			nextHop = BeeRoute_lookfor(beeIf, destAddr);
			if (BEE_NEST_BCAST_NODEID == nextHop)
				nextHop = beeIf->uplink;
			
			// adjust the ttl and from
			*((uint16_t*) &msgbuf[0]) = CTRL_CODE(BeeCtrl_ShortMessage, --tmp, datalen);
			msgbuf[2] = beeIf->nodeId;
			BeeMAC_doSend(beeIf, nextHop, msgbuf, (uint8_t) msglen);
		}
		break;

	case BeeCtrl_OpenTransmit:

		// step.1 look for an idle FwdBuf to receive
		for (k=0; k<BEE_MAC_FORWARD_BUF_COUNT; k++)
		{
			if (0 == recvMB[k].datalen || 0 == recvMB[k].timeout)
			{
				pFwdBuf = &recvMB[k];
				break;
			}
		}

		if (NULL == pFwdBuf)
		{
			// failed to find an available buf, reject the Transmit via TransmitAck(dateLen=nonzero)
			packetSz = BeePacket_composeTransmitAck(beeIf, msgbuf, srcAddr, destAddr, 0xffff); // the in completed packet
			BeeMAC_doSend(beeIf, from, msgbuf, packetSz);
			return;
		}

		// step.2 accept this transmit via recvMB[i], initialize recvMB[i] fields
		memset(pFwdBuf->bitmap, 0, sizeof(pFwdBuf->bitmap));
		pFwdBuf->timeout   = BEE_MAC_FORWARD_BUF_TIMEOUT_RELOAD;
		pFwdBuf->datalen   = datalen;
		pFwdBuf->srcAddr   = srcAddr;
		pFwdBuf->destAddr  = destAddr;
		pFwdBuf->from      = from;
		pFwdBuf->minOffset = datalen;
//		calcBitmapIdx(datalen, &pFwdBuf->bitmapIdxMax, &pFwdBuf->bitmapLastFlag);

		// step.3 acknowledge TransmitAck(dateLen=0) to indicate transmit start
		packetSz = BeePacket_composeTransmitAck(beeIf, msgbuf, srcAddr, destAddr, 0); // the ack of start packet
		BeeMAC_doSend(beeIf, from, msgbuf, packetSz);

		return;

	case BeeCtrl_WithdrawTransmit:
		// step.1 look for the FwdBuf of the mentioned txnId
		for (k=0; k<BEE_MAC_FORWARD_BUF_COUNT; k++)
		{
			if (0 == recvMB[k].datalen || 0 == recvMB[k].timeout)
			{
				pFwdBuf = &recvMB[k];
				break;
			}
		}

		for(k=0; k<BEE_MAC_FORWARD_BUF_COUNT; k++)
		{
			if (recvMB[k].from == from && (0 != recvMB[k].datalen || 0 != recvMB[k].timeout))
				break;
		}

		if (k >=BEE_MAC_FORWARD_BUF_COUNT) // failed to find such a busy buf, do nothing
			return;

		// step.2 set this busy buffer to idle
		pFwdBuf->datalen = pFwdBuf->timeout =0;
		return;

	case BeeCtrl_Transmit:
		// step.1 look for the FwdBuf of the mentioned from-node
		for (k=0; k<BEE_MAC_FORWARD_BUF_COUNT; k++)
		{
			if (recvMB[k].from == from && 0 != recvMB[k].datalen && 0 != recvMB[k].timeout)
			{
				pFwdBuf = &recvMB[k];
				break;
			}
		}
		
		if (NULL == pFwdBuf) // failed to find such a busy buf, do nothing
			return;

		// step2. reload the timeout
		pFwdBuf->timeout   = BEE_MAC_FORWARD_BUF_TIMEOUT_RELOAD;

		// datalen is indeed the offset in this packet
		if (datalen > pFwdBuf->datalen)
			return; // something wrong, ignore this transmit

		// step.3 copy the data into the buf
		memcpy(pFwdBuf->buf + datalen, &msgbuf[3], MAC_DATA_PAYLOAD);

		// step.4 update the bitmap;
		calcBitmapIdx(datalen, &idxBmp, &bit);
		pFwdBuf->bitmap[idxBmp] |= bit;

		if (pFwdBuf->minOffset > datalen)
			pFwdBuf->minOffset = datalen;

		c=0;

		// step.5 scan if it is necessary to issue NACKs
		if (pFwdBuf->minOffset > MAC_DATA_PAYLOAD*4 && c<4)
		{
			// if the transmit is at the middle, just NACK for the recent bitmap byte, maximal 4 NACKs
			if (++idxBmp < BEE_BUF_BITMAP_MAX && 0xff != pFwdBuf->bitmap[idxBmp])
			{
				for (bit =1, k =0; c>0 && k <8; k++, bit <<=1)
				{
					if (pFwdBuf->bitmap[idxBmp] & bit)
						continue;

					// this is a missed packet, send a NACK for it
					// calculate the offset of the bit
					datalen =calcBitmapOffset(pFwdBuf->datalen, idxBmp, bit);
					if (datalen >= pFwdBuf->datalen)
						break; // beyond the range

					packetSz = BeePacket_composeTransmitNack(beeIf, msgbuf, srcAddr, destAddr, datalen); // the in completed packet
					BeeMAC_doSend(beeIf, from, msgbuf, packetSz);
					c++;
				}
			}

			return;
		}

		// the transmit has reached rear begining
		for (idxBmp = 0; idxBmp <BEE_BUF_BITMAP_MAX && c<8; idxBmp++)
		{
			if (0xff == pFwdBuf->bitmap[idxBmp])
				continue;

			for (bit =1, k =0; c>0 && k <8; k++, bit <<=1)
			{
				if (pFwdBuf->bitmap[idxBmp] & bit)
					continue;

				// this is a missed packet, send a NACK for it
				// calculate the offset of the bit
				datalen =calcBitmapOffset(pFwdBuf->datalen, idxBmp, bit);
				if (datalen >= pFwdBuf->datalen)
				{
					idxBmp = BEE_BUF_BITMAP_MAX;
					break; // out of range, break the loop because k increases from 0
				}
				
				packetSz = BeePacket_composeTransmitNack(beeIf, msgbuf, srcAddr, destAddr, datalen); // the in completed packet
				BeeMAC_doSend(beeIf, from, msgbuf, packetSz);
				c++;
			}
		}

		if (0 == pFwdBuf->minOffset && c<=0)
		{
			// reached the offset=0 and there were no NACKs, the transmit should be completed successully,
			// response a ACK to tell transmit completed
			packetSz = BeePacket_composeTransmitAck(beeIf, msgbuf, srcAddr, destAddr, pFwdBuf->datalen); // ACK to transmit completed
			BeeMAC_doSend(beeIf, from, msgbuf, packetSz);
		}

		return;

	case BeeCtrl_TransmitAck:
		// step.1 find TransmitCtx by from, mark 
		for (pTransmitCtx=NULL, k=0; k<BEE_MAC_TRANSMIT_CTX_COUNT; k++)
		{
			if (from == transmitCtxs[k].nextHop)
			{
				pTransmitCtx = &transmitCtxs[k];
				break;
			}
		}

		if (NULL == pTransmitCtx)
			return; // not found

		pTransmitCtx->lenOfAck = datalen;
		return;

	case BeeCtrl_TransmitNack:
		// step.1 find TransmitCtx by the txnId, mark 
		for (pTransmitCtx=NULL, k=0; k<BEE_MAC_TRANSMIT_CTX_COUNT; k++)
		{
			if (from == transmitCtxs[k].nextHop)
			{
				pTransmitCtx = &transmitCtxs[k];
				break;
			}
		}

		if (NULL == pTransmitCtx)
			return; // not found

		calcBitmapIdx(datalen, &tmp, &tmp2); // datalen was the offset in the message
		pTransmitCtx->bitmapNACKs[tmp] |= tmp2;

		return;

/*
		for (tmp=0, k=0; k<BEE_MAC_TRANSMIT_MAX_NACK; k++)
		{
			if (INVAILD_OFFSET_LEN == pTransmitCtx->NACKs[k] && 0==tmp)
			{
				pTransmitCtx->NACKs[k] = datalen;
				tmp=1;
				continue;
			}

			if (pTransmitCtx->NACKs[k] == datalen && 0!=tmp)
				pTransmitCtx->NACKs[k] = INVAILD_OFFSET_LEN;
		}

		return;
*/
	}

}

void Bee_init(BeeIF *beeIf, BeeNodeId_t nodeId)
{
	BeeRoute_reset();

	// about the BeeIF;
	beeIf->nodeId = nodeId;
	beeIf->addr.b[0] = 0xBE;
	beeIf->addr.b[1] = beeIf->addr.b[2] = 0x00;
	beeIf->addr.b[3] = nodeId;
	beeIf->uplink    = BEE_NEST_BCAST_NODEID; // uplink not yet determined

	memset(transmitCtxs, 0x00, sizeof(transmitCtxs));
	memset(recvMB, 0x00, sizeof(recvMB));
}

// -------------------------
//  BeeRoute
// -------------------------
typedef struct _BeeRouteStack
{
	BeeAddress addrs[BEEROUTE_STACK_ADDRS_SZ];
} BeeRouteStack;

typedef struct _BeeNeighbor
{
	BeeNodeId_t  nodeId;
	BeeAddress   addr;
	uint8_t      idxRouteStack;
	uint16_t     timeout;
} BeeNeighbor;

BeeNeighbor theUplink;

BeeNeighbor theNeighborHood[BEEROUTE_NEIGHBORHOOD_MAX];
static BeeRouteStack routeStacks[BEEROUTE_STACKS_MAX];

void BeeRoute_reset()
{
	uint8_t i;
	memset(theNeighborHood, 0, sizeof(theNeighborHood));
	memset(routeStacks, 0, sizeof(routeStacks));
	for (i =0; i<BEEROUTE_NEIGHBORHOOD_MAX; i++)
	{
		theNeighborHood[i].timeout =0;
		theNeighborHood[i].nodeId  =0xff;
		theNeighborHood[i].idxRouteStack =BEEROUTE_STACK_IDX_INVALID;
	}
}

//@return the idx of routeNodes that is allocated
uint8_t BeeRouteStack_allocate()
{
	uint8_t i;
	for (i=0; i<BEEROUTE_STACKS_MAX; i++)
	{
		if (DOMAIN_ADDR_B0_INVALID == routeStacks[i].addrs[0].b[0])
			return i;
	}

	return BEEROUTE_STACK_IDX_INVALID;
}

void BeeRouteStack_free(uint8_t idx)
{
	uint8_t i;
	if (idx<0 || idx>=BEEROUTE_STACKS_MAX)
		return;

	for (i=0; i<BEEROUTE_STACK_ADDRS_SZ; i++)
		routeStacks[idx].addrs[i].b[0] = DOMAIN_ADDR_B0_INVALID;
}

void BeeRouteStack_push(uint8_t idx, BeeAddress dest)
{
	uint8_t i;
	if (idx<0 || idx>=BEEROUTE_STACKS_MAX)
		return;

	// address if the dest already exists in the stack
	for (i=0; i<BEEROUTE_STACK_ADDRS_SZ-1; i++)
	{
		if (routeStacks[idx].addrs[i].dw == dest.dw || DOMAIN_ADDR_B0_INVALID == routeStacks[idx].addrs[i].b[0])
			break;
	}

	for (; i>0; i--)
		routeStacks[idx].addrs[i] = routeStacks[idx].addrs[i-1];

	routeStacks[idx].addrs[0] =dest;
}

void BeeRoute_add(BeeAddress dest, BeeNodeId_t nextHop, uint8_t hops)
{
#ifdef BEEROUTER_ENABLED
#else
	uint16_t i;
	uint16_t maxTimeout = BEEROUTE_TIMEOUT_RELOAD +1;
	BeeNeighbor* pNeighbor = NULL;

	// scan the NeighborHood to address that of the given dest
	for (i=0; i< BEEROUTE_NEIGHBORHOOD_MAX; i++)
	{
		if (0 == theNeighborHood[i].timeout--)
			theNeighborHood[i].timeout =0;

		if (nextHop == theNeighborHood[i].nodeId)
		{
			pNeighbor = &theNeighborHood[i];
			break;
		}
		
		if (maxTimeout > theNeighborHood[i].timeout)
		{
			pNeighbor = &theNeighborHood[i];
			maxTimeout = theNeighborHood[i].timeout;
		}
	}

	if (NULL == pNeighbor) // can not find a proper neighbor row to add
		return;

	pNeighbor->timeout = BEEROUTE_TIMEOUT_RELOAD;
	if (nextHop != pNeighbor->nodeId)
	{
		// this is a new neighbor, free and terminate its route list
		BeeRouteStack_free(pNeighbor->idxRouteStack);
		pNeighbor->idxRouteStack = BEEROUTE_STACK_IDX_INVALID;
		pNeighbor->nodeId = nextHop;
	}

	if (0 == hops)
	{
		// this is a direct neigbor
		pNeighbor->addr = dest;
		return;
	}

	if (BEEROUTE_STACK_IDX_INVALID == pNeighbor->idxRouteStack)
		pNeighbor->idxRouteStack = BeeRouteStack_allocate();

	if (BEEROUTE_STACK_IDX_INVALID == pNeighbor->idxRouteStack)
		return;

	BeeRouteStack_push(pNeighbor->idxRouteStack, dest);
#endif // BEEROUTER_ENABLED
}

BeeNodeId_t BeeRoute_lookfor(BeeIF* beeIf, BeeAddress dest)
{
#ifdef BEEROUTER_ENABLED
	return (NULL == beeIf) ? BEE_NEST_BCAST_NODEID : beeIf->uplink;
#else
	uint16_t neighborRouters[BEEROUTE_NEIGHBORHOOD_MAX];
	uint8_t i, j=0, k=0;

	memset(neighborRouters, 0xffff, sizeof(neighborRouters));

	// 1st round scan for the direct neighbors
	for (i=0; i<BEEROUTE_NEIGHBORHOOD_MAX; i++)
	{
		if (theNeighborHood[i].addr.dw == dest.dw)
			return theNeighborHood[i].nodeId;

		if (BEEROUTE_STACK_IDX_INVALID != theNeighborHood[i].idxRouteStack)
			neighborRouters[j++] = (theNeighborHood[i].nodeId <<8) | (theNeighborHood[i].idxRouteStack &0xff);
	}

	// 2nd round scan for the router nodes
	for (i=0; i<BEEROUTE_NEIGHBORHOOD_MAX && 0xff != (neighborRouters[i] & 0xff); i++)
	{
		k = neighborRouters[i] & 0xff;
		if (BEEROUTE_STACK_IDX_INVALID == k)
			continue;

		for (j=0; j <BEEROUTE_STACK_ADDRS_SZ; j++)
		{
			if (DOMAIN_ADDR_B0_INVALID == routeStacks[k].addrs[j].b[0])
				break;

			if (routeStacks[k].addrs[j].dw == dest.dw)
				return neighborRouters[i] >>8;
		}
	}

	// not found, take the broadcast instead
	return BEE_NEST_BCAST_NODEID;

#endif // BEEROUTER_ENABLED
}

void BeeRoute_delete(BeeAddress dest, BeeNodeId_t nextHop)
{
	uint8_t i;
	BeeRouteStack* pStack=NULL;

	// 1st round scan for the direct neighbors
	for (i=0; i<BEEROUTE_NEIGHBORHOOD_MAX; i++)
	{
		if (nextHop == theNeighborHood[i].nodeId || BEE_NEST_BCAST_NODEID == theNeighborHood[i].nodeId)
			break;
	}

	if (i>=BEEROUTE_NEIGHBORHOOD_MAX || BEE_NEST_BCAST_NODEID == theNeighborHood[i].nodeId)
		return;

	if (dest.dw == theNeighborHood[i].addr.dw)
	{
		// the delete is about the neighbor itself
		BeeRouteStack_free(theNeighborHood[i].idxRouteStack);
		theNeighborHood[i].idxRouteStack = BEEROUTE_STACK_IDX_INVALID;
		theNeighborHood[i].nodeId = BEE_NEST_BCAST_NODEID;
		theNeighborHood[i].timeout = 0;
		return;
	}

	if (BEEROUTE_STACK_IDX_INVALID == theNeighborHood[i].idxRouteStack)
		return;

	pStack = &routeStacks[theNeighborHood[i].idxRouteStack];

	for (i=0; i<BEEROUTE_STACK_ADDRS_SZ; i++)
	{
		if (DOMAIN_ADDR_B0_INVALID == pStack->addrs[i].b[0] || dest.dw == pStack->addrs[i].dw)
			break;
	}

	for (; i<BEEROUTE_STACK_ADDRS_SZ-1; i++)
	{
		memcpy(&pStack->addrs[i], &pStack->addrs[i+1], sizeof(BeeAddress));
	}

	pStack->addrs[BEEROUTE_STACK_ADDRS_SZ-1].b[0] = DOMAIN_ADDR_B0_INVALID;
}
