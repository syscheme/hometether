// ===========================================================================
// Copyright (c) 1997, 1998 by
// CO_OD.h - Variables and Object Dictionary for CANopenNode
//
// Ident : $Id: CO_OD.h$
// Branch: $Name:  $
// Author: janez.paternoster@siol.net
// Desc  : For short description of standard Object Dictionary entries see CO_OD.txt
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/CO_OD.h $
// ===========================================================================

#ifndef __CO_OD_H__
#define __CO_OD_H__

#include "CO_driver.h"

// -----------------------------------------------
// Values for user Object Dictionary Entries: must customize
// -----------------------------------------------
// 0x2101
#define ODD_CANnodeID    0x08 //(1 to 127), default node ID
extern uint8_t      ODE_CANnodeID;
// 0x2102 (0 to 7), default CAN bitrate (in kbps): 0: 10, 1: 20, 2: 50, 3: 125, 4: 250, 5: 500, 6: 800, 7: 1000 
#define ODD_CANbitRate   3 
extern uint8_t      ODE_CANbitRate;

// -----------------------------
// Setup CANopen: must customize
// -----------------------------
#define CO_NO_SYNC            0  //(0 or 1), is SYNC (producer and consumer) used or not.
#define CO_NO_EMERGENCY       1  //(0 or 1), is Emergency message producer used or not.
#define CO_NO_RPDO            2  //(0 to 512*), number of receive PDOs.
#define CO_NO_TPDO            0  //(0 to 512*), number of transmit PDOs.
#define CO_NO_SDO_SERVER      1  //(0 to 128*), number of SDO server channels.
#define CO_NO_SDO_CLIENT      1  //(0 to 128*), number of SDO client channels.
#define CO_NO_CONS_HEARTBEAT  1  //(0 to 255*), number of consumer heartbeat entries.
#define CO_NO_USR_CAN_RX      0  //(0 to ...), number of user CAN RX messages.
#define CO_NO_USR_CAN_TX      1  //(0 to ...), number of user CAN TX messages.
#define CO_MAX_OD_ENTRY_SIZE  10  //(4 to 256), max size of variable in Object Dictionary in bytes.
#define CO_SDO_TIMEOUT_TIME   10  //[in 100*ms] Timeout in SDO communication.
#define CO_NO_ERROR_FIELD     8  //(0 to 254*), size of Pre Defined Error Fields at index 0x1003.
//#define CO_PDO_PARAM_IN_OD       //if defined, PDO parameters will be in Object Dictionary.
//#define CO_PDO_MAPPING_IN_OD     //if defined, PDO mapping will be in Object Dictionary. If not defined, PDO size will be fixed to 8 bytes.
//#define CO_TPDO_INH_EV_TIMER     //if defined, Inhibit and Event timer will be used for TPDO transmission.
//#define CO_VERIFY_OD_WRITE       //if defined, SDO write to Object Dictionary will be verified.
#define CO_OD_IS_ORDERED         //enable this macro, if entries in CO_OD are ordered (from lowest to highest index, then subindex). For longer Object Dictionaries searching is much faster. If entries are not ordered, disable macro.
// #define CO_SAVE_EEPROM           //if defined, ODE_EEPROM data will be saved.

// -----------------------------------------------
// 0x1016 Heartbeat consumer: should customize per needs
// -----------------------------------------------
//00NNTTTT: N=NodeID, T=time in ms
#define ODD_CONS_HEARTBEAT_0  0x00000040L
#define ODD_CONS_HEARTBEAT_1  0x00000000L
#define ODD_CONS_HEARTBEAT_2  0x00000000L
#define ODD_CONS_HEARTBEAT_3  0x00000000L
#define ODD_CONS_HEARTBEAT_4  0x00000000L
#define ODD_CONS_HEARTBEAT_5  0x00000000L
#define ODD_CONS_HEARTBEAT_6  0x00000000L
#define ODD_CONS_HEARTBEAT_7  0x00000000L

