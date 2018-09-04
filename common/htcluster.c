#include "htcluster.h"
#include "textmsg.h"

#define LOOPBACK_TEST

#ifndef HTN_NEIGHBORS_MAX
#  define HTN_NEIGHBORS_MAX (8)
#endif // HTN_NEIGHBORS_MAX

#define HTCMSG_HEADER_LEN   (2)
#define OBJ_OP_HEADER_LEN  (4)
#define OBJ_VAL_POSITION    (HTCMSG_HEADER_LEN +OBJ_OP_HEADER_LEN)
#define OBJ_VALS_PAYLOAD_LEN_MAX  (HTDDL_MSG_LEN_MAX -OBJ_VAL_POSITION -OBJ_SIGNATURE_LEN)

////////////////////////////////////////////////////////////////////

NodeId_t nidThis = 0x12345678;
HtNode   htnThis; // see HtNode_init()

#define MSG_PAYLOAD_LEN(MSG)   (MSG[1] >>4)
#define MSG_CMD(MSG)           (MSG[0])
#define MSG_TTL(MSG)           (MSG[1] & 0x0f)

// ---------------------------------------------------------------------------
// declaration of private functions
// ---------------------------------------------------------------------------
static hterr HtCache_nextHop(const uint8_t eNid, HtNextHop* nextHop);
static hterr HtNode_pvSendMSG(const HtNextHop* nextHop, uint8_t* msg, uint16_t msglen, uint16_t msgmax, uint8_t flags);
// static hterr HtNode_pvSendMSG(const HtNextHop* nextHop, const uint8_t* msg, uint8_t msgPayloadLen);
hterr  HtNode_doSetObject(HtNetIf* netif, uint8_t objectId, uint8_t subIdx, const uint8_t* value, uint8_t vallen);
#define HtNode_doSyncClock(netif, clock_msec)  HtNode_doSetObject(netif, ODID_clockmsec, 0, (uint8_t*)&(clock_msec), sizeof(uint32_t))

// ---------------------------------------------------------------------------
// Downlink caches
// ---------------------------------------------------------------------------
#if HTN_FUNC_HAS_DOWNLINK 
typedef struct _RouteEntry
{
	HtNextHop hop;
	uint8_t   weight; // high-4bit=pwrlevel, low-4bit=hops calculated from ttl
} RouteEntry;

LRUMap HTCache_route;     // map of nodeId to HtNextHop, // NeighborEntry
LRUMap HTCache_eNids;     // map of eNid to dest NodeId_t
// LRUMap HTCache_neighborToDest; // map of nodeId to nodeIdNeighbor

void  HtNode_init(HtNetIf* netifUpLink)
{
	memset(&htnThis, 0x00, sizeof(htnThis));

//	htnThis.nodeState    = ens_Disconnected;
	htnThis.nodeFuncs    = HTN_FUNC_TYPE;
	htnThis.uplink.netif = netifUpLink;

	if (netifUpLink)
		HtNetIf_setHwAddr(netifUpLink, (uint8_t*)&nidThis, sizeof(nidThis));

#if HTN_FUNC_HAS_DOWNLINK
	LRUMap_init(&HTCache_route, sizeof(NodeId_t), sizeof(RouteEntry), HTN_NEIGHBORS_MAX);
	LRUMap_init(&HTCache_eNids, sizeof(uint8_t),  sizeof(NodeId_t), HTN_NEIGHBORS_MAX+2);
#endif // HTN_FUNC_HAS_DOWNLINK

#if HTN_FUNC_IS_EDGE
	htnThis.eNid = HTDICT_ENID_EDGE;
#endif // HTN_FUNC_IS_EDGE
}

#define ASSERT_ROUTER(_HTNode) if (0 == ((_HTNode).nodeFuncs & HTN_FUNC_REPEATER) || (_HTNode).uplink.nextNid <=0) return ERR_NOT_SUPPORTED;

