#include "canbus.h"

// ---------------------------------------------------------------------------
// CAN bit rates - Registers setup
// ---------------------------------------------------------------------------
typedef struct _CAN_BaudRateConfig
{
	uint8_t Prescaler;   //(1...64)Baud Rate Prescaler
	uint8_t SJW;         //(1...4) SJW time
	uint8_t BS1;      //(1...8) Phase Segment 1 time
	uint8_t BS2;      //(1...8) Phase Segment 2 time
} CAN_BaudRateConfig;

const static CAN_BaudRateConfig CAN_BaudRateConfig500Kbps= {1, CAN_SJW_1tq, CAN_BS1_13tq, CAN_BS2_2tq};  // 500kbps, (1+13)/(1+13+2)=87.5%

// ----------------------------------------
// CAN_Configuration
// ----------------------------------------
uint8_t CAN_setBaud(uint8_t baudId)
{
	CAN_InitTypeDef              CAN_InitStruct;
	const CAN_BaudRateConfig*    baudConf = NULL;

	// determin baud to take and the baudConf
	switch(baudId)
	{
	case CanBaud_10Kbps:
	case CanBaud_20Kbps:
	case CanBaud_50Kbps:
	case CanBaud_125Kbps:
	case CanBaud_250Kbps:
	case CanBaud_500Kbps:
	case CanBaud_800Kpbs:
	case CanBaud_1Mbps:
	default:
		baudConf = &CAN_BaudRateConfig500Kbps;
		baudId   = CanBaud_500Kbps;
		break;
	}

	// CAN cell init
	CAN_StructInit(&CAN_InitStruct);				  //���Ĵ���ȫ�����ó�Ĭ��ֵ

	CAN_InitStruct.CAN_TTCM=DISABLE;			   //��ֹʱ�䴥��ͨ�ŷ�ʽ
	CAN_InitStruct.CAN_ABOM=DISABLE;			   //��ֹCAN�����Զ��رչ���
	CAN_InitStruct.CAN_AWUM=DISABLE;			   //��ֹ�Զ�����ģʽ
	CAN_InitStruct.CAN_NART=DISABLE;			   //��ֹ���Զ��ش�ģʽ
	CAN_InitStruct.CAN_RFLM=DISABLE;			   //��ֹ����FIFO����
	CAN_InitStruct.CAN_TXFP=DISABLE;			   //��ֹ����FIFO���ȼ�
#ifdef CAN_LOOPBACK
	CAN_InitStruct.CAN_Mode=CAN_Mode_Loopback;
#else
	CAN_InitStruct.CAN_Mode=CAN_Mode_Normal;		  //����CAN������ʽΪ�����շ�ģʽ
#endif

	CAN_InitStruct.CAN_SJW       =baudConf->SJW;			  //��������ͬ����ת��ʱ������
	CAN_InitStruct.CAN_BS1       =baudConf->BS1;			  //�����ֶ�1��ʱ��������
	CAN_InitStruct.CAN_BS2       =baudConf->BS2;			  //�����ֶ�2��ʱ��������
	CAN_InitStruct.CAN_Prescaler =baudConf->Prescaler;				  //����ʱ�����ӳ���Ϊ1����
	CAN_Init(CAN1, &CAN_InitStruct);			   //�����ϲ�����ʼ��CAN1�˿�

	return baudId;
}

void CAN_Configuration(void)
{
	CAN_FilterInitTypeDef  CAN_FilterInitStruct;

	// CAN register init
	CAN_DeInit(CAN1);									  //��λCAN1�����мĴ���

	CAN_setBaud(CanBaud_500Kbps);

	// CAN filter init
	CAN_FilterInitStruct.CAN_FilterNumber         =0;						//ѡ��CAN������0
	CAN_FilterInitStruct.CAN_FilterMode           =CAN_FilterMode_IdMask;	//��ʼ��Ϊ��ʶ/����ģʽ
	CAN_FilterInitStruct.CAN_FilterScale          =CAN_FilterScale_32bit;	//ѡ�������Ϊ32λ
	CAN_FilterInitStruct.CAN_FilterIdHigh         =0x0000;					//��������ʶ�Ÿ�16λ
	CAN_FilterInitStruct.CAN_FilterIdLow          =0x0000;					//��������ʶ�ŵ�16λ
	CAN_FilterInitStruct.CAN_FilterMaskIdHigh     =0x0000;				    //����ģʽѡ���������ʶ�Ż����κŵĸ�16λ
	CAN_FilterInitStruct.CAN_FilterMaskIdLow      =0x0000;				    //����ģʽѡ���������ʶ�Ż����κŵĵ�16λ
	CAN_FilterInitStruct.CAN_FilterFIFOAssignment =CAN_FIFO0;		        //��FIFO 0�����������0
	CAN_FilterInitStruct.CAN_FilterActivation     =ENABLE;				    //ʹ�ܹ�����
	CAN_FilterInit(&CAN_FilterInitStruct);
}
