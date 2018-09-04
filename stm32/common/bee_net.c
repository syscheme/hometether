#include "bee_net.h"

#define INVAILD_OFFSET_LEN         (0xffff)

#define BEE_MAC_HEARDER_LEN        3
#define BEE_MAC_SM_HEARDER_LEN    (BEE_MAC_HEARDER_LEN + BEE_NET_ADDR_LEN*2)

#define BEE_BUF_BITMAP_MAX         (BEE_MAC_MSG_SIZE_MAX/(AIR_PAYLOAD -BEE_MAC_HEARDER_LEN)/8+1)

static BeeAddress _destUplink;

static void BeeRoute_reset();
static void BeeRoute_penaltyNeighbor(BeeNodeId_t neighbor);

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
} BeeOrdiCode_MAC;

#define CTRL_CODE(CMD, TTL, DLEN) ((CMD << 12) | (TTL&0x3) <<10 | (DLEN & 0x3ff))
#define CTRL_CODE_CMD(CTRLCODE)   (CTRLCODE >> 12)
#define CTRL_CODE_TTL(CTRLCODE)   ((CTRLCODE >> 10) & 0x3)
#define CTRL_CODE_DLEN(CTRLCODE)  (CTRLCODE & 0x3ff)
#define MSG_CMD(_MSG)             CTRL_CODE_CMD(*((uint16_t*) (&_MSG[0])))
#define MSG_TTL(_MSG)             CTRL_CODE_TTL(*((uint16_t*) (&_MSG[0])))
#define MSG_DLEN(_MSG)            CTRL_CODE_DLEN(*((uint16_t*) (&_MSG[0])))

#define BEE_MAC_SET_HEADER(MSG, NODEID, CMD, TTL, DLEN) { *((uint16_t*)MSG) = CTRL_CODE(CMD, TTL, DLEN); *(((uint8_t*)MSG) +2) = NODEID &0xff; }

static void BeeMAC_procShortMessage(BeeIF* beeIf, uint8_t* msgbuf, uint8_t datalen);

static uint8_t BeePacket_composeOpenTranmit(BeeIF* beeIf, uint8_t* outmsg, uint8_t ttl, BeeAddress srcAddr, BeeAddress destAddr, uint16_t datalen)
{
	//  0          1          2          3          4          5          6          7          8          9          10              
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// |      ctrl_code      |   from   |                  srcAddr                  |                 destAddr                  |
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// ctrl_code = MLB: 4b cmd + 2b ttl +10b datalen

	if (NULL ==outmsg)
		return 0;

	BEE_MAC_SET_HEADER(outmsg, beeIf->nodeId, BeeCtrl_OpenTransmit, ttl, datalen);

	memcpy(&outmsg[BEE_MAC_HEARDER_LEN], srcAddr.b, BEE_NET_ADDR_LEN);
	memcpy(&outmsg[BEE_MAC_HEARDER_LEN + BEE_NET_ADDR_LEN], destAddr.b, BEE_NET_ADDR_LEN);

	return BEE_MAC_SM_HEARDER_LEN;
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

	BEE_MAC_SET_HEADER(outmsg, beeIf->nodeId, BeeCtrl_WithdrawTransmit, ttl, datalen);

	memcpy(&outmsg[BEE_MAC_HEARDER_LEN], srcAddr.b, BEE_NET_ADDR_LEN);
	memcpy(&outmsg[BEE_MAC_HEARDER_LEN + BEE_NET_ADDR_LEN], destAddr.b, BEE_NET_ADDR_LEN);

	return BEE_MAC_SM_HEARDER_LEN;
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

	if (*datalen > (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN))
		*datalen = (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN);

	BEE_MAC_SET_HEADER(outmsg, beeIf->nodeId, BeeCtrl_Transmit, ttl, offset);

	memcpy(&outmsg[BEE_MAC_HEARDER_LEN], data+offset, *datalen);

	return *datalen +BEE_MAC_HEARDER_LEN;
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

	BEE_MAC_SET_HEADER(outmsg, beeIf->nodeId, BeeCtrl_TransmitAck, 1, datalen);

	memcpy(&outmsg[BEE_MAC_HEARDER_LEN], srcAddr.b, BEE_NET_ADDR_LEN);
	memcpy(&outmsg[BEE_MAC_HEARDER_LEN + BEE_NET_ADDR_LEN], destAddr.b, BEE_NET_ADDR_LEN);

	return BEE_MAC_SM_HEARDER_LEN;
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

	BEE_MAC_SET_HEADER(outmsg, beeIf->nodeId, BeeCtrl_TransmitNack, 1, offset);

	memcpy(&outmsg[BEE_MAC_HEARDER_LEN], srcAddr.b, BEE_NET_ADDR_LEN);
	memcpy(&outmsg[BEE_MAC_HEARDER_LEN + BEE_NET_ADDR_LEN], destAddr.b, BEE_NET_ADDR_LEN);

	return BEE_MAC_SM_HEARDER_LEN;
}

