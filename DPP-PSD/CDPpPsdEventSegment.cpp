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
* @file     CDPpPsdEventSegment.cpp
* @brief    Implements the DPP PSD Event segment.
* @author   Ron Fox
*
*/
#include "CDPpPsdEventSegment.h"
#include <CAENDigitizer.h>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <fstream>

// Register offset definitions not in CAENDigitizerType.h:
#define CFD_SETTINGS                 0x103c            // Per channel
#define DPP_ALGORITHM_CONTROL        0x1080            // Per channel
#define DPP_LOCAL_TRIGGER_MANAGEMENT 0x1084            // per channel
#define DPP_PURGAP                   0x107c
#define DPP_DYNRANGE                 0x1028
#define DPP_FIXED_BASELINE           0x1064
#define PRE_TRIGGER                  0x1038
#define DPP_START_DELAY              0x8170
/**
 * constructor
 *  -  Initializes the current configuration to nullptr.
 *  -  Sets the configuration filename and connection parameters:
 *
 *  @param linkType -   Hardware connecting us to the digitizer (e.g. CONET)
 *  @param linkNum  -   Number of the hardware link connecting us to the digitizer.
 *                      For example the conet fibre number on a A3818.
 *  @param nodeNum  -   Node number on the interface e.g. the position in a conet
 *                      daisy chain.
 *  @param base     -   Base address on VME bus based systems accessed via a bus
 *                      bridge.
 *  @param sourceid - id of the module - used to tag its ring items in e.g. event builder.
 *  @param configFile - Name of the configuration file.
 *
 *  @note The configuration file only has to exist by initialize time
 *       (when the first run is started) it's not an error for it not to
 *       exist at this point.
 *  @note No hardware accesses are done at this time.  That's deferred to initialize
 *        time.
 */
CDPpPsdEventSegment::CDPpPsdEventSegment(
    PSDBoardParameters::LinkType linkType, int linkNum, int nodeNum,
    int base, int sourceid, const char* configFile,  const char* pCheatFile
) :
    m_configFilename(configFile), m_pCurrentConfiguration(nullptr),
    m_linkType(linkType), m_linkNum(linkNum), m_nodeNumber(nodeNum),
    m_base(base), m_handle(-1), m_moduleName(""), m_serialNumber(-1),
    m_nSourceId(sourceid), m_rawBuffer(nullptr), m_pWaveforms(nullptr), m_pCheatFile(pCheatFile)
{
    for(int i =0; i < CAEN_DGTZ_MAX_CHANNEL; i++) {
        m_dppBuffer[i] = nullptr;
        m_nHits[i]     = 0;
        m_nChannelIndices[i]= 0;
    }


}
/**
 * destructor
 *   @note we assume the module is quiescent.
 *   
 */
CDPpPsdEventSegment::~CDPpPsdEventSegment()
{
    delete m_pCurrentConfiguration;
    
    // Free the acquisition buffers:
    
    freeDAQBuffers();
    
}
/**
 * initialize
 *    Initialize the module:
 *    - Connect to the module using the connection parameters supplied.
 *    - Process the configuration file.
 *    - Figure out which one matches us.
 *    - Using that configuration setup the board.
 *    - Using that configuration start acquisition.
 */
void
CDPpPsdEventSegment::initialize()
{
    openModule();
    getModuleInformation();
    PSDParameters systemConfig;
    systemConfig.parseConfigurationFile(m_configFilename.c_str());
    m_pCurrentConfiguration = matchConfig(systemConfig);
    if (!m_pCurrentConfiguration) {
        std::stringstream strErrorMessage;
        strErrorMessage << "The " << m_moduleName << " Serial number: "
            << m_serialNumber
            << " has no matching configuration in the Compass Config file\n";
        strErrorMessage << " Connection type: "
            << (m_linkType == PSDBoardParameters::usb ? "USB" : "CONET")
            << "\nlink number: " << m_linkNum
            << "\nnode number: " << m_nodeNumber
            << "\nbase address: 0x" << std::hex <<  m_base << std::dec;
        throw strErrorMessage.str();
    }
    
    setupBoard();
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()
			).count();


    for (int i=0; i<16; i++)
    {
	  m_triggerCount[i]=0;
          m_missedTriggers[i]=0;
          t[i] = now;
          tmiss[i] = now;
    }


    // 32 -64 bit timestamp adjustments:
    
    memset(m_timestampAdjust, 0, CAEN_DGTZ_MAX_CHANNEL*sizeof(uint64_t));
    memset(m_lastTimestamps, 0, CAEN_DGTZ_MAX_CHANNEL*sizeof(uint32_t));
    
    // Let the external world do this in case we're compound.

}
/**
 * read
 *    Read the next event from the digitizer into the
 *    - The sourceid of the event is set from m_nSourceId
 *    - The event timestamp is converted to nanoseconds with a rollover counter used
 *      to turn it into  a uin64_t nanosecond counter from the standard 32 bit counter.
 *    - It's the caller's responsibility to ensure there's data.
 *
 *  @param void* pBuffer - Where to put one event.
 *  @param size_t maxwords - Maximum # uint16_t words available in pBuffer
 *  @throw std::string - if maxwords is smaller than the event and waveform we need to put.
 *  @return size_t number of 16 bgit wor read. 
 */
size_t
CDPpPsdEventSegment::read(void* pBuffer, size_t maxwords)
{
    if (needBufferFill()) fillBuffer();
    int chan = oldestChannel();
    size_t nBytes = sizeEvent(chan);
    if(nBytes > (maxwords*sizeof(uint16_t))) {
        throw std::string("Event is bigger than event size - increase event buffer size");
    }
    return formatEvent(pBuffer, chan)/sizeof(uint16_t);
}

/**
 * checkTrigger
 *    Returns true if there are events for this digitizer.
 *    This is the case if either we don't need a buffer fill (we have buffered events)
 *    There are events in the digitizer ready to read.
 * @return bool true - if this module can give data.
 */
bool
CDPpPsdEventSegment::checkTrigger()
{
    if (!needBufferFill()) return true;
   /* uint32_t statusRegister;
    throwIfBadStatus(
        CAEN_DGTZ_ReadRegister(m_handle,CAEN_DGTZ_ACQ_STATUS_ADD , &statusRegister) ,
        "Unable to read status register to see if there are events"
    );

    return ((statusRegister & 8) != 0);*/

  fillBuffer();                    // In anticipation of read.
  if (needBufferFill()) {
    return false;
  } else {
    return true;
  }

}
/**
 * disable
 *    Just stops data acquisition in the module:
 */
void
CDPpPsdEventSegment::disable()
{
    throwIfBadStatus(
        CAEN_DGTZ_SWStopAcquisition(m_handle), "Failed to stop acquisition"
    );
    // Since we setup all over again next run, close the digitizer here:

    throwIfBadStatus(CAEN_DGTZ_CloseDigitizer(m_handle), "Failed to close the digitzer");
}

/**
 *  isMaster.
 *     The master is the one with the start mode as software
 * @return - true if the board is the master.
 */
bool
CDPpPsdEventSegment::isMaster()
{
    return m_pCurrentConfiguration->s_startMode == PSDBoardParameters::software;
}

/////////////////////////////////////////////////////////////////////////
//  Private methods:

/**
 * matchConfig
 *   Given a system configuration and that the module typename and serial
 *   have been retrieved from the library, figures out which board configuration
 *   matches us and returns a pointer to it.
 *
 *  @param systemConfig - References the COMPASS system configuration.
 *  @return PSDBoardParameters* - pointer to the matching config.
 *  @retval nullptr - there was no match.
 */
PSDBoardParameters*
CDPpPsdEventSegment::matchConfig(const PSDParameters& systemConfig)
{
    PSDBoardParameters* result = nullptr;
    for (int i =0; i < systemConfig.s_boardParams.size(); i++) {
        if (ourConfig(systemConfig.s_boardParams[i])) {
            result = new PSDBoardParameters;
            *result = systemConfig.s_boardParams[i];
            break;                      // Look no further on a match.
        }
    }
    return result;
}
/**
 * ourConfig
 *   @param board - a reference to a board configuration.
 *   @return bool - True if board is our configuration.
 */
