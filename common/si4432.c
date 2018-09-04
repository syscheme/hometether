#include "htcomm.h"
#include "si4432.h"

uint8_t rf_ch;
uint8_t rf_pwr;
uint8_t rf_dr; 
uint8_t mode = 0;   //普通模式

#define SI4432_SYNC_DWORD  (0x558fa224)
#define SI4432_SYNC_LEN    (2)


//TODO
#define SI4432_IRQ()  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)	//(P2IN & BIT7)  //PB1
//

typedef enum _SIREGS //revV2
{
	SIREG_DeviceType 								= 0x00,
	SIREG_DeviceVersion							= 0x01,
	SIREG_DeviceStatus 							= 0x02,
	SIREG_InterruptStatus1 						= 0x03,
	SIREG_InterruptStatus2 						= 0x04,
	SIREG_InterruptEnable1 						= 0x05,          
	SIREG_InterruptEnable2 						= 0x06,         
	SIREG_OperatingFunctionControl1 				= 0x07,
	SIREG_OperatingFunctionControl2 				= 0x08,
	SIREG_CrystalOscillatorLoadCapacitance 		= 0x09,
	SIREG_MicrocontrollerOutputClock 				= 0x0A,
	SIREG_GPIO0Configuration 						= 0x0B,
	SIREG_GPIO1Configuration 						= 0x0C,         
	SIREG_GPIO2Configuration						= 0x0D,
	SIREG_IOPortConfiguration						= 0x0E,
	SIREG_ADCConfiguration						= 0x0F,
	SIREG_ADCSensorAmplifierOffset				= 0x10,
	SIREG_ADCValue								= 0x11,
	SIREG_TemperatureSensorControl				= 0x12,
	SIREG_TemperatureValueOffset					= 0x13,
	SIREG_WakeUpTimerPeriod1 						= 0x14,          
	SIREG_WakeUpTimerPeriod2 						= 0x15,         
	SIREG_WakeUpTimerPeriod3 						= 0x16,         
	SIREG_WakeUpTimerValue1						= 0x17,
	SIREG_WakeUpTimerValue2						= 0x18,
	SIREG_LowDutyCycleModeDuration 				= 0x19,       
	SIREG_LowBatteryDetectorThreshold  			= 0x1A,
	SIREG_BatteryVoltageLevel 					= 0x1B,                          
	SIREG_IFFilterBandwidth  						= 0x1C,                           
	SIREG_AFCLoopGearshiftOverride				= 0x1D,
	SIREG_AFCTimingControl 						= 0x1E,                              
	SIREG_ClockRecoveryGearshiftOverride 			= 0x1F,              
	SIREG_ClockRecoveryOversamplingRatio 			= 0x20,              
	SIREG_ClockRecoveryOffset2 					= 0x21,                       
	SIREG_ClockRecoveryOffset1 					= 0x22,                       
	SIREG_ClockRecoveryOffset0 					= 0x23,                     
	SIREG_ClockRecoveryTimingLoopGain1 			= 0x24,              
	SIREG_ClockRecoveryTimingLoopGain0 			= 0x25,             
	SIREG_ReceivedSignalStrengthIndicator 		= 0x26,          
	SIREG_RSSIThresholdForClearChannelIndicator 	= 0x27,   
	SIREG_AntennaDiversityRegister1				= 0x28,
	SIREG_AntennaDiversityRegister2				= 0x29,
	SIREG_DataAccessControl 						= 0x30,                          
	SIREG_EZmacStatus 							= 0x31,                                  
	SIREG_HeaderControl1 							= 0x32,                               
	SIREG_HeaderControl2 							= 0x33,                              
	SIREG_PreambleLength 							= 0x34,                               
	SIREG_PreambleDetectionControl 				= 0x35,                    
	SIREG_SyncWord3 								= 0x36,                                   
	SIREG_SyncWord2 								= 0x37,                                   
	SIREG_SyncWord1 								= 0x38,                               
	SIREG_SyncWord0 								= 0x39,                                
	SIREG_TransmitHeader3							= 0x3A,                       
	SIREG_TransmitHeader2 						= 0x3B,                             
	SIREG_TransmitHeader1 						= 0x3C,                              
	SIREG_TransmitHeader0 						= 0x3D,                             
	SIREG_TransmitPacketLength 					= 0x3E,                         
	SIREG_CheckHeader3 							= 0x3F,                                
	SIREG_CheckHeader2 							= 0x40,                              
	SIREG_CheckHeader1 							= 0x41,                             
	SIREG_CheckHeader0 							= 0x42,                            
	SIREG_HeaderEnable3 							= 0x43,                               
	SIREG_HeaderEnable2 							= 0x44,                                 
	SIREG_HeaderEnable1 							= 0x45,                                
	SIREG_HeaderEnable0 							= 0x46,                              
	SIREG_ReceivedHeader3 						= 0x47,                          
	SIREG_ReceivedHeader2 						= 0x48,                         
	SIREG_ReceivedHeader1 						= 0x49,                           
	SIREG_ReceivedHeader0 						= 0x4A,                             
	SIREG_ReceivedPacketLength					= 0x4B,
	SIREG_AnalogTestBus 							= 0x50,                              
	SIREG_DigitalTestBus 							= 0x51,                          
	SIREG_TXRampControl 							= 0x52,                             
	SIREG_PLLTuneTime 							= 0x53,                            
	SIREG_CalibrationControl 						= 0x55,                     
	SIREG_ModemTest 								= 0x56,                               
	SIREG_ChargepumpTest 							= 0x57,                    
	SIREG_ChargepumpCurrentTrimming_Override 		= 0x58,         
	SIREG_DividerCurrentTrimming				 	= 0x59,    
	SIREG_VCOCurrentTrimming 						= 0x5A,                           
	SIREG_VCOCalibration_Override 				= 0x5B,                    
	SIREG_SynthesizerTest 						= 0x5C,                              
	SIREG_BlockEnableOverride1 					= 0x5D,                        
	SIREG_BlockEnableOverride2 					= 0x5E,                      
	SIREG_BlockEnableOverride3 					= 0x5F,                       
	SIREG_ChannelFilterCoefficientAddress 		= 0x60,             
	SIREG_ChannelFilterCoefficientValue 			= 0x61,            
	SIREG_CrystalOscillator_ControlTest 			= 0x62,               
	SIREG_RCOscillatorCoarseCalibration_Override 	= 0x63,    
	SIREG_RCOscillatorFineCalibration_Override 	= 0x64,      
	SIREG_LDOControlOverride 						= 0x65,                          
	SIREG_DeltasigmaADCTuning1			 		= 0x67,
	SIREG_DeltasigmaADCTuning2			 		= 0x68,
	SIREG_AGCOverride1					 		= 0x69,
	SIREG_AGCOverride2 							= 0x6A,
	SIREG_GFSKFIRFilterCoefficientAddress 		= 0x6B,            
	SIREG_GFSKFIRFilterCoefficientValue 			= 0x6C,              
	SIREG_TXPower 								= 0x6D,                                   
	SIREG_TXDataRate1 							= 0x6E,                            
	SIREG_TXDataRate0 							= 0x6F,                              
	SIREG_ModulationModeControl1 					= 0x70,                   
	SIREG_ModulationModeControl2 					= 0x71,                   
	SIREG_FrequencyDeviation 						= 0x72,                            
	SIREG_FrequencyOffset 						= 0x73,                            
	SIREG_FrequencyChannelControl					= 0x74,
	SIREG_FrequencyBandSelect 					= 0x75,                        
	SIREG_NominalCarrierFrequency1	 			= 0x76,                    
	SIREG_NominalCarrierFrequency0 				= 0x77,                    
	SIREG_FrequencyHoppingChannelSelect 			= 0x79,               
	SIREG_FrequencyHoppingStepSize 				= 0x7A,                     
	SIREG_TXFIFOControl1 							= 0x7C,                        
	SIREG_TXFIFOControl2 							= 0x7D,    
	SIREG_RXFIFOControl 							= 0x7E,                               
	SIREG_FIFOAccess								= 0x7F, 
} SIREGS;

