#ifndef __htdstate_H__
#define __htdstate_H__

#include "htdomain.h"

// -----------------------------
// basic state HtdState
// -----------------------------
class HtdState
{
//	friend class HomeTetherDomain;

public:
	HtdState(HomeTetherDomain& domain); //, StateOrdinalCode state);
	virtual ~HtdState() {}

	uint32_t setTimeout(uint32_t timeout);
	bool isSilentHours(); // whether it is in [21:00 - 9:00]

	void moveToState(StateOrdinalCode targeState);
	void OnTimer();
	void OnMotionChanged(const HomeTetherDomain::MotionList& mlist, size_t cBed, size_t cOuter);

protected:

//	virtual void OnEnter();
//	virtual void OnLeave() {}

	/*
	virtual void OnGlobalStateChanged(int nodeId, ResourceInfo eventInfo) {}
	virtual void OnDeviceStateChanged(int nodeId, ResourceInfo eventInfo) {}
	virtual void OnPulsesReceived(int nodeId, ResourceInfo eventInfo) {}
	virtual void OnHearbeat(int nodeId, ResourceInfo eventInfo) {}
	*/

//	virtual void OnTimer();
//	virtual void OnMotionChanged(const HomeTetherDomain::MotionList& list, size_t cBed, size_t cOuter) {}

protected:
	HomeTetherDomain& _domain;

public:
	static const char* stateStr(StateOrdinalCode state);
};

#endif // __htdstate_H__