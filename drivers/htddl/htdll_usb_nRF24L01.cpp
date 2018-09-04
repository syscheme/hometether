extern "C" {
#include <windows.h>
#include "../lwip/lwip_htddl.h"
}

#include "../../../../SDK/usb2ish/usbio.h"

// this program only support a single usb stick
uint8_t byDevIndex = 0xff; // initialize as not connected
eth_htddl_t* gEth = NULL;

typedef enum
{
	DEV_I2C = 0,
	DEV_SPI,
	DEV_HDQ,
	DEV_CAN,
	DEV_GPIO,
	DEV_ADC,
	DEV_PWM,
	DEV_TRIG = 13,
	DEV_ALL = 14,
	NUM_OF_DEV,
} enu_dev;

enu_dev devID = DEV_SPI;
uint32_t dwReadCnt;
uint8_t* pReadBuf;
uint32_t dwWriteCnt;
uint8_t* pWriteBuf;

bool bRunning;
void* pMyData;
uint8_t byDevAddr;
uint8_t byRateIndex;
uint32_t dwTimeout;
uint32_t reserved1;
uint8_t byRateIndexTrig;

static void usb_nRF24L01_close()
{
	if (byDevIndex == 0xFF)
		return;

	// sounds like currently connected, disconnect it first
	if (bRunning)
	{
		if (!USBIO_ExitTrig(byDevIndex))
		{
			LWIP_DEBUGF(NETIF_DEBUG, ("htddl_lwip_usb_nRF24L01_init: byDevIndex[%d] USBIO_ExitTrig err\n", byDevIndex));
			return;
		}

		//			TRIG_dev->bRunning = false;
		bRunning = false;
	}

	if (!USBIO_CloseDevice(byDevIndex))
	{
		LWIP_DEBUGF(NETIF_DEBUG, ("htddl_lwip_usb_nRF24L01_init: byDevIndex[%d] USBIO_CloseDevice err\n", byDevIndex));
		return;
	}

	byDevIndex = 0xFF;
	LWIP_DEBUGF(NETIF_DEBUG, ("htddl_lwip_usb_nRF24L01_init: byDevIndex[%d] closed\n", byDevIndex));
}

// an implementation of htddl_lwip_init_f
void htddl_lwip_usb_nRF24L01_init(eth_htddl_t* pEth)
{
	// step 1. determin the devIdx
	usb_nRF24L01_close();

	byDevIndex = USBIO_OpenDevice();
	if (byDevIndex == 0xFF)
	{
		LWIP_DEBUGF(NETIF_DEBUG, ("htddl_lwip_usb_nRF24L01_init: byDevIndex[%d] USBIO_OpenDevice err\n", byDevIndex));
		return;
	}

	LWIP_DEBUGF(NETIF_DEBUG, ("htddl_lwip_usb_nRF24L01_init: byDevIndex[%d] opened\n", byDevIndex));

	USBIO_SPIGetConfig(byDevIndex, &byRateIndex, (DWORD*) &dwTimeout);
#ifdef FM_TIRGER_FUNCTION
	USBIO_TrigGetConfig(byDevIndex, &byRateIndexTrig);
#endif

	gEth = pEth;
}

void htddl_lwip_usb_nRF24L01_read(const uint8_t* cmd, uint8_t cmdLen, uint8_t* buf, uint8_t buflen)
{
	if (!USBIO_SPIRead(byDevIndex, (LPVOID) cmd, cmdLen, buf, buflen))
	{
		LWIP_DEBUGF(NETIF_DEBUG, ("htddl_lwip_usb_nRF24L01_init: byDevIndex[%d] USBIO_SPIRead failed\n", byDevIndex));
		return;
	}
}

void htddl_lwip_usb_nRF24L01_write(const uint8_t* cmd, uint8_t cmdLen, const uint8_t* buf, uint8_t buflen)
{
	if (!USBIO_SPIWrite(byDevIndex, (LPVOID)cmd, cmdLen, (LPVOID)buf, buflen))
	{
		LWIP_DEBUGF(NETIF_DEBUG, ("htddl_lwip_usb_nRF24L01_init: byDevIndex[%d] USBIO_SPIWrite failed\n", byDevIndex));
		return;
	}
}
	
