#ifndef __HTC_H__
#define __HTD_H__

#include "ht_comm.h"
#include "getopt.h"
#include <map>

#define FMT_PATTERN_HEXVAL     "%[0-9A-Za-z]"
#define FMT_PATTERN_DECVAL     "%[0-9]"

#define MAX_VALUES     (20)
#define MAX_VALUE_LEN  (64)

// configurations
extern bool bCGI, bJSON;
extern std::string htd_server;
extern int htd_port, htd_timeout;

// -----------------------------
// class HTClient
// -----------------------------
class HTClient
{
public:
	HTClient(const char*bindIP =NULL);
	virtual ~HTClient();

	const char* readValueAt(uint8_t index);

	bool connect(const char* server="localhost", int serverPort=HTD_SERVER_PORT);

	int exec(const char* req, const char* respFmt, int cValExpected, int timeout);
	int exec2(const char* req, const char* respFmt, int cValExpected, int timeout);

protected:
	int _so;
	bool _bConnected;
	char _valuebuf[MAX_VALUES*MAX_VALUE_LEN];
	char* _values[MAX_VALUES];

	void resetValueList();
};

// -----------------------------
// Sub-Commands
// -----------------------------
void usage(const char* cmd);
int  SubCmd__hello(int argc, char *argv[]);
int  SubCmd__getADC(int argc, char *argv[]);
int  SubCmd__getTemp(int argc, char *argv[]);
int  SubCmd__getMotion(int argc, char *argv[]);
int  SubCmd__getRBuf(int argc, char *argv[]);
int  SubCmd__getState(int argc, char *argv[]);
int  SubCmd__setGState(int argc, char *argv[]);
int  SubCmd__getConfig(int argc, char *argv[]);
int  SubCmd__setConfig(int argc, char *argv[]);
int  SubCmd__reset(int argc, char *argv[]);
int  SubCmd__htcs(int argc, char *argv[]);
int  SubCmd__chstate(int argc, char *argv[]);

#endif // __HTD_H__