bool
CDPpPsdEventSegment::ourConfig(const PSDBoardParameters& board)
{
    return (board.s_modelName == m_moduleName)                  &&
        (board.s_serialNumber  == m_serialNumber)                &&
        (board.s_linkType      == m_linkType)                    &&
        (board.s_linkNum       == m_linkNum)                     &&
        (board.s_node          == m_nodeNumber)                  &&
        (board.s_base          == m_base);
}
/**
 * openModule
 *    Connects to the module, producing a handle (m_handle) used to
 *    communicate with the board.
 *
 *    Implicit parameters are the connection specifications in the
 *    member data (constrution parameters).
 *   @throw std::string if cannot connect.
 */
void
CDPpPsdEventSegment::openModule()
{
    CAEN_DGTZ_ConnectionType linkType =
        (m_linkType == PSDBoardParameters::usb) ?
            CAEN_DGTZ_USB : CAEN_DGTZ_OpticalLink;
    CAEN_DGTZ_ErrorCode status =
        CAEN_DGTZ_OpenDigitizer(
            linkType, m_linkNum, m_nodeNumber, m_base, &m_handle
        );
    throwIfBadStatus(status, "Unable to access the board");
    
}
/**
 * getBoardInfo
 *   Fill in the m_moduleName and m_serialNumber fields.
 *   At this time, m_handle must be open on the digitizer.
 *   @throw std::stirng if cannot get information.
 */
void
CDPpPsdEventSegment::getModuleInformation()
{
    CAEN_DGTZ_BoardInfo_t info;
    CAEN_DGTZ_ErrorCode stat = CAEN_DGTZ_GetInfo(m_handle, &info);
    throwIfBadStatus(stat, "Unable to get board information");
/*
    int lsb, msb, SN;
    lsb = -1; msb = -1; SN = -1; 
    uint32_t temp;
    
    throwIfBadStatus(
            CAEN_DGTZ_ReadRegister(m_handle, 0xF084, &temp),
            "Reading a channel algorithm control register (LSB S/N)"
        );

    lsb = (0xFF & temp);

    throwIfBadStatus(
            CAEN_DGTZ_ReadRegister(m_handle, 0xF080, &temp),
            "Reading a channel algorithm control register (MSB S/N)"
        );
    msb = (0xFF & temp)<<8;

    SN = lsb + msb;
*/

    m_moduleName   = info.ModelName;
    m_serialNumber = info.SerialNumber;
    m_nChans       = info.Channels;            // We'll only set up the channels board has.


    // Figure out the timestamp calibration (stamp -> ns).

    if (info.FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE) {
      m_nsPerTick = 2;
    } else if (info.FamilyCode ==  CAEN_DGTZ_XX725_FAMILY_CODE) {
      m_nsPerTick = 4;
    } else {
      throw std::invalid_argument("This code only supports xx730 and xx725 digitizers");
    }
}
/**
 * setupBoard
 *   Set up acquisition on a board given the configuration
 *   currently in m_pCurrentConfiguration.  Note that m_handle
 *   must, of course be valid.  The  final start of the
 *   board is done in startAcuisition.  This allows boards to be setup,
 *   then the slaves started and, finally  the master started.
 */
