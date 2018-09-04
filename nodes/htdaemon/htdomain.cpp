#include "htdomain.h"
#include "htdstate.h"
#include "../htcan.h"

#ifdef _WIN32
#include "TimeUtil.h"
#endif
#include <algorithm>
// -----------------------------
// HomeTetherDomain
// -----------------------------
HomeTetherDomain _htdomain;

//#define info(...)  syslog(LOG_INFO, __VA_ARGS__) // printf(__VA_ARGS__)
#define debug(...) trace("Domain::" __VA_ARGS__) // printf(__VA_ARGS__)
//#define notice(...) syslog(LOG_NOTICE, __VA_ARGS__) // printf(__VA_ARGS__)


#ifndef _WIN32
extern int ecufd;
#endif // _WIN32

HomeTetherDomain::HomeTetherDomain()
: _state(soc_Easy)
{
#ifndef _WIN32
	pthread_mutexattr_t mattr;
	int ret = pthread_mutexattr_init(&mattr);
	pthread_mutex_init(&_lkNodeMap, &mattr);
#endif // _WIN32
}

HomeTetherDomain::~HomeTetherDomain()
{
#ifndef _WIN32
	pthread_mutex_destroy(&_lkNodeMap);
#endif // _WIN32
}


#define HAS_PREFIX(_STR, _PREFIX) (0 == strncmp(uri, #_PREFIX, sizeof(#_PREFIX)-1))

void HomeTetherDomain::OnEcuPost(const char* url)
{
	uint16_t nodeId =-1;
	char uri[64]="", tmpstr[64];
	VP   vp[6];

	debug("OnEcuPost() parsing: %s\n", url);
	int ret = sscanf(url, "ht:%x/%[^?]?%[^=&]=%x&%[^=&]=%x&%[^=&]=%x&%[^=&]=%x&%[^=&]=%x&%[^=&]=%x", &nodeId, uri,
		vp[0].varname, &vp[0].val, vp[1].varname, &vp[1].val, vp[2].varname, &vp[2].val,
		vp[3].varname, &vp[3].val, vp[4].varname, &vp[4].val, vp[5].varname, &vp[5].val);

	if (ret <3 || nodeId >0x7f || strlen(uri) <=0)
		return;

	snprintf(tmpstr, sizeof(tmpstr)-2, "%s@%x", uri, nodeId); // swap the uri to be ahead of nodeId to accelete indexing
	
	// event processing steps should be:
	// 1) firstly record the parameters into _resourceMap during message receiving
	// 2) after the event has been processed, the resource of this event would be removed from _resourceMap 
	if (HAS_PREFIX(uri, EVT_))
		debug("Domain::OnEcuPost() Event: %s\n", url);

	DomainGuard g(*this);

	ResourceMap::iterator itRes = _resourceMap.find(tmpstr);
	if (_resourceMap.end() == itRes)
	{
		ResourceInfo ri;
		ri.stampLastLog =0;
		ri.nodeId = nodeId; ri.uri = uri;
		_resourceMap.insert(ResourceMap::value_type(tmpstr, ri));
		itRes = _resourceMap.find(tmpstr);
	}

	if (_resourceMap.end() == itRes)
		return;

	itRes->second.stampLast = now();

	for (int i =0; i< (ret -2)/2; i++)
		MAPSET(IntMap, itRes->second.varMap, vp[i].varname, vp[i].val);
}

std::string HomeTetherDomain::ResourceInfoToJSON(const HomeTetherDomain::ResourceInfo& ri)
{
	char jbuf[100];
	snprintf(jbuf, sizeof(jbuf)-2, "{ \"nodeId\": %x, \"uri\": \"%s\", \"asof\": %x, \"metadata\": ", ri.nodeId, ri.uri.c_str(), ri.stampLast);
	return std::string(jbuf) + IntMapToJSON(ri.varMap) + " }";
}

