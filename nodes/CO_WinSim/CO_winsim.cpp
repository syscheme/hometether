// ===========================================================================
// Copyright (c) 1997, 1998 by
// CO_driver.c - Variables and Object Dictionary for CANopenNode
//
// Ident : $Id: CO_Driver.c$
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : For short description of standard Object Dictionary entries see CO_OD.txt
// processor: STM32F10x
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/CO_driver.c $
// ===========================================================================
extern "C" {
#include "CANopen.h"
#include <time.h>
}

// ---------------------------------------------------------------------------
// Variables and functions
// ---------------------------------------------------------------------------

//Node-ID
uint8_t CO_NodeID;                                 //Variable must be int16_tialized here
uint8_t CO_BitRate;                                //Variable must be int16_tialized here

//eeprom
extern uint8_t* CO_EEPROM_point16_ter;
extern ROM uint16_t CO_EEPROM_size;

//function for verifying values at write to Object Dictionary
uint32_t CO_OD_VerifyWrite(ROM CO_objectDictionaryEntry* pODE, void* data);

//other
extern volatile CO_DEFAULT_IntegerType CO_TXCANcount;
#if CO_NO_SYNC > 0
extern volatile uint16_t CO_SYNCwindow;
#endif

//function for recording CAN trace
#if CO_CAN_TRACE_SIZE > 0
void CO_CgiCANtraceRecord(uint8_t* record);
#endif

// ---------------------------------------------------------------------------
// CO_Driver_Init - CANopenNode DRIVER INITIALIZATION
// This is mainline function and is called only in the startup of the program.
// ---------------------------------------------------------------------------
void CO_Driver_Init(void)
{
	// some initialization steps here, such as
	// 1. LED lights
	// 2. read the configuration from EEPROM
	// 3. check the last down reason from EEPROM
	// 4. hook the NMI interrupt, and so on
	// print16_tf("CANopen driver initialized");
}

// ---------------------------------------------------------------------------
// CO_Driver_Remove()
// function is executed on exit of program (no reboot)
// ---------------------------------------------------------------------------
void CO_Driver_Remove(void)
{
	// Deinstall NMI handler
	// save the data int16_to EEPROM
}

// ---------------------------------------------------------------------------
// CO_Driver_OnProcessMain() - PROCESS MICROCONTROLLER SPECIFIC CODE
// this is mainline function and is called cyclically from CO_ProcessMain().
// ---------------------------------------------------------------------------
void CO_Driver_OnProcessMain(void)
{
}

/*
// ---------------------------------------------------------------------------
// CO_Driver_ReadNodeIdBitrate() - READ NODE-ID AND CAN BITRATE
// This is mainline function and is called from Communication reset. Usually
// NodeID and BitRate are read from DIP switches.
// ---------------------------------------------------------------------------
void CO_Driver_ReadNodeIdBitrate(void)
{
	CO_NodeID = ODE_CANnodeID;    //range 1 to 127
	CO_BitRate = ODE_CANbitRate;  //range 0 to 7
	// 0 = 10 kbps    1 = 20 kbps
	// 2 = 50 kbps    3 = 125 kbps
	// 4 = 250 kbps   5 = 500 kbps
	// 6 = 800 kbps   7 = 1000 kbps

	if (CO_NodeID==0 || CO_NodeID>127 || CO_BitRate > 7)
	{
		ErrorReport(ERROR_WrongNodeIDorBitRate, (CO_NodeID<<8)|CO_BitRate);
		CO_NodeID = 1;
		if (CO_BitRate > 7) CO_BitRate = 1;
	}
}
*/

