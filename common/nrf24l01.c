#include "nrf24l01.h"

#define DEBUG

#ifndef nRF_RX_CH
#  define nRF_RX_CH 5
#endif // nRF_RX_CH

// static volatile bool bTxBusy =FALSE;

// Chip Enable Activates RX or TX mode
#define CE_H(_CHIP)	pinSET(_CHIP->pinCE, 1) 
#define CE_L(_CHIP)	pinSET(_CHIP->pinCE, 0) 

// SPI Chip Select
#define CSN_H(_CHIP)  pinSET(_CHIP->pinCSN,1) 
#define CSN_L(_CHIP)  pinSET(_CHIP->pinCSN,0) 

void nRF24L01_configPipes(nRF24L01* chip);

// ---------------------------------------------------------------------------
// definition of nRF24L01 instructions
// ---------------------------------------------------------------------------
#define nRFCmd_READ_REG     0x00  	// 读寄存器指令
#define nRFCmd_WRITE_REG    0x20 	// 写寄存器指令
#define nRFCmd_RX_PLOAD     0x61  	// 读取接收数据指令
#define nRFCmd_TX_PLOAD     0xA0  	// 写待发数据指令
#define nRFCmd_FLUSH_TX     0xE1 	// 冲洗发送 FIFO指令
#define nRFCmd_FLUSH_RX     0xE2  	// 冲洗接收 FIFO指令
#define nRFCmd_REUSE_TX_PL  0xE3  	// 定义重复装载数据指令
#define nRFCmd_NOP          0xFF  	// 保留

// ---------------------------------------------------------------------------
// definition of nRF24L01 register addresses
// ---------------------------------------------------------------------------
#define CONFIG          0x00  // 配置收发状态，CRC校验模式以及收发状态响应方式
#define EN_AA           0x01  // 自动应答功能设置
#define EN_RXADDR       0x02  // 可用信道设置
#define SETUP_AW        0x03  // 收发地址宽度设置
#define SETUP_RETR      0x04  // 自动重发功能设置
#define RF_CH           0x05  // 工作频率设置
#define RF_SETUP        0x06  // 发射速率、功耗功能设置
#define STATUS          0x07  // 状态寄存器
#define OBSERVE_TX      0x08  // 发送监测功能
#define CD              0x09  // 地址检测           
#define RX_ADDR_P0      0x0A  // 频道0接收数据地址
#define RX_ADDR_P1      0x0B  // 频道1接收数据地址
#define RX_ADDR_P2      0x0C  // 频道2接收数据地址
#define RX_ADDR_P3      0x0D  // 频道3接收数据地址
#define RX_ADDR_P4      0x0E  // 频道4接收数据地址
#define RX_ADDR_P5      0x0F  // 频道5接收数据地址
#define TX_ADDR         0x10  // 发送地址寄存器
#define RX_PW_P0        0x11  // 接收频道0接收数据长度
#define RX_PW_P1        0x12  // 接收频道1接收数据长度
#define RX_PW_P2        0x13  // 接收频道2接收数据长度
#define RX_PW_P3        0x14  // 接收频道3接收数据长度
#define RX_PW_P4        0x15  // 接收频道4接收数据长度
#define RX_PW_P5        0x16  // 接收频道5接收数据长度
#define FIFO_STATUS     0x17  // FIFO栈入栈出状态寄存器设置

// ---------------------------------------------------------------------------
// definition of nRF24L01 instructions
// ---------------------------------------------------------------------------
// CONFIG register bitwise definitions
#define FLG_CONFIG_RESERVED	    (1<<7)
#define	FLG_CONFIG_MASK_RX_DR	(1<<6)
#define	FLG_CONFIG_MASK_TX_DS	(1<<5)
#define	FLG_CONFIG_MASK_MAX_RT	(1<<4)
#define	FLG_CONFIG_EN_CRC		(1<<3)
#define	FLG_CONFIG_CRCO		    (1<<2)
#define	FLG_CONFIG_PWR_UP		(1<<1)
#define	FLG_CONFIG_PRIM_RX		(1<<0)

// EN_AA register bitwise definitions
#define FLG_EN_AA_RESERVED		0xC0
#define FLG_EN_AA_ENAA_ALL		0x3F
#define FLG_EN_AA_ENAA_P5		0x20
#define FLG_EN_AA_ENAA_P4		0x10
#define FLG_EN_AA_ENAA_P3		0x08
#define FLG_EN_AA_ENAA_P2		0x04
#define FLG_EN_AA_ENAA_P1		0x02
#define FLG_EN_AA_ENAA_P0		0x01
#define FLG_EN_AA_ENAA_NONE	    0x00