std::string HomeTetherDomain::exportJSON(int nodeId)
{
	std::string jret;
	
	DomainGuard g(*this);
	for(ResourceMap::iterator itRes = _resourceMap.begin(); itRes != _resourceMap.end(); itRes++)
	{
		if (nodeId >0 && itRes->second.nodeId != nodeId)
			continue;

		jret += std::string(",\n") + ResourceInfoToJSON(itRes->second);
	}

	if (jret.length() >2)
		jret = jret.substr(2);

	return std::string("{ \"resources\": [\n") + jret + "\n] }";
}

void HomeTetherDomain::registerStatusLog(StatusLog& stlog)
{
	DomainGuard g(*this);

	std::sort(stlog.varList.begin(), stlog.varList.end()); 
	MAPSET(StatusLogMap, _statusLogMap, stlog.resource, stlog);
}

std::string swapResKeyToUri(const std::string& resKey)
{
	size_t pos = resKey.find('@');
	if (std::string::npos == pos)
		return resKey;
	return resKey.substr(pos+1) + "/" + resKey.substr(0, pos);
}

void HomeTetherDomain::logStatus()
{
	uint32_t stampNow = now();

	DomainGuard g(*this);
	for (StatusLogMap::iterator itConfig = _statusLogMap.begin(); itConfig != _statusLogMap.end(); itConfig++)
	{
		StatusLog& config = itConfig->second;
		std::string qstr;
		char varexp[200];
		for (size_t i=0; i< config.varList.size(); i++)
		{
			snprintf(varexp, sizeof(varexp)-2, "&%s=\0", config.varList[i].c_str());
			qstr += varexp;
		}

		if (!qstr.empty())
			qstr[0] = '?';
		qstr = std::string("GET ht:") + swapResKeyToUri(config.resource) + qstr;

		ResourceMap::iterator itRes = _resourceMap.find(config.resource);
		if (_resourceMap.end() == itRes)
		{
			_pendQueries.push(qstr); // already has domainguard
			continue;
		}

		ResourceInfo& ri = itRes->second;

		if (config.intv <= 0 || (ri.stampLastLog + config.intv) > stampNow || ri.varMap.size() <=0)
			continue;

		_pendQueries.push(qstr);

		if (ri.stampLast >0 && ri.stampLastLog > ri.stampLast)
			continue;

		ri.stampLastLog = stampNow;

		IntMap::iterator itVar;
		std::string statuslogLine;
		int c=0;

		if (config.varList.size() >0)
		{
			for (size_t i=0; i< config.varList.size(); i++)
			{
				itVar = ri.varMap.find(config.varList[i]);
				if (ri.varMap.end() == itVar)
					continue;

				snprintf(varexp, sizeof(varexp)-2, " %s=%x,", itVar->first.c_str(), itVar->second); 
				statuslogLine += varexp;
				c++;
			}
		}
		else
		{
			for (itVar=ri.varMap.begin(); itVar != ri.varMap.end(); itVar++)
			{
				snprintf(varexp, sizeof(varexp)-2, " %s=%x,", itVar->first.c_str(), itVar->second); 
				statuslogLine += varexp;
				c++;
			}
		}

		if (c <=0)
			continue;

		notice("SLOG[%s]:%s", config.resource.c_str(), statuslogLine.c_str());
	}
}

void HomeTetherDomain::flushQueries()
{
	std::string stdQuery;

	while (!_bQuit)
	{
		{
			DomainGuard g(*this);
			if (_pendQueries.empty())
				return;

			debug("%d pending queries\n", _pendQueries.size());

			stdQuery = _pendQueries.front();
			_pendQueries.pop();
		}

		debug("Domain::query: %s\n", stdQuery.c_str());
		stdQuery += "\r\n";
#ifndef _WIN32
		write(ecufd, stdQuery.c_str(), stdQuery.length());
//		write(ecufd, stdQuery.c_str(), stdQuery.length());
#endif // _WIN32

		if (!_bQuit)
			Sleep(1);
	}
}