void
CDPpPsdEventSegment::setupBoard()
{
    CAEN_DGTZ_ErrorCode status;
    
    status = CAEN_DGTZ_Reset(m_handle);
    throwIfBadStatus(status, "Resetting the board");
    
    // Reset the board.  If the user wants to calibrate it then do so:
    
    if (m_pCurrentConfiguration->s_calibrateBeforeStart) {
        throwIfBadStatus(
            CAEN_DGTZ_Calibrate(m_handle),
            "Calibrating board"
        );
    }
    // The acquistion mode is based on s_waveforms and s_energy:
    // If s_energy only then list.
    // If s_waveforms only tghen Oscilloscope.
    // if both then Mixed.
    
    CAEN_DGTZ_DPP_AcqMode_t   acqMode;
    CAEN_DGTZ_DPP_SaveParam_t storeData;
    if (m_pCurrentConfiguration->s_waveforms && m_pCurrentConfiguration->s_energy) {
        acqMode = CAEN_DGTZ_DPP_ACQ_MODE_Mixed;
        storeData = CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime;
    } else if (m_pCurrentConfiguration->s_waveforms) {
        acqMode = CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope;
        storeData = CAEN_DGTZ_DPP_SAVE_PARAM_TimeOnly;
    } else if (m_pCurrentConfiguration->s_energy) {
        acqMode = CAEN_DGTZ_DPP_ACQ_MODE_List;
        storeData = CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime;
    }
    
    throwIfBadStatus(
        CAEN_DGTZ_SetDPPAcquisitionMode(m_handle, acqMode, storeData),
        "Setting DPP Acquisition mode."
    );
    // Set per channel parameters:
    
    CAEN_DGTZ_DPP_PSD_Params_t dppParameters;
    memset(&dppParameters, 0, sizeof(dppParameters));
    uint32_t        enabledChannels(0);
    
    const uint32_t negativeBit = 1 << 16;
    const uint32_t cfdDelayMask     = 0xff;
    const uint32_t cfdSmoothMask    = 0xf << 12;
    const uint32_t cfdFracMask      = 0x300;
    const uint32_t cfdMode          = (1 << 6);   // zero in this bit is led.
    for (int i =0; i < m_nChans; i++) {
        uint32_t chSelect = i << 8;
        
        uint32_t algoControl;
        throwIfBadStatus(
            CAEN_DGTZ_ReadRegister(m_handle, DPP_ALGORITHM_CONTROL | chSelect, &algoControl),
            "Reading a channel algorithm control register (setting polarity)"
        );
            if (m_pCurrentConfiguration->s_channelConfig[i].s_polarity == PSDChannelParameters::positive) {
                algoControl &= (~negativeBit);;                     // Clear negative bit.
            } else {
                algoControl |= negativeBit;                      // set the negative bit.
            }

//	algoControl |= (1<<6); //Set bit 6 to high because compass is doing so too.
	throwIfBadStatus(
            CAEN_DGTZ_WriteRegister(m_handle, DPP_ALGORITHM_CONTROL | chSelect, algoControl),
            "Writing a channel algorithm control register."
        );
	
	 // Dynamic range selection only 0.5 and 2VPP are supported by the board.
        uint32_t ppRangeValue =
            (m_pCurrentConfiguration->s_channelConfig[i].s_dynamicRange == PSDChannelParameters::vpp2V) ?
                0 : 1;

        throwIfBadStatus(
            CAEN_DGTZ_WriteRegister(m_handle, DPP_DYNRANGE | chSelect,ppRangeValue),
            "Setting a channel dynamic range"
        );



        // Set the CFD delay.
        
        uint32_t cfdSettings;
        throwIfBadStatus(
            CAEN_DGTZ_ReadRegister(m_handle, CFD_SETTINGS | chSelect, &cfdSettings),
            "Reading the CFD Settings register set the CFD delay"
        );
        cfdSettings &= ~cfdDelayMask;
        cfdSettings |= cfdDelay(i);
        throwIfBadStatus(
            CAEN_DGTZ_WriteRegister(m_handle, CFD_SETTINGS | chSelect, cfdSettings),
            "Writing the CFD Setings register to set the CFD"
        );
        
        // Samples used to compute baseline
        
        dppParameters.nsbl[i] = nsblValue(i);
        
        // Channel enable mask.
        
        if(m_pCurrentConfiguration->s_channelConfig[i].s_enabled) enabledChannels |= (1 << i);
        

        // Short gate 
        dppParameters.sgate[i] =
            nsToSamples(m_pCurrentConfiguration->s_channelConfig[i].s_shortGate);
        
        // Cfd fraction
        
        uint32_t fractionValue = cfdFraction(i);
        throwIfBadStatus(
            CAEN_DGTZ_ReadRegister(
                m_handle, CFD_SETTINGS | chSelect, &cfdSettings
            ), "Reading CFD settings (set fraction)"
        );
        cfdSettings &= ~cfdFracMask;
        cfdSettings |= fractionValue;
        throwIfBadStatus(
            CAEN_DGTZ_WriteRegister(
                m_handle, CFD_SETTINGS | chSelect, cfdSettings
            ), "Writing CFD Fraction value"
        );
        
        dppParameters.trgho = nsToSamples(          // Trigger hold off -- not actually per ch?
            m_pCurrentConfiguration->s_channelConfig[i].s_triggerHoldoff
        );
        dppParameters.lgate[i] = nsToSamples(         // Full gate length
            m_pCurrentConfiguration->s_channelConfig[i].s_gateLen
        );
	// This value seems to be the percentage of 0xffff *sigh*

        if (m_pCurrentConfiguration->s_channelConfig[i].s_polarity == PSDChannelParameters::positive) {
		m_pCurrentConfiguration->s_channelConfig[i].s_dcOffset = 100.0 - m_pCurrentConfiguration->s_channelConfig[i].s_dcOffset;
	}

	uint32_t dcOffsetValue = 
            m_pCurrentConfiguration->s_channelConfig[i].s_dcOffset*65535.0/100.0;

        throwIfBadStatus(
            CAEN_DGTZ_SetChannelDCOffset(m_handle, i, dcOffsetValue),
            "Setting channel DC Offset"
        );
        dppParameters.csens[i] = coarseGainToSensitivity(i);
        dppParameters.pgate[i] = nsToSamples(
            m_pCurrentConfiguration->s_channelConfig[i].s_gatePre
        );
        dppParameters.selft[i] = 1;
        dppParameters.purh = CAEN_DGTZ_DPP_PSD_PUR_DetectOnly;    // Not actually per channel.
        throwIfBadStatus(
            CAEN_DGTZ_WriteRegister(
                m_handle, PRE_TRIGGER | chSelect,
                m_pCurrentConfiguration->s_channelConfig[i].s_preTrigger
            ), "Setting channel pre-trigger value"
        );

        dppParameters.tvaw[i] = 0;   // Where is this defined????!!?
	dppParameters.trgc[i] = CAEN_DGTZ_DPP_TriggerConfig_Threshold;
    }
    // Program the dpp parameters...
    
    throwIfBadStatus(
        CAEN_DGTZ_SetDPPParameters(m_handle, enabledChannels, &dppParameters),
        "Settging DPP Parameters"
    );
    
    // There are a few items that are per channel that need 
    // register writes:

    for (int i =0; i < m_nChans; i++) {
        uint32_t chSelect = i << 8;

	
	
        // Discriminator mode: Moved here from the loop before SetDPPParameters(), by B.Sudarsan.
        uint32_t algoControl;
        throwIfBadStatus(
            CAEN_DGTZ_ReadRegister(
                m_handle, DPP_ALGORITHM_CONTROL | chSelect, &algoControl
            ), "Reading DPP Algorithm control (set Discriminator mode)"
        );
        if (m_pCurrentConfiguration->s_channelConfig[i].s_discriminatorType ==
            PSDChannelParameters::led) {
            algoControl &= ~cfdMode;
        } else {
            algoControl |= cfdMode;
        };


       if(m_pCurrentConfiguration->s_coincidenceMode == PSDBoardParameters::ExtTrgGate)
		algoControl |= ((0x01<<18));
       else if(m_pCurrentConfiguration->s_coincidenceMode == PSDBoardParameters::ExtTrgVeto)
		algoControl |= ((0x3<<18));


	algoControl &= ~(1<<17);
	//std::cout << std::hex<< algoControl << '\n'<< std::dec;

        throwIfBadStatus(
            CAEN_DGTZ_WriteRegister(
                m_handle, DPP_ALGORITHM_CONTROL | chSelect, algoControl
            ), "Writing DPP Algorithm control (set Discriminator mode)"
        );

        // CFD Smoothing. Note that we'll always enable it but let the smoothing value actually
        // set whether or not it's smoothed.
        
        uint32_t localTriggerManagement;
        throwIfBadStatus(
            CAEN_DGTZ_ReadRegister(m_handle, DPP_LOCAL_TRIGGER_MANAGEMENT | chSelect, &localTriggerManagement),
            "Reading local trigger management register (CFD Smoothing)"
        );
        
        localTriggerManagement &= ~0x07;
	localTriggerManagement &= ~cfdSmoothMask;

        throwIfBadStatus(
            CAEN_DGTZ_WriteRegister(
                m_handle, DPP_LOCAL_TRIGGER_MANAGEMENT| chSelect, localTriggerManagement | cfdSmooth(i) | (1<<9)
            ),
            "Setting CFD SMoothing in local trigger management register."
        ); // (1<<9) sets timestamp mode to 010 which gives extended timestamp, flags and finetimestamp
       

        // Hard to tell but I _think_ the fixed baseline value is in register units.
        // (LSBs).
        
        uint32_t fixedBaselineValue =
            m_pCurrentConfiguration->s_channelConfig[i].s_fixedBline;
        throwIfBadStatus(
            CAEN_DGTZ_WriteRegister(
                m_handle, DPP_FIXED_BASELINE | chSelect, fixedBaselineValue
            ), "Writing channel fixed baseline register value"
        );



	// Pileup rejection gap:
        // Compass claims that this value is in LSB units in the parameter descriptor.. be prepared,
        // however for it to be in mV and need conversion if I'm wrong.
        
	PSDChannelParameters& params(m_pCurrentConfiguration->s_channelConfig[i]);
        uint32_t purGapValue = (uint32_t)(params.s_purGap);
        throwIfBadStatus(
            CAEN_DGTZ_WriteRegister(m_handle, DPP_PURGAP | chSelect, purGapValue),
                "Setting the pile up rejection gap"
        );
	
	// Convert to samples -- we then need to be multiples of
	//  8.  According to the digitzer library we can only
	// set the record length for the even channels - i+1 will get the
	// same record length because it's part of a couple.
	
	if ((i % 2) == 0) {

	
	  double reclen = m_pCurrentConfiguration->s_recordLength;
	  reclen = nsToSamples(reclen);
	  uint32_t reclenReg = reclen;
	  reclenReg = ((reclenReg+7)/8) * 8;   // Round to nearest multiple of 8 (the +7).
	  throwIfBadStatus(
	    CAEN_DGTZ_SetRecordLength(m_handle, reclenReg, i),
	    "Setting up record length"
	  );
	}
	// Can't seem to get 1n34 right without doing it myself:

	throwIfBadStatus(
	   CAEN_DGTZ_WriteRegister(
              m_handle, 0x1034 | chSelect,
	      static_cast<uint32_t>(m_pCurrentConfiguration->s_eventAggregation)
	      ),
	   "Unable to set channel events/aggregate 0x1n34"
	);

	// Pre trigger configured in ns must convert to samples:
	
	uint32_t preTrigSamples = nsToSamples(params.s_preTrigger);
	throwIfBadStatus(
 	    CAEN_DGTZ_SetDPPPreTriggerSize(m_handle, i, preTrigSamples),
	    "Unable to set per channel pre-trigger samples"
	);
	// per channel trigger threshold... seem to have to do this with register writes.
	
	uint32_t thresh = params.s_threshold;
	throwIfBadStatus(
	   CAEN_DGTZ_WriteRegister(m_handle, 0x1060 | chSelect, thresh),
	   "Unable to set per channel trigger threshold (0x1n60)"
	);

	// I don't see (yet) how the shaped trigger width is
	// set in the XML, however it seems to be set to 0xc (730?).
	// for now we'll hard code it in and hope in the future the
	// source of this value comes to light. the register is
	// a width in 16ns units for 725's and 8ns units for the 730.
	// this might mean it's only supposed to be 6 for 725's (I'm
	// developing with a 730)... if we don't see this it
	// should be part of a cheat file addtion.
	// ^ This hunch turned out to be true, which leads to the following conditional assignment

	if(m_nsPerTick==2)
	throwIfBadStatus(
	   CAEN_DGTZ_WriteRegister(m_handle, 0x1070 | chSelect, 0xc),
	   "Unable to set per channel shaped trigger width"
	);

	if(m_nsPerTick==4)
	throwIfBadStatus(
	   CAEN_DGTZ_WriteRegister(m_handle, 0x1070 | chSelect, 0x6),
	   "Unable to set per channel shaped trigger width"
	);
	
	// It seems that the dpp settings function gets the trigger holdoff width
	// register value wrong -- at least for the 730..so I'm going to  
	// compute the correct value here and override:  XML has ns.  The
	// registe ris ns/16 for 725 and ns/8 for the 730. We'll tell the difference
	// by the PS/Channel.

	int trghoDivisor;
	if (m_pCurrentConfiguration->s_psPerSample == 2000) { // V730 500MHz
	  trghoDivisor = 8;
	} else if (m_pCurrentConfiguration->s_psPerSample == 4000) { // V725 250MHz
	  trghoDivisor = 16;
	}
	uint32_t trghoReg = params.s_triggerHoldoff /trghoDivisor;
	throwIfBadStatus(
	   CAEN_DGTZ_WriteRegister(m_handle, 0x1074 | chSelect, trghoReg),
"Unable to set the trigger hold off register value"
	);

/*	throwIfBadStatus(
	    CAEN_DGTZ_WriteRegister(m_handle, DPP_ALGORITHM_CONTROL | chSelect, 0x330042),
	    "Writing a channel algorithm control register."
	);
*/

	uint32_t temp;
        throwIfBadStatus(CAEN_DGTZ_ReadRegister(m_handle, DPP_LOCAL_TRIGGER_MANAGEMENT | chSelect, &temp),"Reading local trigger management register");
      	temp |= 0x10200;
	if(m_pCurrentConfiguration->s_coincidenceMode == PSDBoardParameters::ExtTrgGate)
		temp |= 0x50;
	else if(m_pCurrentConfiguration->s_coincidenceMode == PSDBoardParameters::ExtTrgVeto)
		temp |= 0x40050;

	throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x1084 | chSelect, temp),"Writing a channel algorithm control 2 register.");
	temp = 0;
	temp = m_pCurrentConfiguration->s_coincidenceTriggerOut/(m_nsPerTick*4);
	throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x1070 | chSelect, temp), "Writing shaped trigger width");

	// End per channel settings.
    }
    
    // Board configuration -- hard coded for now
    // extras enabled, charge recording, timestamp recording, auto-flush enabled.

    throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8000, 0xe0115),
		     "Unable to setup board status register");

    

    // I/O levels:
    
    CAEN_DGTZ_IOLevel_t lvl;
    if (m_pCurrentConfiguration->s_ioLevel == PSDBoardParameters::nim) {
        lvl = CAEN_DGTZ_IOLevel_NIM;
    } else {
        lvl = CAEN_DGTZ_IOLevel_TTL;
    }

    throwIfBadStatus(
        CAEN_DGTZ_SetIOLevel(m_handle, lvl),
        "Setting front panel I/O levels"
    );
    // Trigger out signal:
    //   Note this stuff was done by reverse engineering what 
    //   CoMPASS did for each output mode...as the docs for the
    //   registers that have to be set are not so clear about the final results.
    
    setOutputMode();
    
    // Set the enabled channels mask:
    
    throwIfBadStatus(
        CAEN_DGTZ_SetChannelEnableMask(m_handle, enabledChannels),
        "Setting channel enables mask."
    );
    
    // For now set the aggregate organization to 5 -- that's what it works out to in compass

    throwIfBadStatus(
       CAEN_DGTZ_WriteRegister(m_handle, 0x800c, 5), 
       "Unable to set buffer organization"
    );
    

    /*Setup Onboard Coincidences*/
    //std::cout << "\nCoinc mode " << m_pCurrentConfiguration->s_coincidenceMode << " " << PSDBoardParameters::ExtTrgGate;
    switch(m_pCurrentConfiguration->s_coincidenceMode)
	{
		case PSDBoardParameters::ExtTrgGate: 
		case PSDBoardParameters::ExtTrgVeto: uint32_t temp;
						     throwIfBadStatus(CAEN_DGTZ_ReadRegister(m_handle, 0x811c, &temp), "Reading 0x811c");
	  				     	     temp |= ((3<<10)); 
						     throwIfBadStatus( CAEN_DGTZ_WriteRegister(m_handle, 0x811c, temp),  "Unable to set 0x811c");
						     break;
		//case PSDBoardParameters::disabled: 
		default:;
	}

   processCheatFile();

}
/**
 *  startAcquisition
 *     - Set  the clock/start delays.
 *     - Set the clock to propagate from board to board.
 *     - If necessary set external clock mode.
 *     - Set the start synchronizion daisy chain as requested.
 *     - Start/arm the board depending on master-slavedness.
 */