// -----------------------------------------------
// 0x1400 Receive PDO parameters: should customize per needs
// -----------------------------------------------
//COB-ID: if(bit31==1) PDO is not used; bit30=1; bits(10..0)=COB-ID;
#define ODD_RPDO_PAR_COB_ID_0 0x40000286L  //if 0, predefined value will be used (0x200+NODE-ID - read-only)
#define ODD_RPDO_PAR_T_TYPE_0 255
#define ODD_RPDO_PAR_COB_ID_1 0x40000187L  //if 0, predefined value will be used (0x300+NODE-ID - read-only)
#define ODD_RPDO_PAR_T_TYPE_1 255
#define ODD_RPDO_PAR_COB_ID_2 0  //if 0, predefined value will be used (0x400+NODE-ID - read-only)
#define ODD_RPDO_PAR_T_TYPE_2 255
#define ODD_RPDO_PAR_COB_ID_3 0  //if 0, predefined value will be used (0x500+NODE-ID - read-only)
#define ODD_RPDO_PAR_T_TYPE_3 255
#define ODD_RPDO_PAR_COB_ID_4 0xC0000000L
#define ODD_RPDO_PAR_T_TYPE_4 255
#define ODD_RPDO_PAR_COB_ID_5 0xC0000000L
#define ODD_RPDO_PAR_T_TYPE_5 255
#define ODD_RPDO_PAR_COB_ID_6 0xC0000000L
#define ODD_RPDO_PAR_T_TYPE_6 255
#define ODD_RPDO_PAR_COB_ID_7 0xC0000000L
#define ODD_RPDO_PAR_T_TYPE_7 255

// -----------------------------------------------
// 0x1600 Receive PDO mapping: should customize per needs
// -----------------------------------------------
//0xIIIISSDD IIII = index from OD, SS = subindex, DD = data length in bits
// DD must be byte aligned, max value 0x40 (8 bytes)
#define ODD_RPDO_MAP_0_1      0x00000040L
#define ODD_RPDO_MAP_0_2      0x00000000L
#define ODD_RPDO_MAP_0_3      0x00000000L
#define ODD_RPDO_MAP_0_4      0x00000000L
#define ODD_RPDO_MAP_0_5      0x00000000L
#define ODD_RPDO_MAP_0_6      0x00000000L
#define ODD_RPDO_MAP_0_7      0x00000000L
#define ODD_RPDO_MAP_0_8      0x00000000L

#define ODD_RPDO_MAP_1_1      0x00000040L
#define ODD_RPDO_MAP_1_2      0x00000000L
#define ODD_RPDO_MAP_1_3      0x00000000L
#define ODD_RPDO_MAP_1_4      0x00000000L
#define ODD_RPDO_MAP_1_5      0x00000000L
#define ODD_RPDO_MAP_1_6      0x00000000L
#define ODD_RPDO_MAP_1_7      0x00000000L
#define ODD_RPDO_MAP_1_8      0x00000000L

#define ODD_RPDO_MAP_2_1      0x00000040L
#define ODD_RPDO_MAP_2_2      0x00000000L
#define ODD_RPDO_MAP_2_3      0x00000000L
#define ODD_RPDO_MAP_2_4      0x00000000L
#define ODD_RPDO_MAP_2_5      0x00000000L
#define ODD_RPDO_MAP_2_6      0x00000000L
#define ODD_RPDO_MAP_2_7      0x00000000L
#define ODD_RPDO_MAP_2_8      0x00000000L

#define ODD_RPDO_MAP_3_1      0x00000040L
#define ODD_RPDO_MAP_3_2      0x00000000L
#define ODD_RPDO_MAP_3_3      0x00000000L
#define ODD_RPDO_MAP_3_4      0x00000000L
#define ODD_RPDO_MAP_3_5      0x00000000L
#define ODD_RPDO_MAP_3_6      0x00000000L
#define ODD_RPDO_MAP_3_7      0x00000000L
#define ODD_RPDO_MAP_3_8      0x00000000L

#define ODD_RPDO_MAP_4_1      0x00000040L
#define ODD_RPDO_MAP_4_2      0x00000000L
#define ODD_RPDO_MAP_4_3      0x00000000L
#define ODD_RPDO_MAP_4_4      0x00000000L
#define ODD_RPDO_MAP_4_5      0x00000000L
#define ODD_RPDO_MAP_4_6      0x00000000L
#define ODD_RPDO_MAP_4_7      0x00000000L
#define ODD_RPDO_MAP_4_8      0x00000000L

#define ODD_RPDO_MAP_5_1      0x00000040L
#define ODD_RPDO_MAP_5_2      0x00000000L
#define ODD_RPDO_MAP_5_3      0x00000000L
#define ODD_RPDO_MAP_5_4      0x00000000L
#define ODD_RPDO_MAP_5_5      0x00000000L
#define ODD_RPDO_MAP_5_6      0x00000000L
#define ODD_RPDO_MAP_5_7      0x00000000L
#define ODD_RPDO_MAP_5_8      0x00000000L