// ---------------------------------------------------------------------------
// CO_Driver_SetupCAN() - init CAN controller int16_terface
// This is mainline function and is called from Communication reset.
// ---------------------------------------------------------------------------
void CO_Driver_SetupCAN(void)
{
	/*
	CAN_InitTypeDef        CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;

	// TODO: CAN register init
	CAN_DeInit(CAN1);							      //复位CAN1的所有寄存器
	CAN_StructInit(&CAN_InitStructure);				  //将寄存器全部设置成默认值

	// CAN cell init
	CAN_InitStructure.CAN_TTCM      =DISABLE;                       //禁止时间触发通信方式
	CAN_InitStructure.CAN_ABOM      =DISABLE;                       //禁止CAN总线自动关闭管理
	CAN_InitStructure.CAN_AWUM      =DISABLE;                       //禁止自动唤醒模式
	CAN_InitStructure.CAN_NART      =DISABLE;                       //禁止非自动重传模式
	CAN_InitStructure.CAN_RFLM      =DISABLE;                       //禁止接收FIFO锁定
	CAN_InitStructure.CAN_TXFP      =DISABLE;                       //禁止发送FIFO优先级
	CAN_InitStructure.CAN_Mode      =CAN_Mode_Normal;               //设置CAN工作方式为正常收发模式
	CAN_InitStructure.CAN_SJW       =CO_BitRateData[CO_BitRate].SJW;//设置重新同步跳转的时间量子
	CAN_InitStructure.CAN_BS1       =CO_BitRateData[CO_BitRate].BRP;//设置字段1的时间量子数
	CAN_InitStructure.CAN_BS2       =CAN_BS2_7tq;                   //设置字段2的时间量子数
	CAN_InitStructure.CAN_Prescaler =1;                             //配置时间量子长度为1周期
	CAN_Init(CAN1, &CAN_InitStructure);                             //用以上参数初始化CAN1端口

	// CAN filter init
	CAN_FilterInitStructure.CAN_FilterNumber         =0;                     //选择CAN过滤器0
	CAN_FilterInitStructure.CAN_FilterMode           =CAN_FilterMode_IdMask; //初始化为标识/屏蔽模式
	CAN_FilterInitStructure.CAN_FilterScale          =CAN_FilterScale_32bit; //选择过滤器为32位
	CAN_FilterInitStructure.CAN_FilterIdHigh         =0x0000;                //过滤器标识号高16位
	CAN_FilterInitStructure.CAN_FilterIdLow          =0x0000;				 //过滤器标识号低16位
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh     =0x0000;			     //根据模式选择过滤器标识号或屏蔽号的高16位
	CAN_FilterInitStructure.CAN_FilterMaskIdLow      =0x0000;				 //根据模式选择过滤器标识号或屏蔽号的低16位
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment =CAN_FIFO0;		     //将FIFO 0分配给过滤器0
	CAN_FilterInitStructure.CAN_FilterActivation     =ENABLE;			     //使能过滤器
	CAN_FilterInit(&CAN_FilterInitStructure);

	////////////////////////
	uint8_t i;
	// Setup CAN bus
	hal_outportb(CO_ADDRESS_CAN, 1);          //Mode Register - reset mode
	hal_outportb(CO_ADDRESS_CAN+6,            //Bus Timing Register 0
	((CO_BitRateData[CO_BitRate].SJW-1)<<6) |
	(CO_BitRateData[CO_BitRate].BRP-1));

	hal_outportb(CO_ADDRESS_CAN+7,            //Bus Timing Register 1
	((CO_BitRateData[CO_BitRate].PhSeg2-1)<<4) |
	(CO_BitRateData[CO_BitRate].PROP + CO_BitRateData[CO_BitRate].PhSeg1 - 1));

	hal_outportb(CO_ADDRESS_CAN+8, 0x1A);     //Output Control Register
	hal_outportb(CO_ADDRESS_CAN+31, 0xC8);    //Clock Divider Register

	hal_outportb(CO_ADDRESS_CAN+1, 0x0E);     //Command Register - clear...

	//test hardware
	hal_outportb(CO_ADDRESS_CAN+16, 0xAA);    //Acceptance Code Register 0
	hal_outportb(CO_ADDRESS_CAN+17, 0x55);    //Acceptance Code Register 1
	i = hal_inportb(CO_ADDRESS_CAN+16);
	if (i != 0xAA)
	print16_tf("ERROR in communication with SJA1000, read 0x%02X, should be 0xAA\n", i);

	i = hal_inportb(CO_ADDRESS_CAN+17);
	if (i != 0x55)
	print16_tf("ERROR in communication with SJA1000, read 0x%02X, should be 0x55\n", i);

	//set filters and masks to accept all messages
	hal_outportb(CO_ADDRESS_CAN+16, 0xFF);    //Acceptance Code Register 0
	hal_outportb(CO_ADDRESS_CAN+17, 0xFF);    //Acceptance Code Register 1
	hal_outportb(CO_ADDRESS_CAN+18, 0xFF);    //Acceptance Code Register 2
	hal_outportb(CO_ADDRESS_CAN+19, 0xFF);    //Acceptance Code Register 3
	hal_outportb(CO_ADDRESS_CAN+20, 0xFF);    //Acceptance Mask Register 0
	hal_outportb(CO_ADDRESS_CAN+21, 0xFF);    //Acceptance Mask Register 1
	hal_outportb(CO_ADDRESS_CAN+22, 0xFF);    //Acceptance Mask Register 2
	hal_outportb(CO_ADDRESS_CAN+23, 0xFF);    //Acceptance Mask Register 3

	hal_inportb(CO_ADDRESS_CAN+3);            //Interrupt Register - clear int16_terrupts
	//enable receive, transmit and error int16_terrupts
	hal_outportb(CO_ADDRESS_CAN+4, 0x2F);     //Interrupt Enable Register
	hal_outportb(CO_ADDRESS_CAN, 0);          //Mode Register - normal mode
	*/
}

