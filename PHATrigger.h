/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 
##
# @file PHATrigger.h
# @brief Event Trigger for PHA Modules.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#ifndef PHATRIGGER_H
#define PHATRIGGER_H
#include <CEventTrigger.h>
#include <vector>


class PHAEventSegment;



/**
 * @class   PHATrigger
 *   Passed a set of modules and provides a trigger that is the OR of
 *   data ready in the modules.
 *
 *   @note - we don't need a setup or teardown as the individual
 *           modules will enable/disable triggers themselves.
 */
class PHATrigger : public CEventTrigger
{
private:
    std::vector<PHAEventSegment*>   m_modules;

public:
    void addModule(PHAEventSegment* module);
    virtual bool operator()();
};


#endif