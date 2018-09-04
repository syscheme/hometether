#ifndef __htdomain_H__
#define __htdomain_H__

#include "ht_comm.h"
#include <vector>
#include <queue>

#define MAX_NEXT_SLEEP (2000) // 2sec
#define HTD_NODEID (00)
#define DEBUG_CONST 1

// -----------------------------
// HomeTetherDomain
// -----------------------------
class HomeTetherDomain
{
	friend class DomainGuard;
	friend class HtdState;

public:
	typedef std::map <std::string, uint32_t> IntMap;  // var to value
	typedef std::vector <std::string> StrList;
	typedef std::queue <std::string>  StrQueue;
	typedef std::map <std::string, StrList>  StrMap;

	typedef struct _ResourceInfo
	{
		uint16_t    nodeId;
		std::string uri;
		uint32_t    stampLast, stampLastLog;
		IntMap      varMap;
	} ResourceInfo;

	typedef std::map <std::string, ResourceInfo> ResourceMap;    // uri to IntMap

	static std::string ResourceInfoToJSON(const HomeTetherDomain::ResourceInfo& ri);

	static bool& _bQuit;

	typedef struct _StatusLog
	{
		std::string resource;
		uint32_t intv;
		StrList varList;
	} StatusLog;

	// about the configuration lines "preference:"
	typedef struct _PreferenceRes
	{
		std::string resource;
		uint32_t    stampFlush;
		IntMap		varMap;
	} PreferenceRes;

	typedef std::map<uint8_t, PreferenceRes> PreferenceMap; // map from nodeId to ConfigResource that needs to push into the nodes

	typedef enum {
		st_motionInner =0,
		st_motionBedroom,
		st_motionOuter
	} SensorType;

	typedef struct _MotionFlag
	{
		std::string resource;
		SensorType  type;
		uint16_t    flag;
		std::string alias;
		std::string actionH, actionL;
	} MotionFlag;

	typedef std::map<uint32_t, MotionFlag> MotionFlagMap; // index = (nodeId <<16) | motion_bit_flag
	typedef std::map<uint32_t, std::string> ChannelAliasMap; // index = (nodeId <<16) | temp_ch_flag

	typedef std::vector <MotionFlag> MotionList;
	int  getMotionList(MotionList& list, size_t& cBedroom, size_t& cOuter);  
	void applyNodesPreference(uint8_t nodeId =0);
	
	void queueQuery(std::string query);

public:
	HomeTetherDomain();
	virtual ~HomeTetherDomain();

	void loadConfig(const char* pathname="/etc/htd.conf");
	void validateConfig();

private:
	ResourceMap _resourceMap;
	PreferenceMap _preferenceMap;
	MotionFlagMap _motionFlagMap;
	StrMap        _actionMap;
	ChannelAliasMap _tempChMap, _luminChMap;
#ifndef _WIN32
	pthread_mutex_t _lkNodeMap;
#endif // _WIN32

	typedef struct _VP
	{
		char varname[30];
		uint32_t val;
	} VP;

	typedef std::map<std::string, StatusLog> StatusLogMap; // resource To StatusLog
	StatusLogMap _statusLogMap;

	StrQueue _pendQueries;

	// thread-unsaft
	PreferenceMap::iterator _openPreference(uint32_t nodeId, bool createIfNotExist=false);

public:

	void registerStatusLog(StatusLog& stlog);
	virtual void logStatus();
	virtual void flushQueries();

	virtual void OnEcuPost(const char* url);

	std::string exportJSON(int nodeId=-1);

	virtual int doStateScan(void);

	uint32_t watch(uint32_t timeout);
	bool isExpired();

protected:
	StateOrdinalCode _state;
	uint32_t _stampExpire;
	bool _bRounded;
};

extern HomeTetherDomain _htdomain;

#ifndef MAPSET
#  define MAPSET(_MAPTYPE, _MAP, _KEY, _VAL) if (_MAP.end() ==_MAP.find(_KEY)) _MAP.insert(_MAPTYPE::value_type(_KEY, _VAL)); else _MAP[_KEY] = _VAL
#endif // MAPSET

// -----------------------------
// DomainGuard
// -----------------------------
class DomainGuard
{
public:
	DomainGuard(HomeTetherDomain& domain)
#ifndef _WIN32
		:_mutex(domain._lkNodeMap)
	{
		pthread_mutex_lock(&_mutex);
	}
#else
	{}
#endif // _WIN32
	virtual ~DomainGuard()
	{
#ifndef _WIN32
		pthread_mutex_unlock(&_mutex);
#endif // _WIN32
	}

private:
#ifndef _WIN32
	pthread_mutex_t& _mutex;
#endif // _WIN32
};

#endif // __htdomain_H__
