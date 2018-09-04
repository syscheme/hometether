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
	CAN_StructInit(&CAN_InitStruct);				  //将寄存器全部设置成默认值

	CAN_InitStruct.CAN_TTCM=DISABLE;			   //禁止时间触发通信方式
	CAN_InitStruct.CAN_ABOM=DISABLE;			   //禁止CAN总线自动关闭管理
	CAN_InitStruct.CAN_AWUM=DISABLE;			   //禁止自动唤醒模式
	CAN_InitStruct.CAN_NART=DISABLE;			   //禁止非自动重传模式
	CAN_InitStruct.CAN_RFLM=DISABLE;			   //禁止接收FIFO锁定
	CAN_InitStruct.CAN_TXFP=DISABLE;			   //禁止发送FIFO优先级
#ifdef CAN_LOOPBACK
	CAN_InitStruct.CAN_Mode=CAN_Mode_Loopback;
#else
	CAN_InitStruct.CAN_Mode=CAN_Mode_Normal;		  //设置CAN工作方式为正常收发模式
#endif

	CAN_InitStruct.CAN_SJW       =baudConf->SJW;			  //设置重新同步跳转的时间量子
	CAN_InitStruct.CAN_BS1       =baudConf->BS1;			  //设置字段1的时间量子数
	CAN_InitStruct.CAN_BS2       =baudConf->BS2;			  //设置字段2的时间量子数
	CAN_InitStruct.CAN_Prescaler =baudConf->Prescaler;				  //配置时间量子长度为1周期
	CAN_Init(CAN1, &CAN_InitStruct);			   //用以上参数初始化CAN1端口

	return baudId;
}

void CAN_Configuration(void)
{
	CAN_FilterInitTypeDef  CAN_FilterInitStruct;

	// CAN register init
	CAN_DeInit(CAN1);									  //复位CAN1的所有寄存器

	CAN_setBaud(CanBaud_500Kbps);

	// CAN filter init
	CAN_FilterInitStruct.CAN_FilterNumber         =0;						//选择CAN过滤器0
	CAN_FilterInitStruct.CAN_FilterMode           =CAN_FilterMode_IdMask;	//初始化为标识/屏蔽模式
	CAN_FilterInitStruct.CAN_FilterScale          =CAN_FilterScale_32bit;	//选择过滤器为32位
	CAN_FilterInitStruct.CAN_FilterIdHigh         =0x0000;					//过滤器标识号高16位
	CAN_FilterInitStruct.CAN_FilterIdLow          =0x0000;					//过滤器标识号低16位
	CAN_FilterInitStruct.CAN_FilterMaskIdHigh     =0x0000;				    //根据模式选择过滤器标识号或屏蔽号的高16位
	CAN_FilterInitStruct.CAN_FilterMaskIdLow      =0x0000;				    //根据模式选择过滤器标识号或屏蔽号的低16位
	CAN_FilterInitStruct.CAN_FilterFIFOAssignment =CAN_FIFO0;		        //将FIFO 0分配给过滤器0
	CAN_FilterInitStruct.CAN_FilterActivation     =ENABLE;				    //使能过滤器
	CAN_FilterInit(&CAN_FilterInitStruct);
}
