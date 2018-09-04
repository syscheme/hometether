#include "bsp.h"
#include "../htod.h"

#define HTCAN_RX_LED_CH   0
#define HTCAN_TX_LED_CH   0

uint8_t doCanReceive(uint8_t fdCAN, HTCanMessage *m);
void canReceiveLoop(uint8_t fdCAN)
{
	HTCanMessage m;
	
	DQBoard_setLed(HTCAN_RX_LED_CH, 1);

	while(CAN_MessagePending(CAN1, CAN_FIFO0) >0)
	{
		if (0 == doCanReceive(fdCAN, &m))
			continue;

		HtCan_processReceived(fdCAN, &m);
	}

	DQBoard_setLed(HTCAN_RX_LED_CH, 0);
}

// =================== HtCan portal ===================
uint8_t   Ecu_globalState;
uint8_t   Ecu_masterNodeId         =0x43;

// local state
uint16_t  Ecu_localState           =0x00;     // =0;
uint16_t  Ecu_motionState          =0xfc;    // =0;

// ECU configurations
uint8_t   EcuConf_temperatureMask  =0xff;
uint8_t   EcuConf_temperatureIntv  =20;	// 20sec

uint16_t  EcuConf_motionDiffMask   =0xff;
uint8_t   EcuConf_motionDiffIntv   =20;   // 2sec in 100msec
uint16_t  EcuConf_motionOuterMask = 0xf0;
uint16_t  EcuConf_motionBedMask   = 0x02;

uint8_t   EcuConf_lumineDiffMask   =0xff;
uint8_t   EcuConf_lumineDiffIntv   =10;	 // 20sec
uint16_t  EcuConf_lumineDiffTshd;  

// ECU spec
const uint8_t   HtCan_thisNodeId        =HT_NODEID;
const uint8_t   EcuSpec_versionMajor    =0;
const uint8_t   EcuSpec_versionMinor    =1;

// temperature values
uint16_t Ecu_temperature[CHANNEL_SZ_Temperature];

// lumine ADC values
uint16_t Ecu_adc[CHANNEL_SZ_ADC];

// About pulse receiving
uint8_t EcuCtrl_pulseSendChId            =0;
uint8_t EcuCtrl_pulseSendCode            =0;
uint8_t EcuCtrl_pulseSendPflId           =0xff;
uint8_t EcuCtrl_pulseSendBaseIntvX10usec =0;

/*
// About pulse receiving
uint8_t EcuCtrl_pulseRecvChId            =0;
uint8_t EcuCtrl_pulseRecvTimeout         =0; // 0x00 means the receiving buffer would be overwritten if new pulse seq is captured 
uint8_t Ecu_pulseRecvBuf[PULSE_RECV_MAX];
PulseSeq  Ecu_pulseRecvSeq                 ={ 0, PULSE_RECV_MAX, Ecu_pulseRecvBuf };

// About ASK devices
uint32_t EcuCtrl_askDevState;
uint32_t EcuCtrl_askDevTable[ASK_DEVICES_MAX]={0x123, 0x0043330c,}; // the code table of known devices
uint8_t  EcuCtrl_askDevTypes[ASK_DEVICES_MAX]={3, 1,}; // the types of ASK devices, maps to DeviceStateFlag: 0=>dsf_Window, 1=>dsf_GasLeak, 2=>dsf_Smoke, 3=>dsf_OtherDangers
*/

// About alarms

static uint8_t canSend(const HTCanMessage* m)
{
	CanTxMsg TxMessage;
	uint8_t  TxMailbox =0;
	int i;

	if (NULL == m)
		return 0; // failed

	DQBoard_setLed(HTCAN_TX_LED_CH, 1);

	// compose the outgoing message according to input m
	TxMessage.StdId  = m->cob_id;	    //配置报文的标准标识符
	TxMessage.ExtId  = 0x00;			//TODO: where to put cob_id 配置扩屏标识符
	TxMessage.IDE    = CAN_ID_STD;		//报文使用标准标识符+扩屏标识符方式
	TxMessage.RTR    = (m->rtr ? 1:0);	//报文为数据帧
	TxMessage.DLC    = m->len;

	memcpy(TxMessage.Data, m->data, TxMessage.DLC);

	TxMailbox =CAN_Transmit(CAN1, &TxMessage);   //发送报文并返回发送信箱的地址

// TODO: if we have onSent interrupt, HtCan_flushOutgoing() should be triggered by the interrupt
// #ifndef WITH_UCOSII
	// CAN_TransmitStatus returns CANTXOK means CAN driver is currently transmitting the message,
	// wait till it completes
	for( i=999; i && CANTXOK == CAN_TransmitStatus(CAN1, TxMailbox); i--); 
	HtCan_flushOutgoing(MSG_CH_CAN1); // callback to HtCan stack
// #endif // !WITH_UCOSII

	DQBoard_setLed(HTCAN_TX_LED_CH, 0);

	return 1; // succeeded
}

