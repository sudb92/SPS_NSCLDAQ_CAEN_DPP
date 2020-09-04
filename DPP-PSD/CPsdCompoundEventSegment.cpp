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
* @file  CPsdCompoundEventSegent.cpp
* @brief Implement the event segment.
*/

#include "CPsdCompoundEventSegment.h"
#include "CDPpPsdEventSegment.h"
#include <iostream>

/**
 * construction
 *    Just set the next read to index 0.
 */
CPsdCompoundEventSegment::CPsdCompoundEventSegment() :
    m_nextRead(0)
{}
/**
 * destructor
 *    null for now.
 */
CPsdCompoundEventSegment::~CPsdCompoundEventSegment()
{}

/**
 *  initialize
 *     - Warn if there are no modules.
 *     - initialize all of the modules.
 *     - Start the modules that are not masters.
 *     - Start the master module.
 */
void
CPsdCompoundEventSegment::initialize()
{
    for (int i = 0; i < m_modules.size(); i++) {
        m_modules[i]->initialize();
        
        if (!(m_modules[i]->isMaster())) {
            m_modules[i]->startAcquisition();
        }
    }
    // Now initialize the master module:
    
    for (int i = 0; i < m_modules.size(); i++) {
        if (m_modules[i]->isMaster()) {
            m_modules[i]->startAcquisition();
        }
    }


}
/**
 * clear
 *    This is  no-op as modules are destructively read.
 */
void
CPsdCompoundEventSegment::clear()
{
    
}

/**
 * disable
 *    Disable acquisition in all modules.
 */
void
CPsdCompoundEventSegment::disable()
{
    for (int i = 0; i < m_modules.size(); i++) {
        m_modules[i]->disable();
    }
}
/**
 * read
 *    Figure out which module to read from.  Note that it's possible
 *    no module should be read -- that another event segment caused the
 *    trigger.
 * @param pBuffer - where to put the data.
 * @param maxwords - Maximum number of words of data we can read.
 * @return size_t number of bytes actually read.
 */
size_t
CPsdCompoundEventSegment::read(void* pBuffer, size_t maxwords)
{
    /*CDPpPsdEventSegment*  p = nextToRead();
    if (p) {
        return p->read(pBuffer, maxwords);
    } else {
        return 0;
    }*/
    size_t nRead=0;
    size_t nM = m_modules.size();
    for (int i =0; i < nM; i++) {
        if (m_modules[m_nextRead]->checkTrigger()) {
	        nRead = static_cast<size_t>(m_modules[m_nextRead]->read(pBuffer, maxwords));
		break;
	}

        m_nextRead = (m_nextRead + 1) % nM;
//        std::cout << "\n Go here! " << nRead << std::flush;

    }
    return nRead;

}
/**
 * addModule
 *   Add a new module to the modules we're reading. Normally,
 *   this should be done when the system is initialized.  In fact,
 *   new modules could be added any time acqusition is halted.
 *
 * @param p - pointer to the module to add.  The new module must stay in
 *            scope for the duration of program execution.
 * @note Currently I've not implemented any way to remove modules from
 *       the event segment as I figure the normal use case is to setup
 *       the segment at program initialization time -  that it won't be
 *       dynamic (though the Compass configuration file could be).
 */
void
CPsdCompoundEventSegment::addModule(CDPpPsdEventSegment* p)
{
    m_modules.push_back(p);
}
////////////////////////////////////////////////////////////////////////
// Utility methods

/**
 * nextToRead
 *    Figure out which module is going to be read next:
 *  @return DPpPsdEventSegment* pointer to the module to read.
 *  @retval nullptr - if no modules in the event segment have data.
 */
CDPpPsdEventSegment*
CPsdCompoundEventSegment::nextToRead()
{
    // If we go all the way around, there's nothing to read:
    
    CDPpPsdEventSegment* pResult = nullptr;
    for (int i =0; i < m_modules.size(); i++) {
        nextModule();
        if (m_modules[m_nextRead]->checkTrigger()) {
            pResult = m_modules[m_nextRead];
            break;
        }
    }

    return pResult;
}
/**
 * nextModule
 *   advance m_nextModule to the next modules (modulus increment).
 *   @note We assume there's at least one module.
 */
void
CPsdCompoundEventSegment::nextModule()
{
    m_nextRead = (++m_nextRead) % m_modules.size();
}
