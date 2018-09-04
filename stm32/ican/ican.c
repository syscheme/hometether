#include "stm32f10x_gpio.h"
#include "ican.h"
#include "../../common/common.h"
// temporarly
#include "tmr.h"
#include "slave/ican_slave.h"
// temporarly

static ican_temp itemp;

uint8_t iCanFrame_getSrcMacId(iCanFrame* pframe)
{
	return (uint8_t)(pframe->id.src_mac_id);
}

void iCanFrame_setSrcMacId(iCanFrame* pframe, uint8_t src_macid)
{
	pframe->id.src_mac_id = src_macid;
}

uint8_t iCanFrame_getDestMacId(iCanFrame* pframe)
{
	return (uint8_t)(pframe->id.dest_mac_id);
}

void iCanFrame_setDestMacId(iCanFrame* pframe, uint8_t dest_macid)
{
	pframe->id.dest_mac_id = dest_macid;
}

uint8_t iCanFrame_getFuncId(iCanFrame* pframe)
{
	return (uint8_t)(pframe->id.func_id);
}

void iCanFrame_setFuncId(iCanFrame* pframe, uint8_t func_id)
{
	pframe->id.func_id = func_id & 0x0f;
}

uint8_t iCanFrame_getResourceId(iCanFrame* pframe)
{
	return (uint8_t)(pframe->id.source_id);
}

void iCanFrame_setResourceId(iCanFrame* pframe, uint8_t source_id)
{
	pframe->id.source_id = source_id;
}

uint8_t iCanFrame_ackNeeded(iCanFrame* pframe)
{
	return (uint8_t)(pframe->id.ack);
}

void iCanFrame_setAck(iCanFrame* pframe, uint8_t ack)
{
	pframe->id.ack = ack;
}

uint8_t iCanFrame_getSplitFlag(iCanFrame* pframe)
{
	return (uint8_t)((pframe->frame_data[0] >> 6) & 0x03);
}

void iCanFrame_setSplitFlag(iCanFrame* pframe, uint8_t split_flag)
{
	pframe->frame_data[0] &= 0x3f;
	pframe->frame_data[0] |= (split_flag << 6);
}

uint8_t iCanFrame_getSplitNum(iCanFrame* pframe)
{
	return (uint8_t)(pframe->frame_data[0] & 0x3f);
}

void iCanFrame_setSplitNum(iCanFrame* pframe, uint8_t split_num)
{
	pframe->frame_data[0] &= 0xc0;
	pframe->frame_data[0] |= (split_num & 0x3f);
}

void iCanFrame_clearSplitInfo(iCanFrame* pframe)
{
	pframe->frame_data[0] = 0x00;
}

uint8_t iCanFrame_getOffset(iCanFrame* pframe)
{
	return pframe->frame_data[1];
}

void iCanFrame_setOffset(iCanFrame* pframe, uint8_t offset)
{
	pframe->frame_data[1] = offset;
}

uint8_t iCanFrame_getLength(iCanFrame* pframe)
{
	return pframe->frame_data[2];
}

void iCanFrame_setLength(iCanFrame* pframe, uint8_t length)
{
	pframe->frame_data[2] = length;
}

typedef union _can_msg
{
	CanTxMsg txmsg;
	CanRxMsg rxmsg;
} can_msg;


/*
#define iCanFLG_WAIT_TIMER (1<<0)
static void cb_OnTimer_recv(void* pCtx)
{
iCanNode* pNode = (iCanNode*) pCtx;
if (NULL == pNode)
return;
pNode->flags &= ~iCanFLG_WAIT_TIMER;
}

#define watch(_pNODE, _TIMEOUT)       _pNODE->flags |= iCanFLG_WAIT_TIMER; Timer_start(ICAN_TMR, _TIMEOUT, cb_OnTimer_recv, _pNODE)
#define timer_reached(_pNODE)    ((0 == (_pNODE->flags & iCanFLG_WAIT_TIMER)) || Timer_isStepping(ICAN_TMR) <=0)
*/

#define watch(_pNODE, _TIMEOUT)       Timer_start(ICAN_TMR, _TIMEOUT, NULL, _pNODE)
#define timer_reached(_pNODE)        (Timer_isStepping(ICAN_TMR) <=0)

