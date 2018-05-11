/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file PHAEventSegment.cpp
# @brief Implementation of an SBSreadout event segment for CAENPHA modules.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#include "PHAEventSegment.h"
#include "CAENPha.h"
#include "CAENPhaParameters.h"
#include "CAENPhaChannelParameters.h"
#include "DPPConfig.h"
extern "C" {
#include <iniparser.h>
}
#include "pugixml.hpp"
#include <sstream>
#include <string>
#include <iostream>
/**
 *  constructor
 *     We really just save the ini file name.  Nothing else.
 *     The ini file is processed in initialize.  Doesn't even have to exist yet.
 *
 * @param iniFile   - Path to initialization file. Must exist by the time
 *                    initializE() is called.
 */
PHAEventSegment::PHAEventSegment(const char* iniFile, int id) :
    m_pParams(0),
    m_pPha(0),
    m_iniFile(iniFile),
    m_id(id)
{}

/**
 * destructor
 *    There might be (probably is) a lingering m_pPha - it gets destroyed here.
 */
PHAEventSegment::~PHAEventSegment()
{
    delete m_pPha;                        // NO-OP if this is null.
    m_pPha = nullptr;                     // not really needed for destruction
    delete m_pParams;
    m_pParams = nullptr;
}

/**
 * initialize
 *    -   Kill off any driver that's hanging around.
 *    -   Read/parse the init file.
 *    -   Use the init file to extract the sourceid, link typem, link num,
 *        node, base address, start mode, trigger out flag and start delay.
 *    -   Use the parsed init file to extract the global config file and thus
 *        the top configuration dom.
 *    -   For each channel (up to CAEN_DGTZ_MAX_CHANNELS), if the channel is
 *        enabled, get its configuration xml file and use it to construct a
 *        DOM for that channels.
 *    -   Use all of this to construct the CAENPhaParameters object.
 *    -   Use all of that to construct the CAENPha driver that will be used
 *        for this run.
 *  All errors result in an exception which, when caught by the readout framework
 *  should abort the start of run.
 */
void
PHAEventSegment::initialize()
{
    // Kill off any hanging pha driver.
    
    delete m_pPha;
    m_pPha = nullptr;                    // In case we throw and then get called again.
    
    delete m_pParams;
    m_pParams = nullptr;
    
    // Read and parse the .ini file.  Assume that the resulting dict is nullptr
    // for errors:
    
    dictionary* pDict = iniparser_load(m_iniFile.c_str());
    if (!pDict) {
        std::string message = "Could not process .ini file: '";
        message += m_iniFile;
        message += "' in PHAEventSegment::initialize";
        
        throw message;
    }
    try {
        // Connection parameters:
        
        ConnectionParams connection = getConnectionParams(pDict);
        m_linkType = connection.linkType;
        m_nLinkNum = connection.linkNum;
        m_nNode    = connection.ConetNode;
        m_nBase    = connection.VMEBaseAddress;
        
        // Other global parameters:
        
        m_nStartMode = getStartModeCode(pDict, "SOFTWARE");
        m_fTrgOut    =
            static_cast<bool>(
                iniparser_getboolean(pDict, "COMMON:TriggerOutput", 1)
            );
        m_nStartDelay = iniparser_getint(pDict, "COMMON:StartDelay", 0);
        if (m_nStartDelay < 0) {
            std::string message = "Start delay must be non negative in '";
            message += m_iniFile;
            message += "'";
            
            throw message;
        }
        // Make doms needed to create the configuration object:
        
        // Global parameter DOM:
        
        const char* globalParamXML = iniparser_getstring(pDict, "COMMON:GlobalXmlFile", nullptr);
        if (globalParamXML == nullptr) {
            std::string message = "[COMMON] must have a key named GlobalXmlFile and does not in '";
            message += m_iniFile;
            message += "'";
            
            throw message;
        }
        pugi::xml_document globalParamsDom;
        pugi::xml_parse_result xmlstatus = globalParamsDom.load_file(globalParamXML);
        if (!xmlstatus) {
            std::string message = "Failed to parse global parameter XML file from '";
            message  += globalParamXML;
            message  += "' ";
            message  += xmlstatus.description();
            
            throw message;
        }
        
        // Per channel DOMS:
        
        m_channelDocs.clear();      /// TODO: Must delete the xml doc members.
        
        
        for (unsigned i = 0; i < CAEN_DGTZ_MAX_CHANNEL; i++) {
            bool enabled =
                static_cast<bool>(
		   iniparser_getboolean(pDict, makeChannelKey(i, "enabled").c_str(), 0)
	 	);
            if (enabled) {
                const char* channelXML =
		  iniparser_getstring(pDict, makeChannelKey(i, "ConfigXml").c_str(), nullptr);
                if (channelXML == nullptr) {
		  std::string message = "Enabled channels must have a ConfigXml key and ";
                    message += " channel ";
                    message +=  std::to_string(i);
                    message += " does not have one in '";
                    message += m_iniFile;
                    message += "'";
                    
                    throw message;
                }
                pugi::xml_document* pDoc = new pugi::xml_document;
                xmlstatus = pDoc->load_file(channelXML);
                if (!xmlstatus) {
                    std::string message = "XML Parse for channel configuration file channel# ";
                    message += std::to_string(i);
                    message += " failed: " ;
                    message += xmlstatus.description();
                    message += " '";
                    message += m_iniFile;
                    message += "'";
                    
                    throw message;
                    
                }
                std::pair<unsigned, pugi::xml_document*> item(i, pDoc);
                m_channelDocs.push_back(item);
            }
        }
        // Now we can create the configuration and the driver:
        
        m_pParams = new CAENPhaParameters(globalParamsDom, m_channelDocs);
        m_pParams->unpack();
        m_pPha    = new CAENPha(
            *m_pParams, m_linkType, m_nLinkNum, m_nNode, m_nBase,
            m_nStartMode, m_fTrgOut, m_nStartDelay
        );
        
        // Set up the digitizer and start it off:
        
        m_pPha->setup();
    }
    catch(std::pair<std::string, int> phaError) {
        std::cerr << "Failed to initialize a PHA segment: " << phaError.first
            << " error code " << phaError.second;
            iniparser_freedict(pDict);
            throw phaError.first;
    }
    catch(...) {
        // Prevent dictionary leakage on errors:
        
        iniparser_freedict(pDict);
        throw;
    }
}
    
