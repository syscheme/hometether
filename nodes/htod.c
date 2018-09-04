#include "htod.h"

#define EcuSpec_thisNodeId    HtCan_thisNodeId

// -------------------------------
// Definitions of SubIdx arrays
// -------------------------------
// global state
const static ODSubIdx ODSubIdx_globalState[] = {
	{ODType_uint8,    "gstate",            &Ecu_globalState},
	{ODType_uint8,    "masterId",          &Ecu_masterNodeId},
	{0, NULL, NULL}
};

// local state
const static ODSubIdx ODSubIdx_localState[] = {
	{ODType_uint16,    "lstate",           &Ecu_localState},
	{ODType_uint16,    "mstate",           &Ecu_motionState},
	{0, NULL, NULL}
};

// ECU configurations
const static ODSubIdx ODSubIdx_EcuConfigure[] = {
	{ODType_uint8,    "tempMask",          &EcuConf_temperatureMask},
	{ODType_uint8,    "tempIntv",          &EcuConf_temperatureIntv},
	{ODType_uint16,   "motionDiffMask",    &EcuConf_motionDiffMask},
	{ODType_uint16,   "motionOuterMask",   &EcuConf_motionOuterMask},
	{ODType_uint16,   "motionBedMask",     &EcuConf_motionBedMask},
	{ODType_uint8,    "motionDiffIntv",    &EcuConf_motionDiffIntv},
	{ODType_uint8,    "lumineDiffMask",    &EcuConf_lumineDiffMask},
	{ODType_uint8,    "lumineDiffIntv",    &EcuConf_lumineDiffIntv},
	{ODType_uint16,   "lumineDiffTshd",    &EcuConf_lumineDiffTshd},
	{ODType_uint8,    "fwdAdm",            &EcuConf_fwdAdm},
	{ODType_uint8,    "fwdExt",            &EcuConf_fwdExt},
	{0, NULL, NULL}
};

// ECU spec
const static ODSubIdx ODSubIdx_EcuSpec[] = {
	{ODType_uint8,    "nodeId",            &EcuSpec_thisNodeId},
	{ODType_uint16,   "verMajor",          &EcuSpec_versionMajor},
	{ODType_uint8,    "verMinor",          &EcuSpec_versionMinor},
	{0, NULL, NULL}
};

// Ecu_temperature values
const static ODSubIdx ODSubIdx_temperature[] = {
	{ODType_uint16,    "t0",    &Ecu_temperature[0]},
	{ODType_uint16,    "t1",    &Ecu_temperature[1]},
	{ODType_uint16,    "t2",    &Ecu_temperature[2]},
	{ODType_uint16,    "t3",    &Ecu_temperature[3]},
	{ODType_uint16,    "t4",    &Ecu_temperature[4]},
	{ODType_uint16,    "t5",    &Ecu_temperature[5]},
	{ODType_uint16,    "t6",    &Ecu_temperature[6]},
	{ODType_uint16,    "t7",    &Ecu_temperature[7]},
	{0, NULL, NULL}
};

// Ecu_lumine ADC values
const static ODSubIdx ODSubIdx_adc[] = {
	{ODType_uint16,    "a0",    &Ecu_adc[0]},
	{ODType_uint16,    "a1",    &Ecu_adc[1]},
	{ODType_uint16,    "a2",    &Ecu_adc[2]},
	{ODType_uint16,    "a3",    &Ecu_adc[3]},
	{ODType_uint16,    "a4",    &Ecu_adc[4]},
	{ODType_uint16,    "a5",    &Ecu_adc[5]},
	{ODType_uint16,    "a6",    &Ecu_adc[6]},
	{ODType_uint16,    "a7",    &Ecu_adc[7]},
	{ODType_uint16,    "a8",    &Ecu_adc[8]},
	{0, NULL, NULL}
};