void HomeTetherDomain::loadConfig(const char* pathname)
{
	FILE *file = fopen(pathname, "r"); 
	if (NULL == file)
	{
		notice("Domain::error: failed to open conf file %s\n", pathname);
		return;
	}

	char line[512]="";
	while(NULL != fgets(line, sizeof(line)-2, file))
	{
		char arg1[100], arg2[100], arg3[100], arg4[100], arg5[100], arg6[100];
		uint32_t nodeId =0, ival;
//		if ('#' == line[0] || sscanf(line, "%*[ \n\t]#%c", arg1) >=1) // ignore the comments
//			continue;

//		if (sscanf(line, "%*[ \n\t]logstate:%*[ \n\t]%[^;];%[^;];%[^ \t#]", arg1, arg2, arg3) >=3)
		char* p, *q;
//		if (sscanf(line, "logstate:%[^;];%[^;];%[^ \r\n\t#]", arg1, arg2, arg3) >=3
//			|| sscanf(line, "logstate:%*[ \n\t]%[^;];%[^;];%[^ \r\n\t#]", arg1, arg2, arg3) >=3)
		if (sscanf(line, "logstate:%*[ \n\t]%[^;];%[^;];%[^ \r\n\t#]", arg1, arg2, arg3) >=3)
		{
			StatusLog sl;
			p = arg3;
			sl.resource = arg1;
			sl.intv = atoi(arg2) *1000;

			while (q=strchr(p, ','))
			{
				*q++ = '\0';
				if (strlen(p)>0)
					sl.varList.push_back(p);
				p = q;
			}
			if (strlen(p)>0)
				sl.varList.push_back(p);

			_htdomain.registerStatusLog(sl);
			debug("Domain::loadConfig() stateLog: %s;%d;%d vars\n", sl.resource.c_str(), sl.intv/1000, sl.varList.size());
			continue;
		}

		// about preference line such as preference: ecuConfig@10;tempMask=00,tempIntv=12,motionDiffMask=ff,motionOuterMask=ff
		if (sscanf(line, "preference:%*[ \n\t]%[^@]@%[^;];%[^ \r\n\t#]", arg1, arg2, arg3) >=3)
		{
			PreferenceRes cres;
			nodeId =0xffff;
			hex2int(arg2, &nodeId);
			if (nodeId <0 || nodeId >0x7f)
				continue;
			
			snprintf(line, sizeof(line)-2, "%X/%s", nodeId, arg1);
			cres.resource = line;
			cres.stampFlush =0;

			p = arg3;
			while (q=strchr(p, ','))
			{
				char *eq = strchr(p, '=');
				*q++ = '\0';
				if (strlen(p) >0 && NULL !=eq && q >=eq)
				{
					*eq++ = '\0'; hex2int(eq, &ival);
					MAPSET(IntMap, cres.varMap, p, ival);
				}

				p = q;
			}

			if (strlen(p)>0)
			{
				char *eq = strchr(p, '=');
				if (strlen(p) >0 && NULL != eq)
				{
					*eq++ = '\0'; hex2int(eq, &ival);
					MAPSET(IntMap, cres.varMap, p, ival);
				}
			}

			MAPSET(PreferenceMap, _preferenceMap, nodeId, cres);

			debug("Domain::loadConfig() added preference: %s;%d vars\n", cres.resource.c_str(), cres.varMap.size());
			continue;
		}

		// about motion line such as motion: 0002@20;Outer2SE;O;Beep2SWOn;Beep2SWOff
		int nscanf = sscanf(line, "motion:%*[ \n\t]%d@%[^;];%[^;];%[^;];%[^;];%[^ \r\n\t#]", &ival, arg2, arg3, arg4, arg5, arg6);
		if (nscanf >=4)
		{
			MotionFlag mf;
			nodeId =0xffff;
			hex2int(arg2, &nodeId);
			mf.flag = 1 << (ival & 0xf);
			if (0 == mf.flag || nodeId <0 || nodeId >0x7f)
				continue;

			snprintf(line, sizeof(line)-2, "%X/%d", nodeId, (ival & 0xf));
			mf.resource = line;
			mf.alias = arg3;
			ival = (nodeId & 0x7f) << 16 | mf.flag;

			switch(arg4[0])
			{
			case 'B': mf.type = st_motionBedroom; break;
			case 'O': mf.type = st_motionOuter; break;
			case 'I': 
			default:
				mf.type = st_motionInner; break;
			}

			if (nscanf >4)
				mf.actionH = arg5;
			if (nscanf >5)
				mf.actionL = arg6;

			MAPSET(MotionFlagMap, _motionFlagMap, ival, mf);

			debug("Domain::loadConfig() added MotionFlag: %s\n", mf.resource.c_str());
			continue;
		}

		// about action line such as action: Beep2SWOn;PUT 10/ecuRelay?rOn0=10&rOff0=10
		nscanf = sscanf(line, "action:%*[ \n\t]%[^;];%[^\r\n\t#]", arg1, arg2);
		if (nscanf >=2)
		{
			if (strlen(arg1) <=0 || strlen(arg2) <=0)
				continue;

			StrList cmds;

			p = arg2;
			while (q=strchr(p, ','))
			{
				*q++ = '\0';
				if (strlen(p) >0)
					cmds.push_back(p);
			p = q;
			}

			if (strlen(p)>0)
					cmds.push_back(p);

			MAPSET(StrMap, _actionMap, arg1, cmds);

			debug("Domain::loadConfig() added action: %s;%d cmds\n", arg1, cmds.size());
			continue;
		}

		// about tempch line such as tempch: 001@10;temp2SE
		nscanf = sscanf(line, "tempch:%*[ \n\t]%d@%X;%[^ \r\n\t#]", &ival, &nodeId, arg3);
		if (nscanf >=3)
		{
			if (nodeId <=0 || strlen(arg3) <=0)
				continue;

			ival = (nodeId & 0x7f) <<16 | ival;

			MAPSET(ChannelAliasMap, _tempChMap, ival, arg3);
			debug("Domain::loadConfig() added action: %X;%s\n", ival, arg3);
			continue;
		}

		// about luminch line such as luminch: 1@10;Bedroom2SE
		nscanf = sscanf(line, "luminch:%*[ \n\t]%d@%X;%[^ \r\n\t#]", &ival, &nodeId, arg3);
		if (nscanf >=3)
		{
			if (nodeId <=0 || strlen(arg3) <=0)
				continue;

			ival = (nodeId & 0x7f) <<16 | ival;

			MAPSET(ChannelAliasMap, _luminChMap, ival, arg3);
			debug("Domain::loadConfig() added lumin: %X;%s\n", ival, arg3);
			continue;
		}
	}

	fclose(file);
}