// EN_RXADDR register bitwise definitions
#define FLG_EN_RXADDR_RESERVED	0xC0
#define FLG_EN_RXADDR_ERX_ALL	0x3F
#define FLG_EN_RXADDR_ERX_P5	0x20
#define FLG_EN_RXADDR_ERX_P4	0x10
#define FLG_EN_RXADDR_ERX_P3	0x08
#define FLG_EN_RXADDR_ERX_P2	0x04
#define FLG_EN_RXADDR_ERX_P1	0x02
#define FLG_EN_RXADDR_ERX_P0	0x01
#define FLG_EN_RXADDR_ERX_NONE	0x00

// SETUP_AW register bitwise definitions
#define FLG_SETUP_AW_RESERVED	0xFC
#define FLG_SETUP_AW			0x03
#define FLG_SETUP_AW_5BYTES	    0x03
#define FLG_SETUP_AW_4BYTES	    0x02
#define FLG_SETUP_AW_3BYTES	    0x01
#define FLG_SETUP_AW_ILLEGAL	0x00

// SETUP_RETR register bitwise definitions
#define FLG_SETUP_RETR_ARD			0xF0
#define FLG_SETUP_RETR_ARD_4000	    0xF0
#define FLG_SETUP_RETR_ARD_3750	    0xE0
#define FLG_SETUP_RETR_ARD_3500	    0xD0
#define FLG_SETUP_RETR_ARD_3250	    0xC0
#define FLG_SETUP_RETR_ARD_3000	    0xB0
#define FLG_SETUP_RETR_ARD_2750	    0xA0
#define FLG_SETUP_RETR_ARD_2500	    0x90
#define FLG_SETUP_RETR_ARD_2250	    0x80
#define FLG_SETUP_RETR_ARD_2000	    0x70
#define FLG_SETUP_RETR_ARD_1750	    0x60
#define FLG_SETUP_RETR_ARD_1500	    0x50
#define FLG_SETUP_RETR_ARD_1250	    0x40
#define FLG_SETUP_RETR_ARD_1000	    0x30
#define FLG_SETUP_RETR_ARD_750		0x20
#define FLG_SETUP_RETR_ARD_500		0x10
#define FLG_SETUP_RETR_ARD_250		0x00
#define FLG_SETUP_RETR_ARC			0x0F
#define FLG_SETUP_RETR_ARC_15		0x0F
#define FLG_SETUP_RETR_ARC_14		0x0E
#define FLG_SETUP_RETR_ARC_13		0x0D
#define FLG_SETUP_RETR_ARC_12		0x0C
#define FLG_SETUP_RETR_ARC_11		0x0B
#define FLG_SETUP_RETR_ARC_10		0x0A
#define FLG_SETUP_RETR_ARC_9		0x09
#define FLG_SETUP_RETR_ARC_8		0x08
#define FLG_SETUP_RETR_ARC_7		0x07
#define FLG_SETUP_RETR_ARC_6		0x06
#define FLG_SETUP_RETR_ARC_5		0x05
#define FLG_SETUP_RETR_ARC_4		0x04
#define FLG_SETUP_RETR_ARC_3		0x03
#define FLG_SETUP_RETR_ARC_2		0x02
#define FLG_SETUP_RETR_ARC_1		0x01
#define FLG_SETUP_RETR_ARC_0		0x00

// RF_CH register bitwise definitions
#define FLG_RF_CH_RESERVED	0x80

// RF_SETUP register bitwise definitions
#define FLG_RF_SETUP_RESERVED	0xE0
#define FLG_RF_SETUP_PLL_LOCK	0x10
#define FLG_RF_SETUP_RF_DR		0x08
#define FLG_RF_SETUP_RF_PWR	    0x06
#define FLG_RF_SETUP_RF_PWR_0	0x06
#define FLG_RF_SETUP_RF_PWR_6 	0x04
#define FLG_RF_SETUP_RF_PWR_12	0x02
#define FLG_RF_SETUP_RF_PWR_18	0x00
#define FLG_RF_SETUP_LNA_HCURR	0x01

/*
//  the status byte
// ---------------------------------------------------------------------------
#define  STATEFLG_TX_FULL  (1<<0)
#define  STATEFLG_MAX_RT   (1<<4)
#define  STATEFLG_TX_DS    (1<<5)
#define  STATEFLG_RX_DR    (1<<6)
// the FIFO status byte
#define FIFOSTATE_RX_EMPTY (1<<0)
#define FIFOSTATE_RX_FULL  (1<<1)
#define FIFOSTATE_TX_EMPTY (1<<4)
#define FIFOSTATE_TX_FULL  (1<<5)
#define FIFOSTATE_TX_REUSE (1<<6)
*/

