// #include "secu.h"
#include "htcan.h"
#include "../htod.h"

#define LittleEndU16(_X) (_X)
NodeState neighborhood[NEIGHBOR_MAX_NODES];
#define NODE_HEARTBEAT_RELOAD (2)

static char* sdoToRichURI(char* uri, uint8_t maxLen, const HTCanMessage* m);
static char* pdoToRichURI(char* uri, uint8_t maxLen, const HTCanMessage* m);

static void processHeartbeat(uint8_t fdCAN, uint8_t nodeId, uint8_t remoteState, const uint8_t* extData, uint8_t extLen)
{
	int i, k =NEIGHBOR_MAX_NODES;

	//	if (nodeId == EcuSpec_thisNodeId)
	//		return;

	for (i =0; i <NEIGHBOR_MAX_NODES; i++)
	{
		if (nodeId == neighborhood[i].nodeId)
		{
			k= i;
			break;
		}

		// otherwise, take the first free-ed node
		if (k >=NEIGHBOR_MAX_NODES && neighborhood[i].timeout ==0)
			k = i; 
	}

	if (k < NEIGHBOR_MAX_NODES)
	{
		neighborhood[k].nodeId    = nodeId;
		neighborhood[k].canState  = remoteState;
		neighborhood[k].timeout   = NODE_HEARTBEAT_RELOAD;
	}

	OnHeartbeat(fdCAN, nodeId, remoteState, extData[0], extData[1]);
}

void processEmcy(uint8_t fdCAN, const HTCanMessage* m)
{
}

void processPdoRequest(uint8_t fdCAN, const HTCanMessage* m)
{
}

static uint8_t        canState;
static uint8_t flagsPDO2Send =0, syncPDOTimeouts[8];

void procSync()
{
	uint8_t mask =1;
	int i;
	for (i=0, mask =1; pdoDict[i].funcCode && i <sizeof(flagsPDO2Send)*2; i++, mask <<=1)
	{
		if (0 == syncPDOTimeouts[i])
		{
			syncPDOTimeouts[i] = pdoDict[i].syncReload; 
			flagsPDO2Send |= mask;
		}
		else syncPDOTimeouts[i]--;
	}
}

void triggerPdo(uint8_t pdoId)
{
	uint16_t mask =1;
	int i;
	for (i=0, mask =1; pdoDict[i].funcCode && i <sizeof(flagsPDO2Send)*2; i++, mask <<=1)
	{
		if (pdoDict[i].funcCode == pdoId)
		{
			flagsPDO2Send |= mask;
			break;
		}
	}
}

static char* pdoToRichURI(char* txtMsg, uint8_t maxLen, const HTCanMessage* m)
{
	int i=0;
	char *p=txtMsg;
	const PDOObj* pPDO = NULL;
	uint8_t k=0;

	if (NULL ==m || NULL ==txtMsg || maxLen <6)
		return NULL;

	k = m->cob_id >>7;

	for (i=0; pdoDict[i].funcCode && NULL == pPDO; i++)
	{
		if (pdoDict[i].funcCode == k)
		{
			pPDO = &pdoDict[i];
			break;
		}
	}

	if (NULL == pPDO)
		return NULL;

	// find a known PDO if reaches here
	p += snprintf(p, maxLen, "POST ht:%02x/%s?", m->cob_id& 0x7f, pPDO->uri);
	for (i=0, k=0; pPDO->params[i].key && k < m->len; i++)
	{
		switch(pPDO->params[i].type & 0x0f)
		{
		case ODType_uint8: 
			p += snprintf(p, txtMsg +maxLen -2 -p, "%s=%02x&", pPDO->params[i].key, m->data[k]);
			k++;
			break;
		case ODType_uint16:
			p += snprintf(p, txtMsg +maxLen -2 -p, "%s=%04x&", pPDO->params[i].key, *((uint16_t*)&m->data[k]));
			k++, k++;
			break;
		}

		if (txtMsg +maxLen -p <=4)
			break;
	}

	*(--p) = '\0';
	return txtMsg;
}