#ifndef __nRF24L01_H__
#define __nRF24L01_H__

typedef void*    PRF24L01SpiCtx;

// #define NRF2401_POLL

// ---------------------------------------------------------------------------
// definition of nRF24L01 chip
// ---------------------------------------------------------------------------
#define nRF24L01_ADDR_LEN   (5)
// #define nRF24L01_MAX_PLOAD  (32 -4 -nRF24L01_ADDR_LEN) // (32 -2 -nRF24L01_ADDR_LEN) max payload= 32byte - addrlen - 16bit crc
#define nRF24L01_MAX_PLOAD  16

#ifndef nRF_RX_CH
#  define nRF_RX_CH 5
#endif // nRF_RX_CH

typedef struct _nRF24L01
{
	uint32_t       nodeId;
	PRF24L01SpiCtx pCtx;
	//	uint8_t        flags;	  // set of nRF24L01_FLAG_xxxx

	// about the EXTI
	uint32_t     extiLine;

	uint32_t     peerNodeId;
	uint8_t      peerPortNum;
} nRF24L01;

// ---------------------------------------------------------------------------
// method declarations
// ---------------------------------------------------------------------------
// void nRF24L01_init(nRF24L01* chip, PRF24L01SpiCtx pCtx, const IO_PIN pinCE, const IO_PIN pinCSN, const uint8_t localAddr[nRF24L01_ADDR_LEN])
void nRF24L01_init(nRF24L01* chip, uint32_t nodeId, const uint8_t* txAddr, uint8_t rxmode);
// void nRF24L01_init(nRF24L01* chip, PRF24L01SpiCtx pCtx, const IO_PIN pinCE, const IO_PIN pinCSN, const uint8_t localAddr[nRF24L01_ADDR_LEN]);
void nRF24L01_setModeRX(nRF24L01* chip);
void nRF24L01_setModeTX(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum);
void nRF24L01_status(nRF24L01* chip);

void nRF24L01_TxPacket(nRF24L01* chip, uint8_t* bufTX);

// return the pipeId if receive successfully, otherwise 0xff as fail
uint8_t nRF24L01_RxPacket(nRF24L01* chip, uint8_t* bufRX);

extern void OnWirelessMsg(nRF24L01* chip, uint8_t pipeId, uint8_t* bufPlayLoad, uint8_t playLoadLen);

// entry for ExtI
void processIRQ_nRF24L01(nRF24L01* chip);

#endif // __nRF24L01_H__
#define DEBUG

#ifndef nRF_RX_CH
#  define nRF_RX_CH 5
#endif // nRF_RX_CH

static volatile bool bTxBusy = FALSE;

// Chip Enable Activates RX or TX mode
#define CE_H(_CHIP)	// pinH(_CHIP->pinCE) 
#define CE_L(_CHIP)	// pinL(_CHIP->pinCE) 

// SPI Chip Select
#define CSN_H(_CHIP) // pinH(_CHIP->pinCSN) 
#define CSN_L(_CHIP) // pinL(_CHIP->pinCSN) 

void nRF24L01_configPipes(nRF24L01* chip);

// ---------------------------------------------------------------------------
// definition of nRF24L01 instructions
// ---------------------------------------------------------------------------
#define nRFCmd_READ_REG     0x00  	// ¶Á¼Ä´æÆ÷Ö¸Áî
#define nRFCmd_WRITE_REG    0x20 	// Ð´¼Ä´æÆ÷Ö¸Áî
#define nRFCmd_RX_PLOAD     0x61  	// ¶ÁÈ¡½ÓÊÕÊý¾ÝÖ¸Áî
#define nRFCmd_TX_PLOAD     0xA0  	// Ð´´ý·¢Êý¾ÝÖ¸Áî
#define nRFCmd_FLUSH_TX     0xE1 	// ³åÏ´·¢ËÍ FIFOÖ¸Áî
#define nRFCmd_FLUSH_RX     0xE2  	// ³åÏ´½ÓÊÕ FIFOÖ¸Áî
#define nRFCmd_REUSE_TX_PL  0xE3  	// ¶¨ÒåÖØ¸´×°ÔØÊý¾ÝÖ¸Áî
#define nRFCmd_NOP          0xFF  	// ±£Áô