#define NODE_PTR(_PNODE) (NULL == _PNODE ? &thisNode : _PNODE)

uint8_t iCanNode_recvFrame(iCanNode* pLocalNode, iCanFrame* pmsg)
{
	uint8_t mac_id = 0;
	uint8_t master_cyclic = 0;
	uint8_t bDone = FALSE;
	//	bxcan_stat * pstat;
	//	ican_temp* ptmp;

	//	ptmp = &itemp;
	//	pstat = &can_stat[channel];
	pLocalNode = NODE_PTR(pLocalNode);

	memset(&itemp, 0, sizeof(itemp));
	memset(pmsg, 0, sizeof(iCanFrame));

	master_cyclic = iCanNode_CyclicMaster(pLocalNode);
	//	tmr_set_ms(ICAN_TMR, master_cyclic * 40);
	//	tmr_start(ICAN_TMR);
	watch(pLocalNode, master_cyclic * 40);

	while (FALSE == bDone)
	{
		if (timer_reached(pLocalNode))
		{
			Timer_stop(ICAN_TMR);
			return iCanErr_TIME_OUT;
		}

		//		if (pstat->rx_flag1 == 0)
		//			continue;
		if (iCanPendingArrival(pLocalNode->channelId) < 1)
			continue;

		Timer_stop(ICAN_TMR);
		// tmr_reset(ICAN_TMR);

		iCanPortal_recv(pLocalNode->channelId, pmsg);

		//memcpy(tx1_buff, pmsg, sizeof(iCanFrame));
		//uart_send_data(_UART1, 13);

		mac_id = iCanNode_MacId(pLocalNode);
		if ((pmsg->id.dest_mac_id != mac_id) && (pmsg->id.dest_mac_id != 0xff))
			return iCanErr_MAC_UNMATCH;

		itemp.cur_flag = iCanFrame_getSplitFlag(pmsg);

		if (itemp.cur_flag == iCanMsg_NO_SPLIT_SEG)
		{
			itemp.temp_length = pmsg->dlc - 1;
			break; 
		}

		itemp.cur_num = iCanFrame_getSplitNum(pmsg);
		itemp.cur_src_id = iCanFrame_getResourceId(pmsg);

		// handling cur_flag other than iCanMsg_NO_SPLIT_SEG
		switch(itemp.cur_flag)
		{
		case iCanMsg_SPLIT_SEG_FIRST:
			if ((itemp.cur_num != 0) || (itemp.old_num != 0))  //??
				return iCanErr_TRANS;

			itemp.old_flag = itemp.cur_flag;
			itemp.old_num = itemp.cur_num;

			memcpy(itemp.temp_buff + itemp.temp_offset, pmsg->frame_data + 1, pmsg->dlc - 1);
			itemp.temp_offset += (pmsg->dlc - 1);
			itemp.old_src_id = iCanFrame_getResourceId(pmsg);
			memset(pmsg, 0, sizeof(iCanFrame));

			// tmr_start(ICAN_TMR);
			watch(pLocalNode, master_cyclic * 40);
			break;

		case iCanMsg_SPLIT_SEG_MID:
			if ( ((itemp.cur_num - 1) != itemp.old_num) || (itemp.cur_src_id != itemp.old_src_id) || (itemp.temp_offset >= MAX_DATA_BUFF))
				return iCanErr_TRANS;

			itemp.old_flag = itemp.cur_flag;
			itemp.old_num += 1;

			memcpy(itemp.temp_buff + itemp.temp_offset, pmsg->frame_data + 1, pmsg->dlc - 1);
			itemp.temp_offset += (pmsg->dlc - 1);

			// tmr_start(ICAN_TMR);
			watch(pLocalNode, master_cyclic * 40);
			break;

		default:
			if (((itemp.cur_num - 1) != itemp.old_num) || (itemp.cur_src_id != itemp.old_src_id) || (itemp.temp_offset >= MAX_DATA_BUFF))
				return iCanErr_TRANS;

			itemp.old_flag = 0;
			itemp.old_num = 0;

			memcpy(itemp.temp_buff + itemp.temp_offset, pmsg->frame_data + 1, pmsg->dlc - 1);
			itemp.temp_offset += (pmsg->dlc - 1);
			itemp.temp_length = itemp.temp_offset;
			itemp.temp_offset = 0;
			bDone = TRUE;
			break;
		} // switch

	} // while

	return iCanErr_OK;
}		

