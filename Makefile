# Location of CAENCOMM library:

CAENCOMM=CAENComm-1.2
CAENCOMMINC=$(CAENCOMM)/include
CAENCOMMLIB=$(CAENCOM)lib

# Location of CAENVMElib:

CAENVME=CAENVMELib-2.41
CAENVMEINC=$(CAENVME)/include
CAENVMELIB=$(CAENVME)/lib

# Location of CAEN Digitizer library:

CAENDGTZ=CAENDigitizer_2.7.9
CAENDGTZINC=$(CAENDGTZ)/include
CAENDGTZLIB=$(CAENDGTZ)/lib


# Caen compilation flags:

CAENCXXFLAGS= -I$(CAENDGTZINC) -I$(CAENVMEINC) -I$(CAENCOMMINC)
CAENLDFLAGS= 	-L$(CAENDGTZLIB) -lCAENDigitizer -Wl,-rpath=$(CAENDGTZLIB)   \
		-L$(CAENVMELIB)  -lCAENVME       -Wl,-rpath=$(CAENVMELIB)    \
		-L$(CAENCOMMLIB) -lCAENComm      -Wl,-rpath-$(CAENCOMMLIB)
all: parser

parser:  parser.cpp libCaenPha.a libpugi.a
	g++ -g -o parser  parser.cpp    \
	-L. -lCaenPha  -lpugi		\
	$(CAENLDFLAGS)                  \
	-std=c++11

libpugi.a: pugixml.cpp pugixml.hpp pugiconfig.hpp pugiutils.h pugiutils.cpp
	g++ -c pugixml.cpp -std=c++11
	g++ -c pugiutils.cpp -std=c++11
	ar crs  libpugi.a pugixml.o pugiutils.o


libCaenPha.a:  CAENPhaParameters.h CAENPhaParameters.cpp  CAENPhaChannelParameters.h CAENPhaChannelParameters.cpp \
	CAENPha.h CAENPHa.cpp											\
	libpugi.a
	g++ -c $(CAENCXXFLAGS) CAENPhaParameters.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) CAENPhaChannelParameters.cpp -std=c++11
	g++ -c $(CAENCXXFLAGS) CAENPha.cpp  -std=c++11
	ar crs libCaenPha.a CAENPha.o  CAENPhaParameters.o CAENPhaChannelParameters.o