uint8_t BeeMAC_sendShortMessage(BeeIF* beeIf, uint8_t* msgbuf, BeeAddress srcAddr, BeeAddress destAddr, BeeNodeId_t nextHop, uint8_t* data, uint16_t datalen)
{
	// step 1. compose the packet
	//  0          1          2          3          4          5          6          7          8          9          10              
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// |      ctrl_code      |   from   |                  srcAddr                  |                 destAddr                  |
	// +----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
	// ctrl_code = MLB: 4b cmd + 2b ttl +10b datalen

	if (NULL ==msgbuf)
		return 0;

	BEE_MAC_SET_HEADER(msgbuf, beeIf->nodeId, BeeCtrl_ShortMessage, BEE_MAC_TTL_MAX, 0);

	memcpy(&msgbuf[BEE_MAC_HEARDER_LEN], srcAddr.b, BEE_NET_ADDR_LEN);
	memcpy(&msgbuf[BEE_MAC_HEARDER_LEN + BEE_NET_ADDR_LEN], destAddr.b, BEE_NET_ADDR_LEN);

	if (NULL == data)
		datalen=0;
	else
	{
		if (datalen > (AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN))
			datalen = AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN;
		
		memcpy(&msgbuf[BEE_MAC_SM_HEARDER_LEN], data, datalen);
	}

	datalen += BEE_MAC_SM_HEARDER_LEN; // take the datalen as the packet len
	
	// step 2. find the dest in the route table
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
	*idx = (offset +(AIR_PAYLOAD -BEE_MAC_HEARDER_LEN) -1) /(AIR_PAYLOAD -BEE_MAC_HEARDER_LEN); // packageIdx here

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
	if (size % (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN))
		offset = size % (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN) + --offset * (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN);
	else offset *= (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN);

	return offset;
}

static uint8_t BeeMAC_transmitEx(BeeIF* beeIf, uint8_t* msgbuf, uint8_t ttl, BeeAddress srcAddr, BeeAddress destAddr, uint8_t* data, uint16_t datalen)
{
	int i;
	uint8_t packetSz =0;
	TransmitCtx* ctx = NULL;
	int16_t transmitQuota =0, offset, flag;
	uint8_t sendlen = (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN);

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
	{
		BeeRoute_penaltyNeighbor(ctx->nextHop);
		return 0; // either no accept ACK or rejected
	}

	// step 3. perform transmitting
	//  3.1 determin max transmit packets
	transmitQuota= (uint16_t) ((datalen / (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN) +3) *1.3); // maximal 30% retransmit
	transmitQuota += (transmitQuota>>4) <<1; // added the doubled number of rounds of the batch-send, each batch 16 packets
	offset = datalen;
	
	while (transmitQuota-- >0 && ctx->lenOfAck <=0)
	{
		if (0 == offset && transmitQuota <3)
		{
			// the offset=0 packet is special if lost its NACK, just retransmit it for more times to avoid
			sendlen = (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN);
			packetSz = BeePacket_composeTransmit(beeIf, msgbuf, ttl, data, offset, &sendlen);
			BeeMAC_doSend(beeIf, ctx->nextHop, msgbuf, packetSz);
		}

		// the regular loop to send packet from the end one by one
		for (i=0; !ctx->lenOfAck && offset >0 && transmitQuota >0 && i <16; i++)
		{
			offset -= sendlen;
			if (offset < 0) // the last packet with offset =0
				offset =0;

			sendlen = (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN);
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
				sendlen = (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN);
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
			sendlen = (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN);
			packetSz = BeePacket_composeTransmit(beeIf, msgbuf, ttl, data, ctx->NACKs[i], &sendlen);
			BeeMAC_doSend(beeIf, ctx->nextHop, msgbuf, packetSz);
			transmitQuota--;
			ctx->NACKs[i] =INVAILD_OFFSET_LEN;
		}
*/
	}

	offset = ctx->lenOfAck;
	
	// free the ctx
	ctx->nextHop = BEE_NEST_INVALID_NODEID;

	if (offset == datalen)
		return 1;

	packetSz = BeePacket_composeWithdrawTransmit(beeIf, msgbuf, ttl, srcAddr, destAddr, datalen);
	BeeMAC_doSend(beeIf, ctx->nextHop, msgbuf, packetSz);

	BeeRoute_penaltyNeighbor(ctx->nextHop);
	return 0;
}

