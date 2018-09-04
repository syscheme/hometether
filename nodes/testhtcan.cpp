#include "../ZQ_common_conf.h"
#include "../getopt.h"
#include "../NativeThread.h"
#include "../Locks.h"

#include "htcan.c"
#include "htod.c"

#include <queue>
#define strchar strchr

// about flags in EcuConf_forwarder0
#define FORWARDER_DIR_CAN2O   (1<<0)
#define FORWARDER_DIR_O2CAN   (1<<1)

enum _eForwarder_Type {
	fwdt_Binary, fwdt_CompactText, fwdt_RichText
};

char* trimleft(char *s)
{
	while(*s && (' ' == *s || '\t' ==*s))
		s++;
	return s;
}

uint8_t   Ecu_globalState;   // =0;
uint8_t   Ecu_masterNodeId       =0x43;

// local state
uint16_t  Ecu_localState      =0x00;     // =0;
uint16_t  Ecu_motionState     =0xfc;    // =0;

// ECU configurations
uint8_t   EcuConf_temperatureMask; // =0xff;
uint8_t   EcuConf_temperatureIntv; // =0xff;
uint16_t  EcuConf_motionDiffMask;  // =0xff;
uint8_t   EcuConf_motionDiffIntv;  // =0xff;
uint8_t   EcuConf_lumineDiffMask;  // =0xff;
uint8_t   EcuConf_lumineDiffIntv;  // =0xff;
uint16_t  EcuConf_lumineDiffTshd;
uint8_t   EcuConf_forwarder0 = FORWARDER_DIR_CAN2O | FORWARDER_DIR_O2CAN | (fwdt_RichText <<2);
uint8_t   EcuConf_forwarder1 = FORWARDER_DIR_CAN2O | FORWARDER_DIR_O2CAN | (fwdt_CompactText <<2);

// ECU spec
const static uint8_t   HtCan_thisNodeId           =HT_NODEID;
const static uint8_t   EcuSpec_versionMajor    =0;
const static uint8_t   EcuSpec_versionMinor    =1;

// temperature values
uint16_t Ecu_temperature[8];

// lumine ADC values
uint16_t Ecu_lumine[8];

// Control of relay
uint8_t EcuCtrl_relays;
uint8_t EcuCtrl_irOutChId;
uint8_t EcuCtrl_irOutCLeft;
uint8_t EcuCtrl_irOutPfl;
uint8_t EcuCtrl_irInChId;
uint8_t EcuCtrl_irInTimeout;
uint8_t EcuCtrl_irInPfl;

// buffer of IR in and out
uint8_t		 Ecu_inBuf[16];
uint8_t		 Ecu_outBuf[16];

bool bQuit =false;
static ZQ::common::Mutex scrLock;

int IF0_send(const uint8_t* msg, uint8_t len)
{
	printf("eth0 >>%s\n", msg);
	return 0;
}

int IF1_send(const uint8_t* msg, uint8_t len)
{
	printf("rs485 >>%s\n", msg);
	return 0;
}

static char txtMsg[100];
void OnPdoEvent(const HTCanMessage* m, bool local)
{
	int i;
	char *p = txtMsg;
	bool bCompactTxtNeeded =false, bRichTxtNeeded =false;

	if (NULL == m)
		return;

	if (EcuConf_forwarder0 & FORWARDER_DIR_CAN2O)
	{
		switch ((EcuConf_forwarder0 >>2) & 0x3)
		{
		case fwdt_CompactText:  bCompactTxtNeeded = true; break;
		case fwdt_RichText:     bRichTxtNeeded = true;    break;
		}
	}

	if (EcuConf_forwarder1 & FORWARDER_DIR_CAN2O)
	{
		switch ((EcuConf_forwarder1 >>2) & 0x3)
		{
		case fwdt_CompactText:  bCompactTxtNeeded = true; break;
		case fwdt_RichText:     bRichTxtNeeded = true;    break;
		}
	}

	if (bCompactTxtNeeded)
	{
		p += sprintf(p, "POST can:%04x%c%02xV", m->cob_id, m->rtr?'R':'N', m->len);
		for (i=0; i<m->len; i++)
			p += sprintf(p, "%02x", m->data[i]);
		i = strlen(txtMsg);
		if ((EcuConf_forwarder0 & FORWARDER_DIR_CAN2O) && fwdt_CompactText == ((EcuConf_forwarder0 >>2) & 0x3))
			IF0_send((uint8_t*)txtMsg, i);

		if ((EcuConf_forwarder1 & FORWARDER_DIR_CAN2O) && fwdt_CompactText == ((EcuConf_forwarder1 >>2) & 0x3))
			IF1_send((uint8_t*)txtMsg, i);
	}

	if (bRichTxtNeeded)
	{
		i = pdoToRichTxtMsg(m, txtMsg, sizeof(txtMsg)-2);
		if (i>0)
		{
			// found a known SDO if reaches here
			if ((EcuConf_forwarder0 & FORWARDER_DIR_CAN2O) && fwdt_RichText == (EcuConf_forwarder0 >>2 & 0x3))
				IF0_send((uint8_t*)txtMsg, i);

			if ((EcuConf_forwarder1 & FORWARDER_DIR_CAN2O) && fwdt_RichText == (EcuConf_forwarder1 >>2 & 0x3))
				IF1_send((uint8_t*)txtMsg, i);
		}
	}
}