// SIREG_OperatingFunctionControl1
#define OPERA_OpenXtal       (1<<0)
#define OPERA_TuneMode       (1<<1)
#define OPERA_OpenRX         (1<<2)
#define OPERA_OpenTX         (1<<3)
#define OPERA_SelectX32K     (1<<4)
#define OPERA_EnableWakeUp   (1<<5)
#define OPERA_EnableLBD      (1<<6)
#define OPERA_SoftReset      (1<<7)

//==============================================================
// RF specific definitions
//==============================================================

#ifdef  BAND_434
#define FW_VERSION "1.0 434MHz"
//define the default radio frequency
#define FREQ_BAND_SELECT  0x53							//frequency band select
#define NOMINAL_CAR_FREQ1 0x53							//default carrier frequency
#define NOMINAL_CAR_FREQ0 0x40

#endif

#ifdef  BAND_868
#define FW_VERSION "1.0 868MHz"
//define the default radio ferquency
#define FREQ_BAND_SELECT 0x73							//frequency band select
#define NOMINAL_CAR_FREQ1 0x2C							//default carrier frequency
#define NOMINAL_CAR_FREQ0 0x60

#endif

#ifdef  BAND_915
#define FW_VERSION "1.0 915MHz"
//define the default radio ferquency
#define FREQ_BAND_SELECT 0x75							//frequency band select
#define NOMINAL_CAR_FREQ1 0x0A							//default carrier frequency
#define NOMINAL_CAR_FREQ0 0x80