// return bytes of data filled into the given dataBuf
static int getOD(const ODObj* pOdObj, uint8_t subIdx, uint8_t* dataBuf, int8_t maxDataLen)
{
	uint8_t i=1, bCont=true;
	const ODSubIdx* pSubIdx = pOdObj->subIdxes;

	if (NULL == dataBuf || maxDataLen<=0 || NULL == pSubIdx)
		return 0; // out of range

	if (0 == (pOdObj->accessType & ODFlag_Readable))
		return 0;

	for (i =1; pSubIdx[i-1].pVar && i < subIdx; i++); // locate to the subIdx

	if (i != subIdx)
		return 0; // out of range

	for (i=0; i< maxDataLen && pSubIdx[subIdx-1].pVar && bCont; subIdx++)
	{
		switch(pSubIdx[subIdx-1].dateType & 0x03)
		{
		case ODType_uint8:
			*dataBuf++ = *((uint8_t*) pSubIdx[subIdx-1].pVar);
			i++;
			break;
		case ODType_uint16:
			if (i+2 > maxDataLen)
				bCont = false;
			else
			{
				*((uint16_t*) dataBuf) = *((uint16_t*) pSubIdx[subIdx-1].pVar);
				dataBuf++; dataBuf++;
				i++; i++;
			}
		}
	}

	return i;
}

static int setOD(const ODObj* pOdObj, uint8_t subIdx, uint8_t* dataBuf, int8_t maxDataLen)
{
	uint8_t i=0, bCont=true;
	const ODSubIdx* pSubIdx = pOdObj->subIdxes;

	if (NULL == dataBuf || maxDataLen<=0 || NULL == pSubIdx)
		return 0; // out of range

	if (0 == (pOdObj->accessType & ODFlag_Writeable))
		return 0;

	for (i =1; pSubIdx[i-1].pVar && i < subIdx; i++); // locate to the subIdx

	for (i=0; i< maxDataLen && pSubIdx[subIdx-1].pVar && bCont; subIdx++)
	{
		switch(pSubIdx[subIdx-1].dateType & 0x03)
		{
		case ODType_uint8:
			*((uint8_t*) pSubIdx[subIdx-1].pVar) = *dataBuf++;
			i++;
			break;
		case ODType_uint16:
			if (i+1 > maxDataLen)
				bCont = false;
			else
			{
				*((uint16_t*) pSubIdx[subIdx-1].pVar) = *((uint16_t*) dataBuf);
				dataBuf++; dataBuf++;
				i++; i++;
			}
		}
	}

	return i;
}

#define SDO_CLIENTS_MAX_TRANS_LEN   4
#define SDO_MAX_CLIENTS             8
// #define SDO_CLIENT_DEFAULT_TIMEOUT  (5000) // 5sec
#define SDO_CLIENT_DEFAULT_TIMEOUT  (500) // test
#define SDO_CLIENT_ERR_OK           0
#define SDO_CLIENT_ERR_TIMEOUT      1
#define SDO_PEER_ERR                0x0f

typedef struct _SdoClient
{
	uint16_t cobId;
	uint16_t objIndex;
	uint8_t  subIndex;
	uint8_t  dataLen;
	uint16_t msecLeft;
	uint8_t  data[SDO_CLIENTS_MAX_TRANS_LEN];
	void*    pCtx;
} SdoClient;

SdoClient sdoClients[SDO_MAX_CLIENTS];

#define SDO_CMD(_MSG) (_MSG).data[0]
#define SDO_OBJIDX(_MSG) *((uint16_t*) ((_MSG).data+1))
#define SDO_SUBIDX(_MSG) *((uint8_t*) ((_MSG).data+3))

uint8_t HtCan_send(uint8_t fdCAN, const HTCanMessage* m);