void printMsg(const HTCanMessage *m, const char* prefix)
{
	ZQ::common::MutexGuard g(scrLock);
	printf("%s cobid[%04x] r[%d] len[%d]:", prefix, m->cob_id, m->rtr, m->len);
	for (int j=0; j<m->len; j++)
		printf(" %02x", m->data[j]);
	printf("\n");
}

static char sdoTxtMsg[100];
void OnSdoData(const HTCanMessage* m, bool localData)
{
	int i;
	char *p = sdoTxtMsg;
	bool bCompactTxtNeeded =false, bRichTxtNeeded =false;

	if (NULL == m)
		return;

	if (EcuConf_forwarder0 & FORWARDER_DIR_CAN2O)
	{
		switch ((EcuConf_forwarder0 >>2) & 0x3)
		{
		case fwdt_CompactText:  bCompactTxtNeeded = true; break;
		case fwdt_RichText:     bRichTxtNeeded = true;    break;
		}
	}

	if (EcuConf_forwarder1 & FORWARDER_DIR_CAN2O)
	{
		switch ((EcuConf_forwarder1 >>2) & 0x3)
		{
		case fwdt_CompactText:  bCompactTxtNeeded = true; break;
		case fwdt_RichText:     bRichTxtNeeded = true;    break;
		}
	}

	if (bCompactTxtNeeded)
	{
		p += sprintf(p, "POST can:%04x%c%02xV", m->cob_id, m->rtr?'R':'N', m->len);
		for (i=0; i<m->len; i++)
			p += sprintf(p, "%02x", m->data[i]);
		i = strlen(sdoTxtMsg);
		if ((EcuConf_forwarder0 & FORWARDER_DIR_CAN2O) && fwdt_CompactText == ((EcuConf_forwarder0 >>2) & 0x3))
			IF0_send((uint8_t*)sdoTxtMsg, i);

		if ((EcuConf_forwarder1 & FORWARDER_DIR_CAN2O) && fwdt_CompactText == ((EcuConf_forwarder1 >>2) & 0x3))
			IF1_send((uint8_t*)sdoTxtMsg, i);
	}

	if (bRichTxtNeeded)
	{
		i = sdoToRichTxtMsg(m, sdoTxtMsg, sizeof(sdoTxtMsg)-2);
		if (i>0)
		{
			// found a known SDO if reaches here
			if ((EcuConf_forwarder0 & FORWARDER_DIR_CAN2O) && fwdt_RichText == (EcuConf_forwarder0 >>2 & 0x3))
				IF0_send((uint8_t*)sdoTxtMsg, i);

			if ((EcuConf_forwarder1 & FORWARDER_DIR_CAN2O) && fwdt_RichText == (EcuConf_forwarder1 >>2 & 0x3))
				IF1_send((uint8_t*)sdoTxtMsg, i);
		}
	}
}

void OnRemoteHeartBeat(uint8_t nodeId, uint8_t newCanState)
{
	ZQ::common::MutexGuard g(scrLock);
	printf("node[%02x] heartbeat, state[%02x]\n", nodeId, newCanState);
}

void OnSdoUpdated(const ODObj* odEntry, uint8_t subIdx, uint8_t updatedCount)
{
	ZQ::common::MutexGuard g(scrLock);
	printf("local obj[%04x]#%d updated: c[%d]\n", odEntry->objIndex, subIdx, updatedCount);
}