/**
 * clear
 *    We don't want this to do anything.  This is because we want to allow
 *    data to be buffered both in the driver and in the digitizer.
 */
void
PHAEventSegment::clear()
{
  
}
/**
 * disable
 *    Shutdown the digitizer.  This will also clear any data buffered
 *    in the digitizer.
 */
void
PHAEventSegment::disable()
{
  try {
    m_pPha->shutdown();
  }
  catch (std::pair<std::string, int> phaError) {
    std::cerr << "Some failure in shutdown: " << phaError.first
	      << " error code: " << phaError.second << std::endl;
    std::cerr << "May need to power cycle the digitizer to start it again\n";
  }
}
/**
 * read
 *    Reads data from the digitizer to the buffer.
 *    - If the digitizer has no data (there could be more than one)
 *      Nothing is added.
 *    - If the digitizer does have data we'll need to massage it a bit
 *      because if there are waveforms we'll need to properly decode them
 *      as well and put them into the event in a way that makes sense.
 *    - Timestamp is gotten from the event.
 *    - source id is gotten from the configuration (m_id).
 *
 *  @param pBuffer    - pointer to the event buffer.
 *  @param maxwords   - Maximum # of words we're allowed to put in the buffer.
 *  @return size_t    - Total number of words we actually put in the buffer.
 *
 *  @note if the total event size is bigger than maxwords we'll
 *        throw an exception.  This could happen for long waveforms.
 *        The exception will tell the user to increase the max event size.
 *        Timestamp comes from the event and sourcid from m_id
 */
size_t
PHAEventSegment::read(void* pBuffer, size_t maxwords)
{
  std::tuple<int, const CAEN_DGTZ_DPP_PHA_Event_t*, const CAEN_DGTZ_DPP_PHA_Waveforms_t*>
    event = m_pPha->Read();
  
  
  
  // Figure out the event size.  Note that since CAEN_DGTZ_DPP_PHA_Event_t*
  // and CAEN_)DGTZ_DPP_PHA_Waveforms_t almost certainly have pads,
  // we're going to write the data an item at a time into the buffer.
  
  int chan = std::get<0>(event);
  const CAEN_DGTZ_DPP_PHA_Event_t* dppData = std::get<1>(event);
  const CAEN_DGTZ_DPP_PHA_Waveforms_t* wfData = std::get<2>(event);

  if (!(dppData || wfData)) {
    return 0;                            // No event.
  }
  setTimestamp(dppData->TimeTag);        // Event timestamp.
  setSourceId(m_id);                     // Source id from member data.         
  size_t eventSize = computeEventSize(*dppData, *wfData);
  
  if ((eventSize / sizeof(uint16_t)) > maxwords) {
    throw std::string("PHAEventSegment size exceeds maxwords - expand max event size");
  }
  
  // Header consists of the total inclusive size in bytes and the channel #
  
  pBuffer = putLong(pBuffer, eventSize);
  pBuffer = putLong(pBuffer, chan);
  
  // Body is dpp data followed by wf data:
  
  pBuffer = putDppData(pBuffer, *dppData);
  pBuffer = putWfData(pBuffer, *wfData);
  
  
  return (eventSize / sizeof(uint16_t)); 
}


/**
 * checkTrigger
 *  Check to see if the underlying module has data.
 *
 *  @return bool - true if there's data in the underlying module.
 */
bool
PHAEventSegment::checkTrigger()
{
    return m_pPha->haveData();
}

/**
 * putWord
 *    Put a word into the buffer.
 *
 * @param pDest - pointer to where the word goes
 * @param data  -uint16_t to put.
 * @return void* Pointer to the next free slot in the buffer.
 */