void Simulator_send(uint8_t* buf, int len);
typedef struct
{
  uint32_t StdId;
  uint32_t ExtId;
  uint8_t IDE;
  uint8_t RTR;
  uint8_t DLC;
  uint8_t Data[8];
  uint8_t FMI;
} CanMsg;

// ---------------------------------------------------------------------------
// CO_Driver_Post2Txn() - Copy CAN mesage to CAN controller
// param index - index of message in CO_TXCAN
// ---------------------------------------------------------------------------
void CO_Driver_Post2Txn(uint16_t INDEX)
{
/*
CanMsg TxMessage;
	memset(&TxMessage, 0x00, sizeof(TxMessage));
	TxMessage.StdId  =CO_TXCAN[INDEX].Ident.WORD[0];  // use the stand id
	TxMessage.ExtId  =0x00;
	TxMessage.RTR    =CO_IDENT_READ_RTR(CO_TXCAN[INDEX].Ident);
	TxMessage.DLC    =CO_TXCAN[INDEX].NoOfBytes & 0x3f;
	for (int i =0; i < TxMessage.DLC; i++)
		TxMessage.Data[i] =CO_TXCAN[INDEX].Data.BYTE[i];
*/
	Simulator_send((uint8_t*) &CO_TXCAN[INDEX], CO_TXCAN[INDEX].NoOfBytes +4); //sizeof(CO_TXCAN[INDEX]));

/*


#if CO_CAN_TRACE_SIZE > 0
	uint8_t Data[12];
#endif

	CanTxMsg TxMessage;
	uint32_t i = 0;
	uint8_t TxMailbox;
#if 0
	CAN_InitTypeDef CAN_InitStructure;
	CAN_InitStructure.CAN_TTCM=DISABLE;
	CAN_InitStructure.CAN_ABOM=DISABLE;
	CAN_InitStructure.CAN_AWUM=DISABLE;
	CAN_InitStructure.CAN_NART=DISABLE;
	CAN_InitStructure.CAN_RFLM=DISABLE;
	CAN_InitStructure.CAN_TXFP=DISABLE;
	CAN_InitStructure.CAN_Mode=CAN_Mode_Normal;
	CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;
	CAN_InitStructure.CAN_BS1=CAN_BS1_8tq;
	CAN_InitStructure.CAN_BS2=CAN_BS2_7tq;
	CAN_InitStructure.CAN_Prescaler=1;
	CAN_Init(CAN1, &CAN_InitStructure);
#endif

	// transmit a message
	TxMessage.IDE    =CAN_ID_STD;  // using the standard frame in CANopen
	TxMessage.StdId  =CO_TXCAN[INDEX].Ident.WORD[0];  // use the stand id
	TxMessage.ExtId  =0x00;
	TxMessage.RTR    =CO_IDENT_READ_RTR(CO_TXCAN[INDEX].Ident);
	TxMessage.DLC    =CO_TXCAN[INDEX].NoOfBytes & 0x3f;
	for (i =0; i < TxMessage.DLC; i++)
		TxMessage.Data[i] =CO_TXCAN[INDEX].Data.BYTE[i];

	TxMailbox =CAN_Transmit(CAN1, &TxMessage);   // send the message and remember the mailbox

#if 0 
	// test ever complete the sending
	for (i = 0; i < 1000 & CANTXOK == CAN_TransmitStatus(CAN1, TransmitMailbox); i++)
		delayUS(10);

	// test if there are any pending messages
	for (i = 0; i < 1000 & CANTXOK == CAN_MessagePending(CAN1, CAN_FIFO0); i++)
		delayUS(10);
#endif

#if CO_CAN_TRACE_SIZE > 0
	// save to trace buffer
	if (CO_IDENT_READ_RTR(CO_TXCAN[INDEX].Ident) == 0) Data[0] = 't';
	else Data[0] = 'T';
	Data[1] = (uint8_t)CO_IDENT_READ_COB(CO_TXCAN[INDEX].Ident);
	Data[2] = (uint8_t)(CO_IDENT_READ_COB(CO_TXCAN[INDEX].Ident)>>8);
	Data[3] = CO_TXCAN[INDEX].NoOfBytes;
	memcpy(&Data[4], &CO_TXCAN[INDEX].Data.BYTE[0], 8);
	CO_CgiCANtraceRecord(Data);
#endif
*/
}