static char* sdoToRichURI(char* txtMsg, uint8_t maxLen, const HTCanMessage* m)
{
	int i =0, k=0;
	const ODObj* pSDO = NULL;
	const char* verb = "POST";
	char* p = txtMsg;

	// step 1. determine the verb
	switch(m->cob_id >>7)
	{
	case fcSDOtx:
	case fcSDOrx:
		if (SDO_CMD_SET == SDO_CMD(*m))
			verb = "PUT";
		else if (SDO_CMD_GET == SDO_CMD(*m))
			verb = "GET";
		break;
	}

	// step 2. find the object id in the dictionary
	for (i =0; objDict[i].objIndex< 0xffff && objDict[i].uri && SDO_OBJIDX(*m) >objDict[i].objIndex; i++);
	
	if (SDO_OBJIDX(*m) == objDict[i].objIndex)
		pSDO = &objDict[i];

	if (NULL == pSDO)
		return NULL; // no such a SDO known in the dictionary

	p += snprintf(p, maxLen-2, "%s ht:%02x/%s?", verb, m->cob_id & 0x7f, pSDO->uri);
	for (i=0, k=0; (txtMsg + maxLen -p-2)>2 && pSDO->subIdxes && pSDO->subIdxes[i].alias && k <(m->len-4); i++)
	{
		if (i+1 < SDO_SUBIDX(*m)) // skip those before the requested sub index
			continue;

		switch(pSDO->subIdxes[i].dateType & 0x0f)
		{
		case ODType_uint8: 
			p += snprintf(p, txtMsg +maxLen -p-2, "%s=%02x&", pSDO->subIdxes[i].alias, m->data[4+ k]);
			k++;
			break;
		case ODType_uint16:
			p += snprintf(p, txtMsg +maxLen -p-2, "%s=%04x&", pSDO->subIdxes[i].alias, *((uint16_t*)&m->data[4+k]));
			k++, k++;
			break;
		}
	}
	*(--p) = 0x00; // terminate the string by taking away the last char
	return txtMsg;
}


void processSdo(uint8_t fdCAN, const HTCanMessage* m)
{
	uint8_t dataLen=0;
	int i =0;

	HTCanMessage resp;
	resp.cob_id = m->cob_id;
	resp.rtr =0;
	resp.len =4;
	SDO_OBJIDX(resp) = SDO_OBJIDX(*m);
	SDO_SUBIDX(resp) = SDO_SUBIDX(*m);
	SDO_CMD(resp)    = SDO_RESP_ERR;

	switch (SDO_CMD(*m)) // SDO command
	{
	case SDO_CMD_SET: // set SDO
		if ((m->cob_id & 0x7f) != HtCan_thisNodeId)
			return; // ignore the request to other nodes

		for (i =0; objDict[i].subIdxes && SDO_OBJIDX(*m) >objDict[i].objIndex; i++);
		if (SDO_OBJIDX(*m) == objDict[i].objIndex && objDict[i].subIdxes) // find the object
		{
			memcpy(resp.data+4, m->data+4, m->len -4);
			dataLen = setOD(&objDict[i], SDO_SUBIDX(*m), resp.data+4, m->len -4);
			if (dataLen >0)
			{
				SDO_CMD(resp) = SDO_RESP_SET;
				resp.len = dataLen +4;
			}
		}
		break;

	case SDO_CMD_GET: // get SDO
		if ((m->cob_id & 0x7f) != HtCan_thisNodeId)
			return; // ignore the request to other nodes

		for (i =0; objDict[i].subIdxes && SDO_OBJIDX(*m) >objDict[i].objIndex; i++);
		if (SDO_OBJIDX(*m) == objDict[i].objIndex && objDict[i].subIdxes) // find the object
		{
			SDO_CMD(resp) = SDO_RESP_GET;
			dataLen = getOD(&objDict[i], SDO_SUBIDX(*m), resp.data+4, 4);
			if (dataLen >0)
			{
				SDO_CMD(resp) = SDO_RESP_GET;
				resp.len = dataLen +4;
			}
		}
		break;

	case SDO_RESP_ERR:
	case SDO_RESP_GET:
	case SDO_RESP_SET:
		resp.len =0; // no need to send any more response
		for (i =0; i <SDO_MAX_CLIENTS; i ++)
		{
			if (sdoClients[i].msecLeft >0 && m->cob_id == sdoClients[i].cobId && SDO_OBJIDX(*m) == sdoClients[i].objIndex && SDO_SUBIDX(*m) == sdoClients[i].subIndex)
			{
				// find the await client
				sdoClients[i].msecLeft = 10*1000; // ensure this client will not be free-ed by accident before OnSdoClientDone() completes
				sdoClients[i].dataLen = m->len -4;
				memcpy(sdoClients[i].data, m->data+4, sdoClients[i].dataLen);
				OnSdoClientDone(fdCAN, (SDO_RESP_ERR == SDO_CMD(*m)) ? SDO_PEER_ERR : SDO_CLIENT_ERR_OK,
					sdoClients[i].cobId & 0x7f, sdoClients[i].objIndex, sdoClients[i].subIndex, sdoClients[i].data, sdoClients[i].dataLen, sdoClients[i].pCtx);
				sdoClients[i].msecLeft =0; // free this await client
			}
		}

		OnSdoData(fdCAN, m, FALSE);
		break;

	default:
		break; // ignore 
	}

	if (resp.len>0)
	{
		HtCan_send(fdCAN, &resp);
		OnSdoData(fdCAN, &resp, TRUE);
	}
}

