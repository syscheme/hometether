#ifndef __HomeTether_H__
#define __HomeTether_H__

#define DEFAULT_HeartbeatTimeout (10*1000)

enum _gStateFlag
{
	gsf_StateMask       = (0x0f),  // the lowest 4bit as the state ordinal code, 16 states
	gsf_AutoGuard       = (1<<4),  // if timeout at no motions in all inner areas, enter state of soc_Guard automatically
	gsf_DangerSource    = (1<<5),   // if there is some dangerous sources, such as gas/smoke/outerwall
	gsf_Autonomy        = (1<<6)   // if there is no active master, this node will elect master automatically
};

typedef enum _StateOrdinalCode
{
//	// state of slave mode
//	// @monitor and auto-leave
//	//  1) gsf_DangerSource => beep
//	//  2) if gsf_AutoGuard & timeout at no motions => soc_Guard
//	soc_Slave,

	// state of Easy
	// @monitor and auto-leave
	//  1) gsf_DangerSource => beep
	//  2) if gsf_AutoGuard & timeout at no motions => soc_Guard
	soc_Easy,

	// state Of Guard
	// @monitor and leave
	//  1) gsf_DangerSource => beep
	//  2) inner motions => soc_PreAlarm
	soc_PreGuard,

	// state Of Guard
	// @monitor and leave
	//  1) gsf_DangerSource => beep
	//  2) inner motions => soc_PreAlarm
	soc_Guard,       // enter this state by a)command, b)no motions for continuous 30sec when flag gsf_AutoGuard is set, any confirmed motion would l

	// state Of SilentHours
	// @monitor and auto-leave
	//  1) gsf_DangerSource => beep
	//  2) inner motions => soc_PreAlarm
	soc_SilentHours,       // enter this state by a)command, b)no motions for continuous 30sec when flag gsf_AutoGuard is set, any confirmed motion would l

	// state Of PreAlarm
	// @monitor and auto-leave
	//  1) gsf_DangerSource => beep
	//  2) timeout => soc_Alarm
	soc_PreAlarm,

	// state Of PreAlarm
	// @monitor and auto-leave
	//  1) gsf_DangerSource => beep
	soc_Alarm

} StateOrdinalCode;

#define ASK_DEVICE_BASEFLG  (8)  // DeviceStateFlag refer to EcuCtrl_askDevTypes

typedef enum _DeviceStateFlag
{
	// the lowerest 4bits is the StateOrdinalCode
	dsf_MotionChanged   = (1<<0),
	dsf_LuminChanged    = (1<<1),
	dsf_IrTxEmpty       = (1<<2),
	dsf_IrRxAvail       = (1<<3),
	dsf_StateTimer      = (1<<4),
	dsf_Debug           = (1<<5),

	dsf_Window          = (1<<(ASK_DEVICE_BASEFLG)),
	dsf_GasLeak         = (1<<(ASK_DEVICE_BASEFLG+1)),
	dsf_Smoke           = (1<<(ASK_DEVICE_BASEFLG+2)),
	dsf_OtherDangers    = (1<<(ASK_DEVICE_BASEFLG+3)),

//	dsf_LocalMaster     = (1<<15) // the local ECU work as the master according to election
} DeviceStateFlag;

// define IS_DEBUG before refering the following consts
#define VARCONST(_DEBUGVAL, _NORMALVAL) (DEBUG_CONST ? (_DEBUGVAL)  : (_NORMALVAL)) 
#define DEFAULT_MasterHeartbeatTimeoutReload       VARCONST(30*1000, DEFAULT_HeartbeatTimeout) // 10sec

#define STATE_TIMEOUT_Easy                         VARCONST(30*1000, 30*60*1000) // 30min
#define STATE_TIMEOUT_SilentHours                  VARCONST(30*1000, 10*60*1000) // 10min
#define STATE_TIMEOUT_Guard                        VARCONST(30*1000, 60*60*1000) // 1hr
#define STATE_TIMEOUT_PreAlarm                     VARCONST(20*1000, 20*1000) // 20sec
#define STATE_TIMEOUT_Alarm                        VARCONST(30*1000, 60*60*1000) // 1hr


#endif // __HomeTether_H__