static NodeId_t HtCache_find_eNid(const uint8_t eNid)
{
	NodeId_t dest;
	if (ERR_SUCCESS == LRUMap_get(&HTCache_eNids, &eNid, (uint8_t*)&dest, 0))
		return dest;
	return 0;
}

static uint8_t HtCache_calcWeight(uint8_t hops, uint8_t pwrLevel)
{
	return pwrLevel + (0x100 - hops); // dummy calcuation
}

static void HtCache_add_eNid(const uint8_t eNid, const NodeId_t destNid)
{
	LRUMap_set(&HTCache_eNids, &eNid, (const uint8_t*)&destNid);
}

static void HtCache_addRoute(const NodeId_t dest, const NodeId_t neighbor, HtNetIf* netif, uint8_t hops, uint8_t pwrLevel)
{
	RouteEntry re;
	uint8_t weight = HtCache_calcWeight(hops, pwrLevel);
	if (ERR_SUCCESS == LRUMap_get(&HTCache_route, (const uint8_t*)&dest, (uint8_t*)&re, 1))
	{
		if (re.weight > weight) // do not overwrite, ignore
			return;
	}

	re.hop.nextNid = neighbor;
	re.hop.netif     = netif;
	re.weight      = weight;
	LRUMap_set(&HTCache_eNids, (const uint8_t*)&dest, (uint8_t*)&re);
}

static hterr HtRouter_forward(uint8_t* msg, uint16_t msglen, NodeId_t* pDestNodeId)
{
	//  0          1           2           3
	// +----------+-----------+-----------+-----------+----------+----------+----------+----------+----------+
	// |  GETOBJ  |  v  | TTL |  src-eNid | dest-eNid | ...                                                  | 
	// +----------+-----------+-----------+-----------+----------+----------+----------+----------+----------+
	HtNextHop nextHop;
	// step1. adjust the ttl
	uint8_t tmp = MSG_TTL(msg);
	if (0 == tmp--)
		return ERR_OPERATION_ABORTED;

	msg[1] = (MSG_PAYLOAD_LEN(msg) <<4) | (tmp & 0x0f);

	// step2. look for the route record
	if (NULL == pDestNodeId)
	{
		// msg[3] must be a dest eNid, convert it to the dest nodeId
		if (ERR_SUCCESS != HtCache_nextHop(msg[3], &nextHop))
			return ERR_OPERATION_ABORTED;
		
		pDestNodeId = &nextHop.nextNid;
	}
	else nextHop.nextNid = *pDestNodeId;

	// step3. send the new message
	return HtNode_pvSendMSG(&nextHop, msg, msglen, msglen, 0);
}

#endif // HTN_FUNC_HAS_DOWNLINK

hterr HtCache_nextHop(const uint8_t eNid, HtNextHop* nextHop)
{
	if (HTDICT_ENID_INVALID == eNid || NULL == nextHop)
		return ERR_INVALID_PARAMETER;

#if (HTN_FUNC_TYPE != HTN_FUNC_SENSOR)
	{
		RouteEntry re;
		NodeId_t dest = HtCache_find_eNid(eNid);
		if (0 == dest && ERR_SUCCESS == LRUMap_get(&HTCache_route, (const uint8_t*)&dest, (uint8_t*)&re, 0))
		{
			// found in the cache and neighbor associated, take the netif to the neighbor
			memcpy(nextHop, &re.hop, sizeof(HtNextHop));
			return ERR_SUCCESS;
		}
	}
#endif

	// not associated, then take the uplink by default
	memcpy(nextHop, &htnThis.uplink, sizeof(HtNextHop));
	return ERR_SUCCESS;
}

static hterr HtUplink_forward(uint8_t* msg, uint16_t msglen)
{
	// step1. adjust the ttl
	uint8_t tmp = MSG_TTL(msg);
	if (0 == tmp--)
		return ERR_OPERATION_ABORTED;

	msg[1] = (MSG_PAYLOAD_LEN(msg) <<4) | (tmp & 0x0f);

	// step2. double check the func of router
	if (htnThis.uplink.nextNid <=0)
		return ERR_NOT_SUPPORTED;

	// step3. send the adjusted msg to uplink
	return HtNode_pvSendMSG(&htnThis.uplink, msg, msglen, msglen, 0);
}


