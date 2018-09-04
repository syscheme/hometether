#ifndef __ICAN_H__
#define __ICAN_H__

#include "ican_cfg.h"

// -----------------------------
// Declarition of global variables
// -----------------------------
// ican protocol source define 
// 0x00 to 0x7f is used as ican io source
// 0x80 to 0xff is used as ican config source
// ican io source include :
//   di source			do source 			ai source 
//   ao source 			serical0 source		serical1 source 
//   user_def source 	reserve
// ican config source include :
//   device identify source			communicate parameters source
//   io parameters source			io config source		

// ican io source define
#define iCanResource_DI									0x00
#define iCanResource_DO									0x01
#define iCanResource_AI									0x02
#define iCanResource_AO									0x03
#define iCanResource_SERIAL0							0x04
#define iCanResource_SERIAL1							0x05
#define iCanResource_USERDEF							0x06

// ican config source define
#define iCanConf_DEVICE_INFO						0x80
#define iCanConf_COMMUNICATE						0x81
#define iCanConf_IO_PARAM							0x82
#define iCanConf_IO									0x83
#define ICAN_ESTABLISH_CONNECT						0xf7

// ican config source sub offset define
#define OFFSET_VENDOR_ID							0x00
#define OFFSET_PRODUCT_TYPE							0x02
#define OFFSET_PRODUCT_CODE							0x04
#define OFFSET_HARDWARE_VERSION						0x06
#define OFFSET_FIRMWARE_VERSION						0x08
#define OFFSET_SERIAL_NUMBER						0x0a

#define OFFSET_MAC_ID								0x00
#define OFFSET_BAUDRATE								0x01
#define OFFSET_USER_BAUDRATE						0x02
#define OFFSET_CYCLIC_PARAM							0x06
#define OFFSET_CYCLIC_MASTER						0x07
#define OFFSET_COS_TYPE								0x08
#define OFFSET_MASTER_MAC_ID						0x09

#define OFFSET_DI_PARAM								0x00
#define OFFSET_DO_PARAM								0x01
#define OFFSET_AI_PARAM								0x02
#define OFFSET_AO_PARAM								0x03
#define OFFSET_SERIAL0_PARAM						0x04
#define OFFSET_SERIAL1_PARAM						0x05
#define OFFSET_USERDEF_PARAM						0x06

// ican function code define
typedef enum _iCanFunc
{
	iCanFunc_Reserve       =0x00,
	iCanFunc_WRITE,
	iCanFunc_READ,
	iCanFunc_EVE_TRAGER,
	iCanFunc_EST_CONNECT,
	iCanFunc_DEL_CONNECT,
	iCanFunc_DEV_RESET,
	iCanFunc_MAC_CHECK,
	iCanFunc_ERROR         =0x0f,
} iCanFunc;

// ican协议状态机宏
typedef enum _iCanState
{
	iCanState_IDLE 		   =0x00,
	iCanState_CHECK,
	iCanState_RECV,
	iCanState_SEND,
	iCanState_PARSE,
	iCanState_ERROR,
} iCanState;

// ican协议分段传输标志宏
typedef enum _iCanMessageSegment
{
	iCanMsg_NO_SPLIT_SEG		= 0x00,
	iCanMsg_SPLIT_SEG_FIRST,
	iCanMsg_SPLIT_SEG_MID,
	iCanMsg_SPLIT_SEG_LAST,
} iCanMsgSegment; 

// ican协议连接状态
typedef enum _iCanConnState
{
	iCanConn_Connected				= 0x01,
	iCanConn_Timeout,
	iCanConn_Disconnected,
} iCanConnState; 

// ican frame define
typedef struct _iCanId
{
	uint32_t	source_id:8, func_id:4, ack:1, dest_mac_id:8, src_mac_id:8, rev:3;
} iCanId;

typedef struct _iCanFrame
{
	iCanId	id;
	uint8_t	dlc;
	uint8_t	frame_data[8];
} iCanFrame;

typedef struct _iCanResField
{ 
	uint8_t* data;
	uint8_t  len;
} iCanResField;

