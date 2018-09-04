#include "../../common/common.h"
#include "ican.h"
#include "tmr.h"
#include "ican_slave.h"

static iCanNode thisNode;
static ican_temp itemp;

static uint8_t iCanNode_OnReadDI(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t * p)
{
	if (pNode->resource.DI.len <= 0)
		return iCanErr_LEN_ZERO;

	if (offset > pNode->resource.DI.len)
		return iCanErr_BAD_PARAMS;

	len = min(len, pNode->resource.DI.len - offset);
	memcpy(p, pNode->resource.DI.data + offset, len);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnReadAI(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t * p)
{
	if (pNode->resource.AI.len == 0)
		return iCanErr_LEN_ZERO;

	if (offset > pNode->resource.AI.len)
		return iCanErr_BAD_PARAMS;

	len = min(len, pNode->resource.AI.len);
	memcpy(p, pNode->resource.AI.data + offset, len);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnReadSerial0(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t * p)
{
	if(pNode->resource.serial0.len == 0)
		return iCanErr_LEN_ZERO;

	if (offset > pNode->resource.serial0.len)
		return iCanErr_BAD_PARAMS;

	len = min(len, pNode->resource.serial0.len - offset);
	memcpy(p, pNode->resource.serial0.data + offset, len);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnReadSerial1(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t * p)
{
	if(pNode->resource.serial1.len == 0)
		return iCanErr_LEN_ZERO;

	if(offset > pNode->resource.serial1.len)
		return iCanErr_BAD_PARAMS;

	len = min(len, pNode->resource.serial1.len - offset);
	memcpy(p, pNode->resource.serial1.data + offset, len);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnReadUserDef(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t* p)	
{
	if(pNode->resource.userdef.len == 0)
		return iCanErr_LEN_ZERO;

	if(offset > pNode->resource.userdef.len)
		return iCanErr_BAD_PARAMS;

	len = min(len, pNode->resource.userdef.len - offset);
	memcpy(p, pNode->resource.userdef.data + offset, len);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnReadDeviceInfo(iCanNode* pNode, uint8_t offset, uint8_t length, uint8_t* p)
{
	uint16_t tmp = sizeof(iCanDevInfo);

	if(offset > tmp)
		return iCanErr_BAD_PARAMS;

	tmp -= offset;
	memcpy(p, ((uint8_t*) &pNode->devInfo) + offset, (length > tmp) ? tmp : length);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnReadCommunicate(iCanNode* pNode, uint8_t offset, uint8_t length, uint8_t * p)	
{
	uint16_t tmp = sizeof(iCanCommCfg);

	if (offset > tmp)
		return iCanErr_BAD_PARAMS;

	tmp -= offset;
	memcpy(p, ((uint8_t*) &pNode->commCfg) + offset, (length > tmp) ? tmp : length);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnReadIoParams(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t * p)
{
	uint8_t i = 0;
	iCanResField* pFld = (iCanResField*) &pNode->resource;

	if (offset > 7) // iCan only has 7 different types of resources: DI, DO, AI, AO, serial0, serial1, userdef
		return iCanErr_BAD_PARAMS;

	len = min(len, 7 -offset);

	for (i=offset; i < len; i++, p++)
		*p = pFld[i].len;

	return iCanErr_OK;
}	

static uint8_t iCanNode_OnWriteDO(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t * p)
{
	if (pNode->resource.DO.len <= 0)
		return iCanErr_LEN_ZERO;

	if(offset > pNode->resource.DO.len)
		return iCanErr_BAD_PARAMS;

	len = min(len, pNode->resource.DO.len - offset);
	memcpy(((uint8_t*) &pNode->resource.DO.data) + offset, p, len);

	return iCanErr_OK;	
}

static uint8_t iCanNode_OnWriteAO(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t * p)
{
	if(pNode->resource.AO.len == 0)
		return iCanErr_LEN_ZERO;

	if(offset > pNode->resource.AO.len)
		return iCanErr_BAD_PARAMS;

	len = min(len, pNode->resource.AO.len - offset);
	memcpy(((uint8_t*) &pNode->resource.AO.data) + offset, p, len);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnWriteSerial0(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t * p)
{
	if(pNode->resource.serial0.len == 0)
		return iCanErr_LEN_ZERO;

	if(offset > pNode->resource.serial0.len)
		return iCanErr_BAD_PARAMS;

	len = min(len, pNode->resource.serial0.len - offset);
	memcpy(((uint8_t*) &pNode->resource.serial0.data) + offset, p, len);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnWriteSerial1(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t *p)
{
	if (pNode->resource.serial1.len == 0)
		return iCanErr_LEN_ZERO;

	if (offset > pNode->resource.serial1.len)
		return iCanErr_BAD_PARAMS;

	len = min(len, pNode->resource.serial1.len - offset);
	memcpy(((uint8_t*) &pNode->resource.serial1.data) + offset, p, len);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnWriteUserDef(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t * p)
{
	if(pNode->resource.userdef.len == 0)
		return iCanErr_LEN_ZERO;

	if(offset > pNode->resource.userdef.len)
		return iCanErr_BAD_PARAMS;

	len = min(len, pNode->resource.userdef.len - offset);
	memcpy(((uint8_t*) &pNode->resource.userdef.data) + offset, p, len);

	return iCanErr_OK;
}

static uint8_t iCanNode_OnWriteIoParams(iCanNode* pNode, uint8_t offset, uint8_t len, uint8_t * p)
{
	uint8_t i=0;
	iCanResField* pFld = (iCanResField*) &pNode->resource;

	if (offset > 7)
		return iCanErr_BAD_PARAMS;

	len = min(len, 7 - offset);

	for (i=offset; i < len; i++, p++)
		pFld[i].len = *p;

	return iCanErr_OK;
}

static uint8_t iCanNode_dispatchRead(iCanFrame* pmsg)
{
	uint8_t src_id = 0;
	uint8_t offset = 0;
	uint8_t length = 0;
	uint8_t *buff = NULL;
	uint8_t ret = 0;
	ican_temp * p = NULL;

	p = &itemp;
	src_id = iCanFrame_getResourceId(pmsg);
	offset = iCanFrame_getOffset(pmsg);
	length = iCanFrame_getLength(pmsg);

	if(length > 7)
		buff = p->temp_buff;
	else
		buff = pmsg->frame_data + 1;

	memset(buff, 0, length);
	p->temp_length = length;

	switch(src_id)
	{
	case iCanResource_DI:          ret = iCanNode_OnReadDI(&thisNode, offset, length, buff);          break;
	case iCanResource_AI:          ret = iCanNode_OnReadAI(&thisNode, offset, length, buff);          break;
	case iCanResource_SERIAL0:     ret = iCanNode_OnReadSerial0(&thisNode, offset, length, buff);     break;							
	case iCanResource_SERIAL1:     ret = iCanNode_OnReadSerial1(&thisNode, offset, length, buff);     break;							
	case iCanResource_USERDEF:     ret = iCanNode_OnReadUserDef(&thisNode, offset, length, buff);     break;							
	case iCanConf_DEVICE_INFO:     ret = iCanNode_OnReadDeviceInfo(&thisNode, offset, length, buff);  break;					
	case iCanConf_COMMUNICATE:     ret = iCanNode_OnReadCommunicate(&thisNode, offset, length, buff); break;					
	case iCanConf_IO_PARAM:        ret = iCanNode_OnReadIoParams(&thisNode, offset, length, buff);    break;	
	default:                       ret = iCanErr_RESOURCE_ID;							
	}

	return ret;	
}

static uint8_t iCanNode_dispatchWrite(iCanFrame* pmsg)
{
	uint8_t src_id = 0;
	uint8_t offset = 0;
	uint8_t length = 0;
	uint8_t *buff = NULL;
	uint8_t ret = 0;
	ican_temp * p = NULL;

	p = &itemp;
	src_id = iCanFrame_getResourceId(pmsg);
	if (p->temp_length == 0)
	{
		offset = iCanFrame_getOffset(pmsg);
		length = iCanFrame_getLength(pmsg);
	}
	else
	{
		offset = p->temp_buff[0];
		length = p->temp_buff[1];
	}

	if (length <= 5)
		buff = pmsg->frame_data + 3;
	else
		buff = p->temp_buff + 2;

	switch(src_id)
	{
	case iCanResource_DO:          ret = iCanNode_OnWriteDO(&thisNode, offset, length, buff);       break;
	case iCanResource_AO:          ret = iCanNode_OnWriteAO(&thisNode, offset, length, buff);       break;								
	case iCanResource_SERIAL0:     ret = iCanNode_OnWriteSerial0(&thisNode, offset, length, buff);  break;						
	case iCanResource_SERIAL1:     ret = iCanNode_OnWriteSerial1(&thisNode, offset, length, buff);  break;						
	case iCanResource_USERDEF:     ret = iCanNode_OnWriteUserDef(&thisNode, offset, length, buff);  break;						
	case iCanConf_IO:              ret = iCanNode_OnWriteIoParams(&thisNode, offset, length, buff); break;
	default:                       ret = iCanErr_RESOURCE_ID;
	}

	return ret;
}

static uint8_t ican_slave_event_triger(iCanFrame* pmsg)
{
	return iCanErr_OK;
}

static uint8_t ican_slave_establish_connect(iCanFrame* pmsg)
{
	uint8_t last_master_id = 0;
	uint8_t src_id = 0;

	src_id = iCanFrame_getResourceId(pmsg);
	last_master_id = iCanNode_MasterMacId(&thisNode);

	if (src_id != 0xf7)
		return iCanErr_RESOURCE_ID;

	if ((last_master_id != 0xff) && (last_master_id != pmsg->frame_data[1]))
		return iCanErr_NODE_BUSY;

	iCanNode_setMasterMacId(&thisNode,  pmsg->frame_data[1]); // ican_set_master_mac_id(pmsg->frame_data[1]);
	iCanNode_setCyclicMaster(&thisNode, pmsg->frame_data[2]); // ican_set_cyclic_master(pmsg->frame_data[2]);

	thisNode.slaveState = iCanConn_Connected;

	return iCanErr_OK;
}

static uint8_t ican_slave_delete_connect(iCanFrame* pmsg)
{
	uint8_t src_id = 0;
	uint8_t last_master_id = 0;

	src_id = iCanFrame_getResourceId(pmsg);
	last_master_id = iCanNode_MasterMacId(&thisNode);

	if (src_id != 0xf7)
		return iCanErr_RESOURCE_ID;

	if(last_master_id == 0xff)
		return iCanErr_NODE_EXIST;

	if(last_master_id != pmsg->frame_data[1])
		return iCanErr_DEL_NODE;

	iCanNode_setMasterMacId(&thisNode,  0xff); // ican_set_master_mac_id(0xff);
	iCanNode_setCyclicMaster(&thisNode, 0x00); // ican_set_cyclic_master(0x00);

	thisNode.slaveState = iCanConn_Disconnected;

	return iCanErr_OK;	
}

static uint8_t ican_slave_device_reset(iCanFrame* pmsg)
{
	return iCanErr_OK;
}

static uint8_t ican_slave_macid_check_response(iCanFrame* pmsg)
{
	uint32_t data = 0;
	uint8_t mac_id = 0;
	uint8_t * p =(uint8_t *)(&data);
	ican_temp * ptmp;

	ptmp = &itemp;

	mac_id = iCanNode_MacId(&thisNode); //ican_get_dev_mac_id();

	if (mac_id != pmsg->frame_data[1])
		return iCanErr_MAC_UNMATCH;

	memset(pmsg->frame_data, 0, 8);

	data = iCanNode_SerialNum(&thisNode); // ican_get_serial_number();

	pmsg->frame_data[1] = mac_id;
	pmsg->frame_data[2] = p[0];
	pmsg->frame_data[3] = p[1];
	pmsg->frame_data[4] = p[2];
	pmsg->frame_data[5] = p[3];

	ptmp->temp_length = 5;
	return iCanErr_OK;
}


static uint8_t ican_slave_parse(iCanFrame* pmsg)  
{
	uint8_t ret = 0;

	switch(pmsg->id.func_id)
	{
	case iCanFunc_READ :
		ret = iCanNode_dispatchRead(pmsg);
		break;

	case iCanFunc_WRITE :
		ret = iCanNode_dispatchWrite(pmsg);
		break;

	case iCanFunc_EVE_TRAGER :
		ret = ican_slave_event_triger(pmsg);
		break;

	case iCanFunc_EST_CONNECT :
		ret = ican_slave_establish_connect(pmsg);
		break;

	case iCanFunc_DEL_CONNECT :
		ret = ican_slave_delete_connect(pmsg);
		break;

	case iCanFunc_DEV_RESET :
		ret = ican_slave_device_reset(pmsg);
		break;

	case iCanFunc_MAC_CHECK :
		ret = ican_slave_macid_check_response(pmsg);
		break;

	default :
		ret = iCanErr_FUNC_ID;
		break;
	}

	return ret;
}

static uint8_t ican_slave_error_response(uint8_t channel, iCanFrame* pmsg, uint8_t err)
{
	uint8_t ret = 0;
	uint8_t tmp = 0;
	ican_temp * ptmp;

	ptmp = &itemp;

	if(pmsg->id.dest_mac_id == 0xff)
		return iCanErr_OK;

	tmp = iCanFrame_getDestMacId(pmsg);
	iCanFrame_setDestMacId(pmsg, iCanFrame_getSrcMacId(pmsg));
	iCanFrame_setSrcMacId(pmsg, tmp);

	if (err != iCanErr_OK)
	{
		pmsg->id.ack = 1;
		switch(pmsg->id.func_id)
		{
		case iCanFunc_READ :
		case iCanFunc_WRITE :
		case iCanFunc_EST_CONNECT :
		case iCanFunc_DEL_CONNECT :
		case iCanFunc_DEV_RESET :
			memset(pmsg->frame_data, 0, 8);
			pmsg->frame_data[1] = err;
			pmsg->id.func_id = 0x0f;
			ptmp->temp_length = 1;
			ret = iCanNode_sendFrame(&thisNode, pmsg);
			break;

		case iCanFunc_EVE_TRAGER :
			break;
		}

		return err;
	}

	pmsg->id.ack = 1;
	switch(pmsg->id.func_id)
	{
	case iCanFunc_READ :
	case iCanFunc_MAC_CHECK :
		ret = iCanNode_sendFrame(&thisNode, pmsg);
		break;

	case iCanFunc_EST_CONNECT :
		memset(pmsg->frame_data, 0, 8);
		ptmp->temp_length = 0;
		ret = iCanNode_sendFrame(&thisNode, pmsg);
		if(ret == iCanErr_OK)
			ret = ICAN_SETUP_CONNECT;
		break;

	case iCanFunc_WRITE :
	case iCanFunc_DEL_CONNECT :
		memset(pmsg->frame_data, 0, 8);  
		ptmp->temp_length = 0;
		ret = iCanNode_sendFrame(&thisNode, pmsg);
		break;

	case iCanFunc_DEV_RESET :
		memset(pmsg->frame_data, 0, 8);
		ptmp->temp_length = 0;
		ret = iCanNode_sendFrame(&thisNode, pmsg);
		if(ret == iCanErr_OK)
			;//dev_reset();
		break;

	case iCanFunc_EVE_TRAGER :break;
	}

	return ret;
}

uint8_t ican_slave_poll(uint8_t channel)
{
	uint8_t ret = 0;
	uint8_t state = 0;
	iCanFrame frame;
	state = iCanState_RECV;

	while(1)
	{
		switch (state)
		{
		case iCanState_RECV :
			ret = iCanNode_recvFrame(&thisNode, &frame);
			if(ret != iCanErr_OK)
				return ret;
			state = iCanState_PARSE;
			break;

		case iCanState_PARSE :
			ret = ican_slave_parse(&frame);
			if(ret != iCanErr_OK)
				return ret;
			state = iCanState_SEND;
			break;

		case iCanState_SEND :
			ret = ican_slave_error_response(channel, &frame, ret);
			if(ret == ICAN_SETUP_CONNECT)
			{
				state = iCanState_RECV;
				break;
			}

			if(ret != iCanErr_OK)
				return ret;

			return iCanErr_OK;
		}
	}
}

void ican_slave_init(uint8_t channelId)
{
	iCanNode_init(&thisNode, channelId);
	memset(&itemp, 0, sizeof(ican_temp));

	//	bxcan_init(channel, addr);
	CAN_Configuration(thisNode.channelId, thisNode.commCfg.user_baud_rate);

}

void ican_slave_task(void * pdata)
{
	uint8_t ret = 0;
	//	bxcan_stat * pstat;
	//	pstat = &can_stat[ICAN_SLAVE_CHANNEL];

	ican_slave_init(ICAN_SLAVE_CHANNEL);

	while(1)
	{
		//		if (pstat->rx_flag1 != 0)
		if (iCanPendingArrival(ICAN_SLAVE_CHANNEL) != 0)
		{
			ret = ican_slave_poll(ICAN_SLAVE_CHANNEL);
			if(ret == iCanErr_OK || ret == iCanErr_MAC_UNMATCH)
				iCanPortal_LED_canOK();
			else
				iCanPortal_LED_canErr();
		}
	}
}