// #define ASSERT_MSG_CRC(LASTIDX)	if (msg[LASTIDX] != calcCRC8(msg, LASTIDX))	return
void HtNode_procMsg(HtNetIf* netif, uint8_t* msg, uint16_t msglen, uint8_t powerLevel)
{
	// HtNextHop nh;
	NodeId_t theParty;
	uint8_t  payloadLen=0, ttl=0;
	if (NULL ==msg || msglen < 2)
		return;

	payloadLen = MSG_PAYLOAD_LEN(msg);
	ttl    = MSG_TTL(msg);
	if (msglen < payloadLen+2)
		return; // ignore the incompleted messages

	//TODO: the tail must be signature if msglen > vlen+2, should validate

	switch (MSG_CMD(msg))
	{
	case HTCMSG_NEIGHB: // neigbor Advertisement
		//  0          1           2           3           4          5          6          7          8          9          10              
		// +----------+-----------+-----------+----------+----------+----------+----------+----------+----------+----------+----------+
		// |  NEIGHB  |  00 | TTL |  src-eNid |                srcNodeId                  |                   orphanNodeId            |
		// +----------+-----------+-----------+----------+----------+----------+----------+----------+----------+----------+----------+
		// step1. validate the checksum
		// ASSERT_MSG_CRC(11);

#ifndef LOOPBACK_TEST
		// filter the loopback message
		if (htnThis.eNid == msg[2])
			return;
#endif // LOOPBACK_TEST

#if HTN_FUNC_IS_ROUTER || HTN_FUNC_IS_EDGE
		// step2. refresh the neighor cache
		theParty = *((uint32_t*) (msg+7));
		HtCache_addRoute(theParty, *((uint32_t*) (msg+3)), netif, (HTC_TTL_MAX - ttl), powerLevel);

		// step2.1 refresh the eNid cache
		if (HTDICT_ENID_INVALID != msg[2])
		{
			// refresh the cache about the routine to the eNid
			HtCache_add_eNid(msg[2], theParty);
		}
#endif // HTN_FUNC_IS_ROUTER || HTN_FUNC_IS_EDGE

#if HTN_FUNC_IS_ROUTER
		// step 3. procedures as a router, forward by adjusting srcNid = nidThis
		// adjust the srcNid = nidThis
		memcpy(msg+3, &nidThis, sizeof(uint32_t));

		// forward to the uplink, up to the TTL to terminate the forwarding
		HtUplink_forward(msg, msglen);
#endif // HTN_FUNC_IS_ROUTER

#if HTN_FUNC_IS_EDGE
		// step 4. perform as a Edge
		HtEdge_OnNodeAdvertize(netif, msg[2], theParty, *((uint32_t*) (msg+3)));
#endif // HTN_FUNC_IS_EDGE

		return;

	case HTCMSG_ADOPT: // from unlink to downlink
		//  0          1           2           3           4          5          6          7          8          9          10              
		// +----------+-----------+-----------+-----+-----+----------+----------+----------+----------+----------+----------+----------+
		// |   ADOPT  |  00 | TTL |   eNid    |                 orphanNodeId               |               accessKey                   |
		// +----------+-----------+-----------+-----+-----+----------+----------+----------+----------+----------+----------+----------+
		// step1. validate the checksum
		// ASSERT_MSG_CRC(11);

		// step2. perform if this is the orphan
		theParty = *((uint32_t*) (msg+3));
		if (nidThis == theParty)
		{
			htnThis.eNid = msg[2];
			htnThis.accessKeyToDict = *((uint32_t*) (msg+7));
			htnThis.ctrlBits |= HTN_CTRL_REGOBJS;

			return;
		}

#if HTN_FUNC_IS_EDGE
		// step 3. perform if as a dictionary
		return; // do nothing

#elif HTN_FUNC_IS_ROUTER
		// step 4.1 save the mapping from dictId to nodeId by taking worst hops and powerLevel
		HtCache_add_eNid(msg[2], theParty);

		// step 4.2 forward the msg to the downlink
		HtRouter_forward(msg, msglen, (NodeId_t*)&theParty);

		// step 4.3 set the orphan's uplink if this is the last hop to the dest
		if ((ERR_SUCCESS == HtCache_nextHop(msg[2], &nh)) && (theParty == nh.nextNid))
			HtNode_sendSETOBJ(msg[2], ODID_uplink, 0, (uint8_t*)&nidThis, sizeof(NodeId_t));

		return;
#else // HTN_FUNC_IS_ROUTER
		return;
#endif // HTN_FUNC

	case HTCMSG_GETOBJ:  // always from uplink to downlink
		//  0          1           2           3           4          5          6          7          8          9
		// +----------+-----------+-----------+-----------+----------+----------+----------+----------+----------+----------+
		// |  GETOBJ  |0or4 | TTL |  src-eNid | dest-eNid |  objId   |  subidx  | Clock-msec (if src-eNid==HTDICT_ENID_EDGE)| 
		// +----------+-----------+-----------+-----------+----------+----------+----------+----------+----------+----------+
		// step1. validate the checksum
		// ASSERT_MSG_CRC(8);
		// ObjectId:
		//		when the objectId=0x00, it is to list the objectIds of the node, can be fetched by the subindex
		//	    when the highest 4bits is [0000~00001], the 32 object ids can be indecied byte-by-byte, so that its coverage will be 250*1 bytes by a one-byte subindex
		//	    when the highest 4bits is [0010~00011], the 32 object ids can be indecied dword-by-dword, so that the coverage is 250*4=1000 bytes by subidx
		//	    when the highest 4bits is 0011, the 16 object ids can be indecied qword-by-qword, so that the coverage is 250*8=2000 bytes by subidx
		//	ObjectSubIdx: 1byte, 0xfa~0xff is reserved
		//		0xff is reserved to read the objectType of the object, up to vlen to determine length of the typeid
		//		0xfe is reserved to read the uri of the object, up to vlen to determine length of the uri string

#ifndef LOOPBACK_TEST
		// filter the loopback message
		if (htnThis.eNid == msg[2])
			return;
#endif // LOOPBACK_TEST

		// step1. clock-sync with the Edge
		if (HTDICT_ENID_EDGE == *(msg +HTCMSG_HEADER_LEN) && payloadLen >= (OBJ_VAL_POSITION + sizeof(uint32_t)))
			HtNode_doSyncClock(netif, *(uint32_t*) (msg +OBJ_VAL_POSITION));

		// step2. response of the get is about this node 
		if (htnThis.eNid == msg[3]) // post the value to htnThis.uplink.nextNid
		{
			// trigger the response with the value expected via 
			if (ERR_SUCCESS == HtNode_reportValToDest(msg[2], msg[4], msg[5]))
				HtNode_OnObjectQueried(netif, msg[4], msg[5]); 
			return;
		}

		// step4. forward if as a router
#if HTN_FUNC_IS_ROUTER
		HtRouter_forward(msg, msglen, NULL);
#endif // HTN_FUNC_IS_ROUTER

		return;

	case HTCMSG_RPTOBJ: // from downlink to uplink
		//  0          1           2           3           4          5          6                     
		// +----------+-----------+-----------+-----------+----------+----------+---...----+----------+----------+
		// |  RPTOBJ  | vlen| TTL |  src-eNid | dest-eNid |  ObjId   |  subIdx  | value up to ObjRange|   sign   |
		// +----------+-----------+-----------+-----------+----------+----------+---...----+----------+----------+

#ifndef LOOPBACK_TEST
		// filter the loopback message
		if (htnThis.eNid == msg[2] || htnThis.eNid == msg[3])
			return;
#endif // LOOPBACK_TEST

		if (htnThis.eNid == msg[3] || 0xff == msg[3])
		{
			HtNode_OnObjectValues(msg[2], msg[4], msg[5], msg +OBJ_VAL_POSITION, payloadLen -OBJ_VAL_POSITION);
			return;
		}

#if HTN_FUNC_IS_ROUTER
		// step3. perform if as a router
		HtUplink_forward(msg, msglen);
#endif // HTN_FUNC_IS_ROUTER
		return;

	case HTCMSG_SETOBJ: // frpm uplink to downlink
		//  0          1           2           3           4          5          6                     
		// +----------+-----------+-----------+-----------+----------+----------+---...----+----------+----------+
		// |  SETOBJ  | vlen| TTL |  src-eNid | dest-eNid |  ObjId   |  subIdx  | value up to ObjRange|   sign   |
		// +----------+-----------+-----------+-----------+----------+----------+---...----+----------+----------+

#ifndef LOOPBACK_TEST
		// filter the loopback message
		if (htnThis.eNid == msg[2])
			return;
#endif // LOOPBACK_TEST

		if (msg[3] == htnThis.eNid) // this is the node
		{
			// TODO: valicate the signature
			// set the object value
			HtNode_doSetObject(netif, msg[4], msg[5], msg +OBJ_VAL_POSITION, payloadLen - (OBJ_VAL_POSITION -HTCMSG_HEADER_LEN));
			return;
		}

#if HTN_FUNC_IS_ROUTER
		HtRouter_forward(msg, msglen, NULL);
#endif // HTN_FUNC_IS_ROUTER

		return;
	}
}