HomeTetherDomain::PreferenceMap::iterator HomeTetherDomain::_openPreference(uint32_t nodeId, bool createIfNotExist)
{
	PreferenceMap::iterator itPref = _preferenceMap.find(nodeId);
	if (_preferenceMap.end() == itPref)
	{
		PreferenceRes cres;
		char resKey[20];
		snprintf(resKey, sizeof(resKey)-2, "%X/ecuConfig", nodeId);
		cres.resource = resKey;
		cres.stampFlush =0;
		MAPSET(PreferenceMap, _preferenceMap, nodeId, cres);
		itPref = _preferenceMap.find(nodeId);
	}

	return itPref;
}

void HomeTetherDomain::validateConfig()
{
	// convert the motionFlags to ecuConfig?motionDiffMask&motionBedMask&motionOuterMask
	uint32_t lastNode =0x00;
	uint32_t ival1 =0, ival2=0, ival3=0, flag=0;
	PreferenceMap::iterator itPref;

	ival1 = ival2= ival3 =0;
	for (MotionFlagMap::iterator itM = _motionFlagMap.begin(); itM != _motionFlagMap.end(); itM++)
	{
		uint32_t currentNodId = (itM->first >>16) & 0x7f;
		if (currentNodId != lastNode && lastNode >0)
		{
			itPref = _openPreference(lastNode, true);
			if (_preferenceMap.end() != itPref)
			{
				MAPSET(IntMap, itPref->second.varMap, "motionDiffMask",  ival1);
				MAPSET(IntMap, itPref->second.varMap, "motionBedMask",   ival2);
				MAPSET(IntMap, itPref->second.varMap, "motionOuterMask", ival3);
				debug("Domain::validateConfig() node[%X]: motionDiffMask[%X] motionBedMask[%X] motionOuterMask[%X]", lastNode, ival1, ival2, ival3);
			}

			ival1 = ival2= ival3 =0;
		}

		lastNode = currentNodId;

		flag = (itM->first& 0xffff); // the itM->first has been converted to flag already

		ival1 |= flag; // the motionDiffMask;
		if (st_motionBedroom == itM->second.type)
			ival2 |= flag; // the motionBedMask
		else if (st_motionOuter == itM->second.type)
			ival3 |= flag; // motionOuterMask
	}

	if (0 != ival1 && lastNode >0)
	{
		itPref = _openPreference(lastNode, true);
		if (_preferenceMap.end() != itPref)
		{
			MAPSET(IntMap, itPref->second.varMap, "motionDiffMask",  ival1);
			MAPSET(IntMap, itPref->second.varMap, "motionBedMask",   ival2);
			MAPSET(IntMap, itPref->second.varMap, "motionOuterMask", ival3);
			debug("Domain::validateConfig() node[%X]: motionDiffMask[%X] motionBedMask[%X] motionOuterMask[%X]", lastNode, ival1, ival2, ival3);
		}
	}

	// convert the tempch to ecuConfig?tempMask
	ival1 = ival2= ival3 =0;
	for (ChannelAliasMap::iterator itT = _tempChMap.begin(); itT != _tempChMap.end(); itT++)
	{
		uint32_t currentNodId = (itT->first >>16) & 0x7f;
		if (currentNodId != lastNode && lastNode >0)
		{
			itPref = _openPreference(lastNode, true);
			if (_preferenceMap.end() != itPref)
			{
				MAPSET(IntMap, itPref->second.varMap, "tempMask",  ival1);
				debug("Domain::validateConfig() node[%X]: tempMask[%X]", lastNode, ival1);
			}

			ival1 =0;
		}

		lastNode = currentNodId;
		flag = 1 << (itT->first& 0xf);

		ival1 |= flag; // the tempMask;
	}

	if (ival1 && lastNode >0)
	{
		itPref = _openPreference(lastNode, true);
		if (_preferenceMap.end() != itPref)
		{
			MAPSET(IntMap, itPref->second.varMap, "tempMask",  ival1);
			debug("Domain::validateConfig() node[%X]: tempMask[%X]", lastNode, ival1);
		}
	}

	// convert the luminch to ecuConfig?lumineDiffMask
	ival1 = ival2= ival3 =0;
	for (ChannelAliasMap::iterator itT = _luminChMap.begin(); itT != _luminChMap.end(); itT++)
	{
		uint32_t currentNodId = (itT->first >>16) & 0x7f;
		if (currentNodId != lastNode && lastNode >0)
		{
			itPref = _openPreference(lastNode, true);
			if (_preferenceMap.end() != itPref)
			{
				MAPSET(IntMap, itPref->second.varMap, "lumineDiffMask",  ival1);
				debug("Domain::validateConfig() node[%X]: lumineDiffMask[%X]", lastNode, ival1);
			}

			ival1 =0;
		}

		lastNode = currentNodId;
		flag = 1 << (itT->first& 0xf);

		ival1 |= flag; // the tempMask;
	}

	if (ival1 && lastNode >0)
	{
		itPref = _openPreference(lastNode, true);
		if (_preferenceMap.end() != itPref)
		{
			MAPSET(IntMap, itPref->second.varMap, "lumineDiffMask",  ival1);
			debug("Domain::validateConfig() node[%X]: lumineDiffMask[%X]", lastNode, ival1);
		}
	}
}