// STATUS register bitwise definitions
#define FLG_STATUS_RESERVED					    0x80
#define FLG_STATUS_RX_DR						0x40
#define FLG_STATUS_TX_DS						0x20
#define FLG_STATUS_MAX_RT						0x10
#define FLG_STATUS_RX_P_NO						0x0E
#define FLG_STATUS_RX_P_NO_RX_FIFO_NOT_EMPTY	0x0E
#define FLG_STATUS_RX_P_NO_UNUSED				0x0C
#define FLG_STATUS_RX_P_NO_5					0x0A
#define FLG_STATUS_RX_P_NO_4					0x08
#define FLG_STATUS_RX_P_NO_3					0x06
#define FLG_STATUS_RX_P_NO_2					0x04
#define FLG_STATUS_RX_P_NO_1					0x02
#define FLG_STATUS_RX_P_NO_0					0x00
#define FLG_STATUS_TX_FULL						0x01

// FIFO_STATUS register bitwise definitions
#define FLG_FIFO_STATUS_RESERVED                0x8C
#define FLG_FIFO_STATUS_TX_REUSE                0x40
#define FLG_FIFO_STATUS_TX_FULL                 0x20
#define FLG_FIFO_STATUS_TX_EMPTY                0x10
#define FLG_FIFO_STATUS_RX_FULL                 0x02
#define FLG_FIFO_STATUS_RX_EMPTY                0x01


// OBSERVE_TX register bitwise definitions
#define FLG_OBSERVE_TX_PLOS_CNT                 0xF0
#define FLG_OBSERVE_TX_ARC_CNT                  0x0F

// CD register bitwise definitions
#define FLG_CD_RESERVED                         0xFE
#define FLG_CD_CD                               0x01

// RX_PW_P0 register bitwise definitions
#define FLG_RX_PW_P0_RESERVED	0xC0

// RX_PW_P0 register bitwise definitions
#define FLG_RX_PW_P0_RESERVED	0xC0

// RX_PW_P1 register bitwise definitions
#define FLG_RX_PW_P1_RESERVED	0xC0

// RX_PW_P2 register bitwise definitions
#define FLG_RX_PW_P2_RESERVED	0xC0

// RX_PW_P3 register bitwise definitions
#define FLG_RX_PW_P3_RESERVED	0xC0

// RX_PW_P4 register bitwise definitions
#define FLG_RX_PW_P4_RESERVED	0xC0

// RX_PW_P5 register bitwise definitions
#define FLG_RX_PW_P5_RESERVED	0xC0

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// declarations of static func
// ---------------------------------------------------------------------------
uint8_t nRF24L01_readBuff(nRF24L01* chip, uint8_t reg, uint8_t *pBuf, uint8_t Len);
uint8_t nRF24L01_writeBuff(nRF24L01* chip, uint8_t reg, const uint8_t *pBuf, uint8_t len);
uint8_t nRF24L01_readReg(nRF24L01* chip, uint8_t reg);
uint8_t nRF24L01_writeReg(nRF24L01* chip, uint8_t reg, uint8_t value);
#define nRF24L01_cmd nRF24L01_readReg

/*	replaced by delayXusec()
// ---------------------------------------------------------------------------
// 延时函数,非精确延时
// ---------------------------------------------------------------------------
void delayUsec(u32 n)
{
u32 i;

while(n--)
{
i=2;
while(i--);
}
}
*/

static void nRF24L01_config(nRF24L01* chip, uint8_t config)
{
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +CONFIG, config);       //0x0E;
	delayXusec(1500);

	CE_H(chip);

	/*
	if((config & FLG_CONFIG_PRIM_RX) == 0)
	CE_L(chip); // TX mode
	else
	{
	// RX mode
	if (FLG_CONFIG_PWR_UP & config)
	CE_L(chip);	  // active RX mode
	else CE_H(chip);
	}
	*/
}