SdoClient* newSdoClient(uint16_t cobId, uint16_t objIndex, uint8_t subIndex, uint16_t timeout, void* pCtx)
{
	SdoClient* pClient =NULL;
	int i =0;
	if (timeout<=0)
		timeout = SDO_CLIENT_DEFAULT_TIMEOUT;

	for (i =0; i <SDO_MAX_CLIENTS; i ++)
	{
		if (cobId == sdoClients[i].cobId && objIndex == sdoClients[i].objIndex && subIndex == sdoClients[i].subIndex)
		{
			if (sdoClients[i].msecLeft >0)
				return NULL; // another client is await at the same query, quit this request

			pClient = &sdoClients[i]; // take this client
			break;
		}

		// otherwise, take the first free-ed client
		if (NULL == pClient && sdoClients[i].msecLeft ==0)
			pClient = &sdoClients[i]; 
	}

	if (NULL != pClient)
	{
		pClient->msecLeft = timeout;
		pClient->cobId = cobId;
		pClient->objIndex = objIndex;
		pClient->subIndex = subIndex;
		pClient->pCtx = pCtx ? pCtx : pClient;
		pClient->dataLen =0;
	}

	return pClient;
}

int setSDO_async(uint8_t fdCAN, uint8_t nodeId, uint16_t objIndex, uint8_t subIndex, uint8_t* value, uint8_t valueLen, void* pCtx)
{
	HTCanMessage m;
	SdoClient* pClient = NULL;
	if (valueLen<= 0 || NULL ==value)
		return 0;
	else if (valueLen > 4)
		valueLen =4;

	m.cob_id = (nodeId & 0x7f) | (fcSDOtx <<7);
	SDO_CMD(m) = SDO_CMD_SET;
	SDO_OBJIDX(m) = objIndex;
	SDO_SUBIDX(m) = subIndex;
	m.len = valueLen+4;
	m.rtr = 0;

	memcpy(&m.data[3], value, valueLen);

	pClient = newSdoClient(m.cob_id, objIndex, subIndex, 0, pCtx);
	if (NULL ==pClient)
		return 0;

	return HtCan_send(fdCAN, &m) ? valueLen:0;
}

int getSDO_async(uint8_t fdCAN, uint8_t nodeId, uint16_t objIndex, uint8_t subIndex, void* pCtx)
{
	HTCanMessage m;
	SdoClient* pClient = NULL;

	m.cob_id = (nodeId & 0x7f) | (fcSDOtx <<7);
	SDO_CMD(m)    = SDO_CMD_GET;
	SDO_OBJIDX(m) = objIndex;
	SDO_SUBIDX(m) = subIndex;
	m.len = 4;
	m.rtr = 0;

	pClient = newSdoClient(m.cob_id, objIndex,subIndex, 0, pCtx);
	if (NULL ==pClient)
		return 0;

	return HtCan_send(fdCAN, &m) ? 1:0;
}

void processNMT(uint8_t fdCAN, uint8_t nmtCmd)
{
	switch(nmtCmd)
	{
	case nmt_enterPreOp:
		canState= HtCanState_PreOperational;
		sendHeartbeat(fdCAN);
		break;

	case nmt_startRemote:
		HtCan_init(fdCAN);
		break;

	case nmt_stopRemote:
		HtCan_stop(fdCAN);
		break;

	case nmt_resetNode:
	case nmt_resetComm:
		HtCan_stop(fdCAN);
		doResetNode();
		break;
	}
}

#define TXFIFO_SZ (8)

HTCanMessage txFIFO[TXFIFO_SZ];
uint8_t txHeader=0, txTail=0;

bool bHTCanSendBusy =FALSE;

bool FIFO_tryWrite(const HTCanMessage* m)
{
	uint8_t newheader = txHeader+1; 
	if (newheader >= TXFIFO_SZ)
		newheader =0;

	if (newheader == txTail)
		return false;

	txFIFO[txHeader] = *m;
	txHeader = newheader;
	return true;
}

bool FIFO_tryRead(HTCanMessage* m)
{
	uint8_t newtail = txTail+1; 
	if (newtail >= TXFIFO_SZ)
		newtail =0;

	if (newtail == txHeader || txTail == txHeader)
		return false;

	*m = txFIFO[txTail];
	txTail = newtail;
	return true;
}