void OnSdoClientDone(uint8_t errCode, uint8_t peerNodeId, uint16_t objIdx, uint8_t subIdx, uint8_t* data, uint8_t dataLen, void* pCtx)
{
	ZQ::common::MutexGuard g(scrLock);
	if (SDO_CLIENT_ERR_OK == errCode)
		printf("peer[%02x] ack: obj[%04x]#%d c[%d]\n", peerNodeId, objIdx, subIdx, dataLen);
	else if (SDO_CLIENT_ERR_TIMEOUT == errCode)
		printf("peer[%02x] timeout: obj[%04x]#%d c[%d]\n", peerNodeId, objIdx, subIdx, dataLen);
	else if (SDO_PEER_ERR == errCode)
		printf("peer[%02x] err: obj[%04x]#%d c[%d]\n", peerNodeId, objIdx, subIdx, dataLen);
}

void doResetNode()
{
}

std::queue <HTCanMessage> bus;
ZQ::common::Mutex busLock;

uint8_t doCanSend(const HTCanMessage *m)
{
	{
		ZQ::common::MutexGuard g(busLock);
		bus.push(*m);
	}
	printMsg(m, "S>>");
	HtCan_processSent();
	return 1;
}

void usage();

void sdoTest()
{
	uint8_t v[] = {0x11, 0x11, 0x11, 0x11};
	setSDO_async(HT_NODEID, 0x2004, 5, v, 4, NULL);
	getSDO_async(7, 0x2004, 5, NULL);

	//	for (int j=0; j< 20; j++)
	//		testBuf[j] = j;

	static const HTCanMessage testMsgs[] = {
		{ fcSDOtx<<7 | 7, 0, 8, {SDO_RESP_GET, 0x04, 0x20, 0x05, 0x12, 0x34, 0x56, 0x78} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x01, 0x20, 0x01} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 6, {SDO_CMD_SET, 0x00, 0x20, 0x01, 0x55, 0x77} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x00, 0x20, 0x01} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x04, 0x20, 0x01} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x04, 0x20, 0x08} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 8, {SDO_CMD_SET, 0x04, 0x20, 0x03, 0x12, 0x34, 0x56, 0x78} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x04, 0x20, 0x02} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x04, 0x20, 0x04} },

		{ 0xffff, 0, 4, ""},
	};

	for (int i =0; 0xffff != testMsgs[i].cob_id; i++)
	{
		Sleep(100);
		// printMsg(&testMsgs[i], "R<<");
		// HtCan_processReceived(&testMsgs[i]);
		doCanSend(&testMsgs[i]);
	}
}

void pdoTest()
{
	sendHeartbeat();

	triggerPdo(HTPDO_DeviceStateChanged);
	HtCan_doScan();

	static const HTCanMessage testMsgs[] = {
		{ HTPDO_DeviceStateChanged<<7 | 17, 0, 4, {0x12, 0x34, 0x56, 0x78} },
		{ fcPDO3tx<<7 | 17, 0, 4, {0x12, 0x34, 0x56, 0x78} },

		{ 0xffff, 0, 4, ""},
	};

	for (int i =0; 0xffff != testMsgs[i].cob_id; i++)
	{
		// printMsg(&testMsgs[i], "R<<");
		// HTCan_processReceived(&testMsgs[i]);
		doCanSend(&testMsgs[i]);
	}

}

class ProcLoop : public ZQ::common::NativeThread
{
public:
	ProcLoop() {}
	~ProcLoop() {}

protected:
	virtual int run(void)
	{
		while (!bQuit)
		{
#ifdef _DEBUG
			{
				ZQ::common::MutexGuard g(busLock);
				if (!bus.empty())
					HtCan_processSent();
			}

#endif // _DEBUG
			Sleep(1);
			HTCanMessage m;
			bool bNewMsg= false;
			do {
				ZQ::common::MutexGuard g(busLock);
				if (bQuit || bus.empty())
					break;
				m = bus.front();
				bus.pop();
				bNewMsg = true;
			} while (0);

			if (bNewMsg)
			{
				printMsg(&m, "R<<");
				HtCan_processReceived(&m);
			}

			HtCan_doScan();
		}

		return 0;
	}

};

