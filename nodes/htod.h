#ifndef __HTOD_H__
#define __HTOD_H__

#include "ht.h"

#define PULSE_RECV_MAX              (0x30)
#define ASK_DEVICES_MAX              32
#define ASK_DEVICES_TIMEOUT_REFILL   20
#define SUBIDX_MAX                  (0x30)


#define CHANNEL_SZ_LUMIN       (8)
#define CHANNEL_SZ_DS18B20     (8)
#define CHANNEL_SZ_MOTION     (10)
#define CHANNEL_SZ_IrLED       (4)
#define CHANNEL_SZ_IrRecv      (4)
#define CHANNEL_SZ_Relay       (4)

#define CHANNEL_SZ_ADC         (CHANNEL_SZ_LUMIN +1)
#define CHANNEL_SZ_DHT11       (3)

#define CHANNEL_SZ_Temperature (1+CHANNEL_SZ_DS18B20+CHANNEL_SZ_DHT11)
#define CHANNEL_SZ_Humidity    (CHANNEL_SZ_DHT11)

#include "htcan.h"
#include "pulses.h"

// -----------------------------------
// Declare of external variables that refered in the Dictionary
// -----------------------------------

// global state
extern uint8_t   Ecu_globalState;   // =0;
extern uint8_t   Ecu_masterNodeId;  // =0;

// local state
extern uint16_t  Ecu_localState;     // =0;
extern uint16_t  Ecu_motionState;    // =0;

// ECU configurations
extern uint8_t   EcuConf_temperatureMask; // =0xff;
extern uint8_t   EcuConf_temperatureIntv; // =0xff;
extern uint16_t  EcuConf_motionDiffMask;  // =0xff;
extern uint16_t  EcuConf_motionOuterMask;
extern uint16_t  EcuConf_motionBedMask;
extern uint8_t   EcuConf_motionDiffIntv;  // =0xff;

extern uint8_t   EcuConf_lumineDiffMask;  // =0xff;
extern uint8_t   EcuConf_lumineDiffIntv;  // =0xff;
extern uint16_t  EcuConf_lumineDiffTshd;  
extern uint8_t   EcuConf_fwdAdm;
extern uint8_t   EcuConf_fwdExt;

// ECU spec
extern const uint8_t   EcuSpec_thisNodeId;      // =0xff;
extern const uint8_t   EcuSpec_versionMajor;
extern const uint8_t   EcuSpec_versionMinor;

// temperature values
extern uint16_t Ecu_temperature[CHANNEL_SZ_Temperature];

// lumine ADC values
extern uint16_t Ecu_adc[CHANNEL_SZ_ADC];

// Control of relay
extern BlinkPair EcuCtrl_relays[CHANNEL_SZ_Relay];

/*
// About pulse sending
// void PulseSend_byProfileId(uint8_t profileId, uint32_t code, const IO_PIN* pin, uint8_t HeqH, uint32_t baseIntvXusec)

extern uint8_t EcuCtrl_pulseSendChId;
extern uint8_t EcuCtrl_pulseSendCode;
extern uint8_t EcuCtrl_pulseSendPflId; // 0xff means no pending pulses to send
extern uint8_t EcuCtrl_pulseSendBaseIntvX10usec;

// About pulse receiving
extern uint8_t EcuCtrl_pulseRecvChId;
extern uint8_t EcuCtrl_pulseRecvTimeout; // 0x00 means the receiving buffer would be overwritten if new pulse seq is captured 
extern PulseSeq  Ecu_pulseRecvSeq;  // its seqlen=PULSE_RECV_MAX, seq=Ecu_pulseRecvBuf, 
extern uint8_t Ecu_pulseRecvBuf[PULSE_RECV_MAX];

// About the devices state collected via ASK wireless signals by PT2262 or EV1527 encoded
extern uint32_t EcuCtrl_askDevState;
extern uint32_t EcuCtrl_askDevTable[ASK_DEVICES_MAX]; // the code table of known devices
extern uint8_t  EcuCtrl_askDevTypes[ASK_DEVICES_MAX]; // the code table of known devices
*/

// PDO to send
#define HTPDO_GlobalStateChanged      fcPDO1tx   // event of global state changed by the master node
#define HTPDO_DeviceStateChanged      fcPDO2tx   // event of any ecu state changed
#define HTPDO_PulsesReceived		  fcPDO3tx   // event of pulse signal received

// PDO to receive
#define HTPDO_HIDInputed              fcPDO1rx   // event of manual input from other ecu, only to be handled by the master node

#endif // __HTOD_H__