/*
// Control of pulse signals
const static ODSubIdx ODSubIdx_EcuPulse[] = {
	{ODType_uint16,   "poCh",      &EcuCtrl_pulseSendChId},
	{ODType_uint8,    "poCode",    &EcuCtrl_pulseSendCode},
	{ODType_uint8,    "poPfl",     &EcuCtrl_pulseSendPflId},
	{ODType_uint8,    "poIntv",    &EcuCtrl_pulseSendBaseIntvX10usec},

	{ODType_uint16,   "piCh",      &EcuCtrl_pulseRecvChId},
	{ODType_uint8,    "piTimeout", &EcuCtrl_pulseRecvTimeout},
	{ODType_uint16,   "piIntv",    &Ecu_pulseRecvSeq.short_durXusec},
	{ODType_uint8,    "piLen",     &Ecu_pulseRecvSeq.seqlen},
	{0, NULL, NULL}
};

// Pulse recv buffer
const static ODSubIdx ODSubIdx_pulseRecvBuf[] = {
	{ODType_uint8,     "b00",             &Ecu_pulseRecvBuf[0x00]},
	{ODType_uint8,     "b01",             &Ecu_pulseRecvBuf[0x01]},
	{ODType_uint8,     "b02",             &Ecu_pulseRecvBuf[0x02]},
	{ODType_uint8,     "b03",             &Ecu_pulseRecvBuf[0x03]},
	{ODType_uint8,     "b04",             &Ecu_pulseRecvBuf[0x04]},
	{ODType_uint8,     "b05",             &Ecu_pulseRecvBuf[0x05]},
	{ODType_uint8,     "b06",             &Ecu_pulseRecvBuf[0x06]},
	{ODType_uint8,     "b07",             &Ecu_pulseRecvBuf[0x07]},
	{ODType_uint8,     "b08",             &Ecu_pulseRecvBuf[0x08]},
	{ODType_uint8,     "b09",             &Ecu_pulseRecvBuf[0x09]},
	{ODType_uint8,     "b0a",             &Ecu_pulseRecvBuf[0x0a]},
	{ODType_uint8,     "b0b",             &Ecu_pulseRecvBuf[0x0b]},
	{ODType_uint8,     "b0c",             &Ecu_pulseRecvBuf[0x0c]},
	{ODType_uint8,     "b0d",             &Ecu_pulseRecvBuf[0x0d]},
	{ODType_uint8,     "b0e",             &Ecu_pulseRecvBuf[0x0e]},
	{ODType_uint8,     "b0f",             &Ecu_pulseRecvBuf[0x0f]},
	
	{ODType_uint8,     "b10",             &Ecu_pulseRecvBuf[0x10]},
	{ODType_uint8,     "b11",             &Ecu_pulseRecvBuf[0x11]},
	{ODType_uint8,     "b12",             &Ecu_pulseRecvBuf[0x12]},
	{ODType_uint8,     "b13",             &Ecu_pulseRecvBuf[0x13]},
	{ODType_uint8,     "b14",             &Ecu_pulseRecvBuf[0x14]},
	{ODType_uint8,     "b15",             &Ecu_pulseRecvBuf[0x15]},
	{ODType_uint8,     "b16",             &Ecu_pulseRecvBuf[0x16]},
	{ODType_uint8,     "b17",             &Ecu_pulseRecvBuf[0x17]},
	{ODType_uint8,     "b18",             &Ecu_pulseRecvBuf[0x18]},
	{ODType_uint8,     "b19",             &Ecu_pulseRecvBuf[0x19]},
	{ODType_uint8,     "b1a",             &Ecu_pulseRecvBuf[0x1a]},
	{ODType_uint8,     "b1b",             &Ecu_pulseRecvBuf[0x1b]},
	{ODType_uint8,     "b1c",             &Ecu_pulseRecvBuf[0x1c]},
	{ODType_uint8,     "b1d",             &Ecu_pulseRecvBuf[0x1d]},
	{ODType_uint8,     "b1e",             &Ecu_pulseRecvBuf[0x1e]},
	{ODType_uint8,     "b1f",             &Ecu_pulseRecvBuf[0x1f]},
	
	{ODType_uint8,     "b20",             &Ecu_pulseRecvBuf[0x20]},
	{ODType_uint8,     "b21",             &Ecu_pulseRecvBuf[0x21]},
	{ODType_uint8,     "b22",             &Ecu_pulseRecvBuf[0x22]},
	{ODType_uint8,     "b23",             &Ecu_pulseRecvBuf[0x23]},
	{ODType_uint8,     "b24",             &Ecu_pulseRecvBuf[0x24]},
	{ODType_uint8,     "b25",             &Ecu_pulseRecvBuf[0x25]},
	{ODType_uint8,     "b26",             &Ecu_pulseRecvBuf[0x26]},
	{ODType_uint8,     "b27",             &Ecu_pulseRecvBuf[0x27]},
	{ODType_uint8,     "b28",             &Ecu_pulseRecvBuf[0x28]},
	{ODType_uint8,     "b29",             &Ecu_pulseRecvBuf[0x29]},
	{ODType_uint8,     "b2a",             &Ecu_pulseRecvBuf[0x2a]},
	{ODType_uint8,     "b2b",             &Ecu_pulseRecvBuf[0x2b]},
	{ODType_uint8,     "b2c",             &Ecu_pulseRecvBuf[0x2c]},
	{ODType_uint8,     "b2d",             &Ecu_pulseRecvBuf[0x2d]},
	{ODType_uint8,     "b2e",             &Ecu_pulseRecvBuf[0x2e]},
	{ODType_uint8,     "b2f",             &Ecu_pulseRecvBuf[0x2f]},
	{0, NULL, NULL}
};
*/

