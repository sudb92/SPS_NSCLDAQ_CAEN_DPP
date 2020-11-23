/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file CompassEventSegment.h
# @brief CAEN PHA event segment whose configuration comes from Compass.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#ifndef COMPASSEVENTSEGMENT_H
#define COMPASSEVENTSEGMENT_H
#include <CEventSegment.h>
#include <string>
#include <CAENDigitizerType.h>
#include <chrono>

class CAENPha;
class CAENPhaParameters;

/**
 * @class CompassEventSegment
 *    This event segment is much like the PHAEventSegment except that
 *    everything needed to initialize come from a COMPASS configuration
 *    XML file.  At each initialization, the configuration file is reprocessed
 *    in case there are changes and used to setup the digitizer.k
 *
 */
class CompassEventSegment : public  CEventSegment
{
private:
    std::string m_filename;
    CAENPha*    m_board;                    // Board level driver.
    int         m_id;

    CAEN_DGTZ_ConnectionType m_linkType;
    int                      m_nLinkNum;
    int                      m_nNode;
    uint32_t                 m_nBase;
    const char*              m_pCheatFile;
    
public:
    CompassEventSegment(
        std::string filename, int sourceId,
        CAEN_DGTZ_ConnectionType linkType, int linkNum, int node, int base,
        const char* pCheatFile=nullptr
    );
    virtual ~CompassEventSegment();
    
    
    // A great deal of this could possibly be factored into a common base class.
    
    virtual void initialize();
    virtual void clear();
    virtual void disable();
    virtual size_t read(void* pBuffer, size_t maxwords);
    
    /** Code from here down could be refactored into a base class
     *  common to PHAEventSegment and  CompassEventSegment.
     */
    uint32_t           m_triggerCount[16];
    uint32_t           m_missedTriggers[16];
    double t[16];
    double tmiss[16];

    
    // Other publics:
    
    bool checkTrigger();
private:
    size_t computeEventSize(const CAEN_DGTZ_DPP_PHA_Event_t& dppInfo, const CAEN_DGTZ_DPP_PHA_Waveforms_t& wfInfo);
    void   setupBoard(CAENPhaParameters& board);
    // Buffer storage methods
    
    void*  putWord(void* pDest, uint16_t data);
    void*  putLong(void* pDest, uint32_t data);
    void*  putQuad(void* pDest, uint64_t data);
    void*  putDppData(void* pDest, const CAEN_DGTZ_DPP_PHA_Event_t& dpp);
    void*  putWfData(void* pDest, const CAEN_DGTZ_DPP_PHA_Waveforms_t& wf);
    
};


#endif
