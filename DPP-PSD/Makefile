all: libpugi.a libCaenPsd.a

libpugi.a:  pugixml.cpp pugixml.hpp pugiconfig.hpp pugiutils.h pugiutils.cpp
	g++ -c pugixml.cpp -std=c++11
	g++ -c pugiutils.cpp -std=c++11
	ar crs  libpugi.a pugixml.o pugiutils.o
	ranlib libpugi.a

CAENCXXFLAGS=-g -I. -std=c++11 \
	-I../CAENComm-1.2/include \
	-I../CAENDigitizer_2.9.1/include \
	-I../CAENVMELib-2.41/include \
	-I/usr/opt/daq/11.3-018/include \
	-I/usr/opt/daq/11.3-018/include/sbsreadout


CAENLDFLAGS= -lpugi -L.

libCaenPsd.a: PSDParameters.cpp CDPpPsdEventSegment.cpp \
		CPsdCompoundEventSegment.cpp CPsdTrigger.cpp \
		CCompoundTrigger.cpp COneOnlyEventSegment.cpp CAENPSDScalers.cpp
	g++ -c $(CAENCXXFLAGS) PSDParameters.cpp
	g++ -c $(CAENCXXFLAGS) CDPpPsdEventSegment.cpp
	g++ -c $(CAENCXXFLAGS) CPsdCompoundEventSegment.cpp
	g++ -c $(CAENCXXFLAGS) CPsdTrigger.cpp
	g++ -c $(CAENCXXFLAGS) CCompoundTrigger.cpp
	g++ -c $(CAENCXXFLAGS) COneOnlyEventSegment.cpp
	g++ -c $(CAENCXXFLAGS) CAENPSDScalers.cpp
	ar crs libCaenPsd.a PSDParameters.o CDPpPsdEventSegment.o \
				CPsdCompoundEventSegment.o CPsdTrigger.o \
				CCompoundTrigger.o COneOnlyEventSegment.o CAENPSDScalers.o
	ranlib libCaenPsd.a

clean: 
	rm -f *.o
	rm -f *.a