uint8_t iCanNode_sendFrame(iCanNode* pLocalNode, iCanFrame* pmsg)
{
	uint8_t cur_trans = 0;
	uint8_t cur_trans_state = 0;
	ican_temp * ptmp;

	ptmp = &itemp;
	ptmp->temp_offset = 0;

	pLocalNode = NODE_PTR(pLocalNode);

	if (ptmp->temp_length <= 7)
	{
		ptmp->old_flag = 0;
		ptmp->old_num = 0;

		pmsg->dlc = ptmp->temp_length + 1;
		iCanFrame_setSplitFlag(pmsg, ptmp->old_flag);
		iCanFrame_setSplitNum(pmsg, ptmp->old_num);

		iCanPortal_send(pLocalNode->channelId, pmsg);
	}
	else
	{
		//		tmr_set_ms(ICAN_TMR, 1);
		cur_trans = 1;
		cur_trans_state = iCanMsg_SPLIT_SEG_FIRST;

		do
		{
			if(cur_trans_state == iCanMsg_SPLIT_SEG_FIRST)
			{
				ptmp->old_flag = iCanMsg_SPLIT_SEG_FIRST;
				ptmp->old_num = 0;

				pmsg->dlc = 8;
				memcpy(pmsg->frame_data + 1, ptmp->temp_buff + ptmp->temp_offset, pmsg->dlc - 1);
				ptmp->temp_offset += (pmsg->dlc - 1);

				if ((ptmp->temp_length - ptmp->temp_offset) <= 7)
					cur_trans_state = iCanMsg_SPLIT_SEG_LAST;
				else
					cur_trans_state = iCanMsg_SPLIT_SEG_MID;
			}
			else if(cur_trans_state == iCanMsg_SPLIT_SEG_MID)
			{
				ptmp->old_flag = iCanMsg_SPLIT_SEG_MID;
				ptmp->old_num += 1;
				pmsg->dlc = 8;
				memcpy(pmsg->frame_data + 1, ptmp->temp_buff + ptmp->temp_offset, pmsg->dlc - 1);
				ptmp->temp_offset += (pmsg->dlc - 1);

				if ((ptmp->temp_length - ptmp->temp_offset) <= 7)
					cur_trans_state = iCanMsg_SPLIT_SEG_LAST;
			} 
			else
			{
				ptmp->old_flag = iCanMsg_SPLIT_SEG_LAST;
				ptmp->old_num += 1;
				pmsg->dlc = ptmp->temp_length - ptmp->temp_offset + 1;
				memcpy(pmsg->frame_data + 1, ptmp->temp_buff + ptmp->temp_offset, pmsg->dlc - 1);
				ptmp->temp_offset += (pmsg->dlc - 1);
				cur_trans = 0;
			}

			iCanFrame_setSplitFlag(pmsg, ptmp->old_flag);
			iCanFrame_setSplitNum(pmsg, ptmp->old_num);

			iCanPortal_send(NODE_PTR(pLocalNode)->channelId, pmsg);

			memset(pmsg->frame_data, 0, 8);

			// tmr_start(ICAN_TMR);
			// while(tmr_check(ICAN_TMR));
			// tmr_reset(ICAN_TMR);
			watch(pLocalNode, 1);
			while (0 != timer_reached());
			Timer_stop(ICAN_TMR);

		}  while(cur_trans);
		ptmp->temp_offset = 0;
	}

	return iCanErr_OK;
}

