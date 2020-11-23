/*
*-------------------------------------------------------------
 
 CAEN SpA 
 Via Vetraia, 11 - 55049 - Viareggio ITALY
 +390594388398 - www.caen.it

------------------------------------------------------------

**************************************************************************
* @note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the 
* software, documentation and results solely at his own risk.
*
* @file     CAENPHAScalers.cpp
* @brief    Implement the scaler readout class for CAENPHA compass projects.
* @author   Ron Fox
*
*/
#include "CAENPHAScalers.h"
#include  "CompassEventSegment.h"

/**
 * constructor
 *   @param pEventSegment - the associated CompassEvent segment.
 *   @param srcid         - Source id set for scaler events. Defaults to 0.
 */
CAENPHAScalers::CAENPHAScalers(CompassEventSegment* pEventSegment, int srcid) :
    m_pSegment(pEventSegment),
    m_nSourceId(srcid)
{}
/**
 * read
 *    Just grab the counters from the event segment.
 *    note these must be configured as non-incremental.  They are
 *    cumulative over the life of the run.
 */
std::vector<uint32_t>
CAENPHAScalers::read()
{
    std::vector<uint32_t> result;
    for (int i = 0; i < 16; i++) {
        result.push_back(m_pSegment->m_triggerCount[i]);
    }
    for (int i =0; i < 16; i++) {
        result.push_back(m_pSegment->m_missedTriggers[i]);
    }
  //  for (int i =0; i < 16; i++) {
    //    result.push_back(0);
    //}

    
    return result;
}