#endif

/*------------------------------------------------------------------------*/
/*						GLOBAL variables								  */
/*------------------------------------------------------------------------*/


// Chip Enable Activates RX or TX mode
#define SDN_H(_CHIP)	pinSET(_CHIP->pinSDN,1) 
#define SDN_L(_CHIP)	pinSET(_CHIP->pinSDN,0) 

// SPI Chip Select
#define CSN_H(_CHIP)  pinSET(_CHIP->pinCSN,1) 
#define CSN_L(_CHIP)  pinSET(_CHIP->pinCSN,0)

static uint8_t SI4432_RW(SI4432* chip, uint8_t addr, uint8_t data)
{
	CSN_L(chip);
	SPI_RW(chip->spi, addr);
	data = SPI_RW(chip->spi, data);
	CSN_H(chip);
	return data;
}

#define SI4432_writeReg(chip, addr, data)    SI4432_RW(chip, (0x80 | addr), data)
#define SI4432_readReg(chip, addr)           SI4432_RW(chip, addr, 0xff)

static void SI4432_readITandClear(SI4432* chip)
{
	chip->ITstatus = SI4432_readReg(chip, SIREG_InterruptStatus2);
	chip->ITstatus <<=8;
	chip->ITstatus |= SI4432_readReg(chip, SIREG_InterruptStatus1);
}

// ---------------------------------------------------------------------------
// SI4432 initialization
// ---------------------------------------------------------------------------
hterr SI4432_init(SI4432* chip) //, uint32_t nodeId, const uint8_t* txAddr, uint8_t rxmode)
{
	if (FIFO_init(&chip->txFIFO, SI4432_PENDING_TX_MAX, sizeof(pbuf*), 0) <=0)
		return ERR_INVALID_PARAMETER;

	return SI4432_reset(chip);
}

