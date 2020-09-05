/*
CRawUnpacker.cpp

Provides the member definitions for a wrapper class that reads from v1730 ADC data and prepares a CTreeParameterArray.
The CTreeParameterArray is readable by SpecTcl.

Most of the program's structure borrowed from http://docs.nscl.msu.edu/daq/newsite/nscldaq-11.2/x5013.html
B.Sudarsan 
sbalak2@lsu.edu
28 August 2018
*/

#include "CRawUnpacker.h"
#include <BufferDecoder.h>
#include <TCLAnalyzer.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <DataFormat.h>                    // Defines ring item types inter alia.
#include <CRingItem.h>                     // Base class for all ring items.
#include <CRingBufferDecoder.h>
#include <CRingItemFactory.h>
#include "CRingItemDecoder.h"              // Sample code.
#include "CPHAFragmentHandler.h"          // Handle s800 fragments.
#include "CPSDFragmentHandler.h"        // Handle CAESAR fragments.
#include "CMyEndOfEventHandler.h"          // Handle end of event processing.

//Constructor with initialization list
CRawUnpacker::CRawUnpacker()
  : m_values("PSD_PHA_e",16384,0.0,16383.0,"channels",64,0),
    m_timestamps("PSD_PHA_ts",0.0,"ns",64,0)

//Tree called "PSD_PHA_e", has 16+16 channels as branches, each storing an ADC output value in the range 0-16383. Channels 0-15 are from the PSD board, 16-31 are PHA.
//"PSD_PHA_ts" stores timestamps of all events w.r.t channel 13 of the PSD board, which could (say) stand for the scints in an expt
{
}
//Destructor
CRawUnpacker::~CRawUnpacker()
{
}

/*
CRawUnpacker::operator()()
Returns TRUE when event has been successfully parsed.

*/
Bool_t
CRawUnpacker::operator()(const Address_t pEvent, 
                        CEvent& rEvent, 
                        CAnalyzer& rAnalyzer, 
                        CBufferDecoder& rDecoder)
{
    CRingItemDecoder       decoder;
    CPSDFragmentHandler   psdhandler;
    CPHAFragmentHandler phahandler;
    CMyEndOfEventHandler     endhandler;
    
    decoder.registerFragmentHandler(62, &psdhandler);
    decoder.registerFragmentHandler(58, &phahandler);
    decoder.registerEndHandler(&endhandler);

    try {
	 CRingBufferDecoder& actualDecoder(dynamic_cast<CRingBufferDecoder&>(rDecoder));
         void* pRawRingItem = actualDecoder.getItemPointer();
         CRingItem* pRingItem = CRingItemFactory::createRingItem(pRawRingItem);
         std::vector<DppEvent> events = decoder(pRingItem);
	 for(int i=0; i<events.size(); i++)
	 {
		DppEvent event = events[i];
		m_values[event.s_data.first] = event.s_data.second;
		if(event.firmwareType == DppEvent::PSD && (event.Extras2&0x4))
			m_timestamps[event.s_data.first] = event.timeStamp + (event.Extras&0x1ff)*2*1e-3;			
		else 
			m_timestamps[event.s_data.first] = event.timeStamp;
	 }
         delete pRingItem;
    	}
    catch (int errcode) {
        
        std::cerr << "Ring item read failed: " << std::strerror(errcode) << std::endl;
	return kfFALSE;
    }
    catch (std::string msg) {
        std::cout << msg << std::endl;
	return kfFALSE;
    }

  return kfTRUE;


}



