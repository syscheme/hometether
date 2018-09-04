#include "stm32f10x_gpio.h"
#include "ican.h"
#include "../../common/common.h"
// temporarly
#include "tmr.h"
#include "slave/ican_slave.h"
// temporarly

// -----------------------------
// funcs of iCAN portal for STM32F103
// -----------------------------

void iCanISR_OnRx()
{
	/*
	unsigned int val = 0;

	val = raw_readl(CAN1_RFXR(RXFIFO0));

	if(val & (1 << FOVR0))
	{
	can_stat[BXCAN_CHANNEL1].rx_over0 = 1;	
	bit_clr(CAN1_RFXR(RXFIFO0), FOVR0);
	}
	else
	can_stat[BXCAN_CHANNEL1].rx_over0 = 0;

	if(val & (1 << FULL0))
	{
	can_stat[BXCAN_CHANNEL1].rx_full0 = 1;
	bit_clr(CAN1_RFXR(RXFIFO0), FULL0);
	}
	else
	can_stat[BXCAN_CHANNEL1].rx_full0 = 0;

	if(val & 0x03)
	can_stat[BXCAN_CHANNEL1].rx_flag0 = 1;

	RX0_INT_DISABLE(BXCAN_CHANNEL1);
	*/
}

/*
static INT32S can_read(uint8_t channel, can_msg * msg)
{
bxcan_stat * p;

if ((channel > BXCAN_CHANNEL2) || (msg == NULL))
return ERR_CAN_PARAM;

p = &can_stat[channel];

if (p->rx_flag0)
{
p->rx_flag0 = 0;
bxcan_read_can_data(channel, RXFIFO0, msg);
if (channel == BXCAN_CHANNEL1)
p->rx_fnum0 = raw_readl(CAN1_RFXR(RXFIFO0)) & 0x03;
else
p->rx_fnum0 = raw_readl(CAN2_RFXR(RXFIFO0)) & 0x03;

RX0_INT_ENABLE(channel);
}
else if (p->rx_flag1)
{
p->rx_flag1 = 0;
bxcan_read_can_data(channel, RXFIFO1, msg);
if (channel == BXCAN_CHANNEL1)
p->rx_fnum1 = raw_readl(CAN1_RFXR(RXFIFO1)) & 0x03;
else
p->rx_fnum1 = raw_readl(CAN2_RFXR(RXFIFO1)) & 0x03;
RX1_INT_ENABLE(channel);
}
else return ERR_CAN_READ;

return CAN_OK;
}
*/

static INT32S frame2msg(iCanFrame* pframe, CanTxMsg* pTxMsg)
{
	if (NULL == pTxMsg || NULL == pframe)
		return iCanErr_BAD_PARAMS;

	//ican definition of id bits
	//     +28--27+26---------------21+
	//     |  00  |     src MacId     |
	//     +------+-----------------13+
	//     |  00  |     destMacId     |
	//     +------+-------------------+ 
	//     +-12+11--------8+7-----------------0+ 
	//     |ACK|  FuncId   |    resourceId     |
	//     +---+-----------+-------------------+ 
	pTxMsg->StdId  =(pframe->id.source_id & 0xff) | ((pframe->id.func_id & 0x0f) << 8) | ((pframe->id.ack & 0x01) << 12) |
		((pframe->id.dest_mac_id & 0xff) << 13) | ((pframe->id.src_mac_id & 0xff) << 21);

	// ican only take extended data frame
	pTxMsg->RTR    = CAN_RTR_DATA;
	pTxMsg->IDE    = CAN_ID_EXT;  

	// fill in the data
	pTxMsg->DLC    = pframe->dlc;
	memcpy(pTxMsg->Data, pframe->frame_data, min(8, pframe->dlc));

	return iCanErr_OK;
}

static INT32S msg2frame(CanRxMsg* pRxMsg, iCanFrame* pframe)
{
	if (NULL == pRxMsg || NULL == pframe)
		return iCanErr_BAD_PARAMS;

	pframe->id.source_id   =  pRxMsg->StdId & 0xff;
	pframe->id.func_id     = (pRxMsg->StdId >> 8) & 0x0f;
	pframe->id.ack         = (pRxMsg->StdId >> 12) & 0x01;
	pframe->id.dest_mac_id = (pRxMsg->StdId >> 13) & 0xff;
	pframe->id.src_mac_id  = (pRxMsg->StdId >> 21) & 0xff;

	pframe->dlc = pRxMsg->DLC;
	memcpy(pframe->frame_data, pRxMsg->Data, min(8, pframe->dlc));

	return iCanErr_OK;
}

void iCanPortal_send(uint8_t channel, iCanFrame* pframe)
{
	uint8_t txMailbox;
	uint8_t i=0xff;

	CanTxMsg txMsg;
	memset(&txMsg, 0, sizeof(txMsg));

	frame2msg(pframe, &txMsg);

	txMailbox = CAN_Transmit(iCanChannel(channel), &txMsg);
	while (i-- && CAN_TransmitStatus(iCanChannel(channel), txMailbox) != CANTXOK);
}

void iCanPortal_recv(uint8_t channel, iCanFrame* pframe)
{
	CanRxMsg RxMessage;
	uint8_t i=0xff;
	memset(&RxMessage, 0, sizeof(RxMessage));

	// wait for a message to arrive
	while (i-- && CAN_MessagePending(iCanChannel(channel), iCanFIFO(channel)) < 1);

	// receiving
	CAN_Receive(iCanChannel(channel), iCanFIFO(channel), &RxMessage);

	// TODO: enable the CAN interrupt of canCh

	// convert the message to iCAN frame
	msg2frame(&RxMessage, pframe);
}

uint8_t iCanPortal_devAddr(void)
{
	uint8_t addr = 0;

#ifdef CAN_MASTER
	/*
	raw_writel(GPIOE_CRL, 0x44444444);
	bit_clr(GPIOD_ODR, ODR2);

	addr = raw_readl(GPIOE_IDR);
	*/
#endif // CAN_MASTER

	return addr;
}

// -----------------------------
// LEDs of iCAN portal for STM32F103
// -----------------------------
void iCanPortal_LED_sysRun()
{
#ifdef WITH_LED
	do
	{
		unsigned int val = 0;
		val = raw_readl(GPIOB_ODR);
		val ^= (1 << ODR9);
		raw_writel(GPIOB_ODR, val);
	} while(0);
#endif // WITH_LED
}

void iCanPortal_LED_canOK()
{
#ifdef WITH_LED
	do
	{
		unsigned int val = 0;
		val = raw_readl(GPIOB_ODR);
		val ^= (1 << ODR6);
		val |= (1 << ODR7);
		raw_writel(GPIOB_ODR, val);
	} while(0);
#endif // WITH_LED
}

void iCanPortal_LED_canErr()
{
#ifdef WITH_LED
	do
	{
		unsigned int val = 0;
		val = raw_readl(GPIOB_ODR);
		val |= (1 << ODR6);
		val &= ~(1 << ODR7);
		raw_writel(GPIOB_ODR, val);
	} while(0);
#endif // WITH_LED
}

