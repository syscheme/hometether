#include "htdstate.h"

// -----------------------------
// basic state HtdState
// -----------------------------
#define debug(...) trace("HtdState::" __VA_ARGS__) // printf(__VA_ARGS__)

HtdState::HtdState(HomeTetherDomain& domain)
: _domain(domain) // , _execState(state)
{
}


const char* HtdState::stateStr(StateOrdinalCode state)
{
#define CASE_STATE(_ST) case soc_##_ST:  return #_ST
	switch(state)
	{
		CASE_STATE(Slave);
		CASE_STATE(Easy);
		CASE_STATE(PreGuard);
		CASE_STATE(Guard);
		CASE_STATE(SilentHours);
		CASE_STATE(PreAlarm);
		CASE_STATE(Alarm);
	}
#undef CASE_STATE

	return "UNKNOWN";
}

void HtdState::moveToState(StateOrdinalCode targetState)
{
	StateOrdinalCode oldState = _domain._state;	
	if (oldState == targetState)
		return;

	debug("HtdState state[%s(%d)] is leaving for %s(%d)\n", stateStr(oldState), oldState, stateStr(targetState), targetState);

	switch(_domain._state)
	{
	case soc_Slave: 
		break;
	case soc_Easy: 
		break;
	case soc_PreGuard: 
		// TODO: turn off slow beep
		break;
	case soc_Guard: 
		break;
	case soc_SilentHours: 
		break;
	case soc_PreAlarm: 
		// TODO: turn off slow beep
		break;
	case soc_Alarm:
		// TODO: turn off alarm
		break;
	}

	switch(targetState)
	{
	case soc_Slave:
		break;

	case soc_SilentHours:
		// TODO: turn off all alarms
		setTimeout(STATE_TIMEOUT_SilentHours);
		break;

	case soc_PreGuard:
		// TODO: slow beep
		setTimeout(STATE_TIMEOUT_PreAlarm);
		break;

	case soc_Guard:
		setTimeout(STATE_TIMEOUT_Guard);
		break;

	case soc_PreAlarm:
		// TODO: slow beep
		setTimeout(STATE_TIMEOUT_PreAlarm);
		break;

	case soc_Alarm:
		// TODO: turn on alarm
		setTimeout(STATE_TIMEOUT_Alarm);
		break;

	case soc_Easy:
		// TODO: turn off all alarm
	default:
		setTimeout(STATE_TIMEOUT_Easy);
		break;
	}
	
	info("HtdState entered state[%s(%d)] from previous[%s(%d)]\n", stateStr(_domain._state), _domain._state, stateStr(oldState), oldState);
}

void HtdState::OnTimer()
{
	debug("HtdState state[%s(%d)] dispTimeout()\n", stateStr(_domain._state), _domain._state);
	switch(_domain._state)
	{
	case soc_Slave:
		// do nothing
		break;

	case soc_Easy:
		// timeout at ease state means there are no inner motions for long time, 
		// enter either soc_Guard or soc_SilentHours per RTC hours
		if (!isSilentHours())
		{
			setTimeout(STATE_TIMEOUT_Easy);
			break;
		}

#ifdef __CC_ARM
#  warning TODO: even in silent hours, there may be someone work with light on, so scan the lumines then \
		determine whether enter soc_SilentHours
#endif // __CC_ARM
		
		moveToState(soc_SilentHours);
		break;

	case soc_PreGuard: 
		{
			/// if inner motion still exists, enter pre_Alarm, otherwise enter Guard
			HomeTetherDomain::MotionList mlist;
			size_t cInner = 0, cBed, cOuter;
			_domain.getMotionList(mlist, cBed, cOuter);
			cInner = mlist.size() - cOuter;

			moveToState((cInner >0) ? soc_PreAlarm : soc_Guard);
		}
		break;

	case soc_Guard: 
		// never timeout, simply renew
		setTimeout(STATE_TIMEOUT_Guard);
		break;

	case soc_SilentHours: 
		// if RTC hour gets out of [21:00 - 9:00], enter soc_Guard, else simply renew the timeout
		if (!isSilentHours())
			moveToState(soc_Guard);
		else setTimeout(STATE_TIMEOUT_SilentHours);
		break;

	case soc_PreAlarm:
		// timeout but no manual actions to release the warning, simply enter soc_Alarm
		moveToState(soc_Alarm);
		break;

	case soc_Alarm:
		// timeout but no manual action to release the alarm, enter soc_Guard again if inner motions still there
		{
			HomeTetherDomain::MotionList mlist;
			size_t cInner = 0, cBed, cOuter;
			_domain.getMotionList(mlist, cBed, cOuter);
			cInner = mlist.size() - cOuter;

			moveToState((cInner >0) ? soc_PreAlarm : soc_Guard);
		}
		break;
	}
}