// ---------------------------------------------------------------------------
// definition of nRF24L01 register addresses
// ---------------------------------------------------------------------------
#define CONFIG          0x00  // ÅäÖÃÊÕ·¢×´Ì¬£¬CRCÐ£ÑéÄ£Ê½ÒÔ¼°ÊÕ·¢×´Ì¬ÏìÓ¦·½Ê½
#define EN_AA           0x01  // ×Ô¶¯Ó¦´ð¹¦ÄÜÉèÖÃ
#define EN_RXADDR       0x02  // ¿ÉÓÃÐÅµÀÉèÖÃ
#define SETUP_AW        0x03  // ÊÕ·¢µØÖ·¿í¶ÈÉèÖÃ
#define SETUP_RETR      0x04  // ×Ô¶¯ÖØ·¢¹¦ÄÜÉèÖÃ
#define RF_CH           0x05  // ¹¤×÷ÆµÂÊÉèÖÃ
#define RF_SETUP        0x06  // ·¢ÉäËÙÂÊ¡¢¹¦ºÄ¹¦ÄÜÉèÖÃ
#define STATUS          0x07  // ×´Ì¬¼Ä´æÆ÷
#define OBSERVE_TX      0x08  // ·¢ËÍ¼à²â¹¦ÄÜ
#define CD              0x09  // µØÖ·¼ì²â           
#define RX_ADDR_P0      0x0A  // ÆµµÀ0½ÓÊÕÊý¾ÝµØÖ·
#define RX_ADDR_P1      0x0B  // ÆµµÀ1½ÓÊÕÊý¾ÝµØÖ·
#define RX_ADDR_P2      0x0C  // ÆµµÀ2½ÓÊÕÊý¾ÝµØÖ·
#define RX_ADDR_P3      0x0D  // ÆµµÀ3½ÓÊÕÊý¾ÝµØÖ·
#define RX_ADDR_P4      0x0E  // ÆµµÀ4½ÓÊÕÊý¾ÝµØÖ·
#define RX_ADDR_P5      0x0F  // ÆµµÀ5½ÓÊÕÊý¾ÝµØÖ·
#define TX_ADDR         0x10  // ·¢ËÍµØÖ·¼Ä´æÆ÷
#define RX_PW_P0        0x11  // ½ÓÊÕÆµµÀ0½ÓÊÕÊý¾Ý³¤¶È
#define RX_PW_P1        0x12  // ½ÓÊÕÆµµÀ1½ÓÊÕÊý¾Ý³¤¶È
#define RX_PW_P2        0x13  // ½ÓÊÕÆµµÀ2½ÓÊÕÊý¾Ý³¤¶È
#define RX_PW_P3        0x14  // ½ÓÊÕÆµµÀ3½ÓÊÕÊý¾Ý³¤¶È
#define RX_PW_P4        0x15  // ½ÓÊÕÆµµÀ4½ÓÊÕÊý¾Ý³¤¶È
#define RX_PW_P5        0x16  // ½ÓÊÕÆµµÀ5½ÓÊÕÊý¾Ý³¤¶È
#define FIFO_STATUS     0x17  // FIFOÕ»ÈëÕ»³ö×´Ì¬¼Ä´æÆ÷ÉèÖÃ

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

void delayXusec(uint32_t n)
{
}

void nRF24L01_config(nRF24L01* chip, uint8_t config)
{
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + CONFIG, config);       //0x0E;
	delayXusec(1500);

	CE_H(chip);
}

