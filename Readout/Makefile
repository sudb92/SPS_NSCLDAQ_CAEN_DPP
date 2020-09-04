#
#  This establishes which version of NSCLDAQ we're using and where it's installed:
#  Supposedly  you only need to change this definition to update to a newer
#  version of the software:

#INSTDIR=/usr/opt/daq/11.3-018
INSTDIR=/usr/opt/daq/11.4-009

include $(INSTDIR)/etc/SBSRdoMakeIncludes


#
#  The following allow you to add flags to all the default C and C++ compilation
#  rules.  In most cases C and C++ share the same set of compilation flags so:
#
#

CAENCXXFLAGS=-g -std=c++11 \
	-I../CAENComm-1.2/include \
	-I../CAENDigitizer_2.9.1/include \
	-I../CAENVMELib-2.41/include 


USERCCFLAGS=-I.. $(CAENCXXFLAGS) -I../DPP-PSD -I../DPP-PHA
USERCXXFLAGS=$(USERCCFLAGS)

#  If you have additional load flags (e.g. library dirs and libs):

CAENLDFLAGS=-L../CAENDigitizer_2.9.1/lib -lCAENDigitizer \
			-Wl,-rpath=../CAENDigitizer_2.9.1/lib \
	-L../CAENVMELib-2.41/lib -lCAENVME -Wl,-rpath=../CAENVMELib-2.41/lib \
	-L../CAENComm-1.2/lib -lCAENComm -Wl,-rpath=../CAENComm-1.2/lib

USERLDFLAGS= -L../DPP-PSD -L../DPP-PHA -lCaenPsd -lpugi $(CAENLDFLAGS) -lCaenPha


all: Readout psdregdump pharegdump

#
#  This is a list of the objects that go into making the application
#  Make, in most cases will figure out how to build them:

OBJECTS=Skeleton.o

Readout: $(OBJECTS)
	$(CXXLD) -o Readout $(OBJECTS) $(USERLDFLAGS) $(LDFLAGS)

psdregdump: psdregdump.cpp
	$(CXX) -o psdregdump psdregdump.cpp $(CAENLDFLAGS) $(CAENCXXFLAGS)

pharegdump: pharegdump.cpp
	$(CXX) -o pharegdump pharegdump.cpp $(CAENLDFLAGS) $(CAENCXXFLAGS)

clean:
	rm -f $(OBJECTS) Readout psdregdump pharegdump

depend:
	makedepend $(USERCXXFLAGS) *.cpp *.c 


help:
	echo make           - Build Readout.
	echo make clean     - Remove products from previous builds.
	echo make depend    - Add header dependencies to Makefile.