extern const PDOParam* heartbeatExt;

void sendHeartbeat(uint8_t fdCAN)
{
	HTCanMessage m;
	int j, k;
	m.cob_id = fcNODE_GUARD <<7 | HT_NODEID;
	m.rtr =0; m.len=1; m.data[0] = canState;

	if (heartbeatExt)
	{
		for (j=0; heartbeatExt[j].key && m.len<8; j++)
		{
			if (NULL == heartbeatExt[j].pLocalVar)
				continue;

			k =0;
			switch(heartbeatExt[j].type & 0x0f)
			{
			case ODType_uint8: k=1; break;
			case ODType_uint16: k=2; break;
			}

			if (m.len+k>=8)
				continue;

			memcpy(&m.data[m.len], ((uint8_t*)heartbeatExt[j].pLocalVar), k);
			m.len +=k;
		}
	}

	HtCan_send(fdCAN, &m);
}

void HtCan_init(uint8_t fdCAN)
{
	canState= HtCanState_Bootup;
	sendHeartbeat(fdCAN);

	txHeader = txTail = 0;
	memset(neighborhood, 0x00, sizeof(NodeState)*NEIGHBOR_MAX_NODES);
	memset(syncPDOTimeouts, 0x00, sizeof(syncPDOTimeouts));

	canState= HtCanState_PreOperational;
	sendHeartbeat(fdCAN);

	canState= HtCanState_Operational;
	sendHeartbeat(fdCAN);
}

void HtCan_stop(uint8_t fdCAN)
{
	canState= HtCanState_Stopped;
	sendHeartbeat(fdCAN);
}

uint8_t HtCan_send(uint8_t fdCAN, const HTCanMessage* m)
{
	if (NULL ==m)
		return 0;

	if (!bHTCanSendBusy)
	{
		bHTCanSendBusy = true;
		return doCanSend(fdCAN, m);
	}

	if (FIFO_tryWrite(m))
		return m->len;

	return 0;
}

void HtCan_flushOutgoing(uint8_t fdCAN)
{
	HTCanMessage m;
	bHTCanSendBusy =false;
	if (FIFO_tryRead(&m))
	{
		bHTCanSendBusy = true;
		doCanSend(fdCAN, &m);
	}
}

void HtCan_processReceived(uint8_t fdCAN, const HTCanMessage* m)
{
	uint16_t cob_id = LittleEndU16(m->cob_id);
	uint16_t words[3];
	uint32_t timeStamp =0;

	switch(m->cob_id >> 7)
	{
	case fcSYNC: // can be a SYNC or a EMCY message
		if (cob_id == 0x080)	// SYNC
			procSync();
		else // EMCY
			processEmcy(fdCAN, m);

		break;

	case fcTIME_STAMP:
	// Usually the Time-Stamp object represents an absolute time in milliseconds after midnight and the number of
	// days since January 1, 1984. This is a bit sequence of length 48 (6 byte).

	//TODO: call to adjust clock
		if (m->len >=6 && (m->cob_id & 0x7f) < HtCan_thisNodeId)
		{
			memcpy(words, m->data, 6);
			words[0] = (words[0] >> 10) | (words[1] << 6);
			words[1] = (words[1] >> 10) | (words[2] << 6);
			words[2] >>= 10;
			memcpy(&timeStamp, words, sizeof(timeStamp));
			// adjust per (>>10) equals divided by 1024 instead of 1000
			timeStamp += (timeStamp >>6) + (timeStamp >>7) + (timeStamp >>11) + (timeStamp >>14)+ (timeStamp >>17);
			OnClock(fdCAN, timeStamp);
		}
		break;

		// about the PDOs
	case fcPDO1tx:
	case fcPDO1rx:
	case fcPDO2tx:
	case fcPDO2rx:
	case fcPDO3tx:
	case fcPDO3rx:
	case fcPDO4tx:
	case fcPDO4rx:
		{
			if ((*m).rtr == 0)
				OnPdoEvent(fdCAN, m, false);
			else processPdoRequest(fdCAN, m);
		}
		break;

	case fcSDOtx:
	case fcSDOrx:
		processSdo(fdCAN, m);
		break;

	case fcNODE_GUARD:
#ifndef _DEBUG
		if ((m->cob_id & 0x7f) != HtCan_thisNodeId)
#endif //_DEBUG
			processHeartbeat(fdCAN, m->cob_id&0x7f, m->data[0]&0x7f, &m->data[1], (m->len >1)?(m->len -1) :0);
		break;

	case fcNMT:
		if (m->len ==1 || (m->len >1 && ( 0== m->data[1] || HtCan_thisNodeId == m->data[1])))
			processNMT(fdCAN, m->data[0]);

		break;
	}
}