// ---------------------------------------------------------------------------
// nRF24L01 initialization
// ---------------------------------------------------------------------------
void nRF24L01_init(nRF24L01* chip, uint32_t nodeId, const uint8_t* txAddr, uint8_t rxmode)
{
	uint8_t en_aa = FLG_EN_AA_ENAA_NONE;
	uint8_t config = FLG_CONFIG_EN_CRC | FLG_CONFIG_CRCO | FLG_CONFIG_PWR_UP;
	uint8_t buff[nRF24L01_ADDR_LEN] = { 0 };

	if (NULL == chip)
		return;

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
	nRF24L01_readBuff(chip, TX_ADDR, buff, nRF24L01_ADDR_LEN); //debug ²âÊÔÔ­À´µÄ±¾µØµØÖ·£º¸´Î»ÖµÊÇ£º0xE7 0xE7 0xE7 0xE7 0xE7
#endif // DEBUG

	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + SETUP_AW, FLG_SETUP_AW_5BYTES);	 // address-wide demo=FLG_SETUP_AW_5BYTES
	memcpy(buff, &chip->peerNodeId, sizeof(uint32_t));
	buff[4] = chip->peerPortNum;

	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG + TX_ADDR, buff, nRF24L01_ADDR_LEN);
	// the local address to receive auto ack from the transmittee
	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG + RX_ADDR_P0, buff, nRF24L01_ADDR_LEN);
#ifdef DEBUG
	nRF24L01_readBuff(chip, TX_ADDR, buff, nRF24L01_ADDR_LEN); //debug: test the tx address just written into
#endif // DEBUG

	// the rx addresses
	memcpy(buff, &chip->nodeId, sizeof(uint32_t));
	buff[4] = 0x00;

	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RX_PW_P0, nRF24L01_MAX_PLOAD);

#if nRF_RX_CH > 0
	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG + RX_ADDR_P1, buff, nRF24L01_ADDR_LEN);   // the local address to receive
	// the payload wide of the pipes, demo=0x00//ÉèÖÃ½ÓÊÕÊý¾Ý³¤¶È£¬±¾´ÎÉèÖÃÎª32×Ö½Ú
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RX_PW_P1, nRF24L01_MAX_PLOAD);
	en_aa |= FLG_EN_AA_ENAA_P1;
#endif // nRF_RX_CH

	buff[4]++;
#if nRF_RX_CH > 1
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RX_ADDR_P2, buff[4]);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RX_PW_P2, nRF24L01_MAX_PLOAD);
	en_aa |= FLG_EN_AA_ENAA_P2;
#endif // nRF_RX_CH

	buff[4]++;
#if nRF_RX_CH > 2
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RX_ADDR_P3, buff[4]);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RX_PW_P3, nRF24L01_MAX_PLOAD);
	en_aa |= FLG_EN_AA_ENAA_P3;
#endif // nRF_RX_CH

	buff[4]++;
#if nRF_RX_CH > 3
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RX_ADDR_P4, buff[4]);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RX_PW_P4, nRF24L01_MAX_PLOAD);
	en_aa |= FLG_EN_AA_ENAA_P4;
#endif // nRF_RX_CH

	buff[4]++;
#if nRF_RX_CH > 4
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RX_ADDR_P5, buff[4]);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RX_PW_P5, nRF24L01_MAX_PLOAD);
	en_aa |= FLG_EN_AA_ENAA_P5;
#endif // nRF_RX_CH

	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + EN_AA, en_aa);    //  ÆµµÀ0×Ô¶¯	ACKÓ¦´ðÔÊÐí	
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + EN_RXADDR, en_aa); // FLG_EN_RXADDR_ERX_NONE); // demo=0x03
	//nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +SETUP_RETR,FLG_SETUP_RETR_ARD_500 | FLG_SETUP_RETR_ARC_3);	 // demo=FLG_SETUP_RETR_ARC_3
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + SETUP_RETR, FLG_SETUP_RETR_ARD_500 | FLG_SETUP_RETR_ARC_10);	 // demo=FLG_SETUP_RETR_ARC_3
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RF_CH, 40);             //   ÉèÖÃÐÅµÀ¹¤×÷Îª2.4GHZ£¬ÊÕ·¢±ØÐëÒ»ÖÂ, demo=0x02
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + RF_SETUP, 0x0f);   		  //ÉèÖÃ·¢ÉäËÙÂÊÎª1MHZ£¬·¢Éä¹¦ÂÊÎª×î´óÖµ0dB, demo=0x0f

	nRF24L01_writeReg(chip, nRFCmd_FLUSH_TX, 0xff);
	nRF24L01_writeReg(chip, nRFCmd_FLUSH_RX, 0xff);
	nRF24L01_config(chip, config);
}