/////////////////////////////////
// {GET|SET|POST|ACK} <nodeId><objectUri>?<var>=<value>[&<var>=<value>]
// <nodeId><objectUri> means the dest object for GET|SET, but means source object for POST|ACK
static const char* Verbs[] = { "GET", "SET", "POST",  NULL };
typedef enum {	VERB_GET, VERB_SET, VERB_POST, VERB_MAX } HT_VERB_t;
int formatHtMessage(char* msg, uint8_t verb, const uint16_t nodeId, const char* objectUri, int cparams, char* params[], char* values[])
{
	char* p =msg;
	int i=0;
	if (NULL == p || NULL ==objectUri || verb >=VERB_MAX)
		return 0;

	p += sprintf(p, "%s %x%s ", Verbs[verb], nodeId, objectUri);
	for (i=0; i < cparams && params[i]; i++)
		p += sprintf(p, "%s=%s;", params[i], values[i] ? values[i]:"");

	return (p-msg);
}

void OnHtMessage(uint8_t verb, const uint16_t nodeId, const char* objectUri, int cparams, char* params[], char* values[])
{
}

bool isUriCh(const char c)
{
	return (c =='.' || c >='/' && c <= '9' || c>='a' && c<='z' || c>='A' && c <='Z');
}


#define MAX_PARAMS (10)

void processHtTxtMessage(char* msg)
{
	char* p =msg, *varname=NULL, *value=NULL;
	uint8_t verb=0, v, iparam=0;
	char* objectUri =NULL;
	char* params[MAX_PARAMS], *values[MAX_PARAMS];
	uint16_t nodeId =0;
	HTCanMessage m;
	 
	// determin the verb
	for(verb=0; Verbs[verb]; verb++)
	{
		if (0== memcmp(msg, Verbs[verb], sizeof(Verbs[verb])-1))
		{
			p += sizeof(Verbs[verb]);
			break;
		}
	}

	if (verb >=VERB_MAX)
		return;

	p=trimleft(p);

	if (NULL != (varname = strchar(p, ':')))
	{
		// message speceified the protocal
		*varname = '\0'; objectUri = p; p = varname+1; // temperary save the proto str in objectUri
		if (0 == strcmp(objectUri, "can") && compactTxtToCanMsg(p, &m) >0)
		{
			v = m.cob_id & 0x7f;
			if (0 == v || EcuSpec_thisNodeId == v)
				HtCan_processReceived(&m);

			if (0 == v || EcuSpec_thisNodeId != v)
				HtCan_send(&m);
		}
	}

	// parse the node_id
	nodeId=0;
	for(; p && *p; p++)
	{
		v = hexChVal(*p);
		if (v >0x0f) break;
		nodeId <<=4; nodeId += v;
	}

	// parse for the objectUri
	while (p && *p && *p!='/') p++;

	for (objectUri=p; p && *p && isUriCh(*p); p++);
	if (p && *p)
		*p++ ='\0';

	// parse for var=val pairs
	for (iparam=0; p && *p && iparam <MAX_PARAMS; iparam++)
	{
		p = trimleft(p); params[iparam]=p; values[iparam]=NULL;
		if (p && NULL != (p = strchar(p, '&')))
			*p++='\0';

		values[iparam] = strchar(params[iparam], '=');

		if (NULL != values[iparam])
			*values[iparam]++='\0';
	}

//	for (int i=0; i )

	OnHtMessage(verb, nodeId, objectUri, iparam, params, values);
}

/////////////////////////////////

int main(int argc, char* argv[])
{
	char htmsg[][100] = {
		"POST can:10R02V112233",
		"POST can:590N08V2002200112345678",
		"POST can:590N04V40022001",
		"POST 10/2001?rqwrq=sdfsaf&safsf=we",
		"SET 10/2001?rqwrq=sdfsaf&safsf=we",
		"GET 10/2001?rqwrq=sdfsaf&safsf=we",
		""
	};

	for(int i=0; htmsg[i] && htmsg[i][0]; i++)
		processHtTxtMessage(htmsg[i]);

	HtCan_init();

	ProcLoop proc;
	proc.start();


	char c =0;
	while ('q' != (c=getc(stdin)))
	{
		switch(c)
		{
		case 's': sdoTest(); break;
		case 'p': pdoTest(); break;
		case 'q': bQuit =true; return 1;
		}
	}

	HtCan_stop();
	Sleep(1000);

	//	pdoTest();

	// recvTest();
	return 0;
}

void usage()
{
	printf("Usage: HC -p <SerialPort>] [-d <descId>] [-s <srcId>] [-c <command>]\n");
	printf("       HC -h\n");
	printf("HC command line\n");
	printf("options:\n");
	printf("\t-p   serail port name to communication\n");
	printf("\t-c   the command to execute\n");
	printf("\t-s   specify the source ID of this node");
	printf("\t-d   the ID of destination node");
	printf("\t-h   display this help\n");
}