extern uint8_t GW_dispatchCANbyFd(uint8_t fdCAN, const HTCanMessage* m);

uint8_t doCanSend(uint8_t fdCAN, const HTCanMessage* m)
{
	if (fdCAN == MSG_CH_CAN1)
		canSend(m);

	GW_dispatchCANbyFd(fdCAN, m);
	return 1;
}

// -------------------------------------------------------------------------
// portal entry doCanReceive()
// parse/passes a received CAN message to the stack
// @param m          pointer to message received
// @return 1 if a message received
// -------------------------------------------------------------------------
uint8_t doCanReceive(uint8_t fdCAN, HTCanMessage *m)
{
	CanRxMsg RxMessage;	 // fwlib msg struct
	if (NULL ==m)
		return 0;

	memset(&RxMessage, 0x00, sizeof(RxMessage));
	CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);	// call fwlib to receive a RxMessage

	// convert the RxMessage to CanFestival definition
	m->cob_id = RxMessage.StdId;  // TODO: where is cob_id
	m->rtr    = RxMessage.RTR ? 1:0;
	m->len   =  RxMessage.DLC;
	memcpy(m->data, RxMessage.Data, m->len);

	return 1;
}

void doResetNode(void)
{
	DQBoard_reset();
}

// Events that portal may needs to hook
// ------------------------------------
extern uint32_t Timeout_MasterHeartbeat;
extern uint32_t Timeout_State;
void OnRemoteHeartBeat(uint8_t nodeId, uint8_t remoteCanState, const uint8_t* extData, uint8_t extLen)
{
	if (((nodeId & 0xf0) >= HtCan_thisNodeId) || (ecs_Operational != remoteCanState))
	{
		// a lower priority node or masternode advertizes, ignore
		return;
	}

	if (Ecu_masterNodeId == nodeId)
	{
		Timeout_MasterHeartbeat = DEFAULT_MasterHeartbeatTimeoutReload;
		if (extData[0] != Ecu_globalState)
			State_enter((StateOrdinalCode) extData[0]);
		return;
	}

	Ecu_masterNodeId = extData[1];
	Timeout_MasterHeartbeat = DEFAULT_MasterHeartbeatTimeoutReload;
	State_enter((StateOrdinalCode) extData[0]);
	triggerPdo(HTPDO_GlobalStateChanged);
}

void OnClock(uint8_t fdCAN, uint32_t secSinc198411)
{
	int32_t diff = (int32_t) DQBoard_getTime(NULL);
	diff = (diff & 0xffffff) - (secSinc198411 %60*60*24);
	if (diff < 0) // abs the diff
 		diff =-diff;

	if (diff > 5)
	{	// necessary do clock adjusting if the clock has more than 5sec diff
	
		// 1/1/1984 was a sunday, convert the inputed secSinc198411 to our format per DQ
		secSinc198411 = (secSinc198411 %60*60*24) + ((secSinc198411 /60*60*24) %7) << 24;
		DQBoard_updateClock(secSinc198411);
	}
}

void OnSdoClientDone(uint8_t fdCAN, uint8_t errCode, uint8_t peerNodeId, uint16_t objIdx, uint8_t subIdx, uint8_t* data, uint8_t dataLen, void* pCtx)
{
}

#if 0

