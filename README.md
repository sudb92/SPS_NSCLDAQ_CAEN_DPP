### NSCLDAQ-CAEN-DPP
----------------

* A collection of utilities that interface CAEN digitizers 1730/25 running DPP-PHA or DPP-PSD firmwares with NSCLDAQ
* List of utilities
	+ XML Editor
	+ SpecTcl-Analyser
	+ scalerDisplay
	+ Readout
	+ RawDPPRingAnalyser-ROOT
	+ EvbDPPRingAnalyser-ROOT
* List of dependencies
	+ nscldaq-11.4-009 (The version tested on)
	+ spectcl-5.3-008
	+ pugixml-1.8
	+ CAENVMELib-2.41
	+ CAENDigitizer_2.7.9 or CAENDigitizer_2.7.9
	+ CAENComm-1.2

----------------------------------
* Brief description of utilities:

#### XML Editor
----------

* Run from terminal as 'tclsh tweaker.tcl'
* Allows real time edits to an XML file generated by Compass

#### SpecTcl-Analyser
----------------

+ Borrows from the ReadNSCLDAQFiles framework in http://docs.nscl.msu.edu/daq/newsite/nscldaq-11.4/c4826.html
+ Abstract class CFragmentHandler inherits to class CDppFragmentHandler which can hold/return a DppEvent
+ CDppFragmentHandler inherits to CPSDFragmentHandler and CPHAFragmentHandler
+ The unique size for PSD and PHA fragments is used by CRawUnpacker.cpp to distinguish them with 	CRingItemDecoder
+ TODO: PSD Fine timestamp lives in 'Extras'- Needs better implementation before testing with focalplane signals
+ Run as ./SpecTcl 

#### ScalerDisplay
-------------

+ Readout updates counts in increments of 128, 1024 or 8192 events per channel
+ TODO: Edit readout to store time taken between N events, and use them to better estimate rates?
+ Run as ScalerDisplay scaler.def from terminal

#### RawRingAnalyser-DPP
------------------------

+ Parses unbuilt DPP ringbuffer data to either print a formatted stream to the console, or save it into a ROOT tree
+ Format: ./Analyser <file/ringname> <option>
+ open: ROOT - save to root tree, DUMP - print to screen
+ Compass tree is saved with the naming template compass_run_x.root in the directory specified in evt2root_input.txt

#### EvbRingAnalyser-DPP
------------------------

+ Parses eventbuilt DPP ringbuffer data, usage same as above


#### Readout
-------

+ Kindly refer to the README within the Readout folder

