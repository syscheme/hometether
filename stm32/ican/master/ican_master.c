#include "../../common/common.h"
#include "ican.h"
#include "tmr.h"
#include "ican_master.h"

// static ican_master imaster;
iCanNode slaves[ICAN_SLAVE_NUM];
static iCanFrame  ican_msg;
static ican_temp   itemp;

static uint8_t iCanFrame_parse(iCanFrame* pmsg, uint8_t* buff, uint8_t len)
{
	uint8_t ret = 0;
	ican_temp * ptmp;

	ptmp = &itemp;

	switch(pmsg->id.func_id)
	{
	case iCanFunc_READ :
		if(ptmp->temp_length <= 7)
		{
			if(len > ptmp->temp_length)
				memcpy(buff, pmsg->frame_data + 1, ptmp->temp_length);
			else
				memcpy(buff, pmsg->frame_data + 1, len);
		}
		else
		{
			if(len > ptmp->temp_length)
				memcpy(buff, ptmp->temp_buff, ptmp->temp_length);
			else
				memcpy(buff, ptmp->temp_buff, len);
		}
		break;

	case iCanFunc_WRITE :
	case iCanFunc_EST_CONNECT :
	case iCanFunc_DEL_CONNECT :
	case iCanFunc_DEV_RESET :
		if (pmsg->id.func_id == 0x0f)
			ret = pmsg->frame_data[1];
		else
			ret = iCanErr_OK;
		return ret;

	case iCanFunc_EVE_TRAGER :
		break;
	}

	return iCanErr_OK;
}

uint8_t ican_master_read(uint8_t slave_id, uint8_t src_id, uint8_t offset, uint8_t len, uint8_t* buff)
{
	uint8_t state = 0;
	uint8_t ret = 0;
	iCanFrame* pmsg;
	ican_temp * ptmp;

	pmsg = &ican_msg;
	ptmp = &itemp;

	if ((src_id != iCanResource_DI) && (src_id != iCanResource_AI) && (src_id != iCanResource_SERIAL0) && (src_id != iCanResource_SERIAL1) && 
		(src_id != iCanResource_USERDEF) && (src_id != iCanConf_DEVICE_INFO) && (src_id != iCanConf_COMMUNICATE) && (src_id != iCanConf_IO_PARAM))
		return iCanErr_RESOURCE_ID;

	memset(pmsg, 0, sizeof(iCanFrame));
	pmsg->id.src_mac_id = MASTER_MAC_ID;
	pmsg->id.dest_mac_id = slave_id;
	pmsg->id.source_id = src_id;
	pmsg->id.func_id = iCanFunc_READ;
	pmsg->id.ack = 0;
	pmsg->frame_data[1] = offset;
	pmsg->frame_data[2] = len;
	ptmp->temp_length = 2;

	state = iCanState_SEND;

	while(1)
	{
		switch(state)
		{
		case iCanState_SEND :
			ret = iCanNode_sendFrame(&thisNode, pmsg);
			if (ret != iCanErr_OK)
				return ret;
			state = iCanState_RECV;
			break;

		case iCanState_RECV :
			ret = iCanNode_recvFrame(&thisNode, pmsg);
			if (ret != iCanErr_OK)
				return ret;
			state = iCanState_PARSE;
			break;

		case iCanState_PARSE:
			ret = iCanFrame_parse(pmsg, buff, len);
			if (ret != iCanErr_OK)
				return ret;

			return iCanErr_OK;
		}
	}
}

uint8_t ican_master_write(uint8_t slave_id, uint8_t src_id, uint8_t offset, uint8_t len, uint8_t* buff)
{
	uint8_t state = 0;
	uint8_t ret = 0;
	iCanFrame* pmsg;
	ican_temp * ptmp;

	pmsg = &ican_msg;
	ptmp = &itemp;

	if ((src_id != iCanResource_DO) && (src_id != iCanResource_AO) && (src_id != iCanResource_SERIAL0) && (src_id != iCanResource_SERIAL1) && 
		(src_id != iCanResource_USERDEF) && (src_id != iCanConf_IO))
		return iCanErr_RESOURCE_ID;

	memset(pmsg, 0, sizeof(iCanFrame));
	pmsg->id.src_mac_id  = MASTER_MAC_ID;
	pmsg->id.dest_mac_id = slave_id;
	pmsg->id.source_id   = src_id;
	pmsg->id.func_id     = iCanFunc_WRITE;
	pmsg->id.ack         = 0;	

	if (len <= 5)
	{
		pmsg->frame_data[1] = offset;
		pmsg->frame_data[2] = len;	
		memcpy(pmsg->frame_data + 3, buff, len);
	}
	else
	{
		memset(ptmp->temp_buff, 0, len);
		ptmp->temp_buff[0] = offset;
		ptmp->temp_buff[1] = len;
		memcpy(ptmp->temp_buff + 2, buff, len);
	}

	ptmp->temp_length = len + 2;	

	state = iCanState_SEND;

	while(1)
	{
		switch(state)
		{
		case iCanState_SEND :
			ret = iCanNode_sendFrame(&thisNode, pmsg);
			if (ret != iCanErr_OK)
				return ret;
			state = iCanState_RECV;
			break;

		case iCanState_RECV :
			ret = iCanNode_recvFrame(&thisNode, pmsg);
			if (ret != iCanErr_OK)
				return ret;
			state = iCanState_PARSE;
			break;

		case iCanState_PARSE:
			ret = iCanFrame_parse(pmsg, buff, len);
			if (ret != iCanErr_OK)
				return ret;

			return iCanErr_OK;
		}
	}	
}