// should be executed every 1 msec
void HtCan_doScan(uint8_t fdCAN)
{
	int i =0, j=0, k=0;
	uint16_t mask=1, issuedPdo=0;
	static uint8_t _msec = 20;
	HTCanMessage m;
	static uint8_t _n20msec = 0;
//	static uint16_t hbi=0;

	// step 1. scan and issue the outgoing PDOs
	for (i=0, mask=1; pdoDict[i].funcCode && i <sizeof(flagsPDO2Send)*2; i++, mask<<=1)
	{
		if (0 == (mask & flagsPDO2Send))
			continue;

		m.cob_id = pdoDict[i].funcCode <<7 | HT_NODEID;
		m.rtr =0; m.len=0;
		for (j=0; pdoDict[i].params[j].key && m.len<8; j++)
		{
			if (NULL == pdoDict[i].params[j].pLocalVar)
				continue;

			k =0;
			switch(pdoDict[i].params[j].type & 0x0f)
			{
			case ODType_uint8: k=1; break;
			case ODType_uint16: k=2; break;
			}

			if (m.len+k>=8)
				continue;

			memcpy(&m.data[m.len], ((uint8_t*)pdoDict[i].params[j].pLocalVar), k);
			m.len +=k;
		}

		HtCan_send(fdCAN, &m);
		issuedPdo |= mask;
		OnPdoEvent(fdCAN, &m, true);
	}

	flagsPDO2Send &= ~issuedPdo; // reset the flags

	// step 2. scan the SDO clients every 20msec
	if (_msec--)
		return;

	_msec = 20;
	for (i =0; i <SDO_MAX_CLIENTS; i ++)
	{
		if (sdoClients[i].msecLeft <=0)
			continue;

		if (--sdoClients[i].msecLeft ==0)
		{
			sdoClients[i].dataLen =0;
			OnSdoClientDone(fdCAN, SDO_CLIENT_ERR_TIMEOUT, sdoClients[i].cobId & 0x7f, sdoClients[i].objIndex, sdoClients[i].subIndex, sdoClients[i].data, sdoClients[i].dataLen, sdoClients[i].pCtx);
			sdoClients[i].msecLeft =0;
		}
	}

	// step 3. issue heartbeat every 5sec = 255 * 20msec
	if (_n20msec--)
		return;

	sendHeartbeat(fdCAN);
//	trace("hearbeat %04d mstate[%x], lstate[%x], gstate[%x], master[%x]\r\n", hbi++, Ecu_motionState, Ecu_localState, Ecu_globalState, Ecu_masterNodeId);

	// step 4. scan the neighorhood if any node has gone
	for (i=0; i <NEIGHBOR_MAX_NODES; i++)
	{
		if (neighborhood[i].timeout--)
			continue;
		neighborhood[i].timeout =0;
	}
}

uint8_t HtCan_parseCompactText(const char* msg, HTCanMessage* m)
{
	uint8_t v, i;
	const char* p = NULL;
	if (NULL == msg || NULL ==m)
		return 0;

	memset(m, 0x00, sizeof(HTCanMessage));
	p = strchar(msg, ':');

	if (NULL != p) // seek to can:
		msg = p+1;

	// parse for cobId
	for(; *msg; msg++)
	{
		v = hexchval(*msg);
		if (v >0x0f) break;
		m->cob_id <<=4; m->cob_id += v;
	}

	// parse for RTR
	if (*msg == 'R')
		m->rtr = 1;
	else if (*msg == 'N')
		m->rtr = 0;
	else return 0; // invalid

	// parse for len
	v = hexchval(*++msg);
	m->len <<=4; m->len += v;
	v = hexchval(*++msg);
	if (0 == (v & 0xf0))
	{ m->len <<=4; m->len += v; msg++; }
	
	if (m->len<=0 || m->len >8)
		m->len =0;

	if (*msg++ != 'V')
		return 0; // invalid

	// parse for data
	for(i=0; i< m->len; i++)
	{
		m->data[i] = hexchval(*msg++);
		m->data[i] <<=4; m->data[i] += hexchval(*msg++);
	}

	return m->len;
}