uint8_t BeeMAC_transmit(BeeIF* beeIf, uint8_t* msgbuf, BeeAddress srcAddr, BeeAddress destAddr, uint8_t* data, uint16_t datalen)
{
	return BeeMAC_transmitEx(beeIf, msgbuf, BEE_MAC_TTL_MAX, srcAddr, destAddr, data, datalen);
}

typedef struct _RecvMsgBuf
{
	uint8_t    buf[BEE_MAC_MSG_SIZE_MAX];
	uint8_t    from; 
	BeeAddress srcAddr, destAddr;
	uint16_t   datalen; // the total bytes of the message. 0 means this buffer is idle
	uint8_t    bitmap[BEE_BUF_BITMAP_MAX]; // bitmap identifies the good data received (in (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN)) 
	uint8_t    timeout; // the down-counting timeout, in 100msec. 0 means idle
	uint16_t   minOffset;
} RecvMsgBuf;

RecvMsgBuf  recvMB[BEE_MAC_FORWARD_BUF_COUNT];

#define OFFSET_TO_BITMAP_POS(_OFFSET, _IDXBMP, _FLAG)  { _IDXBMP = (_OFFSET +(AIR_PAYLOAD -BEE_MAC_HEARDER_LEN) -1) /(AIR_PAYLOAD -BEE_MAC_HEARDER_LEN); _FLAG = 1 <<(_IDXBMP %8); _IDXBMP /= 8; }