uint8_t ican_master_event_trager(uint8_t slave_id, uint8_t src_id, uint8_t offset, uint8_t len, uint8_t* buff)
{
	return iCanErr_OK;
}

uint8_t ican_master_establish_connect(uint8_t slave_id)
{
	uint8_t state = 0;
	uint8_t ret = 0;
	iCanFrame* pmsg;
	ican_temp * ptmp;

	pmsg = &ican_msg;
	ptmp = &itemp;

	memset(pmsg, 0, sizeof(iCanFrame));
	pmsg->id.src_mac_id = MASTER_MAC_ID;
	pmsg->id.dest_mac_id = slave_id;
	pmsg->id.source_id = ICAN_ESTABLISH_CONNECT;
	pmsg->id.func_id = iCanFunc_EST_CONNECT;
	pmsg->id.ack = 0;
	pmsg->frame_data[1] = MASTER_MAC_ID;
	pmsg->frame_data[2] = CYCLIC_MASTER;
	ptmp->temp_length = 2;

	state = iCanState_SEND;

	while(1)
	{
		switch(state)
		{
		case iCanState_SEND :
			ret = iCanNode_sendFrame(&thisNode, pmsg);
			if (ret != iCanErr_OK)
				return ret;
			state = iCanState_RECV;
			break;

		case iCanState_RECV :
			ret = iCanNode_recvFrame(&thisNode, pmsg);
			if (ret != iCanErr_OK)
				return ret;
			state = iCanState_PARSE;
			break;

		case iCanState_PARSE:
			ret = iCanFrame_parse(pmsg, NULL, 0);
			if (ret != iCanErr_OK)
				return ret;

			return iCanErr_OK;
		}
	}
}

uint8_t ican_master_delete_connect(uint8_t slave_id)
{
	uint8_t state = 0;
	uint8_t ret = 0;
	iCanFrame* pmsg;
	ican_temp * ptmp;

	pmsg = &ican_msg;
	ptmp = &itemp;

	memset(pmsg, 0, sizeof(iCanFrame));
	pmsg->id.src_mac_id  = MASTER_MAC_ID;
	pmsg->id.dest_mac_id = slave_id;
	pmsg->id.source_id   = ICAN_ESTABLISH_CONNECT;
	pmsg->id.func_id     = iCanFunc_DEL_CONNECT;
	pmsg->id.ack = 0;
	pmsg->frame_data[1]  = MASTER_MAC_ID;
	ptmp->temp_length    = 1;

	state = iCanState_SEND;

	while(1)
	{
		switch(state)
		{
		case iCanState_SEND :
			ret = iCanNode_sendFrame(&thisNode, pmsg);
			if (ret != iCanErr_OK)
				return ret;
			state = iCanState_RECV;
			break;

		case iCanState_RECV :
			ret = iCanNode_recvFrame(&thisNode, pmsg);
			if (ret != iCanErr_OK)
				return ret;
			state = iCanState_PARSE;
			break;

		case iCanState_PARSE:
			ret = iCanFrame_parse(pmsg, NULL, 0);
			if (ret != iCanErr_OK)
				return ret;

			return iCanErr_OK;
		}
	}
}

