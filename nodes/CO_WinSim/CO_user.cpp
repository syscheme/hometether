// ===========================================================================
// Copyright (c) 1997, 1998 by
// CO_OD.c - Variables and Object Dictionary for CANopenNode
//
// Ident : $Id: CO_OD.c$
// Branch: $Name:  $
// Author: janez.paternoster@siol.net
// Desc  : For short description of standard Object Dictionary entries see CO_OD.txt
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/CO_OD.c $
// ===========================================================================

#include "UDPSocket.h"
#include "NativeThread.h"
#include "Locks.h"
#include "TimeUtil.h"

extern "C" {
#include "CANopen.h"
#include <time.h>
}

void HTTask_CANopenNode();

bool bQuit =false;

//0x3000
ROM uint16_t     TempLo = 18;
//below this temperature heater will be turned on
//0x3001
ROM uint16_t     TempHi = 25;
//above this temperature cooler will be turned on
//0x3100
tData1byte       Status;
//0x3200

// ZQ::common::Log logger;

#define DEFAULT_PORT     10000
#define GROUP_ADDR       "223.23.23.23"
#define BIND_ADDR        "0.0.0.0"

ZQ::common::Mutex msgLock;
char msgBuf[1024];
int  msgLen=0;

ZQ::common::InetMcastAddress groupAddr(GROUP_ADDR);
ZQ::common::InetHostAddress bindAddr;
ZQ::common::UDPMulticast*   pSock = NULL;
int port    =DEFAULT_PORT;
int bindport=DEFAULT_PORT;

void USB_LP_CAN1_RX0_IRQHandler(CO_CanMessage* RxMessage);
class McastListener : public ZQ::common::NativeThread, ZQ::common::UDPReceive
{
public:

	McastListener(ZQ::common::InetMcastAddress group, int group_port, ZQ::common::InetHostAddress bind, bool timestamp=false)
		: _group(group), _bind(bind), _gport(group_port), ZQ::common::UDPReceive(bind, group_port)
	{
		_stamp[0]='\0';
	}
	
	virtual ~McastListener()
	{
	}
	
protected:

	bool init(void)	
	{
		UDPReceive::setMulticast(true);
		ZQ::common::Socket::Error err = UDPReceive::join(_group);
		UDPReceive::setCompletion(true); // make the socket blockable
		
		return (err == ZQ::common::Socket::errSuccess);

/*
std::string bindaddr= _store._groupBind.getHostAddress(), grpaddr= _store._groupAddr.getHostAddress();
	storelog(ZQ::common::Log::L_DEBUG, CLOGFMT(CacheStoreProbe, "initializing at: group[%s]:%d"), grpaddr.c_str(), _store._groupPort);

	UDPReceive::setMulticast(true);
	ZQ::common::Socket::Error err = UDPReceive::join(_store._groupAddr);
	UDPReceive::setCompletion(true); // make the socket block-able

	if (err != ZQ::common::Socket::errSuccess)
	{
		storelog(ZQ::common::Log::L_ERROR, CLOGFMT(CacheStoreProbe, "initialze failed at: group:[%s]:%d"), grpaddr.c_str(), _store._groupPort);
		return false;
	}

	return true;
*/
	}
	
