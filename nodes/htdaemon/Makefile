# PLATFORM_CC := arm-linux-
CXX := $(PLATFORM_CC)g++ -c -g
LD  := $(PLATFORM_CC)g++ -lpthread -o
RM  := rm -rf 
CFLAGS := -I../../stm32/common/
CPDBG := $(PLATFORM_CC)objcopy --only-keep-debug
STRIPDBG := $(PLATFORM_CC)objcopy --strip-debug

all : htd htc

htd : htd.o getopt.o htdomain.o htdstate.o ht_comm.o
	$(LD) htd htd.o getopt.o ht_comm.o htdomain.o htdstate.o
	$(CPDBG) htd htd.dbg
	$(STRIPDBG) htd
	$(PLATFORM_CC)objcopy --add-gnu-debuglink=htd.dbg htd
 
htc : htc.o getopt.o htc_cmds.o ht_comm.o
	$(LD) htc htc.o getopt.o ht_comm.o htc_cmds.o
	$(CPDBG) htc htc.dbg
	$(STRIPDBG) htc
	$(PLATFORM_CC)objcopy --add-gnu-debuglink=htc.dbg htc
	grep SubCmd__ htc.h | sed "s/int\s*SubCmd__\([^)]*\)(.*/ln -sf htc \1/g" > htc_symblnk.sh
	grep SubCmd__ htc.h | sed "s/int\s*SubCmd__\([^)]*\)(.*/$(RM) htc \1/g" > htc_symbclr.sh
	. ./htc_symblnk.sh && rm -rf ./htc_symblnk.sh

htd.o : htd.cpp ht_comm.h
	$(CXX) $(CFLAGS) htd.cpp

htc.o : htc.cpp ht_comm.h htc.h
	$(CXX) $(CFLAGS) htc.cpp

htc_cmds.o : htc.cpp ht_comm.h htc.h
	$(CXX) $(CFLAGS) htc_cmds.cpp

htdomain.o : htdomain.cpp htdomain.h
	$(CXX) $(CFLAGS) htdomain.cpp

htdstate.o : htdstate.cpp htdstate.h htdomain.h
	$(CXX) $(CFLAGS) htdstate.cpp

ht_comm.o : ht_comm.cpp ht_comm.h
	$(CXX) $(CFLAGS) ht_comm.cpp

getopt.o : ../../stm32/common/getopt.cpp
	$(CXX) $(CFLAGS) ../../stm32/common/getopt.cpp

clean :
	if [ -e ./htc_symbclr.sh ]; then . ./htc_symbclr.sh; fi
	$(RM) htc htd getopt.o htc.o htd.o htc_cmds.o htdomain.o htc_symblnk.sh htc_symbclr.sh
	