char* HtCan_toCompactText(char* msg, int maxLen, const HTCanMessage* m)
{
	uint8_t i =0;
	if (NULL == msg || maxLen <6 || NULL ==m)
		return msg;

	msg += snprintf(msg, maxLen, "%03x%c%dV", m->cob_id, m->rtr?'R':'N', m->len);
	maxLen -= 7;
	maxLen /= 2;

	for (i=0; (i < maxLen) && (i <m->len); i++)
		sprintf(msg+i*2, "%02x", m->data[i]);

	return msg;
}

// {GET|PUT|POST|ACK} <nodeId><objectUri>?<var>=<value>[&<var>=<value>]
// <nodeId><objectUri> means the dest object for GET|SET, but means source object for POST|ACK
const char* Verbs[] = { "GET", "PUT", "POST",  NULL };

static uint8_t richURItoPDO(HTCanMessage* m, const uint16_t nodeId, const char* objectUri, int cparams, const char* params[], const char* values[])
{
	int i=0, j;
	uint8_t subIdxeMap[SUBIDX_MAX+1];
	uint32_t v;

	const PDOObj* pPDO = NULL;
	for (i=0; 0 !=pdoDict[i].funcCode; i++)
	{
		if (0 == strcmp(pdoDict[i].uri, objectUri))
		{
			pPDO = &pdoDict[i];
			break;
		}
	}

	if (NULL == pPDO)
		return false;

	// found the matched PDO here
	m->cob_id = (pPDO->funcCode << 7) | nodeId;
	m->len =0; m->rtr =0;

	for (i=0; pPDO->params && i < cparams; i++)
	{
		for (j=0; j <SUBIDX_MAX && pPDO->params[j].key; j++)
		{
			if (pPDO->params[j].key && (0 == strcmp(pPDO->params[j].key, params[i])))
			{ subIdxeMap[j] = i; break; }
		}
	}

	for (i=0; pPDO->params[i].key; i++)
	{
		if (0xff == subIdxeMap[i]) // parameters of the PDO is incomplete, fail this coversion
			return false;

		switch(pPDO->params[i].type)
		{
		case ODType_uint8:
			hex2int(values[subIdxeMap[i]], &v);
			m->data[m->len++] = v&0xff; v>>=8;
			break;
		case ODType_uint16: 
			hex2int(values[subIdxeMap[i]], &v);
			m->data[m->len++] = v&0xff; v>>=8;
			m->data[m->len++] = v&0xff; v>>=8;
			break;
		}
	}

	return true;
}