	int run()
	{
		while (!bQuit)
		{
			ZQ::common::InetHostAddress from;
			int sport=0;
#ifdef _MCASTMSG
			int len = UDPReceive::receiveFrom(_buf, sizeof(_buf), from, sport);
			if (len <=0)
				continue;

			OnCapturedData(_buf, len, from, sport);
#else			
			{
				ZQ::common::MutexGuard g(msgLock);
			if (msgLen<=0)
			{
				Sleep(200);
				continue;
			}

			OnCapturedData(msgBuf, msgLen, from, sport);
			msgLen=0;
			}
#endif _MCASTMSG
		}
		
		return 0;
	}
	
	
	int OnCapturedData(const void* data, const int datalen, ZQ::common::InetHostAddress source, int sport)
	{
		if (datalen <=0)
			return 0;
		
		char *p=(char*)data;
		if (_bStamp)
		{
			time_t  longTime;
			time(&longTime);
			struct tm *timeData = localtime(&longTime);
			
			sprintf(_stamp, "%02d:%02d:%02d", timeData->tm_hour, timeData->tm_min, timeData->tm_sec);
		}

		char buf[100], *q= buf;
		for (int j=0; j<30 && j<datalen; j++)
		{
			sprintf(q, "%02x ", *((unsigned char*)&p[j]));
			q += strlen(q);
		}

		Trace(TRACEFMT("recv %d bytes: %s"), datalen, buf);
		CO_CanMessage* pMsg = (CO_CanMessage*) data;

		if (datalen >= 2+ pMsg->NoOfBytes) //sizeof(CO_CanMessage))
			USB_LP_CAN1_RX0_IRQHandler((CO_CanMessage*) data);
		
		return datalen;
	}
	
protected:
	
	ZQ::common::InetMcastAddress _group;
	int _gport;
	ZQ::common::InetHostAddress _bind;
	
	bool _bStamp;
	char _stamp[16];
	
	char _buf[1024];
};

McastListener* pTheListener =NULL;

extern "C" void CO_MODIF_Timer1msIsr CO_ISR_OnTimer1ms(void);

class Timer : public ZQ::common::NativeThread
{
public:
	Timer() { }

protected:
	int run()
	{
		int64   stampNow=0, stampLast=0;
		while (!bQuit)
		{
			stampNow = ZQ::common::now();
			int64 diff = stampNow - stampLast;
			stampLast = stampNow;
			if (diff <= 0)
				continue;

			int i = (int) min(5, diff); 
			for (; i >0; i--)
				CO_ISR_OnTimer1ms();
		}
		return 0;
	}
};

Timer theTimer;

class MainLine : public ZQ::common::NativeThread
{
public:
	MainLine() { }

protected:
	int run()
	{
		HTTask_CANopenNode();
/*	// step 6. now the work loop
	while (!bQuit)
	{
		CO_ProcessMain();
		CO_User_OnProcessMain();

		//TODO: a sleep here but must be interruptable by the RxD
	}
*/
		return 0;
	}
};

MainLine theMain;

BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
    switch(CEvent)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
		bQuit = true;
        break;

    }

    return TRUE;
}

void main()
{
	theMain.start();

	static const char prgs[] = "|/-\\";
	int i =0;
	while(!bQuit)
	{
		i = (++i)%4;
//		printf("\rrunning %c         \r", prgs[i]);
		::Sleep(100);
	}
}

// -----------------------------
// TASK: HTTask_CANopenNode()
// desc: The main CAN service task under uC/OS sched.
// param:
// return: none.
// note: none.
// -----------------------------
void HTTask_CANopenNode()
{
	//// simulator code
	Status.BYTE[0] =0;
	bindAddr.setAddress(BIND_ADDR);
	groupAddr.setAddress(GROUP_ADDR);

	// initial a multicast socket and target/join the group
	if (NULL != pSock)
		return;

	pSock = new ZQ::common::UDPMulticast(bindAddr, DEFAULT_PORT);

	if (NULL == pSock)
		return;

	if (!pSock->isActive())
	{
		printf("Error: %s is not a valid local address\n", bindAddr.getHostAddress());
		delete pSock;
		pSock =NULL;
		exit(1);
	}

	pSock->setGroup(groupAddr, DEFAULT_PORT);

	if (NULL != (pTheListener = new McastListener(groupAddr, DEFAULT_PORT, bindAddr)))
		pTheListener->start();

	theTimer.start();
	//// end of simulator code

	// step 1. initialize CANopenNode driver
	CO_Driver_Init();

	// step 2. initialize user's defintion of CANopenNode
	CO_User_OnDriverInited();

	// step 3. reset communication
	CO_OnNMTResetComm();

	//TODO step 4. setup the 1msec interrupt handling: CO_TimerProc(), and enable the timer

	//TODO step 5. setup CAN Rx/Tx interrupt:
	// a) the intrrupt handler: HTTask_ISR_CANRxTx
	// b) config the interrupt to be via level,
	// c) enable the interrupt

	// step 6. now the work loop
	while (!bQuit)
	{
		CO_ProcessMain();
		CO_User_OnProcessMain();

		//TODO: a sleep here but must be interruptable by the RxD
		Sleep(100);
	}

	theMain.start();

	// step 7. cleaning up the CANopenNode
	CO_User_Remove();    // user's defintion of CANopenNode
	CO_Driver_Remove();   // remove Driver

	//// simulator code
	if (NULL != pSock)
		delete pSock;
	pSock =NULL;

	if (NULL != pTheListener)
	{
		delete pTheListener;
	}
	pTheListener = NULL;
	//// end of simulator code
}

