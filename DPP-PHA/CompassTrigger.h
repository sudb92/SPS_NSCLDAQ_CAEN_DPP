/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 
##
# @file CompassTrigger.h
# @brief Event Trigger for CompassEvent segments.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#ifndef COMPASSTRIGGER_H
#define COMPASSTRIGGER_H

#include <CEventTrigger.h>
#include <vector>

class CompassEventSegment;

/**
 * @class CompassTrigger
 *    Checks for triggers in a collection of CompassEventSegment
 *    modules.
 */
class CompassTrigger : public CEventTrigger
{
private:
    std::vector<CompassEventSegment*> m_modules;
    unsigned                          m_nNextModule;
    void next();                    // module iteration.
public:
    void addModule(CompassEventSegment* module);
    virtual bool operator()();
    
};

#endif