void HomeTetherDomain::applyNodesPreference(uint8_t nodeId)
{
	/// End of validating, prepare the preference flush cmds
	for (PreferenceMap::iterator itPref = _preferenceMap.begin(); itPref != _preferenceMap.end(); itPref++)
	{
		if (nodeId >0 && itPref->first != nodeId)
			continue;

		char queryBuf[100], *p=queryBuf;
		snprintf(queryBuf, sizeof(queryBuf)-2, "PUT ht:%X/ecuConfig?", itPref->first);
		std::string qstr = queryBuf;
		for (IntMap::iterator itV = itPref->second.varMap.begin(); itV != itPref->second.varMap.end(); itV++)
		{
			p += snprintf(p, queryBuf + sizeof(queryBuf)- p -2, "&%s=%X", itV->first.c_str(), itV->second);
			if ((p - queryBuf) >50)
			{
				queueQuery(qstr + (queryBuf+1));
				p = queryBuf;
			}
		}

		if (p > queryBuf)
			queueQuery(qstr + (queryBuf+1));
	}
}

uint32_t HomeTetherDomain::watch(uint32_t timeout)
{
	uint32_t stampNow = now();
	_stampExpire = stampNow + timeout;
	_bRounded = (_stampExpire < stampNow);
	return _stampExpire;
}