// ---------------------------------------------------------------------------
// SI4432 reset
// ---------------------------------------------------------------------------
hterr SI4432_reset(SI4432* chip)
{
	uint8_t i =0;
	pbuf* packet =NULL;

	// clean the txFIFO
	while (ERR_SUCCESS == FIFO_pop(&chip->txFIFO, &packet))
	{
		pbuf_free(packet);
		packet = NULL;
	}

	chip->stampLastIO =0;
	chip->ITstatus = chip->OPstatus =0;

	do {
		CSN_H(chip);
		SDN_H(chip); delayXmsec(200);

		// step 1 turn on the radio by pulling down the PWRDN pin
		SDN_L(chip);

		// step 1.1 wait for the power-on reset sequence
		delayXmsec(30); // at least 15msec before any initialization SPI commands are set to the radio

		// step 2. read the interrupt status to clear the interrupt flags, so that release IRQ pin
		SI4432_readITandClear(chip);

		// step 2.1 disable unnecessary interrupts
		// SI4432_writeReg(chip, SIREG_InterruptEnable2, 0x00);

		// step 3. software reset the chip -> wait for POR interrupt
		chip->ITstatus = 0x0000;
		SI4432_writeReg(chip, SIREG_OperatingFunctionControl1, OPERA_SoftReset); 

		// step 3.1 wait for POR interrupt after the soft reset from the radio (while the nIRQ pin is high)
		for (i =5; i>0 ; i--)
		{
			if (chip->ITstatus & (ITSTATUS_POR|ITSTATUS_CHIPRDY))
				break;
			else delayXmsec(1);
		}
	} while (0 == (chip->ITstatus & (ITSTATUS_POR|ITSTATUS_CHIPRDY)));

	// step 4. read the interrupt status to clear the interrupt flags, so that release IRQ pin
	SI4432_readITandClear(chip);

	// step 4.1 wait for chip ready interrupt from the radio (while the nIRQ pin is high)
	//	while (SI4432_IRQ());

	// step 5. disable unnecessary interrupts, except 'ichiprdy'
	SI4432_writeReg(chip, SIREG_InterruptEnable1, 0x00);
	SI4432_writeReg(chip, SIREG_InterruptEnable2, 0x02);

	// step 5.1 clean interrupts again
	SI4432_readITandClear(chip);

	// step 6. set VCO and PLL, SI4432 revision V2 must be the following:
	SI4432_writeReg(chip, SIREG_VCOCurrentTrimming,     0x7F);
	SI4432_writeReg(chip, SIREG_ChargepumpCurrentTrimming_Override, 0x80);
	SI4432_writeReg(chip, SIREG_DividerCurrentTrimming, 0x40); // 0XC0;
	// step 6.1 SI4432 revision V2 to get better receive performance
	SI4432_writeReg(chip, SIREG_AGCOverride2,           0x0b);
	SI4432_writeReg(chip, SIREG_DeltasigmaADCTuning2,   0x04);
	SI4432_writeReg(chip, SIREG_ClockRecoveryGearshiftOverride,     0x03);

	// step 6.2 set crystal osc load cap register
	SI4432_writeReg(chip, SIREG_CrystalOscillatorLoadCapacitance, 0xD7);

	// ----------------
	// UNSURE SETTINGS
	//reset digital testbus, disable scan test
	SI4432_writeReg(chip, SIREG_DigitalTestBus, 0x00);      
	//select nothing to the Analog Testbus
	SI4432_writeReg(chip, SIREG_AnalogTestBus, 0x0B);

	SI4432_writeReg(chip, SIREG_ClockRecoveryGearshiftOverride, 0x03);
	SI4432_writeReg(chip, SIREG_AFCLoopGearshiftOverride, 0x40);
	// END OF UNSURE SETTING
	// ----------------

	// step 7. set PHY parameters

	// 7.1 frequency: SIREG_FrequencyBandSelect, SIREG_NominalCarrierFrequency1, SIREG_NominalCarrierFrequency0
	SI4432_writeReg(chip, SIREG_FrequencyBandSelect,      FREQ_BAND_SELECT);                  
	SI4432_writeReg(chip, SIREG_NominalCarrierFrequency1, NOMINAL_CAR_FREQ1);
	SI4432_writeReg(chip, SIREG_NominalCarrierFrequency0, NOMINAL_CAR_FREQ0);  

	// 7.2 packet configuration
	// 7.2.1 set preamble length & detection threshold
	SI4432_writeReg(chip, SIREG_PreambleLength,           (SI4432_PREAMBLE_LENGTH <<1));		
	SI4432_writeReg(chip, SIREG_PreambleDetectionControl, (SI4432_PD_LENGTH <<4));

	// 7.2.2 enable the two-byte sync word = 0x2DD4 
	SI4432_writeReg(chip, SIREG_HeaderControl1, 0x00);            
	SI4432_writeReg(chip, SIREG_HeaderControl2, SI4432_SYNC_LEN); // length of sync word = 2bytes  
	SI4432_writeReg(chip, SIREG_SyncWord3, (uint8_t)SI4432_SYNC_DWORD);
	SI4432_writeReg(chip, SIREG_SyncWord2, (uint8_t)(SI4432_SYNC_DWORD>>8));
	SI4432_writeReg(chip, SIREG_SyncWord1, (uint8_t)(SI4432_SYNC_DWORD>>16));
	SI4432_writeReg(chip, SIREG_SyncWord0, (uint8_t)(SI4432_SYNC_DWORD>>24));

	// 7.2.3 enable CRC16
	SI4432_writeReg(chip, SIREG_DataAccessControl, 0x8D);  // 0x0D, 0x85 in pdf sample

	// 7.3 enable FIFO mode and GFSK modulation, MCU only needs to fill the payload data
	//  by letting SI4432 to assemble the packet
	SI4432_writeReg(chip, SIREG_ModulationModeControl2, 0x63);

	// 7.4 GPIOs
	// 7.4.1 set GPIO0 to GND
	SI4432_writeReg(chip, SIREG_GPIO0Configuration, 0x14);  //接收数据

	// 7.4.2 set GPIO1 & GPIO2 to control the TX-RX switch
	SI4432_writeReg(chip, SIREG_GPIO1Configuration, 0x12);  //发送状态
	SI4432_writeReg(chip, SIREG_GPIO2Configuration, 0x15);  //接收状态

	rf_pwr = 0x1F;							//default output power	

	rf_ch = 0;							//default frequency channel
	SI4432_setFrequence(chip, rf_ch);
	SI4432_setPower(chip, rf_pwr);

#ifndef SI4432_COMAPTIBLE_MODE
	SI4432_setBaud(chip, (SIBAUDID)2);	// take 9.6 kbps
#else // SI4432_COMAPTIBLE_MODE
	SI4432_setBaud(chip, (SIBAUDID)0); // take 9.6kbps
#endif // SI4432_COMAPTIBLE_MODE

	return ERR_SUCCESS;
}

