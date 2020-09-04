/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2017.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Authors:
             Ron Fox
             Jeromy Tompkins 
	     NSCL
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

/** @file:  CRingItemDecoder.h
 *  @brief: Provide the interface for the class that decodes ring items.
 *  
 */

#ifndef CRINGITEMDECODER_H            // Multiple include gaurd.
#define CRINGITEMDECODER_H

/* forward class definitions. */

class CDppFragmentHandler;
class CEndOfEventHandler;
class CRingItem;
class CPhysicsEventItem;

// Ordinary C++ includes.

#include <map>
#include <cstdint>
#include <vector>
#include "CDppFragmentHandler.h"
#include "CEndOfEventHandler.h"
/*  CFragmentHandler defines the interface to concrete fragment handlers. */
/*  CEndOFEventHandler similarly defines the interface to concrete end handlers. */

/**
 * CRingItemDecoder - this class is independent of any data analysis
 *                    framework.  In root, for example, it can be used
 *                    directly given a pointer to a CRingItem object.  In
 *                    SpecTcl, you can construct a CRingItem from the return
 *                    value of the CRingBufferDecoder::getItemPointer method
 *                    and pass it to us as well.
 *
 *                    The decoder, for now just outputs as strings all
 *                    ring items that are not PHYSICS_EVENT items.  Those;
 *                    it assumes are event built data and iterates over the fragments.
 *                    calling registered handlers for each source id found.
 *                    If a source id does not have a registered handler, this will
 *                    just report that to stderr and ignore that fragment.
 **/
typedef enum _mode {ROOT, DUMP} ModeEnum;   

class CRingItemDecoder {
private:
    // m_fragmentHandlers maps source ids to fragment handsler object pointers:
    std::map<std::uint32_t, CFragmentHandler*> m_fragmentHandlers;

    std::vector<DppEvent> events;
    ModeEnum runMode;
    CEndOfEventHandler* m_endHandler;
    
public:
    CRingItemDecoder();
    
    void registerFragmentHandler(std::uint32_t sourceId, CFragmentHandler* pHandler);
    void registerEndHandler(CEndOfEventHandler* pHandler);
    std::vector<DppEvent> operator()(CRingItem* pItem);
    void setOutputMode(std::string mode){
	if(mode == "ROOT") runMode = ROOT;
	else 
	{
		runMode = DUMP;
		std::cout << "\nHead\t\tsize\tch\tTS\t\tEn\tEn.sh\tExtr";
		std::cout << "\n----------------------------------------------------------";
	}
    }
    // Handlers for ring item types:

protected:
    std::vector<DppEvent> decodePhysicsEvent(CPhysicsEventItem* pItem);
    void decodeOtherItems(CRingItem* pItem);
};

#endif