hterr HtNode_sendOBJOP(uint8_t GSR, uint8_t desteNid, uint8_t objId, uint8_t subIdx, const uint8_t* value, uint8_t vlen)
{
	//  0          1           2           3           4          5          6                     
	// +----------+-----------+-----------+-----------+----------+----------+---...----+----------+----------+
	// |RPT/SETOBJ| vlen| TTL |  src-eNid | dest-eNid |  ObjId   |  subIdx  | value up to ObjRange|   sign   |
	// +----------+-----------+-----------+-----------+----------+----------+---...----+----------+----------+
#ifdef heap_malloc
	uint8_t* msg = NULL;
#else
	uint8_t msg[HTDDL_MSG_LEN_MAX];
#endif // heap_malloc

	uint8_t i=0, ret=0;
	HtNextHop nextHop;

	vlen &= 0x0f;
	vlen = min(vlen, (HTDDL_MSG_LEN_MAX -6 - OBJ_SIGNATURE_LEN));

	if (HTCMSG_GETOBJ != GSR && (NULL == value || vlen <=0))
		return ERR_OPERATION_ABORTED;

	if (ERR_SUCCESS != HtCache_nextHop(desteNid, &nextHop))
	{
		if ((HTCMSG_RPTOBJ == GSR) && (HTDICT_ENID_EDGE == desteNid))
			nextHop = htnThis.uplink; // go thru the uplink if it is report to the Edge
		else return ERR_NOT_FOUND;
	}

#ifdef heap_malloc
	msg = heap_malloc(8+vlen);
#endif // heap_malloc

	if (NULL == msg)
		return ERR_NOT_ENOUGH_MEMORY;

#if HTN_FUNC_IS_EDGE
	if (HTCMSG_GETOBJ == GSR)
	{
		vlen  = 4;
		value = (const uint8_t*) &gClock_msec;
	}
#endif // HTN_FUNC_IS_EDGE

	msg[i++] = GSR;
	msg[i++] = ((vlen+(6-2)) <<4) | (HTC_TTL_MAX&0x0f); // value len to message len
	msg[i++] = htnThis.eNid; // src-eNid
	msg[i++] = desteNid; // dest-eNid
	msg[i++] = objId;
	msg[i++] = subIdx;

	while (vlen--)
	{
		msg[i++] = *value++;
		// sign =
	}

#if OBJ_SIGNATURE_LEN >0
	msg[i++] = *(((uint8_t*) &sign)); 
#endif
#if OBJ_SIGNATURE_LEN >1
	msg[i++] = *(((uint8_t*) &sign)+1); 
#endif // WITH_OBJ_SIGNATURE

	ret = HtNode_pvSendMSG(&nextHop, msg, i, i, 0);
#ifdef heap_malloc
	heap_free(msg);
#endif // heap_malloc
	return ret;
}