static void SI4432_doTX(SI4432* chip, pbuf* packet)
{
	pbuf_size_t nleft, tmp;
	uint8_t* payload = NULL;
	pbuf* pkt = packet;

	nleft = pkt->tot_len;
	if (nleft > SI4432_MAX_PAYLOAD)
		nleft = SI4432_MAX_PAYLOAD;

	chip->OPstatus |= SI4432_OPFLAG_SEND_IN_PROGRESS;

	// step 1. set the length of packet
	SI4432_writeReg(chip, SIREG_TransmitPacketLength, nleft &0xff);					

	// step 2. fill the payload into FIFO
	for(;pkt && nleft >0; pkt= pkt->next)
	{
		for(tmp = min(nleft, pkt->len), payload=pkt->payload; payload && tmp >0; tmp--, nleft--)
			SI4432_writeReg(chip, SIREG_FIFOAccess, *payload++);
	}

	// step 3. enable the packaet set interrupt only but disable all others
	SI4432_writeReg(chip, SIREG_InterruptEnable1, 0x04);
	SI4432_writeReg(chip, SIREG_InterruptEnable2, 0x00);

	// step 5. kick off sending
	chip->ITstatus &= ~((uint16_t) ITSTATUS_PK_SENT);
	SI4432_writeReg(chip, SIREG_OperatingFunctionControl1, OPERA_OpenTX | OPERA_OpenXtal); // 0x09);

	SI4432_OnSent(chip, packet);
}

void SI4432_flushTxFront(SI4432* chip);

// ---------------------------------------------------------------------------
// SI4432_transmit
// ---------------------------------------------------------------------------
// transmit a packet
hterr SI4432_transmit(SI4432* chip, pbuf* packet)
{
	hterr ret = ERR_INVALID_PARAMETER;
	if (NULL == chip || NULL ==packet)
		return ERR_INVALID_PARAMETER;

	if (0 == (chip->OPstatus & SI4432_OPFLAG_SEND_IN_PROGRESS))
	{
		// the queue seems empty, send this immediately
		SI4432_doTX(chip, packet);
		return ERR_SUCCESS;
	}

	// queue the package into txFIFO
	pbuf_ref(packet);
	ret = FIFO_push(&chip->txFIFO, &packet);

	if (ERR_SUCCESS == ret)
		ret = ERR_IN_PROGRESS;
	else  // failed to queue the out-going packet, free the recent dupdate
		pbuf_free(packet);

	// the stupid Si4432 sometime missed interrupt of sent, do a poll to kick off
	if (FIFO_awaitSize(&chip->txFIFO) >1)
	{
		SI4432_readITandClear(chip);
		if (0 == (ITSTATUS_TX_FULL & chip->ITstatus))
			SI4432_flushTxFront(chip);
	}

	return ret;
}

//receive mode
hterr SI4432_setMode_RX(SI4432* chip)
{
	// step 1. enable receiver chain
	SI4432_writeReg(chip, SIREG_OperatingFunctionControl1, OPERA_OpenRX | OPERA_OpenXtal);

	// step 2. enable the two ITs: a) valid packet received: ipkval, b) crc-err
	SI4432_writeReg(chip, SIREG_InterruptEnable1, 0x03); 
	SI4432_writeReg(chip, SIREG_InterruptEnable2, 0x00); 

	// step 3. clear the interrupt flags, so that release IRQ pin
	SI4432_readITandClear(chip);

	return ERR_SUCCESS;
}

// idle mode
hterr SI4432_setMode_IDLE(SI4432* chip)
{
	SI4432_writeReg(chip, SIREG_OperatingFunctionControl1, OPERA_OpenXtal); // 0x01

	//diasble all ITs
	SI4432_writeReg(chip, SIREG_InterruptEnable1, 0x00);
	SI4432_writeReg(chip, SIREG_InterruptEnable2, 0x00);

	SI4432_readITandClear(chip);

	return ERR_SUCCESS;
}

void SI4432_flushTxFront(SI4432* chip)
{
	pbuf* packet = NULL;
	chip->OPstatus &= ~SI4432_OPFLAG_SEND_IN_PROGRESS; // reset sending flag

	if (ERR_SUCCESS == FIFO_pop(&chip->txFIFO, &packet) && packet)
	{
		SI4432_doTX(chip, packet);
		pbuf_free(packet);
	}
	packet = NULL;
}