// ---------------------------------------------------------------------------
// nRF24L01 initialization
// ---------------------------------------------------------------------------
hterr nRF24L01_init(nRF24L01* chip, uint32_t nodeId, uint8_t rxmode)
{
	uint8_t en_aa  = FLG_EN_AA_ENAA_NONE;
	uint8_t config = FLG_CONFIG_EN_CRC | FLG_CONFIG_CRCO |FLG_CONFIG_PWR_UP;
	uint8_t buff[nRF24L01_ADDR_LEN]={0};

	if (NULL == chip)
		return ERR_INVALID_PARAMETER;

	if (FIFO_init(&chip->txFIFO, nRF24L01_PENDING_TX_MAX, sizeof(pbuf*), 0) <=0)
		return ERR_INVALID_PARAMETER;

	if (rxmode)
		config |= FLG_CONFIG_PRIM_RX;

	if (rxmode & FLG_CONFIG_PWR_UP) // active RX mode
		config |= FLG_CONFIG_PWR_UP;

	en_aa = FLG_EN_AA_ENAA_P0;

	CE_H(chip);  delayXusec(1000);// unselect the chip
	CE_L(chip);  // chip enable
	CSN_H(chip); // SPI disable 

	chip->nodeId = chip->peerNodeId = nodeId;

#ifdef DEBUG
	nRF24L01_readBuff(chip, TX_ADDR, buff, nRF24L01_ADDR_LEN); //debug 测试原来的本地地址：复位值是：0xE7 0xE7 0xE7 0xE7 0xE7
#endif // DEBUG

	nRF24L01_writeReg(chip,  nRFCmd_WRITE_REG +SETUP_AW,    FLG_SETUP_AW_5BYTES);	 // address-wide demo=FLG_SETUP_AW_5BYTES
	memcpy(buff, (void*) &chip->peerNodeId, sizeof(uint32_t));
	buff[4] = chip->peerPortNum;

	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +TX_ADDR,    buff,    nRF24L01_ADDR_LEN);
	// the local address to receive auto ack from the transmittee
	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +RX_ADDR_P0, buff,    nRF24L01_ADDR_LEN);
#ifdef DEBUG
	nRF24L01_readBuff(chip, TX_ADDR, buff, nRF24L01_ADDR_LEN); //debug: test the tx address just written into
#endif // DEBUG

	// the rx addresses
	memcpy(buff, &chip->nodeId, sizeof(uint32_t));
	buff[4] = 0x00;

	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RX_PW_P0,  nRF24L01_MAX_PLOAD); 

#if nRF_RX_CH > 0
	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +RX_ADDR_P1, buff, nRF24L01_ADDR_LEN);   // the local address to receive
	// set the payload length
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RX_PW_P1,  nRF24L01_MAX_PLOAD);
	en_aa |= FLG_EN_AA_ENAA_P1; 
#endif // nRF_RX_CH

	buff[4]++;
#if nRF_RX_CH > 1
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RX_ADDR_P2, buff[4]);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RX_PW_P2,   nRF24L01_MAX_PLOAD); 
	en_aa |= FLG_EN_AA_ENAA_P2; 
#endif // nRF_RX_CH

	buff[4]++;
#if nRF_RX_CH > 2
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RX_ADDR_P3, buff[4]);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RX_PW_P3,   nRF24L01_MAX_PLOAD); 
	en_aa |= FLG_EN_AA_ENAA_P3; 
#endif // nRF_RX_CH

	buff[4]++;
#if nRF_RX_CH > 3
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RX_ADDR_P4, buff[4]);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RX_PW_P4,   nRF24L01_MAX_PLOAD); 
	en_aa |= FLG_EN_AA_ENAA_P4; 
#endif // nRF_RX_CH

	buff[4]++;
#if nRF_RX_CH > 4
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RX_ADDR_P5, buff[4]);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RX_PW_P5,   nRF24L01_MAX_PLOAD);
	en_aa |= FLG_EN_AA_ENAA_P5; 
#endif // nRF_RX_CH

	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +EN_AA,     en_aa);    //  频道0自动	ACK应答允许	
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +EN_RXADDR, en_aa); // FLG_EN_RXADDR_ERX_NONE); // demo=0x03
	//nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +SETUP_RETR,FLG_SETUP_RETR_ARD_500 | FLG_SETUP_RETR_ARC_3);	 // demo=FLG_SETUP_RETR_ARC_3
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +SETUP_RETR, FLG_SETUP_RETR_ARD_500 | FLG_SETUP_RETR_ARC_10);	 // demo=FLG_SETUP_RETR_ARC_3
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RF_CH,     40);             //   设置信道工作为2.4GHZ，收发必须一致, demo=0x02
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +RF_SETUP,  0x0f);   		  //设置发射速率为1MHZ，发射功率为最大值0dB, demo=0x0f

	nRF24L01_writeReg(chip, nRFCmd_FLUSH_TX, 0xff);
	nRF24L01_writeReg(chip, nRFCmd_FLUSH_RX, 0xff);
	nRF24L01_config(chip, config);

	return ERR_SUCCESS;
}

// ---------------------------------------------------------------------------
// nRF24L01 SPI read/write registry utility func
// ---------------------------------------------------------------------------
uint8_t nRF24L01_RW_Reg(nRF24L01* chip, uint8_t reg)
{
	uint8_t status;
	status = SPI_RW(chip->spi, reg); // Select register to read from..
	return status;        // return register value
}

