/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file PHAMultiModuleSegment.cpp
# @brief Implementation of the PHAMultiModuleSegment class.
# @author Ron Fox (rfoxkendo@gmail.com)

*/

#include "PHAMultiModuleSegment.h"
#include "PHAEventSegment.h"


/**
 * constructor:
 *    Just set m_nextRead -> 0.
 */

PHAMultiModuleSegment::PHAMultiModuleSegment() :
   m_nextRead(0)
{}

/**
 * addModule
 *    Add a module to the event segment.
 *
 * @param module - new module to append to m_modules
 */
void
PHAMultiModuleSegment::addModule(PHAEventSegment* module)
{
    m_modules.push_back(module);
}

/**
 * initialize
 *    Iterate over all modules initializing them.
 */
void
PHAMultiModuleSegment::initialize()
{
    for (int i =0; i < m_modules.size(); i++) {
        m_modules[i]->initialize();
    }
}
/**
 * clear
 *    Iterate over all modules and clear them (this is a no-op in the
 *    individual modules but we're not going to assume that).
 */
void
PHAMultiModuleSegment::clear()
{
    for (int i = 0; i < m_modules.size(); i++) {
        m_modules[i]->clear();
    }
}
/**
 * disable
 *    Disable all the modules by iterating over them and invoking their
 *    disable method.
 */
void
PHAMultiModuleSegment::disable()
{
    for (int i =0; i < m_modules.size(); i++) {
        m_modules[i]->disable();
    }
}
/**
 *  read
 *     Iterate over the modules starting with m_nextRead
 *     Attempt to read each module and return when one provides data.
 *     Note that if all modules have been attempted but nobody supplies
 *     data we just return without reading anything (zero length).
 *
 *  @param pBuffer - pointer to the buffer.
 *  @param maxwords - Largest number of words that can be read in uint16_t
 *  @return size_t  - Number of words read. Could be zero.
 */
size_t
PHAMultiModuleSegment::read(void* pBuffer, size_t maxwords)
{
    size_t nM = m_modules.size();
    for (int i =0; i < nM; i++) {
        size_t nRead = m_modules[m_nextRead]->read(pBuffer, maxwords);
        if (nRead) return nRead;
        m_nextRead = (m_nextRead + 1) % nM;   // Circular next module.
    }
    
    return 0;                     // Nobody really had data!!
}