void
CDPpPsdEventSegment::startAcquisition()
{
    // Set the start delays.  These are provided to us in ns and must
    // be converted into delay ticks:
    
    int delayValue = nsToDelay(m_pCurrentConfiguration->s_startDelay);
    //std::cout << "\n Start Delay:" << delayValue;

    throwIfBadStatus(
        CAEN_DGTZ_WriteRegister(m_handle, DPP_START_DELAY, delayValue),
        "Setting the start delay"
    );


        if(m_pCurrentConfiguration->s_startMode == PSDBoardParameters::software)    
	{
	  uint32_t AcqControl;
	     throwIfBadStatus(
		    CAEN_DGTZ_ReadRegister(
		        m_handle, 0x8100, &AcqControl
		    ), "Reading Acq control (startmode)"
		);
     	  AcqControl &= ~0x3; //Unset bits 0,1
     	  AcqControl |= 0x00; //Write them back in as 0b01

	  throwIfBadStatus( CAEN_DGTZ_WriteRegister(m_handle, 0x8100, AcqControl),  "Unable to start acquisition(SW)"); //Arm acquisition

	}

	if(m_pCurrentConfiguration->s_startMode == PSDBoardParameters::sIn)
	{
	  uint32_t AcqControl;
	     throwIfBadStatus(
		    CAEN_DGTZ_ReadRegister(
		        m_handle, 0x8100, &AcqControl
		    ), "Reading Acq control (startmode)"
		);
     	  AcqControl &= ~0x3; //Unset bits 0,1
     	  AcqControl |= 0x1; //Write them back in as 0b01

	  throwIfBadStatus( CAEN_DGTZ_WriteRegister(m_handle, 0x8100, AcqControl),  "Unable to start acquisition(S-IN)");
	}

	if(m_pCurrentConfiguration->s_startMode == PSDBoardParameters::firstTrigger)
	{
	  uint32_t AcqControl;
	     throwIfBadStatus(
		    CAEN_DGTZ_ReadRegister(
		        m_handle, 0x8100, &AcqControl
		    ), "Reading Acq control (startmode)"
		);
     	  AcqControl &= ~0x3; //Unset bits 0,1
     	  AcqControl |= 0x2; //Write them back in as 0b10

	  throwIfBadStatus( CAEN_DGTZ_WriteRegister(m_handle, 0x8100, AcqControl),  "Unable to start acquisition(FirstTrg)");

	  throwIfBadStatus( CAEN_DGTZ_SetRunSynchronizationMode(m_handle, CAEN_DGTZ_RUN_SYNC_TrgOutTrgInDaisyChain), "Unable to start acquisition(FirstTrg)");
	  //throwIfBadStatus( CAEN_DGTZ_SetRunSynchronizationMode(m_handle, CAEN_DGTZ_RUN_SYNC_Disabled), "Unable to start acquisition(FirstTrg)");
	  throwIfBadStatus( CAEN_DGTZ_WriteRegister(m_handle, 0x8108, 1),  "Unable to start acquisition(SW)");
	}

    throwIfBadStatus(CAEN_DGTZ_SWStartAcquisition(m_handle), "Unable to start acquisition");    

/* throwIfBadStatus(
       CAEN_DGTZ_WriteRegister(m_handle, 0x8100, 6), 
       "Unable to arm acquisition"
    );*/
  
  /*Needs an additional software trigger if we have the first board with 'trgout-trgin-auto'*/
  if(m_pCurrentConfiguration->s_startMode == PSDBoardParameters::firstTrigger && m_pCurrentConfiguration->s_triggerOutputMode==PSDBoardParameters::softwareTrigger)
          throwIfBadStatus( CAEN_DGTZ_WriteRegister(m_handle, 0x8108, 1),  "Unable to start acquisition(SW)"); //Start acquisition

}

