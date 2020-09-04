# Location of CAENCOMM library:

CAENCOMM=../CAENComm-1.2
CAENCOMMINC=$(CAENCOMM)/include
CAENCOMMLIB=$(CAENCOMM)/lib

# Location of CAENVMElib:

CAENVME=../CAENVMELib-2.41
CAENVMEINC=$(CAENVME)/include
CAENVMELIB=$(CAENVME)/lib

# Location of CAEN Digitizer library:

CAENDGTZ=../CAENDigitizer_2.7.9
CAENDGTZINC=$(CAENDGTZ)/include
CAENDGTZLIB=$(CAENDGTZ)/lib

DAQROOT=/usr/opt/daq/11.3-018
NSCLDAQCXXFLAGS=-I$(DAQROOT)/include/sbsreadout
INIPARSERCXXFLAGS=-I../iniparser/include


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
	CAENPha.h CAENPha.cpp libpugi.a CompassProject.cpp CompassProject.h CompassEventSegment.cpp CompassEventSegment.h \
	CompassMultiModuleEventSegment.cpp CompassMultiModuleEventSegment.h  CompassTrigger.cpp CompassTrigger.h CAENPHAScalers.h
	g++ -c $(CAENCXXFLAGS) CAENPhaParameters.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) CAENPhaChannelParameters.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) CAENPha.cpp  -std=c++11
	g++ -c $(CAENCXXFLAGS) $(NSCLDAQCXXFLAGS)  \
				 CompassProject.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) $(NSCLDAQCXXFLAGS) $(INIPARSERCXXFLAGS)  \
				 CompassEventSegment.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) $(NSCLDAQCXXFLAGS)  \
				 CompassMultiModuleEventSegment.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) $(NSCLDAQCXXFLAGS)  \
				 CompassTrigger.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) $(NSCLDAQCXXFLAGS) \
				 CAENPHAScalers.cpp -std=c++11
	ar crs libCaenPha.a CAENPha.o  CAENPhaParameters.o \
		CAENPhaChannelParameters.o CompassProject.o CompassEventSegment.o CompassMultiModuleEventSegment.o CompassTrigger.o CAENPHAScalers.o


clean:
	rm -f libCaenPha.a
	rm -f *.o
