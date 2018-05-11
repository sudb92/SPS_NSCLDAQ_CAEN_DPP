/**if
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file PHATrigger.cpp
# @brief Implements the PHATrigger class.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#include "PHATrigger.h"
#include "PHAEventSegment.h"

/**
 * addModule
 *   Adds a new module to the list of modules monitored for triggers.
 *  @param module - pointer to the new module.
 */
void
PHATrigger::addModule(PHAEventSegment* module)
{
    m_modules.push_back(module);
}
/**
 * operator()
 *    Check for triggers.
 * @return bool -true if at least one module has data to read:
 */
bool
PHATrigger::operator()()
{
    for (int i = 0; i < m_modules.size(); i++) {
        if(m_modules[i]->checkTrigger()) return true;
    }
    // If I didn't return yet no triggers.
    
    return false;
}