// Control of relays
const static ODSubIdx ODSubIdx_EcuRelay[] = {
//	{ODType_uint8,    "relay",   &EcuCtrl_relays},
	{ODType_uint8,    "rOn0",    (void*) &EcuCtrl_relays[0].reloadA},
	{ODType_uint8,    "rOff0",   (void*) &EcuCtrl_relays[0].reloadB},
	{ODType_uint8,    "rOn1",    (void*) &EcuCtrl_relays[1].reloadA},
	{ODType_uint8,    "rOff1",   (void*) &EcuCtrl_relays[1].reloadB},
	{ODType_uint8,    "rOn2",    (void*) &EcuCtrl_relays[2].reloadA},
	{ODType_uint8,    "rOff2",   (void*) &EcuCtrl_relays[2].reloadB},
	{ODType_uint8,    "rOn3",    (void*) &EcuCtrl_relays[3].reloadA},
	{ODType_uint8,    "rOff3",   (void*) &EcuCtrl_relays[3].reloadB},
	{0, NULL, NULL}
};

// -----------------------------------
// Definitions of Object Dictionary
// -----------------------------------
const ODObj objDict[] = {
	{ 0x2000, ODFlag_Readable | ODFlag_Writeable,   "global",           ODSubIdx_globalState },
	{ 0x2001, ODFlag_Readable,                      "local",            ODSubIdx_localState },
	{ 0x2002, ODFlag_Readable | ODFlag_Writeable,   "ecuConfig",        ODSubIdx_EcuConfigure },
//	{ 0x2003, ODFlag_Readable | ODFlag_Writeable,   "ecuPulse",         ODSubIdx_EcuPulse },
//	{ 0x2004, ODFlag_Readable | ODFlag_Writeable,   "ecuRelay",         ODSubIdx_EcuRelay },

	{ 0x2010, ODFlag_Readable,                      "temp",             ODSubIdx_temperature },
	{ 0x2011, ODFlag_Readable,                      "adc",              ODSubIdx_adc },

//	{ 0x2020, ODFlag_Readable,                      "rBuf",             ODSubIdx_pulseRecvBuf },

	{ 0x2100, ODFlag_Readable,                      "ecuSpec",          ODSubIdx_EcuSpec },
	{ 0xffff, ODFlag_Readable,                       NULL,              NULL }, 
};

// -----------------------------------
// Definitions of PDO param arrays
// -----------------------------------
const static PDOParam paramsStateChanged[] = {
	{"gState",   ODType_uint8, &Ecu_globalState},
	{"masterId", ODType_uint8, &Ecu_masterNodeId},
	{NULL, 0, NULL} };

const static PDOParam paramsDeviceStateChanged[] = {
	{"lstate", ODType_uint16, &Ecu_localState},
	{"mstate", ODType_uint16, &Ecu_motionState},
	{NULL, 0, NULL} };
/*
const static PDOParam paramsPulseReceived[] = {
	{"chId", ODType_uint8,  &EcuCtrl_pulseRecvChId},
	{"len",  ODType_uint8,  &Ecu_pulseRecvSeq.seqlen},
	{NULL, 0, NULL} };
*/

// -----------------------------------
// Extension data of heartbeat since m.data[1] takes the same definition of PDO param
// same as paramsStateChanged here
// -----------------------------------
const PDOParam* heartbeatExt = paramsStateChanged;

// -----------------------------------
// Definitions of PDO Dictionary
// -----------------------------------
const PDOObj pdoDict[] = {
			{ HTPDO_GlobalStateChanged,   "EVT_gstate",            paramsStateChanged,       1},
			{ HTPDO_DeviceStateChanged,   "EVT_dstate",            paramsDeviceStateChanged, 1},
//			{ HTPDO_PulsesReceived,       "EVT_pulses",            paramsPulseReceived,      1},
			{ fcPDO3tx,                   "PDO3", NULL, 1},
			{ fcPDO4tx,                   "PDO4", NULL, 1},
			{ 0, NULL, NULL }
};

