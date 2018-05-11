/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 
##
# @file CompassTrigger.cpp
# @brief Event Trigger for PHA Modules.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#include "CompassTrigger.h"
#include "CompassEventSegment.h"

/**
 * addModule
 *    Adds a module to the trigger.
 * @param module - pointer to a CompassEventSegment to add.
 */
void
CompassTrigger::addModule(CompassEventSegment* p)
{
    m_modules.push_back(p);
}
/**
 * operator()
 *   @return bool
 *   @retval true if at least one module has data.
 *   @retval false if all modules have no data.
 */
bool
CompassTrigger::operator()
{
    for (int i =0; i < m_modules.size(); i++) {
        if (m_modules[i]->checkTrigger()) return true;
    }
    return false;
}