// ---------------------------------------------------------------------------
// SI4432_doISR
// ---------------------------------------------------------------------------
// IRQ handler
void SI4432_doISR(SI4432* chip)
{
	pbuf* packet = NULL, *p;
	uint8_t nbytes, tmp;
	uint8_t* payload = NULL;

	SI4432_readITandClear(chip);

	// process the IT bits

	// case 1. CRC error
	if (ITSTATUS_CRC_ERR & chip->ITstatus)
	{
		// 1.1 disable the receiver chain
		SI4432_writeReg(chip, SIREG_OperatingFunctionControl1, OPERA_OpenXtal); // 0x01); 

		// 1.2 reset the RX FIFO
		SI4432_writeReg(chip, SIREG_OperatingFunctionControl2, 0x02);
		SI4432_writeReg(chip, SIREG_OperatingFunctionControl2, 0x00);
	}

	// case 2. packet received
	if (ITSTATUS_PK_VALID & chip->ITstatus)
	{
		chip->stampLastIO = gClock_msec; // refresh the stamp for a valid receive

		// 2.1 disable the receiver chain
		SI4432_writeReg(chip, SIREG_OperatingFunctionControl1, OPERA_OpenXtal); // 0x01); 

		// 2.2 read the length of the received packet payload
		nbytes = SI4432_readReg(chip, SIREG_ReceivedPacketLength);

		// 2.3 allocate a packet to receive the data
		if (nbytes <=SI4432_MAX_PAYLOAD)
			packet = pbuf_malloc(nbytes);

		// 2.4 read the payload data
		for(p = packet; p && nbytes >0; p= p->next)
		{
			for(tmp = min(nbytes, p->len), payload=p->payload; payload && tmp >0; tmp--, nbytes--)
				*payload++ = SI4432_readReg(chip, SIREG_FIFOAccess);
		}

		// 2.4.1 read and empty the data left in receive FIFO	if there are any
		while (nbytes-- >0 && 0== (nbytes&0x80))
			tmp = SI4432_readReg(chip, SIREG_FIFOAccess);

		if (NULL != packet)
		{
			// 2.5 callback SI4432_OnPacket() 
			SI4432_OnReceived(chip, packet);

			// 2.6 free the packet
			pbuf_free(packet);
		}

		packet =NULL;

		// 2.7?? reset the RX FIFO
		// SI4432_writeReg(chip, SIREG_OperatingFunctionControl2, 0x02);
		// SI4432_writeReg(chip, SIREG_OperatingFunctionControl2, 0x00);

		// 2.8 enable receive again
		// SI4432_writeReg(chip, SIREG_OperatingFunctionControl1, OPERA_OpenRX | OPERA_OpenXtal); // 0x05
	}

	// case 2. packet sent
	if ((ITSTATUS_PK_SENT | ITSTATUS_TX_EMPTY) & chip->ITstatus)
	{
		chip->stampLastIO = gClock_msec; // refresh the stamp for a valid sent
		SI4432_flushTxFront(chip);
	}

	// put the default mode as RX if nothing to send
	if (0 == (chip->OPstatus & SI4432_OPFLAG_SEND_IN_PROGRESS))
		SI4432_writeReg(chip, SIREG_OperatingFunctionControl1, OPERA_OpenRX | OPERA_OpenXtal); // 0x05

}

/*
// ---------------------------------------------------------------------------
// SI4432_OnPacket()
// ---------------------------------------------------------------------------
hterr SI4432_OnPacket(SI4432* chip, uint8_t * packet, uint8_t * length)
{
	uint8_t i;

	if (0 == SI4432_IRQ())
	{
		i = SI4432_RW(chip, SIREG_InterruptStatus1, 0x00);
		if( (i & 0x01) == 0x01 )
		{
			//CRC error
			//disable receiver 
			SI4432_writeReg(chip, SIREG_OperatingFunctionControl1, 0x01);
			return ERR_CRC;
		}

		if( (i & 0x02) == 0x02 )
		{
			//packet received
			//read buffer
			*length =  SI4432_RW(chip, SIREG_ReceivedPacketLength, 0x00);
			for(i=0;i<*length;i++)
			{
				*packet++ = SI4432_RW(chip, SIREG_FIFOAccess, 0x00);
			}
			//disable receiver 
			SI4432_writeReg(chip, SIREG_OperatingFunctionControl1, 0x01);
			return hterr_RECEIVED;
		}
	}

	return hterr_NO_PACKET;
}

*/

typedef struct _BAUD_CONFIG
{
	uint8_t IFBW, COSR;
	uint8_t CRO2, CRO1, CRO0;
	uint8_t CTG1, CTG0;
	uint8_t TDR1, TDR0, MMC1, FDEV;
} BAUD_CONFIG;

