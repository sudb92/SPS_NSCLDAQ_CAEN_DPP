/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file CompassMultiModuleSegment.cpp
# @brief Implementation of the CompassMultiModuleSegment class.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#include "CompassMultiModuleEventSegment.h"
#include "CompassEventSegment.h"

/**
 *   Constructor -- just initialize m_nextRead (round robbin member).
 */
CompassMultiModuleEventSegment::CompassMultiModuleEventSegment() :
    m_nextRead(0)
{}

/**
 * addModule
 *     Add a module to the collection.
 *  @param p  - pointer to the module to add to the collection.
 */
void
CompassMultiModuleEventSegment::addModule(CompassEventSegment* p)
{
    m_modules.push_back(p);
}
/**
 * initialize
 *    Initialize all modules.
 */
void
CompassMultiModuleEventSegment::initialize()
{
    for (int i =0; i < m_modules.size(); i++) {
        m_modules[i]->initialize();
    }
}
/**
 *  clear
 *     Invoke each module's clear method:
 */
void
CompassMultiModuleEventSegment::clear()
{
    for (int i  = 0; i < m_modules.size(); i++) {
        m_modules[i]->clear();
    }
}
/**
 * disable
 *   Disable all modules
 */
void
CompassMultiModuleEventSegment::disable()
{
    for (int i =0; i < m_modules.size(); i++) {
        m_modules[i]->disable();
    }
}
/**
 * read
 *   Iterate over all modules,  Return the data from the
 *   first one that has some - note that we don't start at 0,
 *   buyt use m_nextRead to ensure some 'balance'.
 *
 * @param pBuffer - pointer to the place to put the data.
 * @param maxwords - Maximum number of words that can be read.
 * @return size_t  - Number of words read - could be zero.
 */
size_t
CompassMultiModuleEventSegment::read(void* pBuffer, size_t maxwords)
{
    size_t nM = m_modules.size();
    for (int i =0; i < nM; i++) {
        size_t nRead = m_modules[m_nextRead]->read(pBuffer, maxwords);
        m_nextRead = (m_nextRead + 1) % nM;
        if (nRead) return nRead;
    }
    return 0;          // Nobody had data after all.
}