uint8_t iCanNode_ping(iCanNode* pLocalNode, uint8_t destMacId)
{
	uint8_t ret = 0;
	uint32_t tmp = 0;
	//	bxcan_stat * p;
	iCanFrame 	frame;
	ican_temp * ptmp;

	pLocalNode = NODE_PTR(pLocalNode);

	//	p = &can_stat[channel];
	//	pmsg = &ican_msg;
	ptmp = &itemp;
	memset(&frame, 0, sizeof(frame));

	frame.id.src_mac_id  = iCanNode_MacId(pLocalNode);
	frame.id.dest_mac_id = destMacId;
	frame.id.source_id   = 0x00;
	frame.id.func_id     = iCanFunc_MAC_CHECK;
	frame.id.ack         = 0;
	ptmp->temp_length    = 5;

	tmp = iCanNode_SerialNum(pLocalNode); // ican_get_serial_number();
	frame.frame_data[1] = iCanNode_MacId(pLocalNode);
	frame.frame_data[2] = tmp & 0xff;
	frame.frame_data[3] = (tmp >> 8) & 0xff;
	frame.frame_data[4] = (tmp >> 16) & 0xff;
	frame.frame_data[5] = (tmp >> 24) & 0xff;

	ret = iCanNode_sendFrame(pLocalNode, &frame);
	if(ret != iCanErr_OK)
		return ret;

	//#ifdef CAN_MASTER
	//	tmr_set_sec(ICAN_TMR, 1);
	//#else
	//tmr_set_ms(ICAN_TMR, 100);
	//#endif // CAN_MASTER
	//	tmr_start(ICAN_TMR);

	watch(pLocalNode,
#ifdef CAN_MASTER
		1
#else
		100
#endif // CAN_MASTER
		);

	while(1)
	{
		// if (tmr_check(ICAN_TMR) > 0)
		if (!timer_reached(pLocalNode))
		{
			if (iCanPendingArrival(pLocalNode->channelId) <1)
				continue;

			// tmr_stop(ICAN_TMR);
			Timer_stop(ICAN_TMR);
			ret = iCanErr_MAC_EXIST;
			break;
		}

		// tmr_stop(ICAN_TMR);
		ret = iCanErr_TIME_OUT;
		break;
	} // while

	if (ret == iCanErr_TIME_OUT)
		return iCanErr_OK;

	return ret;
}

void iCanNode_init(iCanNode* pNode, uint8_t channelId)
{
	uint8_t addr    = iCanPortal_devAddr(); 

	pNode = NODE_PTR(pNode);
	memset(pNode, 0, sizeof(iCanNode));

	pNode->channelId                    = channelId;
	pNode->state                        = iCanState_IDLE;
	pNode->slaveState 			        = iCanConn_Disconnected;

	pNode->devInfo.vendor_id 		    = VENDOR_ID;
	pNode->devInfo.product_type 	    = PRODUCT_TYPE;
	pNode->devInfo.product_code 	    = PRODUCT_CODE;
	pNode->devInfo.hardware_version     = HARDWARE_VERSION;
	pNode->devInfo.firmware_version     = FIRMWARE_VERSION;
	pNode->devInfo.serial_number 	    = SERIAL_NUMBER;

	pNode->commCfg.dev_mac_id 		    = addr & 0x3f;  
	pNode->commCfg.baud_rate 		    = (addr >> 6) & 0x03;
	pNode->commCfg.user_baud_rate 	    = USER_BAUD_RATE;			
	pNode->commCfg.cyclic_param 	    = CYCLIC_PARAM;
	pNode->commCfg.cyclic_master 	    = CYCLIC_MASTER;
	pNode->commCfg.cos_type 		    = COS_TYPE;
	pNode->commCfg.master_mac_id 	    = MASTER_MAC_ID;

	pNode->resource.DI.len 		    = ICAN_DI_LEN;
	pNode->resource.DO.len 		    = ICAN_DO_LEN;
	pNode->resource.AI.len 		    = ICAN_AI_LEN;
	pNode->resource.AO.len 		    = ICAN_AO_LEN;
	pNode->resource.serial0.len 	    = ICAN_SER0_LEN;
	pNode->resource.serial1.len 	    = ICAN_SER1_LEN;
	pNode->resource.userdef.len 	    = ICAN_USER_LEN;

	/*
	//////////////////////////////
	imaster.dev_channel 									= channel;
	imaster.dev_addr 										= MASTER_MAC_ID;
	imaster.dev_status 										= iCanState_IDLE;
	imaster.slave_setup_check 								= 0;

	addr 													= iCanPortal_devAddr();
	imaster.commCfg.dev_mac_id 								= addr & 0x3f;
	imaster.commCfg.baud_rate 								= (addr >> 6) & 0x03;
	imaster.commCfg.user_baud_rate 							= USER_BAUD_RATE;
	imaster.commCfg.cyclic_param 							= CYCLIC_PARAM;
	imaster.commCfg.cyclic_master 							= CYCLIC_MASTER;
	imaster.commCfg.cos_type 								= COS_TYPE;
	imaster.commCfg.master_mac_id 							= MASTER_MAC_ID;
	///////////////////////////////
	*/
}


