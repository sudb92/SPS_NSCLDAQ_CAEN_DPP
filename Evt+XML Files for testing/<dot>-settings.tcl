set EventLogger /usr/opt/daq/11.4-009/bin/eventlog
set EventLoggerRing tcp://localhost/evbout
set EventLogUseNsrcsFlag 1
set EventLogAdditionalSources 1
set EventLogUseGUIRunNumber 0
set EventLogUseChecksumFlag 1
set EventLogRunFilePrefix run
set StageArea /home/daq2/Desktop/NSCLDAQ-CAEN-DPP/evtlog_evb
set run 9
set title Test
set recording 0
set timedRun 0
set duration 1200
set dataSources {{host localhost parameters --no-barriers path /home/daq2/Desktop/NSCLDAQ-CAEN-DPP/Readout-Nov2020-PulserTest3boards/Readout provider SSHPipe sourceid 0 wdir /home/daq2/Desktop/NSCLDAQ-CAEN-DPP/Readout-Nov2020-PulserTest3boards}}
set segmentsize 2