bool HomeTetherDomain::isExpired()
{
	uint32_t stampNow = now();
	if (_bRounded && stampNow <= _stampExpire)
		_bRounded = false;

	return (!_bRounded) && (stampNow>=_stampExpire);
}

int HomeTetherDomain::doStateScan(void)
{
	static uint32_t _lastHtdHeartbeat =0;
	uint32_t stampNow = now();

	// step 0. send the master heartbeat
	if ((stampNow - _lastHtdHeartbeat) > (DEFAULT_HeartbeatTimeout >>1))
	{
		char masterHB[60];
		snprintf(masterHB, sizeof(masterHB)-2, "POST can:%XN03V%02X%02X%02X\r\n\0", fcNODE_GUARD<<7 | HTD_NODEID, HtCanState_Operational, HTD_NODEID, _state);
		debug("Domain::MasterHB[%s(%d)] %s", HtdState::stateStr(_state), _state, masterHB);
#ifndef _WIN32
		write(ecufd, masterHB, strlen(masterHB));
		write(ecufd, masterHB, strlen(masterHB)); // heartbeat twice
#endif // _WIN32

		if (0 != _lastHtdHeartbeat)
		{
			static uint32_t _lastApplyPreference =0;
			if ((stampNow - _lastApplyPreference) > VARCONST(1000*10, 1000*60*10)) // every 10 min
			{
				// flush the preference of all node
				applyNodesPreference(0);
				_lastApplyPreference = stampNow;
			}
		}

		_lastHtdHeartbeat = stampNow;
	}

	int nextSleep = MAX_NEXT_SLEEP;

	// step 1. process all Events that ever recevied
	// step 1.1 reads and also clean all events from _resourceMap
	ResourceMap eventMap;
	{
		DomainGuard g(*this);
		ResourceMap::iterator itEventStart = _resourceMap.lower_bound("EVT_");
		ResourceMap::iterator itEventEnd = _resourceMap.upper_bound("EVTz"); // ''' is the charactor next to '_'
		for (ResourceMap::iterator it = itEventStart; it != itEventEnd; it++)
			eventMap.insert(*it);
		_resourceMap.erase(itEventStart, itEventEnd);
	}

	// step 1.2 process each event
	char queryBuf[200] ="";
	for (ResourceMap::iterator itEvent = eventMap.begin(); itEvent != eventMap.end(); itEvent++)
	{
		// parse for the event name and nodeId
		std::string eventName = itEvent->first;
		char buf[100];
		int nodeId;
		int ret = sscanf(eventName.c_str(), "EVT_%[^@]@%x", buf, &nodeId);
		if (ret < 1)
			nodeId =-1;
		else if (ret >0)
			eventName = buf;
		
		// dispatch per the event type
		if (nodeId >0 && (0 == eventName.compare("hb")))
		{
			// flush the preference of the node to it
			applyNodesPreference(nodeId);
		}
		else if (0 == eventName.compare("gstate"))
		{
			_lastHtdHeartbeat =0;
		}
		else if (0 == eventName.compare("dstate"))
		{
			uint32_t lstate =itEvent->second.varMap["lstate"], mstate =itEvent->second.varMap["mstate"];

			// copy the lstate and mstate into resourceMap
			{
				char resKey[60];
				snprintf(resKey, sizeof(resKey) -2, "local@%X", nodeId);

				DomainGuard g(*this);
				ResourceMap::iterator itres = _resourceMap.find(resKey);
				if (_resourceMap.end() == itres)
				{
					ResourceInfo ri;
					ri.nodeId = nodeId;
					ri.stampLast = stampNow;
					ri.stampLastLog =0;
					ri.uri = "local";
					_resourceMap.insert(ResourceMap::value_type(resKey, ri));
					itres = _resourceMap.find(resKey);
				}

				if (_resourceMap.end() != itres)
				{
					MAPSET(IntMap, itres->second.varMap, "lstate", lstate);
					MAPSET(IntMap, itres->second.varMap, "mstate", mstate);
				}
			}

			if (dsf_MotionChanged & lstate)
			{
				// summary the motion flags and dispatch per state
				MotionList mlist;
				size_t cBed, cOuter;
				if (getMotionList(mlist, cBed, cOuter) >0)
				{
					std::string aliasList;
					for (size_t i =0; i < mlist.size(); i++)
						aliasList += std::string(", ") + mlist[i].alias;
					
					aliasList = aliasList.substr(2);
					debug("Domain::Event[motion]: %s", aliasList.c_str());
					HtdState(*this).OnMotionChanged(mlist, cBed, cOuter);
				}
			}

			if (dsf_LuminChanged & lstate)
			{
				// refresh all ADC values
				snprintf(queryBuf, sizeof(queryBuf)-2, "GET ht:%X/adc?a0=&a1=&a2=&a3=&a4=&a5=&a6=&a7=\r\n\0", nodeId);
				queueQuery(queryBuf);
			} // OnLuminChanged(...);

			if (dsf_IrTxEmpty & lstate)
			{} // OnIrTxEmpty(...);

			if (dsf_IrRxAvail & lstate)
			{} // OnIrRxAvail(...);

	/* ASK Devices
			if (0 == dsf_Window & lstate)
			{} // OnWindow(...);

			if (dsf_GasLeak & lstate)
			{} // OnGasLeak(...);

			if (dsf_Smoke & lstate)
			{} // OnSmoke(...);

			if (dsf_OtherDangers & lstate)
			{} // OnOtherDangers(...);
	*/
		}
		else if (0 == eventName.compare("pulses"))
		{
		}

		Sleep(1);
	} // end of event process, yield cpu shortly

	// step 2. scan the _resourceMap collected, see if there are anything need to do
	{
		DomainGuard g(*this);
	}

	// step 3. the last scan step is about the timeout of state
	if (isExpired())
		HtdState(*this).OnTimer();
	
	// END OF SCANNING, now exec the actions needed
	// ----------------------------------------------

	// step e.1. flush all the pending queries to the tty
	flushQueries();
	Sleep(1);

	// step e.2. log the status upon the configuration
	logStatus();

	// step 5. determin the next sleep
	return nextSleep;
}

