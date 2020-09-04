INSTDIR=/usr/opt/spectcl/5.3-008
DAQDIR=/usr/opt/daq/11.4-009
# Skeleton makefile for 3.1

include $(INSTDIR)/etc/SpecTcl_Makefile.include

#  If you have any switches that need to be added to the default c++ compilation
# rules, add them to the definition below:

USERCXXFLAGS= -std=c++11 -I$(INSTDIR)/include -I$(DAQDIR)/include

#  If you have any switches you need to add to the default c compilation rules,
#  add them to the defintion below:

USERCCFLAGS=$(USERCXXFLAGS)

#  If you have any switches you need to add to the link add them below:

USERLDFLAGS= 
CXXLDFLAGS= -L$(DAQLIB) -lFragmentIndex -ldataformat -ldaqio -lDataFlow -lurl -lException \
	-Wl,"-rpath=$(DAQLIB)" -g
#
#   Append your objects to the definitions below:
#

OBJECTS=MySpecTclApp.o CRawUnpacker.o CRingItemDecoder.o CPSDFragmentHandler.o CPHAFragmentHandler.o CMyEndOfEventHandler.o Parameters2.o 

#
#  Finally the makefile targets.
#


SpecTcl: $(OBJECTS)
	$(CXX)  -o SpecTcl $(OBJECTS) $(CXXLDFLAGS) $(USERLDFLAGS) \
	$(LDFLAGS) 


clean:
	rm -f $(OBJECTS) SpecTcl

depend:
	makedepend $(USERCXXFLAGS) *.cpp *.c

help:
	echo "make                 - Build customized SpecTcl"
	echo "make clean           - Remove objects from previous builds"
	echo "make depend          - Add dependencies to the Makefile. "
