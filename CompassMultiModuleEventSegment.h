/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file CompassMultiModuleEventSegment.h
# @brief An event segment managing several Compass PHA modules.
# @author Ron Fox (rfoxkendo@gmail.com)

*/

#ifndef COMPASSMULTIMODULESEGMENT_H
#define COMPASSMULTIMODULESEGMENT_H
#include <CEventSegment.h>
#include <vector>

class CompassEventSegment;

/**
 *  @class CompassMultiModuleEventSegment
 *    - is to CompassEventSegment as PHAMultiModuleEventSegment is to
 *      PHAEventSegment.
 */
class CompassMultiModuleEventSegment : public CEventSegment
{
private:
    std::vector<CompassEventSegment*> m_modules;
    unsigned                      m_nextRead;

public:
  CompassMultiModuleEventSegment();
    virtual void initialize();
    virtual void clear();
    virtual void disable();
    virtual size_t read(void* pBuffer, size_t maxwords);
public:
  void addModule(CompassEventSegment* p);
};


#endif