uint8_t ican_master_device_reset(uint8_t slave_id)
{
	uint8_t state = 0;
	uint8_t ret = 0;
	iCanFrame* pmsg;
	ican_temp * ptmp;

	pmsg = &ican_msg;
	ptmp = &itemp;

	memset(pmsg, 0, sizeof(iCanFrame));
	pmsg->id.src_mac_id  = MASTER_MAC_ID;
	pmsg->id.dest_mac_id = slave_id;
	pmsg->id.source_id   = 0xff;
	pmsg->id.func_id     = iCanFunc_DEV_RESET;
	pmsg->id.ack         = 0;
	pmsg->frame_data[1]  = slave_id;
	ptmp->temp_length    = 1;

	state = iCanState_SEND;

	while(1)
	{
		switch(state)
		{
		case iCanState_SEND :
			ret = iCanNode_sendFrame(&thisNode, pmsg);
			if (ret != iCanErr_OK)
				return ret;
			state = iCanState_RECV;
			break;

		case iCanState_RECV :
			ret = iCanNode_recvFrame(&thisNode, pmsg);
			if (ret != iCanErr_OK)
				return ret;
			state = iCanState_PARSE;
			break;

		case iCanState_PARSE:
			ret = iCanFrame_parse(pmsg, NULL, 0);
			if (ret != iCanErr_OK)
				return ret;

			return iCanErr_OK;
		}
	}
}

void ican_master_init(uint8_t channel)
{
	uint8_t i = 0;
	uint8_t addr = 0;

	thisNode.channelId 									= channel;
	thisNode.addr 										= MASTER_MAC_ID;
	thisNode.state 										= iCanState_IDLE;
	//	imaster.slave_setup_check 								= 0;
	//	imaster.pstat 											= &can_stat[channel];
	thisNode.devInfo.vendor_id 								= VENDOR_ID;
	thisNode.devInfo.product_type 							= PRODUCT_TYPE;
	thisNode.devInfo.product_code 							= PRODUCT_CODE;
	thisNode.devInfo.hardware_version 						= HARDWARE_VERSION;
	thisNode.devInfo.firmware_version 						= FIRMWARE_VERSION;
	thisNode.devInfo.serial_number 							= SERIAL_NUMBER;

	addr 													= iCanPortal_devAddr();
	thisNode.commCfg.dev_mac_id 								= addr & 0x3f;
	thisNode.commCfg.baud_rate 								= (addr >> 6) & 0x03;
	thisNode.commCfg.user_baud_rate 							= USER_BAUD_RATE;
	thisNode.commCfg.cyclic_param 							= CYCLIC_PARAM;
	thisNode.commCfg.cyclic_master 							= CYCLIC_MASTER;
	thisNode.commCfg.cos_type 								= COS_TYPE;
	thisNode.commCfg.master_mac_id 							= MASTER_MAC_ID;

	for(i = 0; i < ICAN_SLAVE_NUM; i++)
	{
		slaves[i].slaveState 				= iCanConn_Disconnected;

		memset((void *)&(slaves[i].resource), 0, sizeof(iCanResource));
		memset((void *)&(slaves[i].devInfo),  0, sizeof(iCanDevInfo));
		memset((void *)&(slaves[i].commCfg),  0, sizeof(iCanCommCfg));
	}

	memset(&ican_msg, 0, sizeof(iCanFrame));
	memset(&itemp, 0, sizeof(ican_temp));

	// bxcan_init(ICAN_MASTER_CHANNEL, addr);
	CAN_Configuration(thisNode.channelId, thisNode.commCfg.user_baud_rate);
}

void ican_master_task(void* pdata)
{
	unsigned char ret = 0;
	unsigned char i = 0;
	unsigned char m = 0;
	unsigned char channel = 0;
	unsigned char data[ICAN_USER_LEN] = {80};

	channel = ICAN_MASTER_CHANNEL;
	ican_master_init(channel);

	while(1)
	{ 
		for (i = 0; i < ICAN_USER_LEN; i++)
		{
			if (i > 33)
				data[i] = 0x99;
			else
				data[i] = m;
		}

		ret = ican_master_write(1, iCanResource_USERDEF, 0, ICAN_USER_LEN, data);
		if ((ret == iCanErr_OK) || (ret == iCanErr_MAC_UNMATCH))
			iCanPortal_LED_canOK();
		else iCanPortal_LED_canErr();

		delayXmsec(20);
		m++;
	}
}