typedef struct _iCanResource
{
	// iCanResourceFields must be the first member of _iCanResource
	iCanResField DI, DO, AI, AO, serial0, serial1, userdef;
/*	uint8_t* di_data; 
	uint8_t  di_length;

	uint8_t* do_data;
	uint8_t  do_length;

	uint8_t* ai_data;
	uint8_t  ai_length;

	uint8_t* ao_data;
	uint8_t  ao_length;

	uint8_t* serial0_data;
	uint8_t  serial0_length;

	uint8_t* serial1_data;
	uint8_t  serial1_length;

	uint8_t* userdef_data;
	uint8_t  userdef_length;

#if ICAN_DI_LEN
	//byte +00----------------+01----------------+
	//     |    status of motion detections      |
	//     +02----------------+03----------------+
	//     |    up to 8 word-long tempertures    |
	//     |				...					 |
	//     +18----------------+19----------------+
	//     |    up to 4 byte-byte temp-humidity  |
	//     |	     pair    ...				 |
	//     +26----------------+27----------------+
#endif

#if ICAN_DO_LEN
	//byte +00----------------+01----------------+
	//     | mask of employed motion detections  |
	//     +02----------------+03----------------+
	//     | mask of DS18B20  | mask of DH11     |
	//     +04----------------+05----------------+
	//     | relayA           | relayB           |
	//     +06----------------+07----------------+
	//     | mask of lights   |                  |
	//     +08----------------+09----------------+
	uint8_t do_data[ICAN_DO_LEN];
#endif

#if ICAN_AI_LEN
	//byte +00----------------+01----------------+
	//     | 8 byte-long light adc values        |
	//     +08----------------+09----------------+
	uint8_t ai_data[ICAN_AI_LEN];
#endif

#if ICAN_AO_LEN
	uint8_t ao_data[ICAN_AO_LEN];
#endif

#if ICAN_SER0_LEN
	uint8_t serial0_data[ICAN_SER0_LEN];
#endif

#if ICAN_SER1_LEN
	uint8_t serial1_data[ICAN_SER1_LEN];
#endif

#if ICAN_USER_LEN
	uint8_t userdef_data[ICAN_USER_LEN];
#endif
*/
	void * arg;
} iCanResource;

typedef struct _iCanDevInfo
{
	uint16_t vendor_id;
	uint16_t product_type;
	uint16_t product_code;
	uint16_t hardware_version;
	uint16_t firmware_version;
	uint32_t serial_number;
} iCanDevInfo;

typedef struct _iCanCommCfg
{
	uint8_t  	dev_mac_id;
	uint8_t  	baud_rate;
	uint32_t    user_baud_rate;
	uint8_t  	cyclic_param;
	uint8_t  	cyclic_master;
	uint8_t  	cos_type;
	uint8_t  	master_mac_id;
} iCanCommCfg;

/*
typedef struct _ioParams
{
	uint8_t di_length;
	uint8_t do_length;
	uint8_t ai_length;
	uint8_t ao_length;
	uint8_t serial0_length;
	uint8_t serial1_length;
	uint8_t userdef_length;
} iCanIoParams;
*/

typedef enum _iCanError
{
	iCanErr_OK =0,
	iCanErr_BAD_PARAMS,
	iCanErr_FUNC_ID,
	iCanErr_RESOURCE_ID,
	iCanErr_COMMAND,
	iCanErr_COMMUNICATE,
	iCanErr_OPERATE,
	iCanErr_TRANS,
	iCanErr_MAC_UNMATCH,
	iCanErr_TIME_OUT,
	iCanErr_BUFF_OVERFLOW,
	iCanErr_LEN_ZERO,
	iCanErr_NODE_BUSY,
	iCanErr_DEL_NODE,
	iCanErr_NODE_EXIST,
	iCanErr_MAC_EXIST,
	ICAN_SETUP_CONNECT,

} iCanError;

typedef struct _iCanNode
{
	uint8_t 	    channelId;
	uint8_t 	    addr;
	iCanState       state;
//	uint8_t         flags;

	iCanConnState	slaveState;  // the state when this node acts as slave to accept external queries or updates
	iCanDevInfo 	devInfo;
	iCanCommCfg 	commCfg;
	iCanResource 	resource;
//	iCanIoParams	ioParams;

} iCanNode;

extern iCanNode thisNode;

// message temp struct used in implementation
typedef struct _ican_temp
{	
	uint8_t cur_flag;
	uint8_t old_flag;
	uint8_t cur_num;
	uint8_t old_num;
	uint8_t old_src_id;
	uint8_t cur_src_id;
	uint8_t temp_offset;
	uint8_t temp_length;
	uint8_t temp_buff[MAX_DATA_BUFF];
} ican_temp;