#define ODD_RPDO_MAP_6_1      0x00000040L
#define ODD_RPDO_MAP_6_2      0x00000000L
#define ODD_RPDO_MAP_6_3      0x00000000L
#define ODD_RPDO_MAP_6_4      0x00000000L
#define ODD_RPDO_MAP_6_5      0x00000000L
#define ODD_RPDO_MAP_6_6      0x00000000L
#define ODD_RPDO_MAP_6_7      0x00000000L
#define ODD_RPDO_MAP_6_8      0x00000000L

#define ODD_RPDO_MAP_7_1      0x00000040L
#define ODD_RPDO_MAP_7_2      0x00000000L
#define ODD_RPDO_MAP_7_3      0x00000000L
#define ODD_RPDO_MAP_7_4      0x00000000L
#define ODD_RPDO_MAP_7_5      0x00000000L
#define ODD_RPDO_MAP_7_6      0x00000000L
#define ODD_RPDO_MAP_7_7      0x00000000L
#define ODD_RPDO_MAP_7_8      0x00000000L

// -----------------------------------------------
// 0x1800 Transmit PDO parameters: should customize per needs
// -----------------------------------------------
//COB-ID: if (bit31==1) PDO is not used; bit30=1; bits(10..0)=COB-ID;
//T_TYPE: 1-240...transmission after every (T_TYPE)-th SYNC object;
//        254...manufacturer specific
//        255...Device Profile specific, default transmission is Change of State
//I_TIME: 0...65535 Inhibit time in 100µs is minimum time between PDO transmission
//E_TIME: 0...65535 Event timer in ms - PDO is periodically transmitted (0 == disabled)
#define ODD_TPDO_PAR_COB_ID_0 0  //if 0, predefined value will be used (0x180+NODE-ID - read-only)
#define ODD_TPDO_PAR_T_TYPE_0 254
#define ODD_TPDO_PAR_I_TIME_0 0
#define ODD_TPDO_PAR_E_TIME_0 0
#define ODD_TPDO_PAR_COB_ID_1 0  //if 0, predefined value will be used (0x280+NODE-ID - read-only)
#define ODD_TPDO_PAR_T_TYPE_1 254
#define ODD_TPDO_PAR_I_TIME_1 0
#define ODD_TPDO_PAR_E_TIME_1 0
#define ODD_TPDO_PAR_COB_ID_2 0  //if 0, predefined value will be used (0x380+NODE-ID - read-only)
#define ODD_TPDO_PAR_T_TYPE_2 254
#define ODD_TPDO_PAR_I_TIME_2 0
#define ODD_TPDO_PAR_E_TIME_2 0
#define ODD_TPDO_PAR_COB_ID_3 0  //if 0, predefined value will be used (0x480+NODE-ID - read-only)
#define ODD_TPDO_PAR_T_TYPE_3 254
#define ODD_TPDO_PAR_I_TIME_3 0
#define ODD_TPDO_PAR_E_TIME_3 0
#define ODD_TPDO_PAR_COB_ID_4 0xC0000000L
#define ODD_TPDO_PAR_T_TYPE_4 254
#define ODD_TPDO_PAR_I_TIME_4 0
#define ODD_TPDO_PAR_E_TIME_4 0
#define ODD_TPDO_PAR_COB_ID_5 0xC0000000L
#define ODD_TPDO_PAR_T_TYPE_5 254
#define ODD_TPDO_PAR_I_TIME_5 0
#define ODD_TPDO_PAR_E_TIME_5 0
#define ODD_TPDO_PAR_COB_ID_6 0xC0000000L
#define ODD_TPDO_PAR_T_TYPE_6 254
#define ODD_TPDO_PAR_I_TIME_6 0
#define ODD_TPDO_PAR_E_TIME_6 0
#define ODD_TPDO_PAR_COB_ID_7 0xC0000000L
#define ODD_TPDO_PAR_T_TYPE_7 254
#define ODD_TPDO_PAR_I_TIME_7 0
#define ODD_TPDO_PAR_E_TIME_7 0

