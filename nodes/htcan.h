#ifndef __HTCAN_H__
#define __HTCAN_H__

#include "htcomm.h"

#include <stdio.h>
#include <string.h>

#define HT_NODEID             (0x10)
#define HTCAN_COB_NODEID_MASK (0x7f) // 7bit node id from cobid

// Object Dictionary definition
// -----------------------------
#define ODFlag_Readable   (1<<4)
#define ODFlag_Writeable  (1<<5)

enum _eODType
{
	ODType_unknown = 0,
	ODType_uint8, ODType_uint16, ODType_fifo	
};

typedef struct _ODSubIdx
{
	uint8_t  dateType; // bit 0-3: datatype
	const char* alias;
	const void* pVar;
} ODSubIdx;

typedef struct _ODObj
{
	uint16_t    objIndex;
	uint8_t     accessType; // ODFlag_Readable | ODFlag_Writeable
	//	uint8_t     count;
	const char* uri;
	const ODSubIdx*   subIdxes; 
} ODObj;

extern const ODObj objDict[];

// PDO definition
// -----------------------------
typedef struct _PDOParam
{
	const char*   key;
	const uint8_t type; // ODType_uint8 or ODType_uint16
	void* const   pLocalVar;
} PDOParam;

typedef struct _PDOObj
{
	uint16_t        funcCode;
	const char*     uri;
	const PDOParam* params;
	uint8_t         syncReload;
} PDOObj;

extern const PDOObj pdoDict[];

typedef struct {
	uint16_t cob_id;	
	uint8_t rtr;		// remote transmission request. (0 if not rtr message, 1 if rtr message)
	uint8_t len;		// message's length (0 to 8)
	uint8_t data[8];    // message's data
} HTCanMessage;

extern const uint8_t  HtCan_thisNodeId;

// The nodes states 
// -----------------
// values are choosen so, that they can be sent directly for heartbeat messages...
// Must be coded on 7 bits only
typedef enum _eCanState {
	ecs_Initialisation  = 0x00, 
	ecs_Disconnected    = 0x01,
	ecs_Connecting      = 0x02,
	ecs_Preparing       = 0x02,
	ecs_Stopped         = 0x04,
	ecs_Operational     = 0x05,
	ecs_Pre_operational = 0x7F,
	ecs_Unknown_state   = 0x0F
} eCanState;

enum {
	nmt_startRemote =1,
	nmt_stopRemote =2,
	nmt_enterPreOp =128,
	nmt_resetNode  =129,
	nmt_resetComm  =130
};

enum {
	HtCanState_Bootup         = ecs_Initialisation,
	HtCanState_Stopped        = ecs_Stopped,
	HtCanState_Operational    = ecs_Operational,
	HtCanState_PreOperational = ecs_Pre_operational
};

#define NEIGHBOR_MAX_NODES (8)
typedef struct _NodeState
{
	uint8_t nodeId;
	uint8_t canState; // value of eCanState
	uint8_t timeout;
} NodeState;

extern NodeState neighborhood[NEIGHBOR_MAX_NODES];

#define SDO_CMD_SET      (0x20) // 
#define SDO_CMD_GET      (0x40)
#define SDO_RESP_SET     (0x22)
#define SDO_RESP_GET     (0x42)
#define SDO_RESP_ERR     (0x80)

// APIs that user may invoke or portal may trigger
// -----------------------------
void HtCan_init(uint8_t fdCAN);
void HtCan_stop(uint8_t fdCAN);
void HtCan_doScan(uint8_t fdCAN);
void HtCan_flushOutgoing(uint8_t fdCAN);

// portal entry to trigger this HTCan stack
void HtCan_processReceived(uint8_t fdCAN, const HTCanMessage* m);

// utility func exported from this stack
void sendHeartbeat(uint8_t fdCAN);
void triggerPdo(uint8_t pdoId);
int setSDO_async(uint8_t fdCAN, uint8_t nodeId, uint16_t objIndex, uint8_t subIndex, uint8_t* value, uint8_t valueLen, void* pCtx);
int getSDO_async(uint8_t fdCAN, uint8_t nodeId, uint16_t objIndex, uint8_t subIndex, void* pCtx);
uint8_t HtCan_send(uint8_t fdCAN, const HTCanMessage* m);

// APIs about converting between compact text message and HTCanMessage
// -------------------------------------------------------------------
// where the compact message format is: can:<cobid>{R|N}<len>V[data bytes], such as "can:680N3V050000"
uint8_t HtCan_parseCompactText(const char* msg, HTCanMessage* m);
char* HtCan_toCompactText(char* msg, int maxLen, const HTCanMessage* m);

typedef enum { VERB_GET, VERB_PUT, VERB_POST, VERB_MAX } HT_VERB_t;
extern const char* Verbs[];


// APIs about converting between rich URI and HTCanMessage
// -------------------------------------------------------------------
// where the rich URI is in format of {GET|PUT|POST}: ht:<nodeId>/<uri>?[<param>[=<value>][&...]]
// such as: "GET ht:10/global?gstate", "PUT ht:10/global?gstate=1&masterId=2"
char* HtCan_toRichURI(char* uri, uint8_t maxLen, const HTCanMessage* m);

typedef void (*HtCan_OnFillCanMsg_t) (HTCanMessage* pMsg, void* pCtx);
bool HtCan_parseRichURIEx(uint8_t verb, const uint16_t nodeId, const char* objectUri, int cparams, const char* params[], const char* values[], HtCan_OnFillCanMsg_t fFillMsg, void* pCtx);
// bool  HtCan_parseRichURIEx(uint8_t verb, const uint16_t nodeId, const char* objectUri, int cparams, const char* params[], char* values[], HtCan_OnFillCanMsg_t fFillMsg, void* pCtx);

char* HtCan_toJsonVars(char* txtMsg, uint8_t maxLen, const HTCanMessage* m);

// APIs that portal needs to implement
// ------------------------------------
// portal func to call by this HTCan stack to send a message thru CAN bus
uint8_t doCanSend(uint8_t fdCAN, const HTCanMessage* m);
void    doResetNode(void);

// Events that portal may needs to hook
// ------------------------------------
void OnHeartbeat(uint8_t fdCAN, uint8_t fromWhom, uint8_t canState, uint8_t gState, uint8_t masterId);
// void OnSdoUpdated(const ODObj* odEntry, uint8_t subIdx, uint8_t updatedCount);
void OnSdoData(uint8_t fdCAN, const HTCanMessage* m, bool localData);
void OnSdoClientDone(uint8_t fdCAN, uint8_t errCode, uint8_t peerNodeId, uint16_t objIdx, uint8_t subIdx, uint8_t* data, uint8_t dataLen, void* pCtx);
void OnPdoEvent(uint8_t fdCAN, const HTCanMessage* m, bool localEvent);
void OnClock(uint8_t fdCAN, uint32_t secSinc1984Jan1);

//  Function Codes, defined in the canopen DS301 
typedef enum _CO_FuncCode
{
	fcNMT = 0x0, fcSYNC, fcTIME_STAMP, fcPDO1tx, fcPDO1rx, fcPDO2tx, fcPDO2rx,
	fcPDO3tx, fcPDO3rx, fcPDO4tx, fcPDO4rx, fcSDOtx, fcSDOrx, fcNODE_GUARD, fcLSS
} CO_FuncCode;

#endif // __HTCAN_H__