const static BAUD_CONFIG BAUD_CONFIG_TABLE[] =
#ifndef SI4432_COMAPTIBLE_MODE
{
	//set the registers according the selected RF settings in the range mode
	{0x01, 0x83, 0xc0, 0x13, 0xa9, 0x00, 0x05, 0x13, 0xa9, 0x20, 0x3a },  		//DR: 2.4kbps, DEV: +-36kHz, BBBW: 75.2kHz
	{0x04, 0x41, 0x60, 0x27, 0x52, 0x00, 0x0a, 0x27, 0x52, 0x20, 0x48 },		//DR: 4.8kbps, DEV: +-45kHz, BBBW: 95.3kHz
	{0x91, 0x71, 0x40, 0x34, 0x6e, 0x00, 0x18, 0x4e, 0xa5, 0x20, 0x48 },		//DR: 9.6kbps, DEV: +-45kHz, BBBW: 112.8kHz
	{0x26, 0x34, 0x02, 0x75, 0x25, 0x07, 0xFF, 0x9D, 0x49, 0x20, 0x15 },		//DR: 19.2kbps, DEV: +-9.6kHz, BBBW: 28.8kHz
	{0x16, 0x34, 0x02, 0x75, 0x25, 0x07, 0xFF, 0x09, 0xD5, 0x00, 0x1F },		//DR: 38.4kbps, DEV: +-19.2kHz, BBBW: 57.6kHz  
	{0x03, 0x45, 0x01, 0xD7, 0xDC, 0x07, 0x6E, 0x0E, 0xBF, 0x00, 0x2E },		//DR: 57.6kbps, DEV: +-28.8kHz, BBBW: 86.4kHz
	{0x99, 0x34, 0x02, 0x75, 0x25, 0x07, 0xFF, 0x1D, 0x7E, 0x00, 0x5C },		//DR: 115.2kbps, DEV: +-57.6kHz, BBBW: 172.8kHz
};
#else // SI4432_COMAPTIBLE_MODE
{
	// set the registers according the selected RF settings in the compatible mode 
	//revV2
	//	 IFBW, COSR, CRO2, CRO1, CRO0, CTG1, CTG0, TDR1, TDR0, MMC1, FDEV, B_TIME

	{0x8F, 0xE5, 0x80, 0x1A, 0x28, 0x00, 0x08, 0x4e, 0x79, 0x20, 0x90 },		//DR: 9.6kbps, DEV: +-90kHz, BBBW:184.8kHz 

};
#endif // SI4432_COMAPTIBLE_MODE


/*
const uint8_t BAUD_CONFIG RMRfSettings[NMBR_OF_SAMPLE_SETTING1][NMBR_OF_PARAMETER] =		
{
//revV2
//	 IFBW, COSR, CRO2, CRO1, CRO0, CTG1, CTG0, TDR1, TDR0, MMC1, FDEV, B_TIME
{0x01, 0x83, 0xc0, 0x13, 0xa9, 0x00, 0x05, 0x13, 0xa9, 0x20, 0x3a },  		//DR: 2.4kbps, DEV: +-36kHz, BBBW: 75.2kHz
{0x04, 0x41, 0x60, 0x27, 0x52, 0x00, 0x0a, 0x27, 0x52, 0x20, 0x48 },		//DR: 4.8kbps, DEV: +-45kHz, BBBW: 95.3kHz
{0x91, 0x71, 0x40, 0x34, 0x6e, 0x00, 0x18, 0x4e, 0xa5, 0x20, 0x48 },		//DR: 9.6kbps, DEV: +-45kHz, BBBW: 112.8kHz
{0x26, 0x34, 0x02, 0x75, 0x25, 0x07, 0xFF, 0x9D, 0x49, 0x20, 0x15 },		//DR: 19.2kbps, DEV: +-9.6kHz, BBBW: 28.8kHz
{0x16, 0x34, 0x02, 0x75, 0x25, 0x07, 0xFF, 0x09, 0xD5, 0x00, 0x1F },		//DR: 38.4kbps, DEV: +-19.2kHz, BBBW: 57.6kHz  
{0x03, 0x45, 0x01, 0xD7, 0xDC, 0x07, 0x6E, 0x0E, 0xBF, 0x00, 0x2E },		//DR: 57.6kbps, DEV: +-28.8kHz, BBBW: 86.4kHz
{0x99, 0x34, 0x02, 0x75, 0x25, 0x07, 0xFF, 0x1D, 0x7E, 0x00, 0x5C },		//DR: 115.2kbps, DEV: +-57.6kHz, BBBW: 172.8kHz

};
*/

const BAUD_CONFIG CMRfSettings[] =		
{
	//revV2
	//	 IFBW, COSR, CRO2, CRO1, CRO0, CTG1, CTG0, TDR1, TDR0, MMC1, FDEV, B_TIME

	{0x8F, 0xE5, 0x80, 0x1A, 0x28, 0x00, 0x08, 0x4e, 0x79, 0x20, 0x90 },		//DR: 9.6kbps, DEV: +-90kHz, BBBW:184.8kHz 

};