// -----------------------------
// func: CO_User_OnDriverInited()
// desc: this is mainline function and is called only in the startup of the program.
// param:
// return: none.
// note: 
// -----------------------------
void CO_User_OnDriverInited(void)
{
	//   ODE_EEPROM.PowerOnCounter++;

	//   TRISCbits.TRISC4 = 0; PORTCbits.RC4 = 0;
	//   TRISCbits.TRISC5 = 0; PORTCbits.RC5 = 0;
}

// -----------------------------
// func: CO_User_Remove()
// desc: this is called before end of HTTask_CANopenNode.
// param:
// return: none.
// note: 
// -----------------------------
void CO_User_Remove(void)
{
	Trace(TRACEFMT(""));
}

// -----------------------------
// CO stack callback: User_ResetComm()
// desc: this is called after start of program and after CANopen NMT command: Reset
//  Communication.
// param:
// return: none.
// note: 
// -----------------------------
void CO_OnNMTResetComm(void)
{
	//Prepare variables for custom made NMT master message.
	//arguments to CO_IDENT_WRITE(CAN_ID_11bit, RTRbit/*usually 0*/)
	CO_TXCAN[NMT_MASTER].Ident.WORD[0] = CO_IDENT_WRITE(0, 0);
	CO_TXCAN[NMT_MASTER].NoOfBytes = 2;
	CO_TXCAN[NMT_MASTER].NewMsg = 0;
	CO_TXCAN[NMT_MASTER].Inhibit = 0;
}

void CO_OnNMTResetNode(void)
{
	Trace(TRACEFMT(""));
}

// ----------------------------------------------------------
// func: CO_User_OnProcessMain()
// desc: This function is cyclycally called from main(). It is non blocking function.
//       It is asynchronous. Here is longer and time consuming code.
// ----------------------------------------------------------
void ClientTest(void);
void CO_User_OnProcessMain(void)
{
//	Trace(TRACEFMT(""));
	ClientTest();
}

// -----------------------------
// CO stack callback: CO_User_OnProcess1ms()
// desc: this is executed every 1 ms. It is deterministic and has priority over
//       mainline functions.
// param:
// return: none.
// note: 
// -----------------------------
void CO_User_OnProcess1ms(void)
{
	/*
	static unsigned int DeboucigTimer = 0;

	if(DigInp_Button) DeboucigTimer++;
	else DeboucigTimer = 0;

	switch(DeboucigTimer)
	{
	case 1000:  //button is pressed for one seccond
	if(CO_NMToperatingState == NMT_OPERATIONAL)
	{
	CO_TXCAN[NMT_MASTER].Data.BYTE[0] = NMT_ENTER_PRE_OPERATIONAL;
	CO_NMToperatingState = NMT_PRE_OPERATIONAL;
	}
	else
	{
	CO_TXCAN[NMT_MASTER].Data.BYTE[0] = NMT_ENTER_OPERATIONAL;
	CO_NMToperatingState = NMT_OPERATIONAL;
	}

	CO_TXCAN[NMT_MASTER].Data.BYTE[1] = 0;  //all nodes
	CO_TXCANsend(NMT_MASTER);
	break;

	case 5000:  //button is pressed for 5 seconds
	//reset all remote nodes
	CO_TXCAN[NMT_MASTER].Data.BYTE[0] = NMT_RESET_NODE;
	CO_TXCAN[NMT_MASTER].Data.BYTE[1] = 0;  //all nodes
	CO_TXCANsend(NMT_MASTER);
	break;

	case 5010: //button is pressed for 5 seconds + 10 milliseconds
	CO_Reset(); //reset this node
	}

	//display variables
	DigOut_COOLER_IND = STATUS_COOLER;
	DigOut_HEATER_IND = STATUS_HEATER;
	Temperature = RemoteTemperature;
	*/
}