// ---------------------------------------------------------------------------
// nRF24L01_readBuff read a buffer of data
// 用于读数据，reg：为寄存器地址，pBuf：为待读出数据地址，uchars：读出数据的个数
// ---------------------------------------------------------------------------
uint8_t nRF24L01_readBuff(nRF24L01* chip, uint8_t reg, uint8_t *pBuf, uint8_t len)
{
	uint8_t status;

	CSN_L(chip);                   // CSN low, init SPI transaction
	status = nRF24L01_RW_Reg(chip, reg); 	// Select register to write to and read status uint8_t

	for( ; len >0; len--)
		*pBuf++ = SPI_RW(chip->spi, 0);

	CSN_H(chip);                   // CSN high again
	return status;                    // return nRF24L01 status uint8_t
}

// ---------------------------------------------------------------------------
// uint nRF24L01_writeBuff
// ---------------------------------------------------------------------------
uint8_t nRF24L01_writeBuff(nRF24L01* chip, uint8_t reg, const uint8_t *pBuf, uint8_t len)
{
	uint8_t status;

	CSN_L(chip);                   // CSN low, init SPI transaction
	status = nRF24L01_RW_Reg(chip, reg); 	// Select register to write to and read status uint8_t
	for( ; len>0; len--)
		SPI_RW(chip->spi, *pBuf++);

	CSN_H(chip);                   // CSN high again
	return status;                    // return nRF24L01 status uint8_t
}

// ---------------------------------------------------------------------------
/// uchar nRF24L01_readReg(uchar reg)
// ---------------------------------------------------------------------------
uint8_t nRF24L01_readReg(nRF24L01* chip, uint8_t reg)
{
	uint8_t val=0;
	nRF24L01_readBuff(chip, reg, &val, sizeof(val));
	return val;        // return register value
}

#define DEBUG

// ---------------------------------------------------------------------------
// nRF24L01_writeReg
// read/write NRF24L01 register
// ---------------------------------------------------------------------------
uint8_t nRF24L01_writeReg(nRF24L01* chip, uint8_t reg, uint8_t value)
{
	return nRF24L01_writeBuff(chip, reg, &value, 1);   // return nRF24L01 status
}

// ---------------------------------------------------------------------------
// SetRX_Mode(void)
// ---------------------------------------------------------------------------
void nRF24L01_setModeRX(nRF24L01* chip)
{
	uint8_t config = nRF24L01_readReg(chip, CONFIG);

	if((config & FLG_CONFIG_PRIM_RX) != 0)
		return; // already at the RX mode, quit

	CE_L(chip);  // chip enable
	//	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +RX_ADDR_P0, chip->txOrAckAddr, nRF24L01_ADDR_LEN); // the local address to receive auto ack
	nRF24L01_writeReg(chip, nRFCmd_FLUSH_RX, 0xff);
	config |= (FLG_CONFIG_PRIM_RX|FLG_CONFIG_PWR_UP);

	nRF24L01_config(chip, config);
}

// ---------------------------------------------------------------------------
void nRF24L01_setModeTX(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum)
{
	uint8_t txAddr[nRF24L01_ADDR_LEN]={0};
	uint8_t config = nRF24L01_readReg(chip, CONFIG);

	if((config & FLG_CONFIG_PRIM_RX) == 0)
	{
		if (peerNodeId == chip->peerNodeId && peerPortNum == chip->peerPortNum)
			return; // already at the TX mode to the same dest, quit
	}

	chip->peerNodeId = peerNodeId; chip->peerPortNum = peerPortNum;

	memcpy(txAddr, (void*) &chip->peerNodeId, sizeof(uint32_t));
	txAddr[4] = chip->peerPortNum;

	CE_L(chip);  // chip enable
	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +TX_ADDR,    txAddr,   nRF24L01_ADDR_LEN); // 装载接收端地址
	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +RX_ADDR_P0, txAddr,   nRF24L01_ADDR_LEN); // 装载接收端地址

	nRF24L01_writeReg(chip, nRFCmd_FLUSH_TX, 0xff);
	config &= (~FLG_CONFIG_PRIM_RX);
	nRF24L01_config(chip, config);

	//	if (chip->extiLine)
	//		EXTI_ClearITPendingBit(chip->extiLine);
}