#ifndef SPI_GetFlagStatus
#  define SPI_GetFlagStatus      SPI_I2S_GetFlagStatus
#  define SPI_SendData           SPI_I2S_SendData
#  define SPI_ReceiveData        SPI_I2S_ReceiveData
#  define SPI_FLAG_RXNE          SPI_I2S_FLAG_RXNE
#  define SPI_FLAG_TXE           SPI_I2S_FLAG_TXE
#endif

// ---------------------------------------------------------------------------
// nRF24L01_readBuff read a buffer of data
// ---------------------------------------------------------------------------
uint8_t nRF24L01_readBuff(nRF24L01* chip, uint8_t reg, uint8_t *pBuf, uint8_t len)
{
	uint8_t status =0;

	CSN_L(chip);                   // CSN low, init SPI transaction
	htddl_lwip_usb_nRF24L01_read(&reg, 1, &status, 1);
	htddl_lwip_usb_nRF24L01_read(NULL, 0, pBuf, len);
	CSN_H(chip);                   // CSN high again

	return status;
}

// ---------------------------------------------------------------------------
// uint nRF24L01_writeBuff
// ¹¦ÄÜ: ÓÃÓÚÐ´Êý¾Ý£ºÎª¼Ä´æÆ÷µØÖ·£¬pBuf£ºÎª´ýÐ´ÈëÊý¾ÝµØÖ·£¬uchars£ºÐ´ÈëÊý¾ÝµÄ¸öÊý
// ---------------------------------------------------------------------------
uint8_t nRF24L01_writeBuff(nRF24L01* chip, uint8_t reg, const uint8_t *pBuf, uint8_t len)
{
	uint8_t status;

	CSN_L(chip);                   // CSN low, init SPI transaction
	htddl_lwip_usb_nRF24L01_read(&reg, 1, &status, 1);
	htddl_lwip_usb_nRF24L01_write(NULL, 0, pBuf, len);

	CSN_H(chip);                   // CSN high again
	return status;
}

// ---------------------------------------------------------------------------
/// º¯Êý£ºuchar nRF24L01_readReg(uchar reg)
/// ¹¦ÄÜ£ºNRF24L01µÄSPIÊ±Ðò
// ---------------------------------------------------------------------------
uint8_t nRF24L01_readReg(nRF24L01* chip, uint8_t reg)
{
	uint8_t val = 0;
	nRF24L01_readBuff(chip, reg, &val, sizeof(val));
	return val;        // return register value
}

#define DEBUG


// ---------------------------------------------------------------------------
// nRF24L01_writeReg
// ¹¦ÄÜ£ºNRF24L01¶ÁÐ´¼Ä´æÆ÷º¯Êý
// ---------------------------------------------------------------------------
uint8_t nRF24L01_writeReg(nRF24L01* chip, uint8_t reg, uint8_t value)
{
	return nRF24L01_writeBuff(chip, reg, &value, 1);   // return nRF24L01 status
}

// ---------------------------------------------------------------------------
// º¯Êý£ºvoid SetRX_Mode(void)
// ¹¦ÄÜ£ºÊý¾Ý½ÓÊÕÅäÖÃ 
// ---------------------------------------------------------------------------
void nRF24L01_setModeRX(nRF24L01* chip)
{
	uint8_t config = nRF24L01_readReg(chip, CONFIG);

	if ((config & FLG_CONFIG_PRIM_RX) != 0)
		return; // already at the RX mode, quit

	CE_L(chip);  // chip enable
	//	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +RX_ADDR_P0, chip->txOrAckAddr, nRF24L01_ADDR_LEN); // the local address to receive auto ack
	nRF24L01_writeReg(chip, nRFCmd_FLUSH_RX, 0xff);
	config |= (FLG_CONFIG_PRIM_RX | FLG_CONFIG_PWR_UP);

	nRF24L01_config(chip, config);
}

