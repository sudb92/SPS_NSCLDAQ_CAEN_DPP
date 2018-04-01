# Location of CAENCOMM library:

CAENCOMM=CAENComm-1.2
CAENCOMMINC=$(CAENCOMM)/include
CAENCOMMLIB=$(CAENCOMM)/lib

# Location of CAENVMElib:

CAENVME=CAENVMELib-2.41
CAENVMEINC=$(CAENVME)/include
CAENVMELIB=$(CAENVME)/lib

# Location of CAEN Digitizer library:

CAENDGTZ=CAENDigitizer_2.7.9
CAENDGTZINC=$(CAENDGTZ)/include
CAENDGTZLIB=$(CAENDGTZ)/lib

DAQROOT=/usr/opt/daq/11.2-005
NSCLDAQCXXFLAGS=-I$(DAQROOT)/include/sbsreadout
INIPARSERCXXFLAGS=-Iiniparser/include


# Caen compilation flags:

CAENCXXFLAGS= -g -I$(CAENDGTZINC) -I$(CAENVMEINC) -I$(CAENCOMMINC)
CAENLDFLAGS=  -g -L$(CAENDGTZLIB) -lCAENDigitizer -Wl,-rpath=$(CAENDGTZLIB)   \
		-L$(CAENVMELIB)  -lCAENVME       -Wl,-rpath=$(CAENVMELIB)    \
		-L$(CAENCOMMLIB) -lCAENComm      -Wl,-rpath=$(CAENCOMMLIB)
all: libpugi.a libCaenPha.a



libpugi.a: pugixml.cpp pugixml.hpp pugiconfig.hpp pugiutils.h pugiutils.cpp
	g++ -c pugixml.cpp -std=c++11
	g++ -c pugiutils.cpp -std=c++11
	ar crs  libpugi.a pugixml.o pugiutils.o


libCaenPha.a:  CAENPhaParameters.h CAENPhaParameters.cpp  CAENPhaChannelParameters.h CAENPhaChannelParameters.cpp \
	CAENPha.h CAENPha.cpp PHAEventSegment.h PHAEventSegment.cpp config.h config.cpp DPPConfig.h DPPConfig.cpp \
	PHATrigger.h PHATrigger.cpp PHAMultiModuleSegment.h PHAMultiModuleSegment.cpp \
	libpugi.a 
	g++ -c $(CAENCXXFLAGS) CAENPhaParameters.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) CAENPhaChannelParameters.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) CAENPha.cpp  -std=c++11
	g++ -c $(CAENCXXFLAGS) $(INIPARSERCXXFLAGS) config.cpp
	g++ -c $(CAENCXXFLAGS) $(INIPARSERCXXFLAGS) DPPConfig.cpp
	g++ -c $(CAENCXXFLAGS) $(NSCLDAQCXXFLAGS) $(INIPARSERCXXFLAGS) \
				 PHAEventSegment.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) $(NSCLDAQCXXFLAGS)  \
				 PHATrigger.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) $(NSCLDAQCXXFLAGS)  \
				 PHAMultiModuleSegment.cpp -std=c++11
	ar crs libCaenPha.a config.o DPPConfig.o CAENPha.o  CAENPhaParameters.o \
		CAENPhaChannelParameters.o PHAEventSegment.o PHATrigger.o PHAMultiModuleSegment.o


clean:
	rm -f libCaenPha.a
	rm -f *.o