void HtdState::OnMotionChanged(const HomeTetherDomain::MotionList& mlist, size_t cBed, size_t cOuter)
{
	debug("HtdState state[%s(%d)] OnMotionChanged() size[%d]: cBedroom[%d], cOuter[%d]\n", stateStr(_domain._state), _domain._state, mlist.size(), cBed, cOuter);
	switch(_domain._state)
	{
	case soc_Slave:
		break;
	
	case soc_Easy:
		// in easy state, refresh the Timeout_State if there are any motion detected in the inner area
		if ((mlist.size() - cOuter) >0)
			setTimeout(STATE_TIMEOUT_Easy);

		if (cOuter <=0)
			break;

		debug("blink: stay away this wall");
		break;

	case soc_PreGuard: 
		/// if inner motion still exists, enter pre_Alarm, otherwise enter Guard
		{
			HomeTetherDomain::MotionList mlist;
		size_t cInner = 0, cBed, cOuter;
		_domain.getMotionList(mlist, cBed, cOuter);
		cInner = mlist.size() - cOuter;

		moveToState((cInner >0) ? soc_PreAlarm : soc_Guard);
		}
		return;

	case soc_Guard: 
		if ((mlist.size() - cOuter) >0)
			moveToState(soc_PreAlarm);

		if (cOuter<=0)
			break;

		debug("HtdState::TODO warning: leave this wall");
		break;

	case soc_SilentHours: 
		// if motion is firstly occurs out of the bedroom or officeroom, enter soc_PreAlarm
		if (cBed >0)
			moveToState(soc_Easy);
		else if (mlist.size() - cOuter)
			moveToState(soc_PreAlarm);

		if (cOuter >0)
		{
		}

		break;

	case soc_PreAlarm:
	if (cOuter>0)
		debug("HtdState::TODO warning: leave this wall");
		break;

	case soc_Alarm:
		// timeout but no manual action to release the alarm, enter soc_Guard again if inner motions still there
		{
			HomeTetherDomain::MotionList mlist;
			size_t cInner = 0, cBed, cOuter;
			_domain.getMotionList(mlist, cBed, cOuter);
			cInner = mlist.size() - cOuter;

			moveToState((cInner >0) ? soc_PreAlarm : soc_Guard);
		}
		break;
	}
}

uint32_t HtdState::setTimeout(uint32_t timeout)
{
	if (timeout >0)
		return _domain.watch(timeout);
	return 0;
}

bool HtdState::isSilentHours()
{
#ifdef WIN32
	SYSTEMTIME time;
	GetLocalTime(&time);

	if (time.wHour < 9 || time.wHour >21)
		return true;
#else
	struct timeval tv;
	struct tm* ptm=NULL, tmret;
	gettimeofday(&tv,  (struct timezone*)NULL);
	ptm = localtime_r(&tv.tv_sec, &tmret);

	if (NULL != ptm && (ptm->tm_hour <9 || ptm->tm_hour >21))
		return true;
#endif

	return false;
}