static void nRF24L01_doTx(nRF24L01* chip, uint32_t destNodeId, uint8_t destPortNum, uint8_t* bufTX)
{
	if (nRF24L01_readReg(chip, STATUS) & FLG_STATUS_MAX_RT)
		nRF24L01_cmd(chip, nRFCmd_FLUSH_TX);

	chip->flags |= nRF24L01_FLAG_TX_BUSY;
	nRF24L01_setModeTX(chip, destNodeId, destPortNum);

	CE_L(chip);	 //set mode to StandBy I	

	nRF24L01_writeBuff(chip, nRFCmd_TX_PLOAD, bufTX, nRF24L01_MAX_PLOAD); 	// load the outgoing data
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, nRF24L01_readReg(chip, STATUS) | 0x70);  // clear the pending interrupts

	CE_H(chip);  // kick-off sending
	delayXusec(500);

	nRF24L01_OnSent(chip, destNodeId, destPortNum, bufTX);
}


// ---------------------------------------------------------------------------
// nRF24L01_TxPacket(uint8_t * bufTX)
// ---------------------------------------------------------------------------
void nRF24L01_TxPacket(nRF24L01* chip, uint8_t* bufTX)
{
	int i =0;
	uint8_t status = 0;
	while (nRF24L01_FLAG_TX_BUSY & chip->flags) // yield if Tx busy
	{
		delayXusec(10);

		if (nRF24L01_FLAG_TX_BUSY & chip->flags)
		{   // quit if the TX fifo gets empty
			i = nRF24L01_readReg(chip, STATUS);
			status = nRF24L01_readReg(chip, FIFO_STATUS);
			if (i & FLG_STATUS_TX_DS || status & FLG_FIFO_STATUS_TX_EMPTY)
				chip->flags &= ~nRF24L01_FLAG_TX_BUSY;
		}
	}

	chip->flags |= nRF24L01_FLAG_TX_BUSY;

	CE_L(chip);	 //StandBy I模式	
	//	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +TX_ADDR,    chip->txOrAckAddr,   nRF24L01_ADDR_LEN); // 装载接收端地址
	//	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +RX_ADDR_P0, chip->txOrAckAddr,   nRF24L01_ADDR_LEN); // 装载接收端地址
	nRF24L01_writeBuff(chip, nRFCmd_TX_PLOAD, bufTX, nRF24L01_MAX_PLOAD);  // 装载数据	

	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, nRF24L01_readReg(chip, STATUS) | 0x70);  // clear the pending interrupts
	//	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, 0xff);  // clear the pending interrupts

	CE_H(chip);  //置高CE，激发数据发送
	delayXusec(500);

	status =0x33;
	for (i=999999; (nRF24L01_FLAG_TX_BUSY & chip->flags) && i>0; i--)
	{
		status = nRF24L01_readReg(chip, STATUS);
		if (0 == (status & (FLG_STATUS_TX_DS | FLG_STATUS_MAX_RT)))
		{
			delayXusec(10);
			continue;
		}

		if (status & FLG_STATUS_MAX_RT)
			nRF24L01_cmd(chip, nRFCmd_FLUSH_TX);

		chip->flags &= ~nRF24L01_FLAG_TX_BUSY;
	}

	//	status = nRF24L01_readReg(chip, STATUS);
	//	status |= (FLG_STATUS_TX_DS | FLG_STATUS_MAX_RT);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, FLG_STATUS_TX_DS | FLG_STATUS_MAX_RT);
}

// ---------------------------------------------------------------------------
// nRF24L01_RxPacket(uint8_t* bufRX)
// ---------------------------------------------------------------------------
// return the pipeId if receive successfully, otherwise 0xff as fail;
uint8_t nRF24L01_RxPacket(nRF24L01* chip, uint8_t* bufRX)
{
	uint8_t status, pipeId=0xff;

	status =nRF24L01_readReg(chip, STATUS);	// 读取状态寄存其来判断数据接收状况

	//	if (0 == (status & FLG_STATUS_RX_DR))				// 判断是否接收到数据
	//		return 0xff;

	pipeId = (status>>1) & 0x07;
	// CE_L();
	//	nRF24L01_readBuff(chip, nRFCmd_RX_PLOAD, bufRX, nRF24L01_PLOAD_LEN);// read receive payload from RX_FIFO buffer
	nRF24L01_readBuff(chip, nRFCmd_RX_PLOAD, bufRX, nRF24L01_MAX_PLOAD);// read receive payload from RX_FIFO buffer

	// CE_H();

	// nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, status);   // 通过写1来清楚中断标志
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, FLG_STATUS_RX_DR);   // 通过写1来清楚中断标志
	//	BlinkPair
	//	GPIO_SetBits(GPIOA, GPIO_Pin_8);
	return pipeId;
}