/**
 * ThrowIfBadStatus
 *     Throws an std::runtime_error if the parameter isn't CAEN_DGTZ_Success.
 *  @param status - status to check.
 *  @param error -error string to throw.
 */
void
CDPpPsdEventSegment::throwIfBadStatus(CAEN_DGTZ_ErrorCode status, const char* error)
{
    if (status != CAEN_DGTZ_Success) {
        throw std::runtime_error(error);
    }
}
/**
 * nsblValue
 *    Converts a number of baseline samples to the proper code.
 *    Invalid inputs result in an std::invalid_argument execption.
 *
 *  @param chan - Channel Number to compute this for.
 *  @return uint32_t - value to progran in the DPP parameter block for this channel.
 */
uint32_t
CDPpPsdEventSegment::nsblValue(int chan)
{
    switch (m_pCurrentConfiguration->s_channelConfig[chan].s_blineNsMean) {
        case PSDChannelParameters::fixed:
            return 0;
        case PSDChannelParameters::samples16:
            return 1;
        case PSDChannelParameters::samples64:
            return 2;
        case PSDChannelParameters::samples256:
            return 3;
        case PSDChannelParameters::samples1024:
            return 4;
        case PSDChannelParameters::samples4096:
            return 5;
        case PSDChannelParameters::samples16384:
            return 6;
        default:
            throw std::invalid_argument("Invalid value for basline averaging sample count.");
    }
}
/**
 * cfdDelay
 *    Turn the CFD delay in ns into samples.
 * @param chan - the channel to convert.
 * @return the  - samples represented  by that channel's s_cfdDelay
 */
uint32_t
CDPpPsdEventSegment::cfdDelay(int chan)
{
    double delayNs = m_pCurrentConfiguration->s_channelConfig[chan].s_cfdDelay;
    
    
    uint32_t result = nsToSamples(delayNs);
    return result;
}
/**
 * cfdSmooth
 *    Returns the cfd smoothing field of the local trigger management register.
 *
 * @param chan - the channel to return it for.
 * @return uint32_t bits 12-15 of that register in the right position.
 */
uint32_t
CDPpPsdEventSegment::cfdSmooth(int chan)
{
    unsigned smoothSamples = m_pCurrentConfiguration->s_channelConfig[chan].s_cfdSmoothing;
    
    // only some values are allowed
    
    
    switch (smoothSamples) {
        case 1:
            return 0;
        case 2:
            return 1 << 12;
        case 4:
            return 2 << 12;
        case 8:
            return 3 << 12;
        case 16:
            return 4 << 12;
        default:
            throw std::invalid_argument("Invalid CFD Smoothing value");
    }
}
/**
 * nsToSamples
 *    Convert nanoseconds to samples
 *
 *  @param ns - some value in nano seconds.
 *  @return int - number of samples that equates to.
 */
int
CDPpPsdEventSegment::nsToSamples(double ns)
{
    double psPerSample = m_pCurrentConfiguration->s_psPerSample;
    
    return ns /(psPerSample /1000.0) + .5;      // Rounded to nearest sample.
}
/**
 * nsToDelay
 *    Converts a nano second value into a number of trigger clocks.  Note that
 *    the trigger clock is 8ns for sampling times of 500MHz and 16ns for 250ns
 *    clocks.  We only support the 730 and 725 so those are the two cases we care about.
 *
 *  @param ns - number of ns of delay.
 *  @return int - value of delay register.
 */
int
CDPpPsdEventSegment::nsToDelay(double ns)
{
    // Use the sampling frequency to id the type as there are several
    // possible strings for each module type:
    
    double psPerSample = m_pCurrentConfiguration->s_psPerSample;
    if (psPerSample == 2000) {             // 2ns/sample == 730 (500MHz).
        return ns/16;                       // 730 has 8ns trigger clock (according to registers docs), 16ns division for trigger.
    } else if (psPerSample == 4000) {     // 4ns/sample == 725 (250MHz)
        return ns/32;                     // 725 has a 16s trigger clock. (according to web techspecs), 32ns division for trigger.
    } else {                              // unsupported module.
        
        throw std::invalid_argument("(nsToDelay): This module type is not supported!!");
        
    }
}
/**
 * cfdFraction
 *    Return the register value for the requested CFD Fraction
 *    This is a set of bits inside the CFD Register (bits 8/9).
 *
 *  @param chan - the channel to set.
 *  @return     - the two bit value in bits 8/9 of the CFD settings register.
 */
uint32_t
CDPpPsdEventSegment::cfdFraction(int chan)
{
    auto frac = m_pCurrentConfiguration->s_channelConfig[chan].s_cfdFraction;
    
    switch (frac) {
        case PSDChannelParameters::frac25:
            return 0;
        case PSDChannelParameters::frac50:
            return 1 << 8;
        case PSDChannelParameters::frac75:
            return 2 << 8;
        case PSDChannelParameters::frac100:
            return 3 << 8;
        default:
            throw std::invalid_argument("Invalid CFD Fraction value");
    }
}
/**
 * coarseGainToSensitivity
 *     COnverts the enum into a value.  Note that the enumeration elements
 *     closely match the configuration file values but that these don't actually
 *     track the sensitivty values.
 *
 *  @param chan - channel to set.
 *  @return int - sensitivity value
 *  @throw std::invalid_argument - if the value for that channel isn't valid.
 */
int
CDPpPsdEventSegment::coarseGainToSensitivity(int chan)
{
    switch (m_pCurrentConfiguration->s_channelConfig[chan].s_coarseGain) {
    case PSDChannelParameters::lsb2_5fc:
        return 0;
    case PSDChannelParameters::lsb10fc:
        return 1;
    case PSDChannelParameters::lsb40fc:
        return 2;
    case PSDChannelParameters::lsb160fc:
        return 3;
    case PSDChannelParameters::lsb640fc:
        return 4;
    case PSDChannelParameters::lsb2560fc:
        return 5;
    default:
        throw std::invalid_argument("Invalid charte sensitivity value");
    }
}
/**
 * setOutputMode
 *    Set the mode of the TRGOUT signal.
 * 
 */
void
CDPpPsdEventSegment::setOutputMode()
{
    
  // All painstakingly reverse engineered from Compass settings.
    
    switch (m_pCurrentConfiguration->s_triggerOutputMode)
    {
    case PSDBoardParameters::externalTrigger:
      std::cout << "\nPSD: TrgOutMode set to External";
      setLVDSExternalTrigger();
      break;
    case PSDBoardParameters::globalOrTrigger:
      std::cout << "\nPSD: TrgOutMode set to Global OR";
      setLVDSGlobalOrTrigger();
      break;
    case PSDBoardParameters::running:
      setLVDSRun();
      break;
    case PSDBoardParameters::delayedRunning:
      setLVDSDelayedRun();
      break;
    case PSDBoardParameters::sampleClock:
      setLVDSSampleClock();
      break;
    case PSDBoardParameters::pllClock:
      setLVDSPLLClock();
      break;
    case PSDBoardParameters::busy:
      setLVDSBusy();
      break;
    case PSDBoardParameters::pllUnlocked:
      setLVDSPLLLockLost();
      break;
    case PSDBoardParameters::vProbe:
      setLVDSVirtualProbe();
      break;
    case PSDBoardParameters::syncIn:
      std::cout << "\nPSD: TrgOutMode set to S_IN";
      setLVDSSIN();
      break;
    case PSDBoardParameters::softwareTrigger:
      std::cout << "\nPSD: TrgOutMode set to SW";
      setLVDSSwTrigger();
      break;
    case PSDBoardParameters::level1:
      setLVDSLevel1Trigger();
      break;
    case PSDBoardParameters::level0:
      setLVDSLevel0Trigger();
      break;
    default:
        throw std::invalid_argument(
            "Unsupported output mode requested"
        );
    }
    // Set the LVDS I/O to not participate in the trigger or: (Compass does this):

    for (int i =0; i < 8; i++) {
      uint32_t offset = 4*i;
      throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8180+offset, 0),
		       "Unable t write the trigger validation mask.");
    }
}
/**
 * needBufferFill
 *    We need a buffer fill if:
 *    - We don't have any raw, dpp or waveform buffers.
 *    - We have those buffers but our channel indices are all >= the number of hits
 *      in each channel.
 * @return bool - true if we need to fill the buffers.
 */