bool HtCan_parseRichURIEx(uint8_t verb, const uint16_t nodeId, const char* objectUri, int cparams, const char* params[], const char* values[], HtCan_OnFillCanMsg_t fFillMsg, void* pCtx)
{
	int i=0, j, k;
	uint8_t subIdxeMap[SUBIDX_MAX+1];
	uint32_t v;
	const ODObj* pObj = NULL;
	
	HTCanMessage m;
	m.rtr =0;

	switch(verb)
	{
	case VERB_POST:
		if (richURItoPDO(&m, nodeId, objectUri, cparams, params, values))
		{
			if (fFillMsg)
				fFillMsg(&m, pCtx);
			return true;
		}

		m.cob_id = (fcSDOrx <<7) | nodeId;
		SDO_CMD(m) = SDO_RESP_GET;
		break;

	case VERB_PUT:
		m.cob_id = (fcSDOtx <<7) | nodeId;
		SDO_CMD(m) = SDO_CMD_SET;
		break;

	case VERB_GET:
		m.cob_id = (fcSDOtx <<7) | nodeId;
		SDO_CMD(m) = SDO_CMD_GET;
		break;

	default:
		return false;
	}

	// check if this is a SDO message
	hex2int(objectUri, &v);
	for (i=0; NULL != objDict[i].uri; i++)
	{
		if ((v>=0x1000 && v == objDict[i].objIndex) || (0 == strcmp(objectUri, objDict[i].uri)))
		{
			pObj = &objDict[i];
			break;
		}
	}

	if (NULL ==pObj || NULL == pObj->subIdxes) // not find matched uri
		return false;

	SDO_OBJIDX(m) = pObj->objIndex;
	memset(subIdxeMap, 0xff, sizeof(subIdxeMap));

	for (i=0, k=0; i < cparams; i++)
	{
		for (j=0; j <SUBIDX_MAX && pObj->subIdxes[j].pVar; j++)
		{
			if (pObj->subIdxes[j].alias && (0 == strcmp(pObj->subIdxes[j].alias, params[i])))
			{ subIdxeMap[j] = i; k= max(k, j); break; }
		}
	}

	SDO_SUBIDX(m) = 0xff; // initialize with a dummy subidx
	m.len = 4; // yield SDO_CMD, SDO_OBJIDX, SDO_SUBIDX
	
#define FLUSH_MSG(_MSG) { if (fFillMsg) fFillMsg(&m, pCtx); SDO_SUBIDX(m) = 0xff; m.len =4; }

	for (i=0; i<= k && pObj->subIdxes[i].pVar; i++)
	{
		if (0xff == subIdxeMap[i])
		{
			if (VERB_GET != verb && m.len >4) 
				FLUSH_MSG(m); // a discontinue parameter, break into multiple can messages

			continue;
		}

		if (0xff == SDO_SUBIDX(m))
			SDO_SUBIDX(m) = i+1;
		
		if (VERB_GET == verb)
		{
			// SDO_GET take one message for each wished subIndx
			FLUSH_MSG(m);
			continue;
		}

		// when reach here, it is a SDO_SET or RESP
		switch(pObj->subIdxes[i].dateType)
		{
		case ODType_uint8:
			if (m.len +1 > sizeof(m.data))
			{
				FLUSH_MSG(m);
				SDO_SUBIDX(m) = i+1;
			}

			hex2int(values[subIdxeMap[i]], &v);
			m.data[m.len++] = v&0xff; v>>=8;
			break;

		case ODType_uint16: 
			if (m.len +2 > sizeof(m.data))
			{
				FLUSH_MSG(m);
				SDO_SUBIDX(m) = i+1;
			}

			hex2int(values[subIdxeMap[i]], &v);
			m.data[m.len++] = v&0xff; v>>=8;
			m.data[m.len++] = v&0xff; v>>=8;
			break;
		}
	}

	if (0xff != SDO_SUBIDX(m))
		FLUSH_MSG(m);

#undef FLUSH_MSG

	return true;
}


char* HtCan_toRichURI(char* txtMsg, uint8_t maxLen, const HTCanMessage* m)
{
	char *msg = NULL;
	
	if (NULL ==m || NULL ==txtMsg || maxLen <6)
		return NULL;
	
	msg = pdoToRichURI(txtMsg, maxLen, m);
	if (NULL != msg)
		return msg;

	msg = sdoToRichURI(txtMsg, maxLen, m);
	return msg;
}

char* HtCan_toJsonVars(char* txtMsg, uint8_t maxLen, const HTCanMessage* m)
{
	int i =0, k=0;
	const ODObj* pSDO = NULL;
	char* p = txtMsg;
	
	if (NULL ==m || NULL ==txtMsg || maxLen <6)
		return NULL;
	
//	msg = pdoToRichURI(txtMsg, maxLen, m);
//	if (NULL != msg)
//		return msg;


	// step 2. find the object id in the dictionary
	for (i =0; objDict[i].objIndex< 0xffff && objDict[i].uri && SDO_OBJIDX(*m) >objDict[i].objIndex; i++);
	
	if (SDO_OBJIDX(*m) == objDict[i].objIndex)
		pSDO = &objDict[i];

	if (NULL == pSDO)
		return NULL; // no such a SDO known in the dictionary

	for (i=0, k=0; (txtMsg + maxLen -p-2)>2 && pSDO->subIdxes && pSDO->subIdxes[i].alias && k <(m->len-4); i++)
	{
		if (i+1 < SDO_SUBIDX(*m)) // skip those before the requested sub index
			continue;

		switch(pSDO->subIdxes[i].dateType & 0x0f)
		{
		case ODType_uint8: 
			p += snprintf(p, txtMsg +maxLen -p-2, ",\"%s\":0x%02x", pSDO->subIdxes[i].alias, m->data[4+ k]);
			k++;
			break;
		case ODType_uint16:
			p += snprintf(p, txtMsg +maxLen -p-2, ",\"%s\":0x%04x", pSDO->subIdxes[i].alias, *((uint16_t*)&m->data[4+k]));
			k++, k++;
			break;
		}
	}
	return txtMsg;
}