// ---------------------------------------------------------------------------
// USB_LP_CAN1_RX0_IRQHandler() - interrupt ISR to handle CAN Rx
// ---------------------------------------------------------------------------
void USB_LP_CAN1_RX0_IRQHandler(CO_CanMessage* pRxMessage)
{
	CO_CanMessage& RxMessage = *pRxMessage;

/*
	// while (CAN_MessagePending(CAN1, CAN_FIFO0) >0) {
	RxMessage.StdId=0x00;
	RxMessage.ExtId=0x00;
	RxMessage.IDE=0;
	RxMessage.DLC=0;
	RxMessage.FMI=0;
	RxMessage.Data[0]=0x00;
	RxMessage.Data[1]=0x00;
	CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);	  // read CAN1's FIFO to RxMessage
*/

	CO_ISR_OnFrameReceived(CO_IDENT_READ_COB(RxMessage.Ident), RxMessage.NoOfBytes, CO_IDENT_READ_RTR(RxMessage.Ident), RxMessage.Data.BYTE);
	// } // while CAN_MessagePending()
}

// ---------------------------------------------------------------------------
// USB_LP_CAN1_RX0_IRQHandler() - interrupt ISR to handle CAN Tx
// ---------------------------------------------------------------------------
void USB_HP_CAN1_TX_IRQHandler(void)
{
	CO_ISR_OnTxnUnderflow();
}

#ifdef CAN_STATUSERR_INT
// ---------------------------------------------------------------------------
// CAN1_SCE_IRQHandler() - interrupt ISR to handle CAN state change/error
// ---------------------------------------------------------------------------
void CAN1_SCE_IRQHandler(void)
{
	// cases of CAN1_SCE_IRQ
	// 1) error occured, need to read the register CAN->ESR
	// 2) wakedup by seeing SOF on the bus
	// 3) entering sleep mode
	// TODO: prepare the parameters for CO_ISR_OnStateChangedOrError()
	CO_ISR_OnStateChangedOrError(0);
}
#endif // CAN_STATUSERR_INT

#ifdef CO_SAVE_ROM
// ---------------------------------------------------------------------------
// CO_Driver_SaveOD() - Save the OD value to ROM
// param index - index of message in CO_TXCAN
//@ return 0 - succeeded
// ---------------------------------------------------------------------------
uint8_t CO_Driver_SaveOD(ROM CO_objectDictionaryEntry* pODE, int32_t rollSize)
{
	FILE *stream =NULL;
	if ((stream = fopen(CO_OD_ROM_FileName, "at")) != NULL)
	{
		if (rollSize >0 && (filelength(fileno(stream))+1) >= rollSize)
		{
			// file has been too long, create a new one instead
			fclose(stream);

			//make a backup *.bak firstly
			int8_t FileNameBak[256];
			int8_t *pStr;
			strcpy(FileNameBak, CO_OD_ROM_FileName);
			pStr = strchr(FileNameBak, '.');
			if (pStr == 0)
			{
				pStr = FileNameBak + strlen(FileNameBak);
				*pStr = '.';
			}

			*(++pStr) = 'b'; *(++pStr) = 'a'; *(++pStr) = 'k'; *(++pStr) = 0;
			remove(FileNameBak);
			rename(CO_OD_ROM_FileName, FileNameBak);
		}
	}

	//create new file if has been previous closed or not pre-exist
	if (NULL == stream)
	{
		if (stream = fopen(CO_OD_ROM_FileName, "wt")) != NULL)
			return -1;

		// stamp the time
		fputs("*** file created\t", stream); fprintf_date(stream); fputs("\n", stream);
	}

	// write the input OD into the file
	uint8_t *DataCopy = (uint8_t*)pODE->pData;
	int16_t j;
	fprint16_tf(stream, "%04X\t%02X\t0:\t", pODE->index, pODE->subindex);
	for (j=pODE->length-1; j>=0; j--)
		fprint16_tf(stream, "%02X", DataCopy[j]);

	if (CO_OD_ROM_TimeStamp)
	{
		fputs("\t*", stream);
		fprint16_tf_date(stream);
	}

	fputs("\n", stream);
	fclose(stream);
	return 0;   //success
}

#endif // CO_SAVE_ROM


// ---------------------------------------------------------------------------
// CO_Driver_InitInterrupts() - Register and enable the CAN/1msec interrupt
//   Two categories of the interrupt needs to be covered:
//      a) Timer interrupt to genenrate interrupt every 1 msec, should call
//         CO_ISR_OnTimer1ms() to follow up the processing of the CO stack
//      b) CAN-related interrupts to trigger the entries
//         CO_ISR_OnFrameReceived()
//         CO_ISR_OnTxnUnderflow()
//         CO_ISR_OnErr/StateChanged()
// ---------------------------------------------------------------------------
void CO_Driver_InitInterrupts(void)
{
}


void Trace(const char *fmt, ...)
{
	char msg[2048];
	va_list args;

	va_start(args, fmt);
	int nCount = _vsnprintf(msg, 2047, fmt, args);
	va_end(args);
	if(nCount <= 0)
		msg[2047] = '\0';
	else
		msg[nCount] = '\0';

	printf("\n%s", msg);
}