// -----------------------------------------------
// 0x1A00 Transmit PDO mapping: should customize per needs
// -----------------------------------------------
//0xIIIISSDD IIII = index from OD, SS = subindex, DD = data length in bits
//DD must be byte aligned, max value 0x40 (8 bytes)
#define ODD_TPDO_MAP_0_1      0x00000040L
#define ODD_TPDO_MAP_0_2      0x00000000L
#define ODD_TPDO_MAP_0_3      0x00000000L
#define ODD_TPDO_MAP_0_4      0x00000000L
#define ODD_TPDO_MAP_0_5      0x00000000L
#define ODD_TPDO_MAP_0_6      0x00000000L
#define ODD_TPDO_MAP_0_7      0x00000000L
#define ODD_TPDO_MAP_0_8      0x00000000L

#define ODD_TPDO_MAP_1_1      0x00000040L
#define ODD_TPDO_MAP_1_2      0x00000000L
#define ODD_TPDO_MAP_1_3      0x00000000L
#define ODD_TPDO_MAP_1_4      0x00000000L
#define ODD_TPDO_MAP_1_5      0x00000000L
#define ODD_TPDO_MAP_1_6      0x00000000L
#define ODD_TPDO_MAP_1_7      0x00000000L
#define ODD_TPDO_MAP_1_8      0x00000000L

#define ODD_TPDO_MAP_2_1      0x00000040L
#define ODD_TPDO_MAP_2_2      0x00000000L
#define ODD_TPDO_MAP_2_3      0x00000000L
#define ODD_TPDO_MAP_2_4      0x00000000L
#define ODD_TPDO_MAP_2_5      0x00000000L
#define ODD_TPDO_MAP_2_6      0x00000000L
#define ODD_TPDO_MAP_2_7      0x00000000L
#define ODD_TPDO_MAP_2_8      0x00000000L

#define ODD_TPDO_MAP_3_1      0x00000040L
#define ODD_TPDO_MAP_3_2      0x00000000L
#define ODD_TPDO_MAP_3_3      0x00000000L
#define ODD_TPDO_MAP_3_4      0x00000000L
#define ODD_TPDO_MAP_3_5      0x00000000L
#define ODD_TPDO_MAP_3_6      0x00000000L
#define ODD_TPDO_MAP_3_7      0x00000000L
#define ODD_TPDO_MAP_3_8      0x00000000L

#define ODD_TPDO_MAP_4_1      0x00000040L
#define ODD_TPDO_MAP_4_2      0x00000000L
#define ODD_TPDO_MAP_4_3      0x00000000L
#define ODD_TPDO_MAP_4_4      0x00000000L
#define ODD_TPDO_MAP_4_5      0x00000000L
#define ODD_TPDO_MAP_4_6      0x00000000L
#define ODD_TPDO_MAP_4_7      0x00000000L
#define ODD_TPDO_MAP_4_8      0x00000000L

#define ODD_TPDO_MAP_5_1      0x00000040L
#define ODD_TPDO_MAP_5_2      0x00000000L
#define ODD_TPDO_MAP_5_3      0x00000000L
#define ODD_TPDO_MAP_5_4      0x00000000L
#define ODD_TPDO_MAP_5_5      0x00000000L
#define ODD_TPDO_MAP_5_6      0x00000000L
#define ODD_TPDO_MAP_5_7      0x00000000L
#define ODD_TPDO_MAP_5_8      0x00000000L

#define ODD_TPDO_MAP_6_1      0x00000040L
#define ODD_TPDO_MAP_6_2      0x00000000L
#define ODD_TPDO_MAP_6_3      0x00000000L
#define ODD_TPDO_MAP_6_4      0x00000000L
#define ODD_TPDO_MAP_6_5      0x00000000L
#define ODD_TPDO_MAP_6_6      0x00000000L
#define ODD_TPDO_MAP_6_7      0x00000000L
#define ODD_TPDO_MAP_6_8      0x00000000L

#define ODD_TPDO_MAP_7_1      0x00000040L
#define ODD_TPDO_MAP_7_2      0x00000000L
#define ODD_TPDO_MAP_7_3      0x00000000L
#define ODD_TPDO_MAP_7_4      0x00000000L
#define ODD_TPDO_MAP_7_5      0x00000000L
#define ODD_TPDO_MAP_7_6      0x00000000L
#define ODD_TPDO_MAP_7_7      0x00000000L
#define ODD_TPDO_MAP_7_8      0x00000000L

void CO_Init(void);

void CO_User_OnDriverInited(void);
void CO_User_OnProcessMain(void);
void CO_OnNMTResetComm(void);
void CO_User_Remove(void);
void CO_User_OnProcess1ms(void);
void CO_OnNMTResetNode(void); // reset MCU

#define NMT_MASTER        CO_TXCAN_USER

#endif // __CO_OD_H__