// ---------------------------------------------------------------------------
void nRF24L01_setModeTX(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum)
{
	uint8_t txAddr[nRF24L01_ADDR_LEN] = { 0 };
	uint8_t config = nRF24L01_readReg(chip, CONFIG);

	if ((config & FLG_CONFIG_PRIM_RX) == 0)
	{
		if (peerNodeId == chip->peerNodeId && peerPortNum == chip->peerPortNum)
			return; // already at the TX mode to the same dest, quit
	}

	chip->peerNodeId = peerNodeId; chip->peerPortNum = peerPortNum;

	memcpy(txAddr, &chip->peerNodeId, sizeof(uint32_t));
	txAddr[4] = chip->peerPortNum;

	CE_L(chip);  // chip enable
	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG + TX_ADDR, txAddr, nRF24L01_ADDR_LEN); // ×°ÔØ½ÓÊÕ¶ËµØÖ·
	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG + RX_ADDR_P0, txAddr, nRF24L01_ADDR_LEN); // ×°ÔØ½ÓÊÕ¶ËµØÖ·

	nRF24L01_writeReg(chip, nRFCmd_FLUSH_TX, 0xff);
	config &= (~FLG_CONFIG_PRIM_RX);
	nRF24L01_config(chip, config);
}


// ---------------------------------------------------------------------------
// º¯Êý£ºvoid nRF24L01_TxPacket(uint8_t * bufTX)
// ¹¦ÄÜ£º·¢ËÍ tx_bufÖÐÊý¾Ý
// ---------------------------------------------------------------------------
void nRF24L01_TxPacket(nRF24L01* chip, uint8_t* bufTX)
{
	int i = 0;
	uint8_t status = 0;
	while (bTxBusy) // yield if Tx busy
	{
		delayXusec(10);

		if (bTxBusy)
		{   // quit if the TX fifo gets empty
			i = nRF24L01_readReg(chip, STATUS);
			status = nRF24L01_readReg(chip, FIFO_STATUS);
			if (i & FLG_STATUS_TX_DS || status & FLG_FIFO_STATUS_TX_EMPTY)
				bTxBusy = FALSE;
		}
	}

	bTxBusy = TRUE;

	CE_L(chip);	 //StandBy IÄ£Ê½	
	//	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +TX_ADDR,    chip->txOrAckAddr,   nRF24L01_ADDR_LEN); // ×°ÔØ½ÓÊÕ¶ËµØÖ·
	//	nRF24L01_writeBuff(chip, nRFCmd_WRITE_REG +RX_ADDR_P0, chip->txOrAckAddr,   nRF24L01_ADDR_LEN); // ×°ÔØ½ÓÊÕ¶ËµØÖ·
	nRF24L01_writeBuff(chip, nRFCmd_TX_PLOAD, bufTX, nRF24L01_MAX_PLOAD);  // ×°ÔØÊý¾Ý	

	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + STATUS, nRF24L01_readReg(chip, STATUS) | 0x70);  // clear the pending interrupts
	//	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, 0xff);  // clear the pending interrupts

	CE_H(chip);  //ÖÃ¸ßCE£¬¼¤·¢Êý¾Ý·¢ËÍ
	delayXusec(500);

	status = 0x33;
	for (i = 999999; bTxBusy && i>0; i--)
	{
		status = nRF24L01_readReg(chip, STATUS);
		if (0 == (status & (FLG_STATUS_TX_DS | FLG_STATUS_MAX_RT)))
		{
			delayXusec(10);
			continue;
		}

		if (status & FLG_STATUS_MAX_RT)
			nRF24L01_cmd(chip, nRFCmd_FLUSH_TX);

		bTxBusy = FALSE;
	}

	//	status = nRF24L01_readReg(chip, STATUS);
	//	status |= (FLG_STATUS_TX_DS | FLG_STATUS_MAX_RT);
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + STATUS, FLG_STATUS_TX_DS | FLG_STATUS_MAX_RT);
}