hterr SI4432_setBaud(SI4432* chip, SIBAUDID baudId)
{
	SI4432_writeReg(chip, SIREG_IFFilterBandwidth, 				BAUD_CONFIG_TABLE[baudId].IFBW);
	SI4432_writeReg(chip, SIREG_ClockRecoveryOversamplingRatio,	BAUD_CONFIG_TABLE[baudId].COSR);
	SI4432_writeReg(chip, SIREG_ClockRecoveryOffset2, 			BAUD_CONFIG_TABLE[baudId].CRO2);
	SI4432_writeReg(chip, SIREG_ClockRecoveryOffset1, 			BAUD_CONFIG_TABLE[baudId].CRO1);
	SI4432_writeReg(chip, SIREG_ClockRecoveryOffset0, 			BAUD_CONFIG_TABLE[baudId].CRO0);
	SI4432_writeReg(chip, SIREG_ClockRecoveryTimingLoopGain1, 	BAUD_CONFIG_TABLE[baudId].CTG1);
	SI4432_writeReg(chip, SIREG_ClockRecoveryTimingLoopGain0, 	BAUD_CONFIG_TABLE[baudId].CTG0);
	SI4432_writeReg(chip, SIREG_TXDataRate1, 					BAUD_CONFIG_TABLE[baudId].TDR1);
	SI4432_writeReg(chip, SIREG_TXDataRate0, 					BAUD_CONFIG_TABLE[baudId].TDR0);
	SI4432_writeReg(chip, SIREG_ModulationModeControl1, 		BAUD_CONFIG_TABLE[baudId].MMC1);
	SI4432_writeReg(chip, SIREG_FrequencyDeviation, 			BAUD_CONFIG_TABLE[baudId].FDEV);

	return ERR_SUCCESS;
}

typedef struct _FREQ_CONFIG
{
	uint8_t NCF1, NCF0, FHSS, FHCS;
} FREQ_CONFIG;

const static FREQ_CONFIG FREQ_CONFIG_TABLE[] =
#ifdef BAND_868
{
	//parameters of the frequency setting
	{14, 0x2c, 0x60, 45}, //   2.4kbps
	{14, 0x2c, 0x60, 45}, //   4.8kbps
	{14, 0x2c, 0x60, 45}, //   9.6kbps
	{11, 0x2e, 0x40, 55}, //  19.2kbps
	{10, 0x2d, 0x40, 64}, //  38.4kbps
	{ 8, 0x2e, 0xe0, 78}, //  57.6kbps
	{ 6, 0x32, 0x00, 100} // 115.2kbps
};
#elif defined(BAND_915)
{
	//parameters of the frequency setting
	{60, 0x0a, 0x80, 48}, //   2.4kbps
	{60, 0x0a, 0x80, 48}, //   4.8kbps
	{60, 0x0a, 0x80, 48}, //   9.6kbps
	{52, 0x0a, 0x80, 55}, //  19.2kbps
	{45, 0x0a, 0x80, 64}, //  38.4kbps
	{37, 0x0a, 0x80, 78}, //  57.6kbps
	{29, 0x0a, 0x80, 100} // 115.2kbps
};
#else // default BAND_434
{
	//parameters of the frequency setting
	{ 4, 0x53, 0x40, 48}, //   2.4kbps
	{ 4, 0x53, 0x40, 48}, //   4.8kbps
	{ 4, 0x53, 0x40, 48}, //   9.6kbps
	{ 3, 0x58, 0x40, 55}, //  19.2kbps
	{ 3, 0x56, 0xc0, 64}, //  38.4kbps
	{ 2, 0x5a, 0x40, 78}, //  57.6kbps
	{ 1, 0x61, 0xc0, 100} // 115.2kbps
};
#endif

hterr SI4432_setFrequence(SI4432* chip, uint8_t freq)
{
	//set frequency
	SI4432_writeReg(chip, SIREG_NominalCarrierFrequency1,      FREQ_CONFIG_TABLE[freq].NCF1);
	SI4432_writeReg(chip, SIREG_NominalCarrierFrequency0,      FREQ_CONFIG_TABLE[freq].NCF0);
	SI4432_writeReg(chip, SIREG_FrequencyHoppingStepSize,      FREQ_CONFIG_TABLE[freq].FHSS);
	SI4432_writeReg(chip, SIREG_FrequencyHoppingChannelSelect, FREQ_CONFIG_TABLE[freq].FHCS);
	delayXmsec(100); //wait a bit

	return ERR_SUCCESS;
}

hterr SI4432_setPower(SI4432* chip, uint8_t pwr)
{
	SI4432_writeReg(chip, SIREG_TXPower, pwr);
	delayXmsec(100); //wait a bit

	return ERR_SUCCESS;
}

