#ifndef __CANBUS_H__
#define __CANBUS_H__

#include "common.h"

#include  <stm32f10x_conf.h>
#include  <stm32f10x.h>

enum
{
	CanBaud_10Kbps,
	CanBaud_20Kbps,
	CanBaud_50Kbps,
	CanBaud_100Kbps,
	CanBaud_125Kbps,
	CanBaud_250Kbps,
	CanBaud_500Kbps,
	CanBaud_800Kpbs,
	CanBaud_1Mbps
};

uint8_t CAN_setBaud(uint8_t baudId);

/* ----------------------------------------
// template: CANBUS GPIO settings
// ----------------------------------------
	// TODO: CAN_RX=B8, 上拉输入; CAN_TX=B9, 复用推拉输出
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_8;	
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	CAN_Configuration();
// ---------------------------------------*/

#endif // __CANBUS_H__