uint16_t iCanNode_VendorId(iCanNode* pNode)
{
	return NODE_PTR(pNode)->devInfo.vendor_id;
}

uint16_t iCanNode_ProductType(iCanNode* pNode)
{
	return NODE_PTR(pNode)->devInfo.product_type;
}

uint16_t iCanNode_ProductCode(iCanNode* pNode)
{
	return NODE_PTR(pNode)->devInfo.product_code;
}

uint16_t iCanNode_HardwareVersion(iCanNode* pNode)
{
	return NODE_PTR(pNode)->devInfo.hardware_version;
}

uint16_t iCanNode_FirmwareVersion(iCanNode* pNode)
{
	return NODE_PTR(pNode)->devInfo.firmware_version;
}

uint32_t iCanNode_SerialNum(iCanNode* pNode)
{
#ifdef CAN_MASTER
	return SERIAL_NUMBER;
#else
	return NODE_PTR(pNode)->devInfo.serial_number;
#endif // CAN_MASTER
}

uint8_t iCanNode_MacId(iCanNode* pNode)
{
#ifdef CAN_MASTER
	return MASTER_MAC_ID;
#else
	return NODE_PTR(pNode)->commCfg.dev_mac_id;
#endif // CAN_MASTER
}

void iCanNode_setMacId(iCanNode* pNode, uint8_t macId)
{
	NODE_PTR(pNode)->commCfg.dev_mac_id = macId;
}

uint8_t iCanNode_Baudrate(iCanNode* pNode)
{
	return NODE_PTR(pNode)->commCfg.baud_rate;
}

void iCanNode_setBaudrate(iCanNode* pNode, uint8_t rate)
{
	NODE_PTR(pNode)->commCfg.baud_rate = rate;
}

uint32_t iCanNode_UserBaudrate(iCanNode* pNode)
{
	return NODE_PTR(pNode)->commCfg.user_baud_rate;
}

void iCanNode_setUserBaudrate(iCanNode* pNode, uint32_t usr_rate)
{
	NODE_PTR(pNode)->commCfg.user_baud_rate = usr_rate;
}

uint8_t iCanNode_CyclicParam(iCanNode* pNode)
{
	return NODE_PTR(pNode)->commCfg.cyclic_param;
}

void iCanNode_setCyclicParam(iCanNode* pNode, uint8_t time)
{
	NODE_PTR(pNode)->commCfg.cyclic_param = time;
}

uint8_t iCanNode_CyclicMaster(iCanNode* pNode)
{
#ifdef CAN_MASTER
	return CYCLIC_MASTER;
#else
	return NODE_PTR(pNode)->commCfg.cyclic_master;
#endif // CAN_MASTER
}

void iCanNode_setCyclicMaster(iCanNode* pNode, uint8_t time)
{
	if (NULL == pNode)
		pNode = &thisNode;
	pNode->commCfg.cyclic_master = time;
}

uint8_t iCanNode_CosType(iCanNode* pNode)
{
	return NODE_PTR(pNode)->commCfg.cos_type;
}

void iCanNode_setCosType(iCanNode* pNode, uint8_t type)
{
	NODE_PTR(pNode)->commCfg.cos_type = type;
}

uint8_t iCanNode_MasterMacId(iCanNode* pNode)
{
	return NODE_PTR(pNode)->commCfg.master_mac_id;
}

void iCanNode_setMasterMacId(iCanNode* pNode, uint8_t macId)
{
	NODE_PTR(pNode)->commCfg.master_mac_id = macId;
}