void HomeTetherDomain::queueQuery(std::string query)
{
	if (query.empty())
		return;

//	if (std::string::npos == query.find_first_of("\r\n"))
//		query += "\r\n\0";

	DomainGuard g(*this);
	_pendQueries.push(query);
}

int HomeTetherDomain::getMotionList(MotionList& list, size_t& cBedroom, size_t& cOuter)
{
	list.clear();
	cBedroom= cOuter=0;

	// read the mstate of the nodes
	ResourceMap::iterator itLS = _resourceMap.lower_bound("local@");
	ResourceMap::iterator itResEnd = _resourceMap.upper_bound("local@g"); // the nodeId should be hex string

	for (; itLS != itResEnd; itLS++)
	{
		uint32_t currentNodeId = itLS->second.nodeId;
		uint32_t mstateOfNode = itLS->second.varMap["mstate"];
	
		// check each active flag
		uint32_t flag = 1;
		for (size_t i =0; i <16; i++, flag <<=1)
		{
			if (0 == (flag & mstateOfNode))
				continue;

			// look for this active flag in _motionFlagMap
			MotionFlagMap::iterator itM = _motionFlagMap.find( (currentNodeId <<16) | flag);
			if (_motionFlagMap.end() == itM)
				continue;
			
			// put it into the result if known in _motionFlagMap
			list.push_back(itM->second);
			if (st_motionBedroom == itM->second.type)
				cBedroom++;
			else if (st_motionOuter == itM->second.type)
				cOuter++;
		}
	}

	return list.size();
}

