/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file PHAMultiModuleSegment.h
# @brief An event segment managing several PHA modules
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#ifndef  PHAMULTIMODULESEGMENT_H
#define  PHAMULTIMODULESEGMENT_H
#include <CEventSegment.h>
#include <vector>

class PHAEventSegment;

/**
 * class PHAModuleSegment
 *    Provides a 'fair' readout of a set of PHA Modules.  This is done
 *    by rotating through the modules in the list when trying to read rather than
 *    just reading the first module.  This ensures at high data rates, all
 *    modules get checked, not just the first one.
 *    As soon as a module has data its event is returned.  This is needed
 *    because from the event builder point of view a single module is an event
 *    source so an event is one event from one module rather than the collection
 *    of all events.  Note that the trigger will continue to return true
 *    as long as any module has data so we'll just get called again.
 */
class PHAMultiModuleSegment : public CEventSegment
{
private:
    std::vector<PHAEventSegment*>   m_modules;
    unsigned                        m_nextRead;
    
public:
    PHAMultiModuleSegment();
    
    void addModule(PHAEventSegment* module);
    
    // Event segment interface:
    
public:
    virtual void initialize();
    virtual void clear();
    virtual void disable();
    virtual size_t read(void* pBuffer, size_t maxwords);
    
};

#endif