void*
PHAEventSegment::putWord(void* pDest, uint16_t data)
{
    uint16_t* p = reinterpret_cast<uint16_t*>(pDest);
    *p++ = data;
    
    return p;
}
/**
 * putLong
 *  Put a 32 bit long into the buffer.
 *
 * @param pDest - pointer to where the data goes.
 * @param data  - uint32_t to put.
 * @return void* - pointer to the next free slot in the buffer.
 */
void*
PHAEventSegment::putLong(void* pDest, uint32_t data)
{
    uint32_t* p = reinterpret_cast<uint32_t*>(pDest);
    *p++ = data;
    return p;
}

/**
 * putQuad
 *    Put a 64 bit quadword into the buffer.
 *
 * @param pDest - pointer to where the data goes.
 * @param data  - the data.
 * @return void* - points to the next free slot in the buffer.
 */
void*
PHAEventSegment::putQuad(void* pDest, uint64_t data)
{
    uint64_t* p = reinterpret_cast<uint64_t*>(pDest);
    *p++ = data;
    return p;
}

/**
 * computeEventSize
 *    Figure out how big the event is, in bytes, including a 32 bit event size.
 *
 *  @param dppInfo - reference to the CAEN_DGTZ_PHA_Event_t containing the event.
 *  @param wfInfo  - reference to the CAEN_DGTZ_PHA_Waveforms_t containing decoded waveforms.
 *  @return size_t - Number of bytes the event will require in the data stream.
 */
size_t
PHAEventSegment::computeEventSize(
    const CAEN_DGTZ_DPP_PHA_Event_t& dppInfo, const CAEN_DGTZ_DPP_PHA_Waveforms_t& wfInfo
)
{
    size_t result = sizeof(uint32_t);               // Size of event
    
    // The dpp info is a time tag (uint64_t), E, extras, (16 bits )and
    // extras2 (32 bits)):
    
    result += sizeof(uint64_t)   +                    // Time tag.
              2*sizeof(uint16_t) +                    // E, Extras.
              sizeof(uint32_t);                       // Extras2.
              
    // Waveform data size is a bit more complex.  See the putWfData
    // method for considerations.
    
    result += sizeof(uint32_t) + sizeof(uint16_t);  // Ns, and dualtrace flag.
    if (wfInfo.Ns) {
        // If there are samples, there's always at least one trace of 16 bit words:
        
      size_t nBytes = wfInfo.Ns * sizeof(uint16_t);
      result += nBytes;
      
      //  If dual trace there's a second one:
      
      if (wfInfo.DualTrace) {
	result += nBytes;
      }
    }
    return result;
}

/**
 * putDppData
 *    Puts the DPP data into the event buffer.
 * @param pDest - where to put the DPP Data.
 * @param dpp   - Reference to the dpp data.
 * @return - pointer to the next free slot in the buffer.
 */
void*
PHAEventSegment::putDppData(void* pDest, const CAEN_DGTZ_DPP_PHA_Event_t& dpp)
{
    pDest = putQuad(pDest, (dpp.TimeTag));
    pDest = putWord(pDest, (dpp.Energy));
    pDest = putWord(pDest, (dpp.Extras));
    pDest = putLong(pDest, (dpp.Extras2));
    
    return pDest;
}

/**
 * putWfData
 *    Put the waveform data to the output buffer.
 *    We put the following information:
 *    -   ns - number of samples (32 bits)
 *    -   dt - Non zero if ther are two traces (16 bits).
 *             note that we widen this in order to make alignment better.
 *    -   First trace if ns > 0 ns*16 bits.
 *    -   Second trace if ns > 0 && dual trace  ns * 16 bits.
 *
 * @param pDest - pointer to where the data must go
 * @param wf    - Reference to CAEN_DGTZ_PHA_Waveforms_t - the decoded waveforms.
 * @return void* - Pointer to the next free memory of the buffer (where next data goes).
 */
void*
PHAEventSegment::putWfData(void* pDest, const CAEN_DGTZ_DPP_PHA_Waveforms_t& wf)
{
  pDest = putLong(pDest, wf.Ns);                            // # samples.
  pDest = putWord(pDest, wf.DualTrace);
    size_t nBytes = wf.Ns * sizeof(uint16_t);
    if (nBytes > 0) {                                   // There are waveforms:
      uint8_t* p = reinterpret_cast<uint8_t*>(pDest);
        memcpy(p, wf.Trace1, nBytes);               // Write trace 1...
        p +=  nBytes;
        
        if (wf.DualTrace) {                            // Write second trace too:
            memcpy(p, wf.Trace2, nBytes);
            p += nBytes;
        }
	pDest = p;
    }
    return pDest;
}

/**
 * makeChannelKey
 *    Create a channel key for an ini file. The key is of the form:
 *    n.value
 *    WHere n is the channel number and n is a subkey in that section.
 *    e.g for channel 0 and subkey enabled the method returns 0.enabled
 *    which matches the 'enabled' key in [0].
 *
 * @param chan   - Number of the channel.
 * @param subkey - Key in that channel's section.
 *
 * @return std::string  - The key produced.
 */
std::string
PHAEventSegment::makeChannelKey(unsigned chan, const char* subkey)
{
  std::stringstream s;

  s << chan << ":" << subkey;
  return s.str();
}
