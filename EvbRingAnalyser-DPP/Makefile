
# If you setup a different DAQ I believe
# the definitions below get overidden:

DAQROOT=/usr/opt/daq/11.4-009
DAQLIB=$(DAQROOT)/lib
DAQINC=$(DAQROOT)/include

CXXFLAGS = -I$(DAQINC) -std=c++11 -g `root-config --cflags`

CXXLDFLAGS= -L$(DAQLIB) -lFragmentIndex -ldataformat -ldaqio -lDataFlow -lurl -lException \
	-Wl,"-rpath=$(DAQLIB)" -g `root-config --glibs`



#-----------------

# Note this Makefile does not take into account header dependencies
# running makedepend would add those.

OBJECTS= CPHAFragmentHandler.o \
	CMyEndOfEventHandler.o \
	CDPPRingItemDecoder.o \
	CPSDFragmentHandler.o \
	Main.o


Analyzer: $(OBJECTS)
	$(CXX) -o Analyzer $(OBJECTS) $(CXXLDFLAGS)



clean:
	rm -f Analyzer *.o