#if 0
void processIRQ_nRF24L01(nRF24L01* chip)
{
	uint8_t status = 0, fifostatus=0;
	uint8_t pipeId =0xff;
	uint8_t recvBuf_nRF24L01[nRF24L01_MAX_PLOAD +2];

	CE_L(chip);

//	CSN_L(chip);
	status = nRF24L01_readReg(chip, STATUS);     // read the current status byte
	fifostatus = nRF24L01_readReg(chip, FIFO_STATUS);     // read the current fifo status byte
//	CSN_H(chip);

	if (status & FLG_STATUS_TX_DS || fifostatus & FLG_FIFO_STATUS_TX_EMPTY)
	{
		// transfer completed, do nothing but clear bTxBusy and status registry
		chip->flags &= ~nRF24L01_FLAG_TX_BUSY;
	}

	if (status & FLG_STATUS_MAX_RT) 
	{
		// max re-transfer reached, timeout at no ackownledge
//		CSN_L(chip);
		nRF24L01_cmd(chip, nRFCmd_FLUSH_TX);  // clear TX FIFO, resend after clear the interrupt
//		CSN_H(chip);
		chip->flags &= ~nRF24L01_FLAG_TX_BUSY;
	}

	if (status & FLG_STATUS_RX_DR)
	{
		while (0 == (fifostatus & FLG_FIFO_STATUS_RX_EMPTY))
		{
			// received a message here
			pipeId = (status>>1) &0x7;
			// TODO: query for payload value: status = nrf24l01_execute_command(nrf24l01_R_RX_PAYLOAD, data, len, true);
			nRF24L01_readBuff(chip, nRFCmd_RX_PLOAD, recvBuf_nRF24L01, nRF24L01_MAX_PLOAD);  // read from the RX FIFO
			if (pipeId >5)
				break;

			recvBuf_nRF24L01[nRF24L01_MAX_PLOAD] = '\0';
			nRF24L01_OnReceived(chip, pipeId, recvBuf_nRF24L01, nRF24L01_MAX_PLOAD);
			fifostatus = nRF24L01_readReg(chip, FIFO_STATUS);
			// nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, FLG_STATUS_RX_DR);
			status = nRF24L01_readReg(chip, STATUS);
		}

		nRF24L01_cmd(chip, nRFCmd_FLUSH_RX);
	}

	CE_H(chip);
 	delayXusec(500);

	//clear all interrupts in the status register
//	CSN_L(chip);
	status |= (FLG_STATUS_TX_DS | FLG_STATUS_MAX_RT | FLG_STATUS_RX_DR);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, status);
//	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, 0xff);
//	CSN_H(chip);

//	nRF24L01_setModeRX(chip, TRUE); // leave CE_L be set by this step

 /*
	if (0xff != pipeId)
	{
		recvBuf_nRF24L01[nRF24L01_MAX_PLOAD] = '\0';
		OnWirelessMsg(chip, pipeId, recvBuf_nRF24L01, nRF24L01_MAX_PLOAD);
	}
 */

//	GPIO_SetBits(GPIOA, GPIO_Pin_8);
//	if (chip->extiLine)
//		EXTI_ClearITPendingBit(chip->extiLine);
}
#endif //0

void nRF24L01_doISR(nRF24L01* chip)
{
	uint8_t status =0, statusFifo=0;
	uint8_t pipeId =0xff;
	pbuf*   pbuf=NULL;
	uint8_t databuf[nRF24L01_ADDR_LEN+nRF24L01_MAX_PLOAD];
	// uint8_t* pTxBuf =NULL, RxBuf[nRF24L01_MAX_PLOAD+1];

	CE_L(chip);

	status     = nRF24L01_readReg(chip, STATUS);       // read the current status byte
	statusFifo = nRF24L01_readReg(chip, FIFO_STATUS);  // read the current fifo status byte

	if (status & FLG_STATUS_MAX_RT) // max re-transfer reached, timeout withno ackownledge
	{
		nRF24L01_cmd(chip, nRFCmd_FLUSH_TX);  // clear TX FIFO, resend after clear the interrupt
		chip->flags &= ~nRF24L01_FLAG_TX_BUSY;
	}

	if (status & FLG_STATUS_TX_DS || statusFifo & FLG_FIFO_STATUS_TX_EMPTY) // transfer completed
	{
		// do nothing but clear bTxBusy and status registry
		chip->flags &= ~nRF24L01_FLAG_TX_BUSY;
	}

	if (status & FLG_STATUS_RX_DR) // received a message here
	{
		while (0 == (statusFifo & FLG_FIFO_STATUS_RX_EMPTY))
		{
			// received a message here
			pipeId = (status>>1) &0x7;
			// TODO: query for payload value: status = nrf24l01_execute_command(nrf24l01_R_RX_PAYLOAD, data, len, true);
			nRF24L01_readBuff(chip, nRFCmd_RX_PLOAD, databuf, nRF24L01_MAX_PLOAD);  // read from the RX FIFO
			if (pipeId >5)
				break;

			databuf[nRF24L01_MAX_PLOAD] = '\0'; // end with a 0
			nRF24L01_OnReceived(chip, pipeId, databuf, nRF24L01_MAX_PLOAD);
			
			// refresh the values of the chip status
			statusFifo = nRF24L01_readReg(chip, FIFO_STATUS);
			status     = nRF24L01_readReg(chip, STATUS);
		}

		nRF24L01_cmd(chip, nRFCmd_FLUSH_RX);
	}

	if (0 == (chip->flags & nRF24L01_FLAG_TX_BUSY))
	{
		if (ERR_SUCCESS == FIFO_pop(&chip->txFIFO, (void*) &pbuf) && NULL != pbuf)
		{
			pbuf_read(pbuf, 0, databuf, nRF24L01_ADDR_LEN+nRF24L01_MAX_PLOAD);
			nRF24L01_doTx(chip, *(uint32_t*)databuf, *(uint8_t*)(databuf+4), databuf +nRF24L01_ADDR_LEN);
			pbuf_free(pbuf); pbuf = NULL;
		}
	}

	CE_H(chip);
	delayXusec(500);

	//clear all interrupts in the status register
	status |= (FLG_STATUS_TX_DS | FLG_STATUS_MAX_RT | FLG_STATUS_RX_DR);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, status);
}