hterr HtNode_advertize(uint8_t maxHops, uint8_t unicast)
{
	//  0          1           2           3           4          5          6          7          8          9          10              
	// +----------+-----------+-----------+-----------+----------+----------+----------+----------+----------+----------+----------+
	// |  NEIGHB  |  00 | TTL |    eNid   |                 srcNodeId                  |                   orphanNodeId            |
	// +----------+-----------+-----------+-----------+----------+----------+----------+----------+----------+----------+----------+
#define PATLOAD_LEN (1+4+4)

#ifdef heap_malloc
	uint8_t* msg = NULL;
#else
	uint8_t msg[HTDDL_MSG_LEN_MAX];
#endif // heap_malloc

	uint8_t i=0, ret=0;
	HtNextHop nextHop = htnThis.uplink;
	if (!unicast)
		nextHop.nextNid = HTN_NODEID_MCAST;

#ifdef heap_malloc
	msg = heap_malloc(2 + PATLOAD_LEN);
	if (NULL == msg)
		return ERR_NOT_ENOUGH_MEMORY;
#endif // heap_malloc

	if (maxHops > HTC_TTL_MAX)
		maxHops = HTC_TTL_MAX;

	msg[i++] = HTCMSG_NEIGHB;
	msg[i++] = (PATLOAD_LEN<<4) | (maxHops &0x0f);
	msg[i++] = htnThis.eNid;
	*((NodeId_t*)(msg+i)) = nidThis;   i += sizeof(NodeId_t);
	*((NodeId_t*)(msg+i)) = nidThis;   i += sizeof(NodeId_t);

	ret = HtNode_pvSendMSG(&nextHop, msg, i, i, 0);
#ifdef heap_malloc
	heap_free(msg);
#endif // heap_malloc
	return ret;
}

