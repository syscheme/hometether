#ifndef __HT_CLUSTER_H__
#define __HT_CLUSTER_H__

#define HTN_FUNC_SENSOR     FLAG(0)
#define HTN_FUNC_ACTOR      FLAG(1)
#define HTN_FUNC_ROUTER     FLAG(2)
#define HTN_FUNC_EDGE       FLAG(3)
#define HTN_FUNC_ORPHAN     FLAG(7)

#ifndef OBJ_SIGNATURE_LEN
#  define OBJ_SIGNATURE_LEN (0) // default no signature
#endif //

#ifndef HTN_FUNC_TYPE
#  define HTN_FUNC_TYPE  (HTN_FUNC_SENSOR | HTN_FUNC_ROUTER | HTN_FUNC_EDGE)
#endif // HTN_FUNC_TYPE

#define HTN_FUNC_HAS_DOWNLINK   (HTN_FUNC_TYPE & (HTN_FUNC_ROUTER | HTN_FUNC_EDGE))
#define HTN_FUNC_IS_EDGE        (HTN_FUNC_TYPE & HTN_FUNC_EDGE)
#define HTN_FUNC_IS_ROUTER      (HTN_FUNC_TYPE & HTN_FUNC_ROUTER)

#include "htddl.h"
#include "htod.h"

typedef uint32_t NodeId_t; // in htcluster space, the nodeId is 4byte
#define HTN_NODEID_MCAST        ((NodeId_t) 0xffffffff)

#define HTCLU_CMD_MASK    (0xf0 & HTDDL_CMD_MASK)

enum {
	// extentions to HTCMD of htddl
	HTCLU_STARTCMD=0x10, 
	HTCLU_NEIGHB =HTCLU_STARTCMD, 
	HTCLU_ADOPT, 
	HTCLU_GETOBJ, 
	HTCLU_RPTOBJ, 
	HTCLU_SETOBJ 
};

#define HTC_TTL_MAX         (4)

#define HTCMSG_NEIGHB       HTDDL_REQ(HTCLU_NEIGHB)  // neigbor advertisement
#define HTCMSG_ADOPT 	    HTDDL_REQ(HTCLU_ADOPT)
#define HTCMSG_GETOBJ	    HTDDL_REQ(HTCLU_GETOBJ)
#define HTCMSG_RPTOBJ       HTDDL_REQ(HTCLU_RPTOBJ)
#define HTCMSG_SETOBJ       HTDDL_REQ(HTCLU_SETOBJ)

// the reserved EdgeNodeIds
#define HTDICT_ENID_INVALID  (0x00) // the invalid value
#define HTDICT_ENID_EDGE     (0xff) // the dict nodeId of the HtEdge itself

// ---------------------------------------------------------------------------
// HtNode
// ---------------------------------------------------------------------------
typedef struct _HtNextHop
{
	NodeId_t nextNid;
	HtNetIf* netif;
} HtNextHop;

// ---------------------------------------------------------------------------
// HtNode
// ---------------------------------------------------------------------------
typedef struct _HtNode
{
	uint8_t   eNid;        // stand for "nodeId in Edge" that assigned by the HtEdge, dnid below
	HtNextHop uplink;           // the nodeId to the unique unlink node
	uint8_t   nodeFuncs;        // combination of HTN_FUNC_xxxx
	uint32_t  accessKeyToDict;  // assigned by the HtEdge
	__IO__ uint16_t  timeoutUnlink;
	__IO__ uint16_t  ctrlBits;  // the control bits, combiantion of HTN_CTRL_XXXX
} HtNode;

#define HTN_CTRL_REGOBJS     FLAG(0)

#define TIMEOUT_RELOAD_Unlink  (10000) // 10sec

extern NodeId_t nidThis;
extern HtNode   htnThis;

void  HtNode_init(HtNetIf* netifUpLink);

// about object value access
hterr HtNode_sendOBJOP(uint8_t GSR, uint8_t desteNid, uint8_t objId, uint8_t subIdx, const uint8_t* value, uint8_t vlen);
#define HtNode_sendSETOBJ(desteNid, objId, subIdx, value, vlen)           HtNode_sendOBJOP(HTCMSG_SETOBJ, desteNid, objId, subIdx, value, vlen)
#define HtNode_sendRPTOBJ(desteNid, objId, subIdx, value, vlen)           HtNode_sendOBJOP(HTCMSG_RPTOBJ, desteNid, objId, subIdx, value, vlen)
#define HtNode_sendGETOBJ(desteNid, objId, subIdx)                        HtNode_sendOBJOP(HTCMSG_GETOBJ, desteNid, objId, subIdx, NULL,  0)
hterr HtNode_reportValToDest(uint8_t desteNid, uint8_t objId, uint8_t subIdx);