// Convert a CAN frame into HT Message
// where HT Message for CAN bus forward is in the format of
//       CAN:<cob_id>{R:N}<len>V[<bytes>]*;
UNS8 HtMsgToCan(char* htMsg, HTCanMessage* canMsg)
{
	int i;
	uint8_t* p = (uint8_t*) htMsg;
	uint8_t v;

	if (NULL == canMsg || NULL == p)
		return 0;

	if (0 == memcmp(p, PROTO_CANBUS, sizeof(PROTO_CANBUS)-1))
		p += sizeof(PROTO_CANBUS)-1;

	memset(canMsg, 0x00, sizeof(HTCanMessage));

	// parse the cob_id
	for(;*p;p++)
	{
		v = hexChVal(*p);
		if (v >0x0f) return 0;
		canMsg->cob_id <<=4; canMsg->cob_id += v;
	}

	canMsg->rtr = (*p++ == 'R')?1:0;

	// parse the len
	canMsg->len =0;
	for(;*p;p++)
	{
		v = hexChVal(*p);
		if (v >0x0f) return 0;
		canMsg->len <<=4;	canMsg->len += v;
	}

	if (*p++ != 'V')
		return 0;

	for (i =0; i <canMsg->len && *p; i++)
	{
		canMsg->data[i] =0;
		v = hexChVal(*p++);
		if (v >0x0f) return 0;
		canMsg->data[i] += v;	

		v = hexChVal(*p++);
		if (v >0x0f) return 0;
		canMsg->data[i]<<=4; canMsg->data[i] += v;
	}

	return (UNS8) (((char*)p) - htMsg);
}

UNS8 toCanMessage(char* htMsg)
{
	HTCanMessage canMsg;

	if (!HtMsgToCan(htMsg, &canMsg))
		return 0;

	return doCanSend(&canMsg);
}

UNS8 toRS485Message(char* msg)
{
    // in format of RS485:123L2V1234
	uint8_t tmp =0, len;
	uint8_t* p = (uint8_t*) msg, v;

	// parse for the nodeId, save it in tmp
	for(;;p++)
	{
		v = hexChVal(*p);
		if (v >0x0f) break;
		tmp <<=4; tmp += v;
	}

	if (*p++ != 'L')
		; // return 0;
	
	// parse for the len, save it in i
	len =0;
	for(;;p++)
	{
		v = hexChVal(*p);
		if (v >0x0f) break;
		len <<=4; len += v;
	}

	if (*p++ != 'V')
		; // return 0;

	DQBoard_485sendMode(1);
	// send the nodeId and len
	USART_SendData(USART3, tmp);
	USART_SendData(USART3, len);

	for (; len >0 && *p; len--)
	{
		tmp =0;
		v = hexChVal(*p++);
		if (v >0x0f) v=0x0f;
		tmp += v;	

		v = hexChVal(*p++);
		if (v >0x0f) v=0x0f;
		tmp<<=4; tmp += v;

		USART_SendData(USART3, tmp);
	}

	DQBoard_485sendMode(0);
	return (UNS8) (((char*)p) - msg);
}

// {GET|SET|POST|ACK} <nodeId><objectUri> <var>=<value>[;<var>=<value>]
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
	return (c >'!' && c < 0x7f );
}

#define MAX_PARAMS (10)

void processHtTxtMessage(char* msg)
{
	char* p =msg, *varname=NULL, *value=NULL;
	uint8_t verb=0, v, iparam=0;
	char* objectUri =NULL;
	char* params[MAX_PARAMS], *values[MAX_PARAMS];
	uint16_t nodeId =0;
	 
	// determin the verb
	for(verb=0; Verbs[verb]; verb++)
	{
		if (0== memcmp(msg, Verbs[verb], sizeof(Verbs[verb])-1))
		{
			p += sizeof(Verbs[verb])-1;
			break;
		}
	}

	if (verb >=VERB_MAX)
		return;

	// parse the node_id
	p=trimleft(p);
	nodeId=0;
	for(;p && *p;p++)
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
		if (p && NULL != (p = strchar(p, ';')))
			*p++='\0';

		values[iparam] = strchar(params[iparam], '=');

		if (NULL != values[iparam])
			*values[iparam]++='\0';
	}

	OnHtMessage(verb, nodeId, objectUri, iparam, params, values);
}

void OnCanSent(Message *m)
{
	// do nothing here
}


#endif