void BeeMAC_procPacketReceived(BeeIF* beeIf, uint8_t* msgbuf, uint16_t msglen, uint8_t heardfrom) // heardfrom is the local bound interface thru which the message is received
{
	int k, c;
	uint8_t packetSz=0, bit;
	int8_t  idxBmp;
	uint16_t datalen = MSG_DLEN(msgbuf);
	BeeAddress srcAddr, destAddr;
	uint8_t from = msgbuf[2];
	uint8_t tmp, tmp2;
	RecvMsgBuf* pFwdBuf = NULL;
	TransmitCtx* pTransmitCtx = NULL;

	// step 1. about the addresses, add it into routeTable if necessary
	// the addrs may not be valid for some commands
	memcpy(srcAddr.b,  &msgbuf[BEE_MAC_HEARDER_LEN], BEE_NET_ADDR_LEN);
	memcpy(destAddr.b, &msgbuf[BEE_MAC_HEARDER_LEN + BEE_NET_ADDR_LEN], BEE_NET_ADDR_LEN);

	switch(MSG_CMD(msgbuf))
	{
	case BeeCtrl_ShortMessage:
	case BeeCtrl_OpenTransmit:
	case BeeCtrl_WithdrawTransmit:
		if (BEE_NEST_BCAST_NODEID != heardfrom && BEE_NEST_BCAST_NODEID != from)
		{
			// calculate the hops from TTL
			k = BEE_MAC_TTL_MAX - MSG_TTL(msgbuf);
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
		BeeMAC_procShortMessage(beeIf, msgbuf, 0);
		return;

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
			if (0 == recvMB[k].timeout--)
			{
				recvMB[k].timeout=0;
				continue;
			}

			if (recvMB[k].from == from && 0 != recvMB[k].datalen)
			{
				pFwdBuf = &recvMB[k];
				break;
			}
		}
		
		if (NULL == pFwdBuf)
		{
			// failed to find such a busy buf, send a faked ACK to say transmit completed
			packetSz = BeePacket_composeTransmitAck(beeIf, msgbuf, srcAddr, destAddr, 0x3fff);
			BeeMAC_doSend(beeIf, from, msgbuf, packetSz);
			return;
		}

		// step2. reload the timeout
		pFwdBuf->timeout   = BEE_MAC_FORWARD_BUF_TIMEOUT_RELOAD;

		// datalen is indeed the offset in this packet
		if (datalen > pFwdBuf->datalen)
			return; // something wrong, ignore this transmit

		// step.3 copy the data into the buf
		memcpy(pFwdBuf->buf + datalen, &msgbuf[BEE_MAC_HEARDER_LEN], (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN));

		// step.4 update the bitmap;
		calcBitmapIdx(datalen, &idxBmp, &bit);
		pFwdBuf->bitmap[idxBmp] |= bit;

		if (pFwdBuf->minOffset > datalen)
			pFwdBuf->minOffset = datalen;

		c=0;

		// step.5 scan if it is necessary to issue NACKs
		if (pFwdBuf->minOffset > (AIR_PAYLOAD -BEE_MAC_HEARDER_LEN)*4 && c<4)
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

			if (destAddr.dw == beeIf->addr.dw)
			{
				// this message is to self, callback upper layer NET to notice data arrived
				pFwdBuf->timeout = BeeNet_OnDataArrived(beeIf, srcAddr, destAddr, pFwdBuf->buf, pFwdBuf->datalen);
			}
			else
			{
				// this message is to others, call BeeMAC_transmitEx to continue forward by decreasing ttl
				tmp =MSG_TTL(msgbuf);
				if (tmp >0)
				{
					pFwdBuf->timeout = BEE_MAC_FORWARD_BUF_TIMEOUT_RELOAD;
					BeeMAC_transmitEx(beeIf, msgbuf, --tmp, srcAddr, destAddr, pFwdBuf->buf, pFwdBuf->datalen);
				}

				pFwdBuf->timeout =0;
			}
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

	_destUplink.dw =0x08080808; // IP=8.8.8.8
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
#ifndef BEEROUTER_ENABLED
	uint16_t i;
	uint16_t maxTimeout = BEEROUTE_TIMEOUT_RELOAD +1;
	BeeNeighbor* pNeighbor = NULL;

	// step 1. adjust the dest per our address mapping
	switch(dest.b[0])
	{
	case DOMAIN_ADDR_B0_INVALID:
		return; // do nothing

	case DOMAIN_ADDR_B0_BEE:
		break;

	case DOMAIN_ADDR_B0_CAN:
	case DOMAIN_ADDR_B0_USART:
		// adjust for the attached bus
		dest.b[0] = DOMAIN_ADDR_B0_BEE;
		dest.b[2] = dest.b[3] =0; // reserved fields
		break;

	case DOMAIN_ADDR_B0_IP_A:
	case DOMAIN_ADDR_B0_IP_B:
	case DOMAIN_ADDR_B0_IP_C:

	default:
		return;
	}

	// step 2. scan the NeighborHood to address that of the given dest
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
	// step 2.1 create the newly known neighbor by nodeId
	if (nextHop != pNeighbor->nodeId)
	{
		// this is a new neighbor, free and terminate its route list
		BeeRouteStack_free(pNeighbor->idxRouteStack);
		pNeighbor->idxRouteStack = BEEROUTE_STACK_IDX_INVALID;
		pNeighbor->nodeId = nextHop;
		pNeighbor->addr.b[0] = DOMAIN_ADDR_B0_INVALID;
	}

	// step 2.2 if this dest is a direct neighbor
	if (0 == hops)
	{
		// this is a direct neigbor
		pNeighbor->addr = dest;
		return;
	}

	// step 3. insert the dest address into the route stack of this neighbor
	if (BEEROUTE_STACK_IDX_INVALID == pNeighbor->idxRouteStack)
		pNeighbor->idxRouteStack = BeeRouteStack_allocate();

	if (BEEROUTE_STACK_IDX_INVALID == pNeighbor->idxRouteStack)
		return;

	BeeRouteStack_push(pNeighbor->idxRouteStack, dest);
#endif // BEEROUTER_ENABLED
}

BeeNodeId_t BeeRoute_lookfor(BeeIF* beeIf, BeeAddress dest)
{
	uint16_t neighborRouters[BEEROUTE_NEIGHBORHOOD_MAX];
	uint8_t i, j=0, k=0;

	// adjust the dest per our address mapping
	switch(dest.b[0])
	{
	case DOMAIN_ADDR_B0_INVALID:
		return 0; // do nothing

	case DOMAIN_ADDR_B0_BEE:
		break;

	case DOMAIN_ADDR_B0_CAN:
	case DOMAIN_ADDR_B0_USART:
		// adjust for the attached bus
		dest.b[0] = DOMAIN_ADDR_B0_BEE;
		dest.b[2] = dest.b[3] =0; // reserved fields
		break;

	case DOMAIN_ADDR_B0_IP_A:
	case DOMAIN_ADDR_B0_IP_B:
	case DOMAIN_ADDR_B0_IP_C:
		//TODO: if this is a IP GW, return beeIf->nodeId here

	default:
		return (NULL == beeIf) ? BEE_NEST_BCAST_NODEID : beeIf->uplink;
	}

#ifdef BEEROUTER_ENABLED
	return (NULL == beeIf) ? BEE_NEST_BCAST_NODEID : beeIf->uplink;
#else
	memset(neighborRouters, 0xffff, sizeof(neighborRouters));

	// 1st round scan for the direct neighbors
	for (i=0; i<BEEROUTE_NEIGHBORHOOD_MAX; i++)
	{
		if (theNeighborHood[i].addr.dw == dest.dw)
			return theNeighborHood[i].nodeId;

		if (BEEROUTE_STACK_IDX_INVALID != theNeighborHood[i].idxRouteStack)
			neighborRouters[j++] = (theNeighborHood[i].nodeId <<8) | (theNeighborHood[i].idxRouteStack &0xff);
	}

	// 2nd rount scan for the router nodes
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
		if (nextHop == theNeighborHood[i].nodeId)
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
		theNeighborHood[i].addr.b[0] = DOMAIN_ADDR_B0_INVALID;
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

void BeeRoute_penaltyNeighbor(BeeNodeId_t neighbor)
{
	uint8_t i;
	BeeRouteStack* pStack=NULL;

	// 1st round scan for the direct neighbors
	for (i=0; i<BEEROUTE_NEIGHBORHOOD_MAX; i++)
	{
		if (neighbor == theNeighborHood[i].nodeId)
			break;
	}

	if (i>=BEEROUTE_NEIGHBORHOOD_MAX || BEE_NEST_BCAST_NODEID == theNeighborHood[i].nodeId)
		return;

	if (theNeighborHood[i].timeout > (BEEROUTE_TIMEOUT_RELOAD /BEEROUTE_NEIGHBOR_PENALTY_MAX))
	{
		// if penalty doesn't cause the neighbor disabled instantly
		theNeighborHood[i].timeout -= (BEEROUTE_TIMEOUT_RELOAD /BEEROUTE_NEIGHBOR_PENALTY_MAX);
		return;
	}

	// remove such a neighbor by resetting its timeout
	theNeighborHood[i].timeout = 0;
	BeeRouteStack_free(theNeighborHood[i].idxRouteStack);
	theNeighborHood[i].idxRouteStack = BEEROUTE_STACK_IDX_INVALID;
	theNeighborHood[i].nodeId = BEE_NEST_BCAST_NODEID;
	theNeighborHood[i].addr.b[0] = DOMAIN_ADDR_B0_INVALID;
	return;
}


// -------------------------
//  BeeNet
// -------------------------
#define RESP(_ORDC) (0x80 | (_ORDC &0xff))
enum {
	BeeOrdC_PING = 0, 
	BeeOrdC_PONG = RESP(BeeOrdC_PING),
} BeeOrdiCode_NET;

static uint8_t BeeNet_pingEx(BeeIF* beeIf, uint8_t* msgbuf, BeeAddress src, BeeAddress dest, BeeNodeId_t from)
{
	int i=BEE_MAC_SM_HEARDER_LEN;
	BeeNodeId_t nextHop = BeeRoute_lookfor(beeIf, dest);

	if (BEE_NEST_BCAST_NODEID != from)
	{
		i=BEE_MAC_SM_HEARDER_LEN;
		msgbuf[i++] = BeeOrdC_PING;
		for (; i < (AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN) && BEE_NEST_BCAST_NODEID != msgbuf[i++]; i++);

			if (i >= (AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN))
				return 0; // hops overflow

		msgbuf[i] = from;
		for (; i < (AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN); i++)
			msgbuf[i] = BEE_NEST_BCAST_NODEID;
	}

	return BeeMAC_sendShortMessage(beeIf, msgbuf, src, dest, nextHop, msgbuf+BEE_MAC_SM_HEARDER_LEN, AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN);
}

uint8_t BeeNet_ping(BeeIF* beeIf, BeeAddress dest)
{
	uint8_t msgbuf[AIR_PAYLOAD];
	memset(msgbuf, BEE_NEST_BCAST_NODEID, sizeof(msgbuf));

	return BeeNet_pingEx(beeIf, msgbuf, beeIf->addr, dest, BEE_NEST_BCAST_NODEID);
}


void BeeMAC_procShortMessage(BeeIF* beeIf, uint8_t* msgbuf, uint8_t datalen)
{
	uint8_t i, ttl = MSG_TTL(msgbuf), nextHop; // the ttl

	BeeAddress srcAddr, destAddr;
	memcpy(srcAddr.b,  &msgbuf[BEE_MAC_HEARDER_LEN], BEE_NET_ADDR_LEN);
	memcpy(destAddr.b, &msgbuf[BEE_MAC_HEARDER_LEN + BEE_NET_ADDR_LEN], BEE_NET_ADDR_LEN);

	switch(msgbuf[BEE_MAC_SM_HEARDER_LEN])
	{
	case BeeOrdC_PING:
		if (destAddr.dw == beeIf->addr.dw)
		{
			// this is the dest that the PING is looking for, respond a PONG
			i =BEE_MAC_SM_HEARDER_LEN;
			msgbuf[i++] = BeeOrdC_PONG;
			BeeMAC_sendShortMessage(beeIf, msgbuf, beeIf->addr, srcAddr, msgbuf[2], msgbuf+BEE_MAC_SM_HEARDER_LEN, AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN);
			return;
		}

		// this is an intermediate hop, continue to forward
		if (ttl <=0)
			return;

		*((uint16_t*) (msgbuf)) = CTRL_CODE(BeeCtrl_ShortMessage, --ttl, 0);
		BeeNet_pingEx(beeIf, msgbuf, srcAddr, destAddr, msgbuf[2]);
		return;

	case BeeOrdC_PONG:
		if (destAddr.dw == beeIf->addr.dw)
		{
			// this is the dest that the PONG is returning to
			i =BEE_MAC_SM_HEARDER_LEN;
			while (i < (AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN -1) && BEE_NEST_BCAST_NODEID != msgbuf[++i]);
			
			// update the uplink if this PING was to the uplink destination
			if (_destUplink.dw == srcAddr.dw)
				beeIf->uplink = msgbuf[2];

			BeeNet_OnPong(beeIf, srcAddr, msgbuf[2], i-BEE_MAC_SM_HEARDER_LEN-1);
			return;
		}

		// this is an intermediate hop, continue to forward

		// test and adjust ttl
		if (ttl <=0)
			return;

		*((uint16_t*) (msgbuf)) = CTRL_CODE(BeeCtrl_ShortMessage, --ttl, 0);

		i =BEE_MAC_SM_HEARDER_LEN;
		while (i < (AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN -1) && beeIf->nodeId != msgbuf[++i]);
		if (i >=AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN)
			return; // hops overflow

		BeeMAC_sendShortMessage(beeIf, msgbuf, srcAddr, destAddr, msgbuf[i-1], msgbuf+BEE_MAC_SM_HEARDER_LEN, AIR_PAYLOAD -BEE_MAC_SM_HEARDER_LEN);
		return;

	default:
		if (destAddr.dw == beeIf->addr.dw)
		{
			BeeNet_OnDataArrived(beeIf, srcAddr, destAddr, msgbuf+BEE_MAC_SM_HEARDER_LEN, datalen -BEE_MAC_SM_HEARDER_LEN);
			return;
		}

		if (ttl <=0)
			return;

		// adjust the ttl and from
		*((uint16_t*) (msgbuf)) = CTRL_CODE(BeeCtrl_ShortMessage, --ttl, 0);

		// forward this notice to the other
		nextHop = BeeRoute_lookfor(beeIf, destAddr);
		if (BEE_NEST_BCAST_NODEID == nextHop)
			nextHop = beeIf->uplink;

		msgbuf[2] = beeIf->nodeId;
		BeeMAC_doSend(beeIf, nextHop, msgbuf, AIR_PAYLOAD);
		return;
	}
}

void Bee_doScan(BeeIF* beeIf)
{
	if (NULL == beeIf || BEE_NEST_BCAST_NODEID == beeIf->nodeId || BEE_NEST_INVALID_NODEID == beeIf->nodeId)
		return; // invalid beeIf

	if (BEE_NEST_INVALID_NODEID == beeIf->uplink)
		beeIf->uplink = BEE_NEST_BCAST_NODEID;

	if (BEE_NEST_BCAST_NODEID == beeIf->uplink)
		BeeNet_ping(beeIf, _destUplink);
}