bool
CDPpPsdEventSegment::needBufferFill()
{
    // We assume buffer allocation is all or nothing:
    if (!m_rawBuffer) return true;
    for (int i = 0; i < CAEN_DGTZ_MAX_CHANNEL; i++) {
        if (m_nChannelIndices[i] < m_nHits[i]) return false;
    }
    return true;
}
/**
 * fillBuffer
 *    FIll and decode the buffers from the digitizer.  It's the caller's
 *    responsibility to determine that
 *    - A buffer fill is needed.
 *    - There are events in the digitizer that can fill us.
 */
void
CDPpPsdEventSegment::fillBuffer()
{
    uint32_t readSize;
    if (!m_rawBuffer) allocateBuffers();
    throwIfBadStatus(
        CAEN_DGTZ_ReadData(
                m_handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, m_rawBuffer,
                &readSize
        ),
        "Unable to read raw data from the digitizer"
    );
//  if (status != CAEN_DGTZ_Success) return;   // Unable to buffer for some reason.
  if (readSize == 0) return;                    // Nothing to read.

    throwIfBadStatus(
        CAEN_DGTZ_GetDPPEvents(
            m_handle, m_rawBuffer, readSize,
            reinterpret_cast<void**>(m_dppBuffer), m_nHits
        ), "Unable to get dpp events from the raw buffer"
    );
    // Reset the channel indices:
    
    memset(m_nChannelIndices, 0, CAEN_DGTZ_MAX_CHANNEL*sizeof(uint32_t));
    
    

}
/**
 * allocateBuffers
 *    Allocate the buffers for data acquisition.
 *    - Raw buffer.
 *    - DPP Events buffer.
 *    - Decoded waveform buffer.
 */
void
CDPpPsdEventSegment::allocateBuffers()
{

    throwIfBadStatus(
        CAEN_DGTZ_MallocReadoutBuffer(m_handle, &m_rawBuffer, &m_rawBufferSize),
        "Failed to allocated raw readout buffer"
    );
    throwIfBadStatus(
        CAEN_DGTZ_MallocDPPEvents(
            m_handle, reinterpret_cast<void**>(m_dppBuffer), &m_dppBufferSize
        ), "Failed to allocated DPP Event matrix"
    );
    throwIfBadStatus(
        CAEN_DGTZ_MallocDPPWaveforms(
            m_handle, reinterpret_cast<void**>(&m_pWaveforms), &m_wfBufferSize
        ), "Failed to allocate decoded waveform buffers."
    );
   
}
/**
 * oldestChannel
 *    The assumption is that there are hits in at least one channel.
 *    For each channel with hits, figure out the adjusted timestamp.
 *    The adjusted timestamp is a 64 bit timestamp computed from the
 *    Raw hit timestamp and the number of times it's wrapped.
 *    We also detect wraps here using and, if necessary, updating
 *    m_lastTimestamps and m_timestampAdjust
 *
 *  @return uin32_t - the channel with the oldest timestamp.
 *  @throw std::logic_error - if there are no channels with data as that's supposed
 *                       to have been taken care of for us.
 */
uint
CDPpPsdEventSegment::oldestChannel()
{
    uint32_t result = 0xffffffff;           // Illegal channel.
    uint64_t smallest = 0xffffffffffffffff; // ALl timestamps will be smaller than this.
    uint64_t adjustout = 0x0;
    for (int i =0; i < CAEN_DGTZ_MAX_CHANNEL; i++) {
        // Only look at channels with data left buffered:
        
        if (m_nChannelIndices[i] < m_nHits[i]) {
            CAEN_DGTZ_DPP_PSD_Event_t* pHit = &(m_dppBuffer[i][m_nChannelIndices[i]]);
            uint32_t rawTimestamp = pHit->TimeTag;
            
            uint64_t adjust = m_timestampAdjust[i];
            
            // Is there one more adjust:
	    //std::cout << "\n--->" << std::hex<<     rawTimestamp  << "\t" <<  m_lastTimestamps[i] << std::dec << "\n";            
//            if (rawTimestamp <= m_lastTimestamps[i]) adjust += UINT64_C(0x100000000);
            if (rawTimestamp <= m_lastTimestamps[i]) adjust += UINT64_C(0x80000000); //why does this work? :(
            
            // Figure out the 64 bit timestamp:
            
            uint64_t adjustedTimestamp = adjust + rawTimestamp;
            if (adjustedTimestamp < smallest) {
                result = i;
                smallest = adjustedTimestamp;
            }
        }
    }
    
    if (result == 0xffffffff) {
        throw std::logic_error("BUG- CDPpPsdEvent found no valid smallest timestamp!!");
    }
    // We need to commit the last timestamp and timestamp adjust for the  channel.
    // We can retrieve this from the smallest timestamp:
    
    m_timestampAdjust[result] = smallest & 0xffffffff80000000;   // Top 32 bits. (33?)
    m_lastTimestamps[result]  = smallest & 0x7fffffff;           // Bottom 32 bits. (31?)

    //std::cout << "\n" << std::hex<<     m_timestampAdjust[result]  << "\t" <<  m_lastTimestamps[result] << std::dec << "\n";
    
    return result;
}
/**
 * formatEvent
 *    Given a channel:
 *    - Format an event into the event buffer from that channel.
 *    - The resulting event will have the following format:
 *    |  32 bit total event size         |  (Self inclusive bytes).
 *    |  64 bit derived timestamp        |  (also tags the event, as does our source id).
 *    |  16 bit channel number           |
 *    |  32  bit short-gate charge value |
 *    |  32 bit long-gate charge value   |
 *    |  16 bit baseline                 |
 *    |  16 bit pileup rejection indicator |
 *    |  32 bit waveform length          | (0 if traces are not being sasved.)
 *    |  The waveform data if it was taken |
 *
 * @param pBuffer  - Pointer to the buffer describing where the data goes
 * @param chan     - channel from which the data comes.
 * @return size_t  - Size of the events in bytes (same as what's put in the first uint32_t).
 * @note the channels m_nChannelIndices value is incremented to consume the event.
 */