/*******************************************************************************
* Function Name  : CAN_Configuration
* Description    : Configures the CAN controller.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
const CAN_CFG CAN_CFGs[] = {
	// CAN baudrate = RCC_APB1PeriphClock/(CAN_SJW+CAN_BS1+CAN_BS2)/CAN_Prescaler
	// here, it=8M/(1+8+7)/2=250K
	// CiA �Ƽ�������: 1)>800K 75%, 2)>500K 80%, 3)500K- 87.5%	������=(1+bs1)/(1+bs1+bs2)
	// {CAN_SJW_1tq, CAN_BS1_5tq,  CAN_BS2_2tq, 1},	// 1M, 75%, unallowed in iCAN
	// {CAN_SJW_1tq, CAN_BS1_7tq,  CAN_BS2_2tq, 1},	// 800K, 80%, unallowed in iCAN
	{CAN_SJW_1tq, CAN_BS1_13tq, CAN_BS2_2tq, 1},	// iCanBaudIdx_500K, 87.5%
	{CAN_SJW_1tq, CAN_BS1_13tq, CAN_BS2_2tq, 2},  	// iCanBaudIdx_250K, 87.5%
	{CAN_SJW_1tq, CAN_BS1_13tq, CAN_BS2_2tq, 4},  	// iCanBaudIdx_125K, 87.5%
	{CAN_SJW_1tq, CAN_BS1_13tq, CAN_BS2_2tq, 50},  	// iCanBaudIdx_10K,	 87.5%
	// iCanBaudIdx_NotEffect =0xff
};

void CAN_Configuration(uint8_t channelId, uint8_t baudRateIdx)
{
	CAN_TypeDef*           canCh = iCanChannel(channelId);
	CAN_InitTypeDef        CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;

	// CAN register init
	CAN_DeInit(canCh);									  //��λCAN1�����мĴ���
	CAN_StructInit(&CAN_InitStructure);				  //���Ĵ���ȫ�����ó�Ĭ��ֵ

	// CAN cell init
	CAN_InitStructure.CAN_TTCM            =DISABLE;				  //��ֹʱ�䴥��ͨ�ŷ�ʽ
	CAN_InitStructure.CAN_ABOM            =DISABLE;				  //��ֹCAN�����Զ��رչ���
	CAN_InitStructure.CAN_AWUM            =DISABLE;				  //��ֹ�Զ�����ģʽ
	CAN_InitStructure.CAN_NART            =DISABLE;				  //��ֹ���Զ��ش�ģʽ
	CAN_InitStructure.CAN_RFLM            =DISABLE;				  //��ֹ����FIFO����
	CAN_InitStructure.CAN_TXFP            =DISABLE;				  //��ֹ����FIFO���ȼ�
	CAN_InitStructure.CAN_Mode            =CAN_Mode_Normal;		  //����CAN������ʽΪ�����շ�ģʽ

	// set the baudrate = 250Kbps at 8Mhz
	CAN_InitStructure.CAN_SJW             =CAN_CFGs[baudRateIdx].sjw;			  //��������ͬ����ת��ʱ������
	CAN_InitStructure.CAN_BS1             =CAN_CFGs[baudRateIdx].bs1;			  //�����ֶ�1��ʱ��������
	CAN_InitStructure.CAN_BS2             =CAN_CFGs[baudRateIdx].bs2;			  //�����ֶ�2��ʱ��������
	CAN_InitStructure.CAN_Prescaler       =CAN_CFGs[baudRateIdx].prescaler;		  //����ʱ�����ӳ���Ϊ1����

	CAN_Init(canCh, &CAN_InitStructure);				          //�����ϲ�����ʼ��CAN1�˿�

	// CAN filter init
	CAN_FilterInitStructure.CAN_FilterNumber         =channelId;				   //ѡ��CAN������0
	CAN_FilterInitStructure.CAN_FilterMode           =CAN_FilterMode_IdMask;       //��ʼ��Ϊ��ʶ/����ģʽ
	CAN_FilterInitStructure.CAN_FilterScale          =CAN_FilterScale_32bit;	   //ѡ�������Ϊ32λ
	CAN_FilterInitStructure.CAN_FilterIdHigh         =0x0000;					   //��������ʶ�Ÿ�16λ
	CAN_FilterInitStructure.CAN_FilterIdLow          =0x0000;					   //��������ʶ�ŵ�16λ
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh     =0x0000;				   //����ģʽѡ���������ʶ�Ż����κŵĸ�16λ
	CAN_FilterInitStructure.CAN_FilterMaskIdLow      =0x0000;				   //����ģʽѡ���������ʶ�Ż����κŵĵ�16λ
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment =iCanFIFO(channelId);		       //��FIFO 0�����������0
	CAN_FilterInitStructure.CAN_FilterActivation     =ENABLE;				   //ʹ�ܹ�����
	CAN_FilterInit(&CAN_FilterInitStructure);

	// enable the FIFO0 message pending interrupt
	// CAN_ITConfig((0 == channelId) ? CAN_IT_FMP0 : CAN_IT_FMP1, ENABLE);
}