hterr HtEdge_sendADOPT(HtNetIf* netif, uint8_t eNid, NodeId_t destNodeId, NodeId_t thruNodeId, uint32_t accessKey)
{
	//  0          1           2           3           4          5          6          7          8          9          10              
	// +----------+-----------+-----------+-----+-----+----------+----------+----------+----------+----------+----------+----------+
	// |   ADOPT  |  00 | TTL |   eNid    |                 orphanNodeId               |               accessKey                   |
	// +----------+-----------+-----------+-----+-----+----------+----------+----------+----------+----------+----------+----------+
	// step1. validate the checksum
	// ASSERT_MSG_CRC(11);

#ifdef heap_malloc
	uint8_t* msg = NULL;
#else
	uint8_t msg[HTDDL_MSG_LEN_MAX];
#endif // heap_malloc

	uint8_t ret=0, plen=0;

	HtNextHop nextHop;
	nextHop.netif   = netif;
	nextHop.nextNid = thruNodeId;

#ifdef heap_malloc
	msg = heap_malloc(HTDDL_MSG_LEN_MAX);
#endif // heap_malloc
	if (NULL == msg)
		return ERR_NOT_ENOUGH_MEMORY;

	msg[0] = HTCMSG_ADOPT;

	plen = 1 + sizeof(NodeId_t);
	msg[2] = eNid;
	memcpy(msg+3, &destNodeId, sizeof(destNodeId));

	if (0 != accessKey)
	{
		plen += sizeof(uint32_t);
		memcpy(msg +3 + sizeof(destNodeId), (const uint8_t*) &accessKey, sizeof(uint32_t));
	}

	msg[1] = (plen<<4) | (HTC_TTL_MAX & 0x0f);

	ret = HtNode_pvSendMSG(&nextHop, msg, plen+2, HTDDL_MSG_LEN_MAX, 0);
#ifdef heap_malloc
	heap_free(msg);
#endif // heap_malloc
	return ret;
}