size_t
CDPpPsdEventSegment::formatEvent(void* pBuffer, int chan)
{
    // hold a pointer to the event size:
    
    uint32_t* pSize  = static_cast<uint32_t*>(pBuffer);
    uint64_t* pStamp = reinterpret_cast<uint64_t*>(pSize + 1);
    
    // Get a pointer to the hit and fill in the adjusted timestamp value and all
    // the simple stuff from the hit information.
    
    CAEN_DGTZ_DPP_PSD_Event_t* pHit = &(m_dppBuffer[chan][m_nChannelIndices[chan]]);
    m_nChannelIndices[chan]++;
    uint64_t adjustedStamp = pHit->TimeTag;
    adjustedStamp         += m_timestampAdjust[chan];
    *pStamp++ = adjustedStamp*m_nsPerTick;
    //std::cout << "\nTs:" << adjustedStamp*m_nsPerTick;
    
    // Set the timestamp and the source id.
    
    setTimestamp(adjustedStamp*m_nsPerTick);
    setSourceId(m_nSourceId);

    // CHannel number:

    uint16_t* p16 = reinterpret_cast<uint16_t*>(pStamp);
    *p16++ = chan;
    //std::cout << " ch:" << chan;
    
    // now the charges:
    
    uint32_t* p32 = reinterpret_cast<uint32_t*>(p16);
    *p32++  = pHit->ChargeShort;
    *p32++  = pHit->ChargeLong;
    //std::cout << " Elong:" << pHit->ChargeShort;
//    std::cout << " Extras:" << pHit->Extras;
     uint32_t temp = (pHit->Extras&0xf000)>>12;
//     if(temp)
	
     /*temp has the structure: 0b(ABCD) with bit A = trigger lost, B=over range (set when a trigger is lost or over range in a single event)*/
     /* C = set each time 128 triggers are counted, D is set each time 128 triggers are lost */
     if(temp&2)//
     {
//	m_triggerCount[chan] += 128.;//./(double(clock() - t[chan]));
//	t[chan] = clock();
	auto now =    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	m_triggerCount[chan] = static_cast<uint32_t>(128./((now-t[chan])*1.e-3));
	t[chan] = now;
        //std::cout << "\n Extras:" << pHit->Extras << " 1024s:" << temp << " ft:" << (pHit->Extras&0x1ff) << " xt:" << ((pHit->Extras&0xffff0000) >>16);
     }
     if(temp&1)//
     {
	auto now =    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	m_missedTriggers[chan] = static_cast<uint32_t>(128./((now-tmiss[chan])*1.e-3));
	tmiss[chan] = now;
     }

    // Now the baselines and the Pileup rejection flag:
    *p32++ = pHit->Extras;
    //std::cout << std::hex<< "\n Extras: 0x" << pHit->Extras << std::dec << " 1024s:" << temp << " ft:" << (pHit->Extras&0x1ff) << " xt:" << ((pHit->Extras&0xffff0000) >>16);
    /* Old form here */
    //p16 = reinterpret_cast<uint16_t*>(p32);
    //*p16++ = pHit->Baseline;
    //*p16++ = pHit->Pur;
    
    //p32    = reinterpret_cast<uint32_t*>(p16);
    
    /* Now the waveforms:
       While the XML does not yet support dual traces,
       we write the code as if it does as well as handling
       analog probes  The data we write is:
       |  uint32_t size in bytes,                                     |
       |   uint32_t num. samples                                      |
       |   uint8_t nonzero if dual trace mode                        |
       |   uint8_t nonzero if analog probe 1 is on idents the probe   |
       |  Trace one - if num samples > 0  ns*uint16_t                |
       |  Trace two - if dt,                                         //
       
       Note that if number of samples is zero, size in bytes is sizeof(uint32_t)
       and no further data are included.
       
       Note: We assume that sizeEvent has already decoded the waveform as
       that's necessary to determine the size of the event.
   */
    
    uint8_t* p8;
    if (m_pWaveforms->Ns == 0) {
        *p32++ = sizeof(uint32_t);             // NO traces to write.
        p8     = reinterpret_cast<uint8_t*>(p32);  // end of event.
    } else {
        *p32++ = sizeTraces();
        *p32++ = m_pWaveforms->Ns;
         p8    = reinterpret_cast<uint8_t*>(p32);
        *p8++  = m_pWaveforms->dualTrace;
        *p8++  = m_pWaveforms->anlgProbe;
        p16    = reinterpret_cast<uint16_t*>(p8);    // Traces go here:
        memcpy(p16, m_pWaveforms->Trace1, m_pWaveforms->Ns*sizeof(uint16_t));
        p16   += m_pWaveforms->Ns;
        if(m_pWaveforms->dualTrace) {               // second trace
            memcpy(p16, m_pWaveforms->Trace2, m_pWaveforms->Ns*sizeof(uint16_t));
            p16 += m_pWaveforms->Ns;
        }
        p8 = reinterpret_cast<uint8_t*>(p16);      // end of event.
    }
    
    // Figure out the event size and
    // Set it in pSize and return it:
    
    uint32_t nBytes = (p8 - (reinterpret_cast<uint8_t*>(pBuffer)));
    *pSize = nBytes;
    
    return static_cast<size_t>(nBytes);
    
}
/**
 * sizeTraces
 *    Figure out how many bytes of trace data there are to write.
 *    We assume that m_pWaveforms has been decoded into by (e.g.) sizeEvent.
 *
 * @return uint32_t number of bytes of trace data, including the size uint32_t.
 * @note we also assume this is only called if, in fact, there are traces.
 */
uint32_t
CDPpPsdEventSegment::sizeTraces()
{
    // There's a fixed header of a size, sample count, dual trace flag and
    // analog probe selector:
    
    uint32_t result = 2*sizeof(uint32_t) + 2*sizeof(uint8_t);
    
    // The number of traces depends on the dual trace count.
    // Each trace is number of samples * sizeof(uint16_t).
    
    int ntraces = 1;
    if (m_pWaveforms->dualTrace) {
        ntraces = 2;
    }
    result += ntraces*m_pWaveforms->Ns*sizeof(uint16_t);
    return result;
}
/**
 * sizeEvent
 *    Determines the size of an event.
 * @param chan - the channel number.
 * @return size_t - number of bytes in the event.
 * @note to do this, the waveforms, if any, for the event will be decoded.
 */
size_t
CDPpPsdEventSegment::sizeEvent(int chan)
{
    throwIfBadStatus(
        CAEN_DGTZ_DecodeDPPWaveforms(
            m_handle,
            &(m_dppBuffer[chan][m_nChannelIndices[chan]]),
            m_pWaveforms
        ), "Decoding hit waveforms"
    );
    
    // The event consists of the fixed header and optional traces:
    
    size_t result = 3*sizeof(uint32_t) + sizeof(uint64_t) + 2*sizeof(uint16_t);
    
    // Now the waveforms:
    
    if (m_pWaveforms->Ns == 0) {
        result += sizeof(uint32_t);             // Just the empty waveform size.
    } else {
        result += sizeTraces();
    }
    
    
    return result;
}
/**
 *  freeDAQBuffers
 *      Free the dynamically allocated CAEN buffers.  Note that all pointers
 *      will then be set to nullptr to ensure that our code knows next time around,
 *      it needs to reallocate the data.
 */
void
CDPpPsdEventSegment::freeDAQBuffers()
{
    CAEN_DGTZ_FreeReadoutBuffer(&m_rawBuffer);
    CAEN_DGTZ_FreeDPPEvents(m_handle, reinterpret_cast<void**>(m_dppBuffer));
    CAEN_DGTZ_FreeDPPWaveforms(m_handle, reinterpret_cast<void*>(m_pWaveforms));
    
    m_pWaveforms = nullptr;
    m_rawBuffer = nullptr;
    for (int i = 0; i < CAEN_DGTZ_MAX_CHANNEL; i++) {
        m_dppBuffer[i] = nullptr;
    }
}
/**
 * setLVDSLevel0Trigger
 *    Enable the new lvds features and set the LVDS outputs
 *    to reflect the Level 0 board trigger.
 */
void
CDPpPsdEventSegment::setLVDSLevel0Trigger()
{
  // Set the FP LVDS I/O new features register and enable new features:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111),
		   "Unable to set the LVDS I/O New features register");


  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x8100),
		   "Unable to write FP I/O control register.");
}
/**
 setLVDSLevel1Trigger
   Enable the new lvds features and set the LVDS Otputs to reflect the
   level 1 board trigger.
*/
void 
CDPpPsdEventSegment::setLVDSLevel1Trigger()
{
  // Set the FP LVDS I/O new features register and enable new features:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111),
		   "Unable to set the LVDS I/O New features register");


  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0xc100),
		   "Unable to write FP I/O control register.");
}
/**
 setLDVDSSwTrigger
   Set the LVDS outputs to reflect the Software trigger.
*/
void
CDPpPsdEventSegment::setLVDSSwTrigger()
{
  // Enable software trigger:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0x8000ffff),
		   "Unable to enable sw trigger in mask register");

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x810c, 0x8000ffff),
		   "Unable to enable sw trigger in mask register");

  // Enable new LVDS Features:
  int temp = (m_pCurrentConfiguration->s_ioLevel==PSDBoardParameters::nim)? 0 : 1 ;

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x0100+temp),
		   "Unable to enable new lvds featurs in FP IO control register"
		   );
  // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");
}
/**
 setLVDSExternalTrigger
   Set the LVDS outputs to reflect the state of the external trigger.
*/
void
CDPpPsdEventSegment::setLVDSExternalTrigger()
{
  // Enable software trigger:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0x40000000),
		   "Unable to enable sw trigger in mask register");
  // Enable new LVDS Features:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x100),
		   "Unable to enable new lvds featurs in FP IO control register"
		   );
  // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");
}
/**
 setLVDSGlobalOrTrigger
    LVDS outputs reflect the global or of the cou;le triggers.
*/
void
CDPpPsdEventSegment::setLVDSGlobalOrTrigger()
{

  // Set the channel couples to or mode:

  for (int i = 0; i < m_nChans; i++) {
    throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x1084 | (i << 8), 0x10207),
		     "Could not set the per channel DPPAlgorithm control 2 register");
  }

  //Disable local shaped trigger
