/*
*-------------------------------------------------------------
 
 CAEN SpA 
 Via Vetraia, 11 - 55049 - Viareggio ITALY
 +390594388398 - www.caen.it

------------------------------------------------------------

**************************************************************************
* @note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the 
* software, documentation and results solely at his own risk.
*
* @file     CDPpPsdEventSegment.h
* @brief    Header for an event segment for DPP-PSD digitizers.
* @author   Ron Fox
*
*/
#ifndef     CDPPPSDEVENTSEGMENT_H
#define     CDPPPSDEVENTSEGMENT_H

#include <CEventSegment.h>           // Base class from NSCLDAQ
#include "PSDParameters.h"
#include <string>
#include <chrono>
#include <CAENDigitizerType.h>

/**
 * @class CDPpPSdEventSegment
 *    Event segment to read out DPP-PSD digitizers using configurations
 *    from COMPASS to setup the digitizers.
 *
 *    The digitizer is constructed with a specific connection parameter set
 *    that are assumed to be the same as what was used to configure the digitizer
 *    with COMPASS - the digitizer serial number and model name are used used as well
 *    to match the correct PSDBoardParameters from the configuration.
 *    Note that the constructor just supplies the connection params.
 *    initialize is when the configuration is when the configuration is found
 *    and processed.  The assumption is that parsing the configuration file
 *    is relatively inexpensive compared with initialization and running.
 */
class CDPpPsdEventSegment : public CEventSegment
{
private:
    std::string         m_configFilename;
    PSDBoardParameters* m_pCurrentConfiguration;
    
    // Supplied at construction time:
    
    PSDBoardParameters::LinkType m_linkType;
    int                          m_linkNum;
    int                          m_nodeNumber;
    int                          m_base;

    // CAEN Library handle to the digitizer:
    
    int          m_handle;
    
    // Derived from the connection parameters:
    
    std::string  m_moduleName;
    int          m_serialNumber;
    int          m_nChans;
    
    // Used to tag data from the digitizer in event builder fragment e.g.
    
    int m_nSourceId;

    
    // Used to buffer events from the digitizer so that Read can return
    // just the oldest hit from each channnel.
    
    char*     m_rawBuffer;
    uint32_t  m_rawBufferSize;
    CAEN_DGTZ_DPP_PSD_Event_t* m_dppBuffer[CAEN_DGTZ_MAX_CHANNEL];
    uint32_t m_dppBufferSize;
    CAEN_DGTZ_DPP_PSD_Waveforms_t* m_pWaveforms;
    uint32_t m_wfBufferSize;
    uint32_t                   m_nHits[CAEN_DGTZ_MAX_CHANNEL];
    uint32_t                   m_nChannelIndices[CAEN_DGTZ_MAX_CHANNEL];
    uint64_t                   m_timestampAdjust[CAEN_DGTZ_MAX_CHANNEL];
    uint32_t                   m_lastTimestamps[CAEN_DGTZ_MAX_CHANNEL];
    uint64_t m_nsPerTick;
    const char*        m_pCheatFile;
    
public:
    CDPpPsdEventSegment(
        PSDBoardParameters::LinkType linkType, int linkNum, int nodeNum,
        int base, int sourceid, const char* configFile, const char* pCheatFile=0
    );
    virtual ~CDPpPsdEventSegment();

    // Interface methods:
    
  virtual void   initialize();
  virtual size_t read(void* pBuffer, size_t maxwords) ;
  bool    checkTrigger();
  void    disable();
  
  // Support for multiple boards:
  
  bool isMaster();
  void startAcquisition();  

  uint32_t           m_triggerCount[16];
  uint32_t           m_missedTriggers[16];
  double t[16];
  double tmiss[16];


private:
    PSDBoardParameters* matchConfig(const PSDParameters& systemConfig);
    bool ourConfig(const PSDBoardParameters& board);
    void openModule();
    void getModuleInformation();
    void setupBoard();
    void processCheatFile();
    
    
    
    void throwIfBadStatus(CAEN_DGTZ_ErrorCode status, const char* msg);
    uint32_t  nsblValue(int chan);
    uint32_t  cfdDelay(int chan);
    uint32_t  cfdSmooth(int chan);
    int       nsToSamples(double ns);
    int       nsToDelay(double ns);
    uint32_t  cfdFraction(int chan);
    int       coarseGainToSensitivity(int chan);
    void      setOutputMode();
    bool      needBufferFill();
    void      fillBuffer();
    void      allocateBuffers();
    uint32_t  oldestChannel();
    size_t    formatEvent(void* pBuffer, int chan);
    size_t    sizeEvent(int chan);
    void      freeDAQBuffers();
    uint32_t  sizeTraces();
    void      setLVDSLevel0Trigger();
    void      setLVDSLevel1Trigger();
    void      setLVDSSwTrigger();
    void      setLVDSExternalTrigger();
    void      setLVDSGlobalOrTrigger();
    void      setLVDSRun();
    void      setLVDSDelayedRun();
    void      setLVDSSampleClock();
    void      setLVDSPLLClock();
    void      setLVDSBusy();
    void      setLVDSPLLLockLost();
    void      setLVDSVirtualProbe();
    void      setLVDSSIN();
};


#endif