// ---------------------------------------------------------------------------
// º¯Êý£ºuint8_t nRF24L01_RxPacket(uint8_t* bufRX)
// ¹¦ÄÜ£ºÊý¾Ý¶ÁÈ¡ºó·ÅÈçrx_buf½ÓÊÕ»º³åÇøÖÐ
// ---------------------------------------------------------------------------
// return the pipeId if receive successfully, otherwise 0xff as fail;
uint8_t nRF24L01_RxPacket(nRF24L01* chip, uint8_t* bufRX)
{
	uint8_t status, pipeId = 0xff;

	status = nRF24L01_readReg(chip, STATUS);	// ¶ÁÈ¡×´Ì¬¼Ä´æÆäÀ´ÅÐ¶ÏÊý¾Ý½ÓÊÕ×´¿ö

	//	if (0 == (status & FLG_STATUS_RX_DR))				// ÅÐ¶ÏÊÇ·ñ½ÓÊÕµ½Êý¾Ý
	//		return 0xff;

	pipeId = (status >> 1) & 0x07;
	// CE_L();
	//	nRF24L01_readBuff(chip, nRFCmd_RX_PLOAD, bufRX, nRF24L01_PLOAD_LEN);// read receive payload from RX_FIFO buffer
	nRF24L01_readBuff(chip, nRFCmd_RX_PLOAD, bufRX, nRF24L01_MAX_PLOAD);// read receive payload from RX_FIFO buffer

	// CE_H();

	// nRF24L01_writeReg(chip, nRFCmd_WRITE_REG +STATUS, status);   // Í¨¹ýÐ´1À´Çå³þÖÐ¶Ï±êÖ¾
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + STATUS, FLG_STATUS_RX_DR);   // Í¨¹ýÐ´1À´Çå³þÖÐ¶Ï±êÖ¾
	// the IRQ pin : GPIO_SetBits(GPIOA, GPIO_Pin_8);
	return pipeId;
}

void processIRQ_nRF24L01(nRF24L01* chip)
{
	uint8_t status = 0, fifostatus = 0;
	uint8_t pipeId = 0xff;
	uint8_t recvBuf_nRF24L01[nRF24L01_MAX_PLOAD + 2];

	CE_L(chip);

	//	CSN_L(chip);
	status = nRF24L01_readReg(chip, STATUS);     // read the current status byte
	fifostatus = nRF24L01_readReg(chip, FIFO_STATUS);     // read the current fifo status byte
	//	CSN_H(chip);

	if (status & FLG_STATUS_TX_DS || fifostatus & FLG_FIFO_STATUS_TX_EMPTY)
	{
		// transfer completed, do nothing but clear bTxBusy and status registry
		bTxBusy = FALSE;
	}

	if (status & FLG_STATUS_MAX_RT)
	{
		// max re-transfer reached, timeout at no ackownledge
		//		CSN_L(chip);
		nRF24L01_cmd(chip, nRFCmd_FLUSH_TX);  // clear TX FIFO, resend after clear the interrupt
		//		CSN_H(chip);
		bTxBusy = FALSE;
	}

	if (status & FLG_STATUS_RX_DR)
	{
		while (0 == (fifostatus & FLG_FIFO_STATUS_RX_EMPTY))
		{
			// received a message here
			pipeId = (status >> 1) & 0x7;
			// TODO: query for payload value: status = nrf24l01_execute_command(nrf24l01_R_RX_PAYLOAD, data, len, true);
			nRF24L01_readBuff(chip, nRFCmd_RX_PLOAD, recvBuf_nRF24L01, nRF24L01_MAX_PLOAD);  // read from the RX FIFO
			if (pipeId >5)
				break;

			recvBuf_nRF24L01[nRF24L01_MAX_PLOAD] = '\0';
			OnWirelessMsg(chip, pipeId, recvBuf_nRF24L01, nRF24L01_MAX_PLOAD);
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
	nRF24L01_writeReg(chip, nRFCmd_WRITE_REG + STATUS, status);
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
}

void OnWirelessMsg(nRF24L01* chip, uint8_t pipeId, uint8_t* bufPlayLoad, uint8_t playLoadLen)
{
	htddl_procRX(gEth, bufPlayLoad, playLoadLen);
}