hterr nRF24L01_transmit(nRF24L01* chip, uint32_t destNodeId, uint8_t destPortNum, uint8_t* payload)
{
	pbuf* pbuf = NULL;
	// uint8_t* pBuf = NULL;
	if (NULL == chip || NULL ==payload)
		return ERR_INVALID_PARAMETER;

	if (0 == (chip->flags & nRF24L01_FLAG_TX_BUSY))
	{
		// the queue seems empty, send this immediately
		nRF24L01_doTx(chip, destNodeId, destPortNum, payload);
		return ERR_SUCCESS;
	}

	// queue the package into txFIFO
	pbuf = pbuf_malloc(nRF24L01_ADDR_LEN+nRF24L01_MAX_PLOAD);
	if (NULL == pbuf)
		return ERR_NOT_ENOUGH_MEMORY;
	
	// fill the pbuf and push into the queue
	pbuf_write(pbuf, 0, (uint8_t*)&destNodeId,  4);
	pbuf_write(pbuf, 4, (uint8_t*)&destPortNum, 1);
	pbuf_write(pbuf, nRF24L01_ADDR_LEN, payload, nRF24L01_MAX_PLOAD);

	if (ERR_SUCCESS != FIFO_push(&chip->txFIFO, (void*)pbuf))
	{
		pbuf_free(pbuf);
		return ERR_OVERFLOW;
	}

	return ERR_IN_PROGRESS;
}

#ifdef HT_DDL

static hterr nRF24L01NetIf_doTX(HtNetIf *netif, pbuf* packet, hwaddr_t destAddr)
{
	uint32_t destNodeId, tmp=0;
	uint8_t* buf = heap_malloc(nRF24L01_MAX_PLOAD);
	pbuf_read(packet, 0, buf, min(nRF24L01_MAX_PLOAD, packet->tot_len));
	destNodeId = *((uint32_t*) destAddr);
	if (0xE0 == (0xE000 & destNodeId))
		; // multicast
	
	tmp = nRF24L01_transmit(netif->pDriverCtx, destNodeId, tmp, buf);
	heap_free(buf);
	return tmp;
}

hterr nRF24L01NetIf_attach(nRF24L01* chip, HtNetIf* netif, const char* name)
{
	if (NULL == chip || NULL == netif)
		return ERR_INVALID_PARAMETER;

	HtNetIf_init(netif, chip, name, NULL,
				 nRF24L01_MAX_PLOAD, (uint8_t*) &chip->nodeId, 4, 0,
				 NetIf_OnReceived_Default, nRF24L01NetIf_doTX, NULL);
	chip->netif = netif;

	return ERR_SUCCESS;
}

void nRF24L01_OnReceived(nRF24L01* chip, uint8_t pipeId, uint8_t* payload, uint8_t payloadLen)
{
	pbuf* packet = pbuf_mmap(payload, payloadLen);
	if (NULL != chip && NULL != chip->netif && NULL != chip->netif->cbReceived && NULL != packet)
		chip->netif->cbReceived(chip->netif, packet, HTDDL_DEFAULT_POWERLEVEL);
	pbuf_free(packet);
}

#endif // HT_DDL