#define RemoteTemperature CO_RPDO(0).WORD[0]

#ifdef CO_VERIFY_OD_WRITE
void CO_User_VerifyWriteODEntry(ROM CO_objectDictionaryEntry* pODE, void* data)
{
	unsigned int index = pODE->index;

	if (index > 0x1200 && index <= 0x12FF) index &= 0xFF80;//All SDO
	if (index > 0x1400 && index <= 0x1BFF) index &= 0xFE00;//All PDO

	if (index < 0x2000)
		return 0x08000020L;

	switch (index)
	{
		unsigned char i;
	case 0x2101://CAN Node ID
		if (*((unsigned char*)data) < 1)
			return 0x06090032L;  //Value of parameter written too low
		else if (*((unsigned char*)data) > 127)
			return 0x06090031L;  //Value of parameter written too high
		break;

	case 0x2102://CAN Bit RAte
		if (*((unsigned char*)data) > 7)
			return 0x06090031L;  //Value of parameter written too high
		break;

	case 0x3000://TempLo
		if ((*((unsigned int*)data) > 35) || 
			(*((unsigned int*)data) >= TempHi))
			return 0x06090031L;  //Value of param. written too high
		break;
	case 0x3001://TempHi
		if ((*((unsigned int*)data) < 5) || 
			(*((unsigned int*)data) <= TempLo))
			return 0x06090032L;  //Value of param. written too low
		break;

	}//end switch
	return 0L;
}

#endif // CO_VERIFY_OD_WRITE

// extern uint8_t CO_ErrorStatusBits[ERROR_NO_OF_BYTES];

// -----------------------------
// table CO_OD_User
// desc: the definition of Object Dictionary consists of OD_ENTRY
// note: entries of CO_objectDictionaryEntry should be sorted. If not, disable the macro CO_OD_IS_ORDERED
// -----------------------------
ROM CO_objectDictionaryEntry CO_OD_User[] = {

	// sect 0x2000-0x5FFF   Manufacturer specific
	// Manufacturer specific OD entries starts here:
	// -----------------------------------------------
	OD_ENTRY(0x2100, 0x00, ATTR_RO, CO_ErrorStatusBits),

	OD_ENTRY(0x2101, 0x00, ATTR_RW|ATTR_ROM, ODE_CANnodeID),
	OD_ENTRY(0x2102, 0x00, ATTR_RW|ATTR_ROM, ODE_CANbitRate),

#if CO_NO_SYNC > 0
	OD_ENTRY(0x2103, 0x00, ATTR_RW, CO_SYNCcounter),
	OD_ENTRY(0x2104, 0x00, ATTR_RO, CO_SYNCtime),
#endif

	OD_ENTRY(0x2106, 0x00, ATTR_RO, ODE_EEPROM.PowerOnCounter),

	OD_ENTRY(0x3000, 0x00, ATTR_RW|ATTR_ROM, TempLo),
	OD_ENTRY(0x3001, 0x00, ATTR_RW|ATTR_ROM, TempHi),
	OD_ENTRY(0x3100, 0x00, ATTR_RO, Status),
	OD_ENTRY(0x3200, 0x00, ATTR_RWW, RemoteTemperature),
};

// number of OD entries in this Object Dictionary
ROM uint16_t CO_OD_Size_User = sizeof(CO_OD_User) / sizeof(CO_objectDictionaryEntry);