//  for (int i = 0; i < m_nChans; i++) {
//    throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x1084 | (i << 8), 0x200),
//		     "Could not set the per channel DPPAlgorithm control 2 register");
//  }

  //  Enable the couples to participate

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0xff),
		   "Could not set the FP-GPO Trigger enable mask register");
  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x100),
		   "Could not write the FP-IO control register");
 // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");
}
/**
 setLVDSRun
   Set the lVDS outputs to reflect the board is running
*/
void
CDPpPsdEventSegment::setLVDSRun()
{
  // set the global trigger mask:triggers -> FPIO.

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0x80000000),
		   "Failed to write the global trigger mask");

  // Enable new features; trgout reflects run state

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x10100),
		   "Failed to write the FP I/O COntrol register");

 // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");
}
/**
 setLVDSDelayedRun
    set the lvds output to reflect a delayed run state.
*/
void
CDPpPsdEventSegment::setLVDSDelayedRun()
{
   // set the global trigger mask:triggers -> FPIO.

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0x80000000),
		   "Failed to write the global trigger mask");

  // Enable new features; trgout reflects delayed run state

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x110100),
		   "Failed to write the FP I/O COntrol register");

 // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");
}
/**
 setLVDSSampleCLock
   Sets the lVDS outputs to reflect the sampling clock.
*/
void
CDPpPsdEventSegment::setLVDSSampleClock()
{
   // set the global trigger mask:triggers -> FPIO.

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0x80000000),
		   "Failed to write the global trigger mask");

  // Enable new features; trgout reflects Sample Clock

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x50100),
		   "Failed to write the FP I/O COntrol register");

 // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");
}

/** 
 *  setLVDSPLLClock
 *    Set the LVDS outputs to reflect the clock
 *    reconstructed by the PLL.
 */
void
CDPpPsdEventSegment::setLVDSPLLClock()
{
   // set the global trigger mask:triggers -> FPIO.

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0x80000000),
		   "Failed to write the global trigger mask");

  // Enable new features; trgout reflects Motherboard virtual probe
  // and set that probe to the CLK Phase (presumably the PLL Recovered 
  // clock?

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x90100),
		   "Failed to write the FP I/O COntrol register");

 // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");
}
/**
 setLVDSBusy
    Set the LVDS outputs to reflect the busy signal.
    this is done by selecting motherboard probes and for
    those probes to be the BUSY/UNLOCK.without the PLL Lock loss
    bit set. in the FP I/O control register.
*/
void
CDPpPsdEventSegment::setLVDSBusy()
{
   // set the global trigger mask:triggers -> FPIO.

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0x80000000),
		   "Failed to write the global trigger mask");

  // Enable new features; trgout reflects Motherboard virtual probe
  // and set that probe to the Busy/Unlock but not setting bit
  // 20 (PLL Lock lost).

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0xd0100),
		   "Failed to write the FP I/O COntrol register");

 // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");
}
/**
 *  setLVDSPllLockLost
 *    Sets the LVDS Outputs to reflect PLL synchronization lost.  
 *    this is done the same way as setting the busy but
 *    additionally setting bit 20 to select the PLL Lock loss rather
 *    than busy output.
 */
void
CDPpPsdEventSegment::setLVDSPLLLockLost()
{
   // set the global trigger mask:triggers -> FPIO.

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0x80000000),
		   "Failed to write the global trigger mask");

  // Enable new features; trgout reflects Motherboard virtual probe
  // and set that probe to the Busy/Unlock but not setting bit
  // 20 (PLL Lock lost).

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x1d0100),
		   "Failed to write the FP I/O COntrol register");

 // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");
}
/**
 setLVDSVirtualProbe
    Sets the LVDS to reflect the output of the channel probes.
*/
void 
CDPpPsdEventSegment::setLVDSVirtualProbe()
{
   // set the global trigger mask:triggers -> FPIO.

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0x80000000),
		   "Failed to write the global trigger mask");

  //   Select the channel virtual probes.

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x20100),
		   "Failed to write the FP I/O COntrol register");

 // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");
}
/**
  setLVDSSIN
    The TRGOUT/LVDS outputs become the SIN input to propagate
    the daisy chain
*/
void
CDPpPsdEventSegment::setLVDSSIN()
{   // set the global trigger mask:triggers -> FPIO.

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0x80000000),
		   "Failed to write the global trigger mask");

  // Enable new features; trgout reflects SIN


  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x811c, 0x30100),
		   "Failed to write the FP I/O COntrol register");

 // Set the lVDS to reflect triggers:

  throwIfBadStatus(CAEN_DGTZ_WriteRegister(m_handle, 0x81a0, 0x1111), 
		   "Unable to set LVDS output");

}


static std::string trim( std::string str )
{
    // remove trailing white space
    while( !str.empty() && std::isspace( str.back() ) ) str.pop_back() ;

    // return residue after leading white space
    std::size_t pos = 0 ;
    while( pos < str.size() && std::isspace( str[pos] ) ) ++pos ;
    return str.substr(pos) ;
}


/**
 * processCheatFile
 *    Process the register cheat file.
 *    If m_pCheatFile is nullptr, nothing is done.
 *    Otherwise, the cheat file is opened and processed line by line.
 *    The cheat file has the following format:
 *    operation address value
 *    Operation is one of
 *       - . - set the value,
 *       - # ignore the line (comment)
 *       - | or the value into the register.
 *       - * And the value into the registger
 *       
 *    address - is the address of the register to be modified.
 *    value   - is the value that's either set or ored into the
 *    register depending on the operation.
 *    operation, addresss and value must  be separated by whitespace.
 *    Lines with errors result in warnings but are ignored.
 *    Borrowed from CAENPha.h implementation in DPP-PHA library
 */
void CDPpPsdEventSegment::processCheatFile()
{
  if (!m_pCheatFile) return;
  std::ifstream infile(m_pCheatFile);
  if (!infile) {
    std::cerr << "Cheat file: " << m_pCheatFile << "could not be opened\n";
    return;
  }
  while (!infile.eof()) {
    std::string line;
    std::getline(infile, line);
    line = trim(line);              // Lose leading and trailing whitespace.
    if (!line.empty()) {            // Ignore blank lines which are empty after trimming.
      std::stringstream s(line);
      char op;
      std::string sAddr;
      std::string sValue;
      uint32_t  addr;
      uint32_t  value;
      op = '\0';
      s >> op >> sAddr >> sValue;
      
      
      if (s.fail())  {  // Note that I don't think the decode can fail going into strings...
        std::cerr << "Warning: '" << line << "' was in error\n";
        s.clear(std::ios_base::failbit);
      } else {
        
        // What we do depends on the operation:
        
        // If the operation is not a comment, we can decode the address and
        // value:
        
        if (op != '#')   {
          addr  = strtoul(sAddr.c_str(), nullptr, 0);
          value = strtoul(sValue.c_str(), nullptr, 0);
        }
        switch (op) {
          case '#':                                    // comment.
            break;
          case '.':                                   // set:
            {
              CAEN_DGTZ_WriteRegister(m_handle, addr, value);
            }
            break;
          case '|':                                 // bitwise or.
            {
              uint32_t currentValue;
              CAEN_DGTZ_ReadRegister(m_handle, addr, &currentValue);
              value |= currentValue;
              CAEN_DGTZ_WriteRegister(m_handle, addr, value);
            }
            break;
          case '*':                               // bitwise and.
            {
              uint32_t currentValue;
              CAEN_DGTZ_ReadRegister(m_handle, addr, &currentValue);
              value &= currentValue;
              CAEN_DGTZ_WriteRegister(m_handle, addr, value);
            }
            break;
          default:                               // unrecognized operation.
            std::cerr << "Unrecognized operation in '" << line << "'\n";
            break;
        }
      }
    }
  }
}