hterr HtNode_pvSendMSG(const HtNextHop* nextHop, uint8_t* msg, uint16_t msglen, uint16_t msgmax, uint8_t flags)
{
	pbuf* packet=NULL;
	if (NULL == nextHop || NULL == nextHop->netif)
		return ERR_INVALID_PARAMETER;

	// step1. calculate the crc8
	// if ((flags & SIGN) && msgmax>msglen)
	// msg[msglen] = calcCRC8(msg, msglen); msglen++ 

	//	return (htddl_doTX(nextHop.netif, (uint8_t*) pDestNodeId, msg, msglen) >=msglen) ? ERR_SUCCESS: ERR_IO;
	packet = pbuf_mmap(msg, msglen);
	// flags is take as ret in the following
	if (packet)
	{
		flags = NetIf_doTransmit(nextHop->netif, packet, &nextHop->nextNid);
		pbuf_free(packet); packet=NULL;
	}

	return flags;
}

static uint32_t eventMask2Send =0;

hterr HtNode_triggerEvent(uint8_t eventId)
{
	uint32_t mask =1;
	uint8_t  i =0;
	for (i=0, mask =1; ODID_NONE != eventDict[i].idEvent && i <sizeof(eventMask2Send)*8; i++, mask <<=1)
	{
		if (eventDict[i].idEvent == eventId)
		{
			eventMask2Send |= mask;
			return ERR_SUCCESS;
		}
	}

	return ERR_NOT_FOUND;
}

static hterr HtNode_listObjsToDest(uint8_t desteNid, uint8_t startSubIdx)
{
	uint8_t i,j;
	uint8_t buf[OBJ_VALS_PAYLOAD_LEN_MAX] = {0};
	for (i=0; i<startSubIdx && ODID_NONE != ObjDict[i].idObject; i++);

	for (j=0; j< sizeof(buf) && ODID_NONE != ObjDict[i].idObject; i++, j++)
	{
		buf[j] = ObjDict[i].idObject;
	}

	return HtNode_sendRPTOBJ(desteNid, 0x00, startSubIdx, buf, j);
}

hterr HtNode_reportValToDest(uint8_t desteNid, uint8_t objId, uint8_t subIdx)
{
	MemoryRange mr ={NULL, 0};
	const OD_Object* pOD = NULL;
	hterr ret = ERR_SUCCESS;

	// when the objectId=0x00, it is to list the objectIds of the node, can be fetched by the subindex
	if (0 == objId)
		return HtNode_listObjsToDest(desteNid, subIdx);

	ret = HtOD_locateObj(objId, subIdx, &mr, &pOD);

	if (ERR_SUCCESS != ret || mr.len<=0)
		return ret;

	// found a object as parameter, send its value
	return HtNode_sendRPTOBJ(desteNid, objId, subIdx, (uint8_t*)mr.addr, (uint8_t)mr.len);
}

hterr HtNode_doSetObject(HtNetIf* netif, uint8_t objectId, uint8_t subIdx, const uint8_t* value, uint8_t vallen)
{
	hterr ret = HtOD_doSetObject(objectId, subIdx, value, vallen);
	if (ERR_SUCCESS != ret)
		return ret;

	switch(objectId) // for some reserved Put
	{
	case ODID_uplink: // this must follow HTCMSG_ADOPT, turns to ADOPTED by resetting timeoutUnlink
#if !HTN_FUNC_IS_EDGE
		if (NULL != netif)
			htnThis.uplink.netif = netif; // the netif thru which to set uplink must be the netif to the unlink

		htnThis.timeoutUnlink = TIMEOUT_RELOAD_Unlink;
		if (HTDICT_ENID_INVALID == htnThis.eNid)
			htnThis.timeoutUnlink =0;
#endif // !HTN_FUNC_IS_EDGE

		break;

	case ODID_clockmsec:
		// TODO: adjust the clock of chip
		break;
	}

	// callback the upperlayers
	HtNode_OnObjectChanged(netif, objectId, subIdx, value, vallen);
	return ret;
}

