/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
t# 

##
# @file PHAEventSegment.h
# @brief Event segment for a PHA Module.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#ifndef PHAEVENTSEGMENT_H
#define PHAEVENTSEGMENT_H
#include <CEventSegment.h>
#include <string>
#include "pugixml.hpp"
#include <vector>

#include <CAENDigitizerType.h>

class CAENPha;
class CAENPhaParameters;
class CExperiment;

/**
 ** @class PHAEventSegment
 **     An SBSReadout event segment that reads out CAEN PHA modules.
 **     The configuration of this sort of segment is interesting.
 **     -  There's a segment .ini file which describes XML files that
 **        are read to define the configuration of the digitizer an each channel.
 **        See the comments in the implementation of initialize for the format
 **        of that file.
 **     -  The .ini file is read each initialize() time.  This also implies
 **        that the CAENPha object encapsulated by this object is
 **        Created at each init time (begin of run).
 **     -  As mentioned abofvce, the event segment encapsulates a
 **        CAENPha object whi8ch does all the real work.
 **/


class PHAEventSegment : public CEventSegment
{
private:
    CAENPhaParameters* m_pParams;
    std::vector<std::pair<unsigned, pugi::xml_document*> > m_channelDocs;
    CAENPha*    m_pPha;
    std::string m_iniFile;
    
    int                      m_id;
    CAEN_DGTZ_ConnectionType m_linkType;
    int                      m_nLinkNum;
    int                      m_nNode;
    uint32_t                 m_nBase;
    CAEN_DGTZ_AcqMode_t      m_nStartMode;
    bool                     m_fTrgOut;
    unsigned                 m_nStartDelay;
    
public:
    PHAEventSegment(const char* iniFile, int srcId);
    virtual ~PHAEventSegment();
    
    // Event segment entry points
    
    virtual void   initialize();
    virtual void   clear();
    virtual void   disable();
    virtual size_t read(void* pBuffer, size_t maxwords) ;

    // Other public entries.
    
    bool checkTrigger();
    
    
    // Utilities
private:
    
    std::string makeChannelKey(unsigned chan, const char* subkey);
    size_t computeEventSize(const CAEN_DGTZ_DPP_PHA_Event_t& dppInfo, const CAEN_DGTZ_DPP_PHA_Waveforms_t& wfInfo);
    
    // Buffer storage methods
    
    void*  putWord(void* pDest, uint16_t data);
    void*  putLong(void* pDest, uint32_t data);
    void*  putQuad(void* pDest, uint64_t data);
    void*  putDppData(void* pDest, const CAEN_DGTZ_DPP_PHA_Event_t& dpp);
    void*  putWfData(void* pDest, const CAEN_DGTZ_DPP_PHA_Waveforms_t& wf);
    
};


#endif