/////////////////////////

void Simulator_send(uint8_t* buf, int len)
{
	bool bSent =false;
#ifdef _MCASTMSG
	if (NULL != pSock && pSock->send((void*)buf, len) >0)
	{
//		logger.hexDump(ZQ::common::Log::L_INFO, buf, len, "Message sent");
		printf("\rSent %d bytes: ", len);
		for (int j=0; j<len; j++)
			printf("%02x ", buf[j]);
		printf("\n");
	}
#else			
	ZQ::common::MutexGuard g(msgLock);
	memcpy(msgBuf, buf, len);
	msgLen = len;
	bSent = true;

#endif _MCASTMSG
	if (!bSent)
		return;
	char msg[100], *p = msg;
	for (int j=0; j < sizeof(msg)/3 && j<len; j++)
	{ sprintf(p, "%02x ", buf[j]); p +=strlen(p); } 

	Trace(TRACEFMT("sent %d bytes: %s"), len, msg);
}

char* toHexString(char* strbuf, unsigned char* value, int len)
{
	char *p =strbuf;
	if (NULL ==p)
		return "";

	for (int j=0; j < len; j++)
	{ sprintf(p, "%02x ", value[j]); p +=strlen(p); } 

	return strbuf;
}

///////////////////////////////////////
typedef struct {
   unsigned char nodeID;
   unsigned int  index;
   unsigned char subIndex;
   unsigned char length;
} SCVar;

SCVar SCVars[] = {
	{ 0x08, 1, 2, 1},
	{ 0, 0, 0, 0},
};

void ClientTest(void)
{
   char strbuf[1024];
   static unsigned int stepNo =0;
   static int idxSCVar =0;
   int c=0;

   static int tick =0;
//   if (++tick % 10)
//	   return;
   tick =0;

   unsigned char SCValue[CO_MAX_OD_ENTRY_SIZE];
   static unsigned int SC_AbortCode;

   switch (stepNo)
   {
   case 0: 
	   if (0 == SCVars[idxSCVar].index && 0 == SCVars[idxSCVar].nodeID)
	   {
			idxSCVar =0;
			break;
	   }

	   Trace(TRACEFMT("reading NodeId[%x], index[%x/%x], len[%d]"), SCVars[idxSCVar].nodeID, SCVars[idxSCVar].index, SCVars[idxSCVar].subIndex, SCVars[idxSCVar].length);
      stepNo++; break;

   case 1:
	   c = CO_SDOclient_setup(0, 0, 0, SCVars[idxSCVar].nodeID);
	   if (c == -2)
	   {
		   Trace(TRACEFMT("CO_SDOclient_setup() error(%d)"), c);
		   idxSCVar++;
		   stepNo=0; break;
	   }

      stepNo++; break;

   case 2:
      CO_SDOclientUpload_init(0, SCVars[idxSCVar].index, SCVars[idxSCVar].subIndex, SCValue, CO_MAX_OD_ENTRY_SIZE);
      stepNo++; break;

   case 3:
      c = CO_SDOclientUpload(0, &SC_AbortCode, &SCVars[idxSCVar].length);
	  if (c == 0)
	  {       //success?
		  Trace(TRACEFMT("read NodeId[%x], index[%x/%x], len[%d]: %s"), SCVars[idxSCVar].nodeID, SCVars[idxSCVar].index, SCVars[idxSCVar].subIndex, SCVars[idxSCVar].length,
			  toHexString(strbuf, SCValue, SCVars[idxSCVar].length));
		  stepNo++; break;
	  }
      
	  if(c < 0)
	  {   //error
         if(c == -10)
		  Trace(TRACEFMT("read Abort: %0x"), SC_AbortCode);
         else if(c == -11)
		  Trace(TRACEFMT("SDO timed out."));
         else Trace(TRACEFMT("Error: %d"), c);

		 stepNo++; break;
      }
      break;

    case 4:
		idxSCVar++;
		 stepNo=0; break;
   }
}