// ---------------------------------------------------------------------------
// entries to drive HtNode
// ---------------------------------------------------------------------------
void HtNode_procMsg(HtNetIf* netif, uint8_t* msg, uint16_t msglen, uint8_t powerLevel);
// should be executed every 1 msec
void HtNode_do1msecScan(void); // HtNetIf* netif);

// ---------------------------------------------------------------------------
// portal for HtNode
// ---------------------------------------------------------------------------
#define HTN_EVENT_POST_HOOK

void HtNode_OnObjectQueried(HtNetIf* netif, uint8_t objectId, uint8_t subIdx);
void HtNode_OnObjectChanged(HtNetIf* netif, uint8_t objectId, uint8_t subIdx, const uint8_t* newValue, uint8_t vlen);

// remote object values
void HtNode_OnObjectValues(uint8_t dictNid, uint8_t objectId, uint8_t subIdx, uint8_t* value, uint8_t vlen);
#ifdef HTN_EVENT_POST_HOOK
void HtNode_OnEventIssued(uint8_t klvc, const KLV eklvs[]);
#endif // HTN_EVENT_POST_HOOK

// ---------------------------------------------------------------------------
// portal for HtEdge
// ---------------------------------------------------------------------------
#if HTN_FUNC_IS_EDGE
void HtEdge_OnNodeAdvertize(HtNetIf* netif, uint8_t eNid, NodeId_t theNode, NodeId_t nidBy);

hterr HtEdge_sendADOPT(HtNetIf* netif, uint8_t eNid, NodeId_t destNodeId, NodeId_t thruNodeId, uint32_t accessKey);
#endif // HTN_FUNC_IS_EDGE

// -----------------------------
// Some reserved data object
// -----------------------------
enum {
	ODID_dnid   = ODID_reserved04,      // always maps to htnThis.DNID, 1Byte, read only
//	ODID_nodeState, // always maps to htnThis.nodeState, 1Byte, read only, refer to HTNode_State
	ODID_uplink = ODID_reserved05,    // always maps to htnThis.nidUplink, 4Byte, read/write
	// muted: ODID_accessKey, // always maps to htnThis.accessKeyToDict, 4Bytes, 
};

#define HTNODE_OD_DECLARE_START()    USR_OD_DECLARE_START() \
		OD_DECLARE(dnid,      ODTID_BYTE,  ODF_Readable,               &htnThis.eNid, 1 ) \
		OD_DECLARE(uplink,    ODTID_DWORD, ODF_Readable|ODF_Writeable, &htnThis.uplink.nextNid, sizeof(NodeId_t) )
		// muted: OD_DECLARE(ODID_accessKey,      ODF_Readable|ODF_Writeable, 0, "uplink",    &htnThis.nidUplink, sizeof(NodeId_t) ), 
		// OD_DECLARE(nodeState, ODTID_BYTES, ODF_Readable,               &htnThis.nodeState, 1 ) \

#define HTNODE_OD_DECLARE_END() USR_OD_DECLARE_END()

// sample OD declaration:
// uint16_t data1, data2;
// enum {
// ODID_data1 = ODID_USER_MIN, ODID_data2
// };

// USR_OD_DECLARE_START()
// 	OD_DECLARE(data1,  0x0001, ODF_Readable, &data1, 2)
// 	OD_DECLARE(data2,  0x0001, ODF_Readable, &data1, 2)
// USR_OD_DECLARE_END()

#define ODEVENT_DECLARE(EVENTNAME, OBJIDS)   { ODEvent_##EVENTNAME, #EVENTNAME, OBJIDS},
#define USR_ODEVENT_DECLARE_START() \
	uint32_t eventFlags =0; \
	const OD_Event eventDict[] = { \
		ODEVENT_DECLARE(Heartbeat, EVENTOID_OF_Heartbeat)
#define USR_ODEVENT_DECLARE_END()   ODEVENT_DECLARE(NONE, NULL) };

hterr HtNode_triggerEvent(uint8_t eventId);

// sample OD declaration:
// typedef enum {
//	ODEvent_USER_Event1 = ODEvent_USER_MIN, 
// } EventId;
//
// static const uint8_t event_objIds[] = {ODID_data1, ODID_data2, ODID_NONE};
// USR_ODEVENT_DECLARE_START()
// 	ODEVENT_DECLARE(ODEvent_USER_Event1, event_objIds)
// USR_ODEVENT_DECLARE_END()

/*
// ---------------------------------------------------------------------------
// HTNode_State
// ---------------------------------------------------------------------------
typedef enum _HTNode_State
{
	ens_Initialisation  = 0,    // node is being initialized and not operational
	ens_Disconnected,           // node completed initialization but appears as an orphan with no uplink
	ens_Adopting,               // node is adopted but not yet complete handshaking
	ens_Operational,            // node has been adopted with a qualified uplink, and becomes opertional
	ens_Stopping        = 0x3F, // node is about to stop
	ens_Unknown         = 0x7F
} HTNode_State;
*/

#endif // __HT_CLUSTER_H__
