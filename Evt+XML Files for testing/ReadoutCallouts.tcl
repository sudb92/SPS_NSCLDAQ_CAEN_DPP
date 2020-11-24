package require evbcallouts
#package require evbrdocallouts

proc OnStart {} {
  ::EVBC::useEventBuilder
  ::EVBC::initialize -gui 1 -restart true -destring evbout -glombuild true -glomdt 50000
}
EVBC::configParams window 20;
#proc startEVBSources {} {
#EVBC::startRingSource tcp://localhost/daq2 {} [list 1 3] {"PSD"}
#}
#::EVBC::registerRingSource sourceURI tstamplib ids info ?expectHeaders? ?oneshot? ?timeout? ?timeoffset?

EVBC::registerRingSource tcp://localhost/daq2 {} [list 0 1 2] {"PSD-PHA"} 1 1 0 0 