#define HtNode_sendHeartbeat(NIC)    HtNode_triggerEvent(ODEvent_Heartbeat)
const uint8_t EVENTOID_OF_Heartbeat[] = {ODID_nodeId, ODID_NONE};

// should be executed every 1 msec
void HtNode_do1msecScan() // HtNetIf* netif)
{
	uint8_t i;
	uint16_t j=1;
	static uint8_t _msec = 20;
	static uint8_t _n20msec = 0;

	KLV eklvs[TEXTMSG_ARGVS_MAX];
	uint8_t klvc= sizeof(eklvs)/ sizeof(KLV);

	// step 0. read the clock
	// gClock_msec = getTickXmsec();

	// step 1. decrease the timeouts
	if (0 == htnThis.timeoutUnlink--)
		htnThis.timeoutUnlink =0;

	htddl_doTimerScan();

	// step 2. scan and issue the outgoing events
	for (i=0; ODEvent_NONE != eventDict[i].idEvent && i<16; i++)
	{
		const uint8_t* pId= NULL;

		j = (1<<i);
		if (0 == (j & eventMask2Send))
			continue;

		eventMask2Send &= ~j;

		klvc = HtOD_composeEvent(&eventDict[i], eklvs, klvc);
		if (klvc <=0)
			continue;

		// report the full variable until end, TODO: should be replaced by taking eklvs[i]
		for (pId=eventDict[i].objIds; NULL !=pId && ODID_NONE != *pId; pId++)
		{
			for (j=0; j < ODSubIdx_ReserveStart; j++)
			{
				if (ERR_SUCCESS != HtNode_reportValToDest(HTDICT_ENID_EDGE, *pId, (uint8_t)j))
					break;
			}
		}

#ifdef HTN_EVENT_POST_HOOK
		HtNode_OnEventIssued(klvc, eklvs);
#endif // HTN_EVENT_POST_HOOK

	}

	// step 2.1 see if it is necessary to report the object list to the Edge
	if (HTN_CTRL_REGOBJS & htnThis.ctrlBits && HTDICT_ENID_INVALID != htnThis.eNid)
	{
		const OD_Object* pObj = NULL;
		htnThis.ctrlBits &= ~((uint32_t) HTN_CTRL_REGOBJS);

		for (pObj=ObjDict; NULL != pObj && ODEvent_NONE != pObj->idObject; pObj++)
		{
//			HtNode_sendREGOBJ(pObj);
		}
	}

	// step 3. scan the SDO clients every 20msec
	if (_msec--)
		return;

	_msec = 20;

#if HTN_FUNC_HAS_DOWNLINK 
	// TODO: timeout the iterations in LRUMaps of downlinks
#endif // HTN_FUNC_HAS_DOWNLINK

	// step 3. issue heartbeat every 5sec = 255 * 20msec
	if (_n20msec--)
		return;

	// advertize orphan if this is not adopted, otherwise heartbeat to the uplink
#if !HTN_FUNC_IS_EDGE
	if (0 == htnThis.timeoutUnlink)
	{
		htnThis.nodeFuncs |= HTN_FUNC_ORPHAN; // turn on the flag per this is an orphan
		if (htnThis.uplink.nextNid || HTDICT_ENID_INVALID != htnThis.eNid)
		{
			// force to reset uplink/eNid, then callback upper layer 
			htnThis.uplink.nextNid = 0x0000;
			htnThis.eNid = HTDICT_ENID_INVALID;
			HtNode_OnObjectChanged(NULL, ODID_uplink, 0, (const uint8_t*)&htnThis.uplink.nextNid, sizeof(NodeId_t));
		}
	
		HtNode_advertize(HTC_TTL_MAX, 0);
	}
	else
	{
		HtNode_advertize(0, 0); // the single mcast hop to the direct neighbors only
		// HtNode_advertize(HTC_TTL_MAX, 1); // the unicast hops thru unlink chains
	}
#elif defined(LOOPBACK_TEST)
	// loopback test at EdgeNode
	HtNode_advertize(0, 0);
#endif // !HTN_FUNC_IS_EDGE
}

