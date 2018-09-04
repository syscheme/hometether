#include "htdomain.h"
#include "TimeUtil.h"

bool bQuit =false;
bool& HomeTetherDomain::_bQuit = bQuit;

int main(int argc, char **argv)
{
	int64 time = ZQ::common::TimeUtil::ISO8601ToTime("2013-05-31T08:11:43.000043Z");
	time = ZQ::common::TimeUtil::ISO8601ToTime("2013-05-31T08:11:43.123043Z");

	_htdomain.loadConfig("./htd.conf");
	_htdomain.validateConfig();

	_htdomain.OnEcuPost("ht:10/ecuConfig?tempMask=ff&tempIntv=05&motionDiffMask=00ff");
	_htdomain.OnEcuPost("ht:20/ecuConfig?tempMask=ff&tempIntv=05&motionDiffMask=00ff");
	_htdomain.OnEcuPost("ht:10/ecuConfig?lumineDiffMask=ff&lumineDiffIntv=04&lumineDiffTshd=01f4");
	_htdomain.OnEcuPost("ht:10/adc?a2=0000&a3=0000");
	_htdomain.OnEcuPost("ht:10/adc?a7=0000&a5=0000");
	_htdomain.OnEcuPost("ht:10/EVT_dstate?lstate=1&mstate=00ff");

	std::string json = _htdomain.exportJSON();
	_htdomain.doStateScan();

	HomeTetherDomain::StatusLog sl;
	sl.resource = "20/ecuConfig";
	sl.intv = 10000;
	_htdomain.registerStatusLog(sl);
	 
	sl.resource = "20/adc";
	sl.intv = 3000;
	sl.varList.push_back("a5");
	sl.varList.push_back("a3");
	_htdomain.registerStatusLog(sl);

	_htdomain.logStatus();

	_htdomain.flushQueries();
	_htdomain.flushQueries();

	return 0;
}