// -----------------------------
// funcs of iCAN frame
// -----------------------------
uint8_t iCanFrame_getSrcMacId(iCanFrame* pframe);
void    iCanFrame_setSrcMacId(iCanFrame* pframe, uint8_t src_macid);

uint8_t iCanFrame_getDestMacId(iCanFrame* pframe);
void    iCanFrame_setDestMacId(iCanFrame* pframe, uint8_t dest_macid);

uint8_t iCanFrame_getFuncId(iCanFrame* pframe);
void    iCanFrame_setFuncId(iCanFrame* pframe, uint8_t func_id);

uint8_t iCanFrame_getResourceId(iCanFrame* pframe);
void    iCanFrame_setResourceId(iCanFrame* pframe, uint8_t source_id);

uint8_t iCanFrame_ackNeeded(iCanFrame* pframe);
void    iCanFrame_setAck(iCanFrame* pframe, uint8_t ack);

uint8_t iCanFrame_getSplitFlag(iCanFrame* pframe);
void    iCanFrame_setSplitFlag(iCanFrame* pframe, uint8_t split_flag);

uint8_t iCanFrame_getSplitNum(iCanFrame* pframe);
void    iCanFrame_setSplitNum(iCanFrame* pframe, uint8_t split_num);
void    iCanFrame_clearSplitInfo(iCanFrame* pframe);

uint8_t iCanFrame_getOffset(iCanFrame* pframe);
void    iCanFrame_setOffset(iCanFrame* pframe, uint8_t offset);

uint8_t iCanFrame_getLength(iCanFrame* pframe);
void    iCanFrame_setLength(iCanFrame* pframe, uint8_t length);

// -----------------------------
// funcs of iCAN portal
// -----------------------------
void    iCanPortal_send(uint8_t channel, iCanFrame* pframe);
void    iCanPortal_recv(uint8_t channel, iCanFrame* pframe);
uint8_t iCanPortal_devAddr(void);

// -----------------------------
// about LED signals
// -----------------------------
void iCanPortal_LED_sysRun(void);
void iCanPortal_LED_canOK(void);
void iCanPortal_LED_canErr(void);

// -----------------------------
// about the iCAN node
// -----------------------------
void     iCanNode_init(iCanNode* pNode, uint8_t channelId);
uint8_t  iCanNode_ping(iCanNode* pLocalNode, uint8_t destMacId);
uint8_t  iCanNode_sendFrame(iCanNode* pLocalNode, iCanFrame* pmsg);
uint8_t  iCanNode_recvFrame(iCanNode* pLocalNode, iCanFrame* pmsg);

uint16_t iCanNode_VendorId(iCanNode* pNode);
uint16_t iCanNode_ProductType(iCanNode* pNode);
uint16_t iCanNode_ProductCode(iCanNode* pNode);
uint16_t iCanNode_HardwareVersion(iCanNode* pNode);
uint16_t iCanNode_FirmwareVersion(iCanNode* pNode);
uint32_t iCanNode_SerialNum(iCanNode* pNode);
uint8_t  iCanNode_MacId(iCanNode* pNode);
void     iCanNode_setMacId(iCanNode* pNode, uint8_t macId);
uint8_t  iCanNode_Baudrate(iCanNode* pNode);
void     iCanNode_setBaudrate(iCanNode* pNode, uint8_t rate);
uint32_t iCanNode_UserBaudrate(iCanNode* pNode);
void     iCanNode_setUserBaudrate(iCanNode* pNode, uint32_t usr_rate);
uint8_t  iCanNode_CyclicParam(iCanNode* pNode);
void     iCanNode_setCyclicParam(iCanNode* pNode, uint8_t time);
uint8_t  iCanNode_CyclicMaster(iCanNode* pNode);
void     iCanNode_setCyclicMaster(iCanNode* pNode, uint8_t time);
uint8_t  iCanNode_CosType(iCanNode* pNode);
void     iCanNode_setCosType(iCanNode* pNode, uint8_t type);
uint8_t  iCanNode_MasterMacId(iCanNode* pNode);
void     iCanNode_setMasterMacId(iCanNode* pNode, uint8_t macId);

#endif  // __ICAN_H__
