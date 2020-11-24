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
* @file     CAENPha.cpp
* @brief    Implementation of low lever driver class for CAEN PHA firmware digitizers.
* @author   Ron Fox (rfoxkendo@gmail.com)
*
*
*/
#include "CAENPha.h"
#include <vector>
#include <stdexcept>
#include <CAENDigitizerType.h>
#include <CAENDigitizer.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <sstream>

static std::string trim( std::string str )
{
    // remove trailing white space
    while( !str.empty() && std::isspace( str.back() ) ) str.pop_back() ;

    // return residue after leading white space
    std::size_t pos = 0 ;
    while( pos < str.size() && std::isspace( str[pos] ) ) ++pos ;
    return str.substr(pos) ;
}
/////////////////////////////////////////////////////////////////////////
/**
 * Constructor
 *   Squirrel away the configuration.
 *
 * @param config - reference to a configuration object.
 * @param linkType - Type of link connecting us to the digitzer.
 * @param linknum  - Link number.
 * @param node     - Connet node if appropriate.
 * @param base     - base address if appropriate
 * @param startmode - Determines the start and multiboard synch modes:
 * @param trgout  - True if GPO is triggerout else it's synch.
 * @parm  delay   - start delay for clock synchronization.
 * @param pCheatCFile - Pointer to register cheat file - nullptr means don't cheat.
 */
CAENPha::CAENPha(
    CAENPhaParameters& config, CAEN_DGTZ_ConnectionType linkType, int linknum,
    int node, uint32_t base,
    CAEN_DGTZ_AcqMode_t startMode, bool trgout, unsigned delay, const char* pCheatFile
  ) :
  m_configuration(config),
  m_startMode(startMode),
  m_trgout(trgout),
  m_startDelay(delay),
  m_rawBuffer(0),
  m_rawSize(0),
  m_dppSize(0),
  m_pWaveforms(0),
  m_wfSize(0),
  m_pCheatFile(pCheatFile)
  
{
  CAEN_DGTZ_ErrorCode status = CAEN_DGTZ_OpenDigitizer(linkType, linknum, node, base, &m_handle);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Open failed", status);
  }
  conet_node = node;
  for (int i =0; i < CAEN_DGTZ_MAX_CHANNEL; i++) {
    m_dppBuffer[i]  = 0;
    m_nDppEvents[i] = 0;
    m_nOffsets[i]   = 1;
    m_nTimestampAdjusts[i] = 0;
    m_nLastTimestamp[i]    = 0;
  }
  
  status = CAEN_DGTZ_GetInfo(m_handle, &m_info);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("GetBoardInfo failed", status);
  }
}
/**
 * destructor - close the board.
 */
CAENPha::~CAENPha()
{
  CAEN_DGTZ_CloseDigitizer(m_handle);
}

/**
 * setup
 *   This is the monster method.  Initialize the DPP-PHA mode for the
 *   module (assumed where needed to be a 725).
 */
void
CAENPha::setup()
{
  CAEN_DGTZ_ErrorCode status;
  CAEN_DGTZ_BoardInfo_t boardInfo;


  status = CAEN_DGTZ_GetInfo(m_handle, &boardInfo);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Get info failed", status);
  }

  status = CAEN_DGTZ_Reset(m_handle);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Board Reset failed", status);
  }

  // We only support the 725 and 730 (250Mhz and 500Mhz).
  // set m_nsPerTick appropriately or complain about the digitizer type:

  if(boardInfo.FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE) {
    m_nsPerTick = 2;
    m_nsPerTrigger = 16;
  } else if (boardInfo.FamilyCode == CAEN_DGTZ_XX725_FAMILY_CODE) {
    m_nsPerTick = 4;
    m_nsPerTrigger = 32;
  } else {
    throw std::pair<std::string, int>("Un supported digitizer family", boardInfo.FamilyCode);
  }

  m_enableMask = setChannelMask();
  
  // Set individual trigger, mb1, propagate triggers, TRG validation(?)

  CAEN_DGTZ_WriteRegister(m_handle, 0x8008, 0xffffffff);    // Clear all bits.
  status = CAEN_DGTZ_WriteRegister(m_handle, 0x8004, 0x0100e0115);
 // status = CAEN_DGTZ_WriteRegister(m_handle, 0x8004, 0x14e0105);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Write board config register failed", status);
  }
  // Acquisition mode is either Mixed or list - 0 for mixed, 1 for list.

  status = CAEN_DGTZ_SetDPPAcquisitionMode(
     m_handle,
     m_configuration.acqMode == 1 ?
        CAEN_DGTZ_DPP_ACQ_MODE_List : CAEN_DGTZ_DPP_ACQ_MODE_Mixed, 
     CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime
     );
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Could not set acquisition mode", status);
  }
  // Waveform acquisition window length:  TODO:   Make division board independent.

  uint32_t rlen = m_configuration.recordLength/m_nsPerTick; // Reclen in ticks from ns.
  status = CAEN_DGTZ_SetRecordLength(m_handle, rlen);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Set record length failed", status);
  }
  // Enable data flush for slow rates: (bit set register)
  //status = CAEN_DGTZ_WriteRegister(m_handle, 0x8004, 1);
  //if (status != CAEN_DGTZ_Success) {
  //  throw std::pair<std::string, int>("Enable flush mode failed", status);
  //}
 
  setTriggerAndSyncMode();
  sleep(1);

 // setPerChannelParameters();
  
  // Use a pretty generic buffer organization for now:
  status = CAEN_DGTZ_SetDPPEventAggregation(m_handle, 0, 0);  // Let board/lib figure it out
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Set DPP Event aggregation failed", status);
  }
  status = CAEN_DGTZ_SetMaxNumAggregatesBLT(m_handle, 255);   // max Buffers/read.
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Set max transfer aggregation failed", status);
  }
  // Set up the trigger/coincidence mode.
  
  setCoincidenceTriggers(); //Does not really set coincidences : Sudarsan B. 
  setPerChannelParameters();
  calibrate();
  
  // Allocate data buffers and start the digitizer:
  
  status = CAEN_DGTZ_MallocReadoutBuffer(m_handle, &m_rawBuffer, &m_rawSize);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to malloc readout buffer", status);
  }
  status = CAEN_DGTZ_MallocDPPEvents(m_handle, (void**)m_dppBuffer, &m_dppSize);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to allocate DPP Event struct", status);  
  }
  status = CAEN_DGTZ_MallocDPPWaveforms(m_handle, (void**)&m_pWaveforms, &m_wfSize);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to malloc DPP Waveform storage", status);
  }
  processCheatFile();
  
  // If in unsynchronized mode, this starts acquisition. If in synchronized mode,
  // this arms acquisition.
  
  status = CAEN_DGTZ_SWStartAcquisition(m_handle);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to start or arm acquisition", status);
  }

  std::cout << "\nWaiting for acquisition run..";

}

/**
 * shutdown
 *   Turn off data taking in the digitizer.
 */
void
CAENPha::shutdown()
{
  CAEN_DGTZ_ErrorCode status = CAEN_DGTZ_SWStopAcquisition(m_handle);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to stop acquisition", status);
  }
  // Free the data buffers allocated when the digitizer was setup.
  
  status = CAEN_DGTZ_FreeReadoutBuffer(&m_rawBuffer);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to free readout bufer", status);
  }
  m_rawBuffer = 0;
#ifdef FREE_WAVEFORMS  
  status = CAEN_DGTZ_FreeDPPWaveforms(m_handle, &m_pWaveforms);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to free DPP waveform storage", status);
  }
#endif
  m_pWaveforms = 0;
  
  status = CAEN_DGTZ_FreeDPPEvents(m_handle, reinterpret_cast<void**>(&m_dppBuffer));
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to free dpp events buffer", status);
  }
  for (int i=0; i < CAEN_DGTZ_MAX_CHANNEL; i++) {
    m_dppBuffer[i] = 0;
    m_nDppEvents[i] = 0;
    m_nOffsets[i]  =1;
    m_nTimestampAdjusts[i] = 0;
    m_nLastTimestamp[i]    = 0;
  }
}
/**
 * haveData
 *  true if the digitizer has data that can be read.
 */
bool
CAENPha::haveData()
{
  if (dataBuffered()) return true;      // Un delivered buffered data.
  
  // CAEN (Alberto) Says I should just try the read... 
 /*     uint32_t statusRegister;
  
      status= CAEN_DGTZ_ReadRegister(m_handle,CAEN_DGTZ_ACQ_STATUS_ADD , &statusRegister);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Unable to read status register to see if there are events", status);
  }
*/
//        
  

  
  fillBuffers();                    // In anticipation of read.
  if (!dataBuffered()) {
    return false;
  } else {
    return true;
  }
  
}
/**
 * Read
 *    Return the event not yet returned with the earliest timestamp.
 *
 * @return std::tuple<int, const CAEN_DGTZ_DPP_PHA_Event_t*, CAEN_DGTZ_DPP_PHA_Waveforms_t>
 *    - The first item of the pair is a channel number and the second item
 *      is a pointer to the event struct, third are the decoded waveforms from
 *      the event.
 * @note it is possible for the event pointer to be null.  This means that
 *       although the digitizer showed there was data, fillBuffer
 *       was not able to either read anything or decode any dpp events from it.
 *       In that case, invoke haveData again to check for data and, retry
 *       the read/decode.  Naturally, in that case, the channel number is
 *       meaningless.
 */
//Version borrowed from DPP-PHA4
std::tuple<int, const CAEN_DGTZ_DPP_PHA_Event_t*, const CAEN_DGTZ_DPP_PHA_Waveforms_t*>
CAENPha::Read()
{
  if (!dataBuffered()) {
    
    return std::make_tuple(
      0,
      reinterpret_cast<CAEN_DGTZ_DPP_PHA_Event_t*>(0),
      reinterpret_cast<CAEN_DGTZ_DPP_PHA_Waveforms_t*>(0)
    );  // no data.
  }
  // Get the channel with the lowest timestamp:
  
  int channel = findEarliest();
  
  // Get the pointer to the data and do necessary book keeping:
  
  int offset  = m_nOffsets[channel];
  CAEN_DGTZ_DPP_PHA_Event_t* pData = &(m_dppBuffer[channel][offset]);
  CAEN_DGTZ_DecodeDPPWaveforms(m_handle, pData, m_pWaveforms);
  offset++;
  m_nOffsets[channel] = offset;
  
  // Adjust the 32 bit timestamp into a 64 bit one:
  //std::cout<< '\n' << pData->TimeTag << '\t' << pData->TimeTag*m_nsPerTick;
  pData->TimeTag = pData->TimeTag*m_nsPerTick; //pre-process, convert to ns
  

  if (pData->TimeTag < m_nLastTimestamp[channel]) {
    if(m_nsPerTick == 2) m_nTimestampAdjusts[channel] += 0x100000000;
    else m_nTimestampAdjusts[channel] += 0x200000000;  //Why does this work? 
	// Rolled over (hopefully only once).
    // m_nTimestampAdjusts[channel] += 0x00000000;
  }
  m_nLastTimestamp[channel] = pData->TimeTag;
  pData->TimeTag |= m_nTimestampAdjusts[channel]; // Fold in the wraps.
  
  //pData->TimeTag *= m_nsPerTick;
  //std::cout << "\tTimetag:"<<pData->TimeTag;
  
  // Construct/return the result:
  
  return std::make_tuple(channel,  pData, m_pWaveforms);
}





/*------------------------------------------------------------------------------
 * private utilities.

/**
 * setChannelMask
 *   Compute the channel mask and set it:
 *   Modify only if the enabled flag is true (for compass).
 */

int
CAENPha::setChannelMask()
{
  int enableMask = 0;
  for (int i =0; i < m_configuration.m_channelParameters.size(); i++) {
    if (m_configuration.m_channelParameters[i].second->enabled)  {
      enableMask |= (1 << m_configuration.m_channelParameters[i].first);
    }
  }
  int status = CAEN_DGTZ_SetChannelEnableMask(m_handle, enableMask);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to set channel enables", status);
  }
  return enableMask;
}

/**
 * setTriggerAndSyncModes
 *  Set trigger and synchronization modes:
 *
 */
void
CAENPha::setTriggerAndSyncMode()
{
  int status = CAEN_DGTZ_SetIOLevel(m_handle, static_cast<CAEN_DGTZ_IOLevel_t>(m_configuration.IOLevel));
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to set I/O level", status);
  }

  // Start and synch mode are coupled:

  
  // Regardless, the start mode is the m_startMode:
  
  status = CAEN_DGTZ_SetAcquisitionMode(m_handle, m_startMode);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to set acq mode to sw controlled", status);
  }
  
  switch (m_startMode) {
  case CAEN_DGTZ_SW_CONTROLLED :
    std::cout << "\nPHA Start Mode: Software";
    status = CAEN_DGTZ_SetRunSynchronizationMode(m_handle, CAEN_DGTZ_RUN_SYNC_Disabled);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set run sync mode to Disabled", status);
    }
    status = CAEN_DGTZ_WriteRegister(m_handle, 0x8170, 2*m_startDelay/m_nsPerTrigger);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set run sync mode to Disabled", status);
    }
    
    break;
  case CAEN_DGTZ_FIRST_TRG_CONTROLLED :
    std::cout << "\nPHA Start Mode: FIRST_TRG ";
    status = CAEN_DGTZ_SetRunSynchronizationMode(m_handle, CAEN_DGTZ_RUN_SYNC_TrgOutTrgInDaisyChain);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set run sync mode to Disabled", status);
    }
    status = CAEN_DGTZ_WriteRegister(m_handle, 0x8170, 2*m_startDelay/m_nsPerTrigger);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set run sync mode to Disabled", status);
    }
    break;
  case CAEN_DGTZ_S_IN_CONTROLLED:
    std::cout << "\nPHA Start Mode: Sync_in ";
    status = CAEN_DGTZ_SetRunSynchronizationMode(m_handle, CAEN_DGTZ_RUN_SYNC_TrgOutSinDaisyChain);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set run start on SIN", status);
    }
    status = CAEN_DGTZ_WriteRegister(m_handle, 0x8170, 2*m_startDelay / m_nsPerTrigger);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set start delay", status);
    }    
    //   Software start will arm the board.
    break;
  default:
    throw std::pair<std::string, int>("Invalid start mode value", -1);
  }
  // Set internal/external trigger mode:

  switch (m_configuration.triggerSource) {
  case CAENPhaParameters::internal:
    std::cout << "\nPHA: Internal trigger on this board";
    status = CAEN_DGTZ_WriteRegister(m_handle, CAEN_DGTZ_TRIGGER_SRC_ENABLE_ADD, 0x800000FF);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Unable to enable internal trigger source", status);
    }
    break;
  case CAENPhaParameters::external:
    std::cout << "\nPHA: External trigger on this board";
    setRegisterBits(0x8080, 24, 24, 1);	// supposed to fall through. This disables per channel triggers.
  case CAENPhaParameters::both:
    std::cout << "\nPHA: Both trigger on this board";
    status = CAEN_DGTZ_WriteRegister(m_handle, CAEN_DGTZ_TRIGGER_SRC_ENABLE_ADD, 0xc00000ff); // internal/external.
    //status = CAEN_DGTZ_WriteRegister(m_handle, CAEN_DGTZ_TRIGGER_SRC_ENABLE_ADD, 0x80000000); // internal/external.
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Unable to set external trigger", status);
    }
    break;
  default:
    throw std::pair<std::string, int>("Invalid trigger source", -1);
  }
  
  // Trigger output mode:
  
  if (m_trgout) {
    status = CAEN_DGTZ_WriteRegister(m_handle, 0x8110, 0xFF&m_enableMask);
    std::cout << "\nTrg Out Enable mask:" << std::hex << m_enableMask << std::dec;
  } else {
    setRegisterBits(0x811c, 16, 17, 0x3);     // Note status must be CAEN_DGTZ_Success from last.
  }
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Unable to set GPO mode", status);
  }

  //Not sure if this works
  // Set the Front Panel I/O control register.
  status = CAEN_DGTZ_WriteRegister(m_handle, 0x811c, m_configuration.ioctlmask); //
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Unable to set GPO mode", status);
  }  

 uint32_t currentValue; 
 if(m_configuration.s_startMode == CAEN_DGTZ_S_IN_CONTROLLED)
	currentValue = 0x30000;
 if(m_configuration.s_startMode == CAEN_DGTZ_SW_CONTROLLED)
	currentValue = 0xC100;


  if(m_configuration.OnboardCoinc == CAENPhaParameters::TrgInGated)
  {
	currentValue |= 0x0d00;
  }
  else if(m_configuration.OnboardCoinc == CAENPhaParameters::TrgInVeto)
  {
	currentValue |= 0x0d00;
  }
   else
	currentValue |= 0x10100;

    status = CAEN_DGTZ_WriteRegister(m_handle,0x811c, currentValue);

    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Unable to set external trigger", status);
    }

    switch(m_configuration.trgoutmode)
    {
	case CAENPhaParameters::TRGOUT_MODE_LEVEL0:     setRegisterBits(0x811c,14, 14, 0x0); 
							setRegisterBits(0x811c,15, 15, 0x1);
							break;
	case CAENPhaParameters::TRGOUT_MODE_LEVEL1:	setRegisterBits(0x811c,14, 14, 0x1); 
							setRegisterBits(0x811c,15, 15, 0x1);
							break;
	case CAENPhaParameters::TRGOUT_MODE_SW_TRG:
	case CAENPhaParameters::TRGOUT_MODE_EXT_TRG:
	case CAENPhaParameters::TRGOUT_MODE_GLOBAL_OR_TRG:  for(int i=0; i<16; i++) //Work in progress
								setRegisterBits(0x10a0 | (i << 8), 0, 2, 0x7);

							    setRegisterBits(0x810c, 0, 2, 0x0);
	case CAENPhaParameters::TRGOUT_MODE_RUN:
	case CAENPhaParameters::TRGOUT_MODE_DELAYED_RUN: 
	case CAENPhaParameters::TRGOUT_MODE_SAMPLE_CLK:
	case CAENPhaParameters::TRGOUT_MODE_PLL_CLK:
	case CAENPhaParameters::TRGOUT_MODE_BUSY:
	case CAENPhaParameters::TRGOUT_MODE_PLL_UNLOCK:
	case CAENPhaParameters::TRGOUT_MODE_VPROBE: 
	case CAENPhaParameters::TRGOUT_MODE_SYNCIN:;
    }


}

/**
 * setCoincidenceTriggers
 *   Channel (couples) can participate in coincidence triggering.  This requires
 *   (according to DigiTES) that the trigger mode be internal only.
 *   Here are the comments from DigiTES that are relevant:
 *   
 *
  Principle of operation: when the coincidence are enabled in hardware (FPGA),
  each channel makes its own trigger (self-trigger) which must be validated
  by a trigger validation signal coming from the central FPGA in the mother board.
  There is a trigger mask (one per channel) that defines the logic
  used to generate the trigger validations on channel basis. This can be the OR,
  AND or Majority of the self triggers coming from the participating channels.
  it is also possible to include the external trigger (TRG-IN connector) and the
  LVDS I/Os.
  NOTE1: in the x730, two channels (couple) share the same trigger mask and
         trigger validation signal, so the coincidence takes place on couple
         basis;
	 it is also possible to use a local cross-validation (odd channel
	 validates even channel and viceversa)
  NOTE2: it is possible to negate the coincidence logic, that is use the
         validation signal to reject events intead of accept (anti-coincidence).
         The logic of the trigger mask does not change
  NOTE3: besides the individual trigger validation signals, there is a common
         signal (typically coming from TRG-IN) that can be used as a global
         inhibit  or gate for the self triggers. This option is not managed here.

 INDIVIDUAL_TRIGGER_MASK_REGISTER (address 0x8080 + i*4, i=channel/couple number)
      [ 7: 0] = participating channel mask (refers to couples in the x730):
               defines which self-triggers (i.e. from which channels) go into
               the combination
      [ 9: 8] = combination logic: 0=OR, 1=AND, 2=MAJORITY
      [12:10] = majority level
      [29] = enable LVDS(i) to be ORed in the trigger validation
      [30] = enable external trigger to be ORed in the trigger validation
  
  I confess the logic is essentially copied from DigiTES
*/
void
CAENPha::setCoincidenceTriggers()
{
  // If there's no channels with coincidence enabled we have no work to do:
  
  bool useCoincidenceTrigger(false);
  for (int i =0; i < m_configuration.coincidenceSettings.size(); i++) {
    if (m_configuration.coincidenceSettings[i].s_enabled) {
      useCoincidenceTrigger = true;
      break;                                // Only need one.
    }
  }
  if (!useCoincidenceTrigger) return;
  
  std::cout << "Note coincidence triggers are enabled.\n";
  
  // Note that we're supporting the 725 and 730 so the trigger mask
  // is for channel pairs.    The participating channels is therefore derived
  // from the enabled channels pairwise -- assume the coincidence settings
  // vector has all channels accounted for below:
  
  
  
  // Figure out the coincidence trigger mask


  
  uint32_t ChTrgMask = 0;
  uint32_t majLevel  = 1000;             // No board with 1000 channels.
  int      status;
  for (int i =0; i < m_configuration.coincidenceSettings.size(); i++) {
    int ch = m_configuration.coincidenceSettings[i].s_channel;
    
    if ((m_enableMask & (1 << ch)) != 0) {
      ChTrgMask |= 1 << (ch/2);
    }
    int chLevel = m_configuration.coincidenceSettings[i].s_majorityLevel;
    if (chLevel < majLevel)   majLevel  = chLevel;
  }
  // Now make the individual channel settings:
  
  for (int item = 0; item < m_configuration.coincidenceSettings.size(); item++) {
    int i;                                  // Compatible with code lifted from digiTES
    i = m_configuration.coincidenceSettings[item].s_channel;
    int maskindex = i/2;                                    // Mask bit # for ch.
    int CoincWindow = m_configuration.coincidenceSettings[item].s_window;
    
    status  = CAEN_DGTZ_WriteRegister(m_handle, 0x1070 + (i << 8), CoincWindow);
    status |= CAEN_DGTZ_WriteRegister(m_handle, 0x106c + (i << 8), 10);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set channel coincidence window", status);
    }
    // The XML seems not to have support for anti-coincidence:
    
    setRegisterBits(0x8080, 18, 19, 1);
    
    // We wander off into terra incognita - merrily setting undocumented bits
    // in the Couple N self-trigger logic bits. Start by sending the OR
    // of the couple to the trigger validation logic and make validation
    // come from other couples (trigger validation)?  Enable
    // trigger validation input an enable veto (so DigiTES says).
    
    setRegisterBits(0x1084 + (i << 8), 0, 1, 3);
    setRegisterBits(0x1084 + (i << 8), 4,5, 1);
    setRegisterBits(0x1084 + (i << 8), 2,2, 1);
    setRegisterBits(0x1084 + (i << 8), 6,6, 1);
    
    // Remainder sets the trigger type:  Majority or AND.  Digites calles these
    // COINC_MAJORITY and COINC_AND_ALL.
    //
    
    if (m_configuration.coincidenceSettings[item].s_operation == CAENPhaParameters::Majority) {
      status = CAEN_DGTZ_WriteRegister(m_handle, 0x8180 + maskindex*4, 0x200 | ChTrgMask | ((majLevel - 1) << 10 ));
     std::cout << "\nMajority";
        
    } else if (m_configuration.coincidenceSettings[item].s_operation == CAENPhaParameters::And) {
      status = CAEN_DGTZ_WriteRegister(m_handle, 0x8180 + maskindex * 4 , 0x100 | ChTrgMask);
     std::cout << "\nAnd";
    }
else  std::cout << "\nElse";
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int> ("Unable to set coincidencde mode/mask/majority level", status);
    }
    
  }
  
}

/**
 * setPerChannelParameters
 *    Set the DC offset, the baseline dc offset and the pretrigger.
 *    Assumption V730 or 725 - so the sampling frequency is 500Mhz (2ns/sample)
 *    The pretrigger is (I think) ns in the configuration...
 *    There's a common pretrigger.

Updated by Sudarsan B : sbalak2@lsu.edu
 */
void
CAENPha::setPerChannelParameters()
{
  std::vector<std::pair<unsigned, CAENPhaChannelParameters*> >&
    chParams(m_configuration.m_channelParameters);

  // Now the DPP Parameters:
  CAEN_DGTZ_DPP_PHA_Params_t dppParams;
  memset(&dppParams, 0, sizeof(CAEN_DGTZ_DPP_PHA_Params_t)); // Probably not needed.
  CAEN_DGTZ_ErrorCode status;

  // Fill in the elements present in chParams:
  for (unsigned i = 0; i < chParams.size(); i++) {
    int ch = chParams[i].first;
    CAENPhaChannelParameters& params(*(chParams[i].second));

//Edits made by hand to be numbered as indicated below: B.Sudarsan, Jan 2018.
//1
  //Set pretrigger    
  uint32_t preTrigSamples = static_cast<uint32_t>(params.preTrigger/(m_nsPerTick));
//no.of.samples = time-in-ns/(4*m_nsPerTick) according to manual, but it doesn't work. This form above, does.

  status = CAEN_DGTZ_SetDPPPreTriggerSize(m_handle, ch, preTrigSamples);
//  setRegisterBits(0x1038 + (ch << 8), 0,8,preTrigSamples*4);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int> ("Failed to set DPP Pre trigger size", status);
  }

//2
    //Set DCOffset
    //printf("\nOffset:%u %d",params.dcOffset,(params.dcOffset*0xFFFF)/16384);
    double dcoffset_fixed = params.dcOffset;
    //std::cout << "\n" << params.dcOffset;
    if(params.polarity == CAENPhaChannelParameters::positive)
	dcoffset_fixed = 65535 - dcoffset_fixed;

    status = CAEN_DGTZ_SetChannelDCOffset(m_handle, ch, dcoffset_fixed);  // Offset comes in units of % of max range
    if(status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set channel dc offset", status);
    }



    // The *1000 takes things to ns.  The divisions take the results to the 
    // units used by the register that's being programmed.  For now these are
    // hard coded for a 725.  Need to compute the actual units using the digitizer
    // frequency.

    dppParams.M[ch] = params.decayTime * 1000;                  // Units of nseconds
    dppParams.m[ch] = params.trapFlatTop * 1000;                // Units of nsecondes
    dppParams.k[ch] = params.trapRiseTime * 1000;	        
    dppParams.ftd[ch] = params.flattopDelay * 1000;             
//3
    dppParams.a[ch]   = params.rccr2smoothing;
//4
    dppParams.b[ch]   = params.inputRiseTime; 
    dppParams.thr[ch] = params.threshold;
    dppParams.nsbl[ch] = params.BLMean;
    dppParams.nspk[ch] = params.peakMean;
//5
    dppParams.pkho[ch] = params.peakHoldoff *2* m_nsPerTick; //Make changes in CompassProject.cpp for CH_PEAK_HOLDOFF
    dppParams.blho[ch] = params.baselineHoldoff * 1000;     
//6
    dppParams.trgho[ch] = params.triggerHoldoff*8;   //Make changes in CompassProject.cpp 

    dppParams.twwdt[ch] = params.triggerValidationWidth * 1000 / m_nsPerTick;
    dppParams.dgain[ch] = params.digitalGain;
    dppParams.enf[ch]   = params.trapGain;                                   // ?
    dppParams.decimation[ch] = params.decimation;
    dppParams.enskim[ch]     = params.energySkim ? 1 : 0;
    dppParams.blrclip[ch]    = params.baselineClip ? 1 : 0;
    dppParams.dcomp[ch]      = params.fastTriggerCorrection ? 1 : 0;
    dppParams.eskimlld[ch]   = params.lld;
    dppParams.eskimuld[ch]   = params.uld;
    dppParams.trapbsl[ch]    = params.baselineAdjust;
    dppParams.decimation[ch]  =  0;


    uint32_t polarity = 0;
    if(params.polarity == CAENPhaChannelParameters::negative)
	polarity = 1;

    //Set polarity using DPP Algo register at 0x1n80 bit 16
   status = CAEN_DGTZ_SetChannelPulsePolarity(m_handle,ch,static_cast<CAEN_DGTZ_PulsePolarity_t>(polarity));
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to write channel polarity", status);
    }

    // NewPHA parameters
  // For input range, for now only support 730/725, values 10, and 9 for 0.5 and 2.0Vpp respectively
    uint32_t rangeReg(0);
    if (params.range == 10) {
      rangeReg = 0;
    } else if (params.range == 9) {
      rangeReg  = 1;
    } else {
      throw std::pair<std::string, int>("Input range value not compatible with supported digitizers", params.range);
    }
    uint32_t rangeAddr = 0x1028 | (ch << 8);
    status = CAEN_DGTZ_WriteRegister(m_handle, rangeAddr, rangeReg);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to write range register", status);
    }

    status = CAEN_DGTZ_WriteRegister(m_handle, 0x10a0 + (ch << 8), 0x10000);
    //status = CAEN_DGTZ_WriteRegister(m_handle, 0x10a0 + (ch << 8), 0x0);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to write DPP control 2 register", status);
    }

    int fgRegisterValue = fineGainRegister(params.fineGain, dppParams.k[ch], dppParams.M[ch]);
    status = CAEN_DGTZ_WriteRegister(m_handle, 0x104c | (ch << 8), fgRegisterValue);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to write fine gain register", status);
    }
  }
  status = CAEN_DGTZ_SetDPPParameters(m_handle, m_enableMask, &dppParams);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Unable to set dpp parameters", status);
  }



//For parameters not defined in dppParams, we're forced to go over all the channels again. 
for (unsigned i = 0; i < chParams.size(); i++)
{
    int ch = chParams[i].first;


   //By default, the rollover event is turned 'ON' by Compass 1.3.0
   //DON'T turn this on to have it agree with CoMPASS, it messes with evb!
   CAENPhaChannelParameters& params(*(chParams[i].second));
   if(params.fakeevt_ttroll_en){
	   setRegisterBits(0x1080 | (ch<<8), 26, 26, 0);
   }

    //Stuff is weird. The fine gain register value is written to the finegain register as well as the first 6 bits of DPP Algo control
    int fgRegisterValue = fineGainRegister(params.fineGain, dppParams.k[ch], dppParams.M[ch]);
    setRegisterBits(0x1080 | (ch<<8),0,5,fgRegisterValue);

    double trgholdoff_gen = (2*params.triggerHoldoff/m_nsPerTrigger); //Tested to work for 1725
    setRegisterBits(0x1074 | (ch<<8),0,9,static_cast<int>(trgholdoff_gen));

    double pkholdoff_gen = (2*params.peakHoldoff/m_nsPerTrigger);
    setRegisterBits(0x1078 | (ch<<8),0,9,static_cast<int>(pkholdoff_gen));

   setRegisterBits(0x1080 | (ch<<8), 27, 27, 0);


  uint32_t ShapedTriggerWidth = ((uint32_t)m_configuration.shapTrgWidth/(4*m_nsPerTick)) & 0x3ff;  
  setRegisterBits(0x1084 | (ch<<8), 0,9,ShapedTriggerWidth);
 // setRegisterBits(0x1084 | (ch<<8), 0,9,ShapedTriggerWidth);
 
  switch(m_configuration.OnboardCoinc)
  {
	case CAENPhaParameters::TrgInGated: setRegisterBits(0x1080 | (ch<<8), 18, 19, 0x1); //Setup coincidence mode
					    setRegisterBits(0x10a0 | (ch<<8), 14, 15, 0x0); //Unset veto, if any
					    setRegisterBits(0x10a0 | (ch<<8), 4, 5, 0x1); //Validate from motherboard
					    setRegisterBits(0x10a0 | (ch<<8), 6, 6, 0x1); //Enable local validation
  					    break;
	case CAENPhaParameters::TrgInVeto:   setRegisterBits(0x1080 | (ch<<8), 18, 19, 0x3); //Setup anti coincidence mode
					   setRegisterBits(0x10a0 | (ch<<8), 14, 15, 0x1); //Setup common veto among all channels. 0x810c sets the veto
					   setRegisterBits(0x10a0 | (ch<<8), 4, 5, 0x1); //Validate from motherboard signal (ext trg)
					   setRegisterBits(0x10a0 | (ch<<8), 6, 6, 0x1); //Enable local validation for the above thing to work
  					   break;
	case CAENPhaParameters::None:      setRegisterBits(0x1080 | (ch<<8), 18, 19, 0x0); //Setup normal mode, no coincidence
					   setRegisterBits(0x10a0 | (ch<<8), 14, 15, 0x0); //Disable all vetos
					   //setRegisterBits(0x10a0 | (ch<<8), 4, 5, 0x0); //Disable trigger validation mode
					   setRegisterBits(0x10a0 | (ch<<8), 6, 6, 0x0); //Disable local trigger validation
   }
}    

}




/**
 * calibrate
 *   Perform digitizer calibration:
 *
 */
void CAENPha::calibrate()
{
  CAEN_DGTZ_ErrorCode status = CAEN_DGTZ_Calibrate(m_handle);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Calibration of board failed", status);
  }
}
/**
 * setRegisterBits
 *   Sets a bit mask in a register.  Note that we can distinguish between broadcast and individual rgs
 *
 *  @param addr - register address.
 *  @param start_bit - first bit of field (Lowest order).
 *  @param end_bit   - last bit of field (Highest order).
 *  @param value     - Value to put in that field.
 */
void
CAENPha::setRegisterBits(uint16_t addr, int start_bit, int end_bit, int val)
{
  // Compute the mask and the field.

  uint32_t  mask = 0;
 
  int ret;
  uint32_t reg;
  for(int i=start_bit; i<=end_bit; i++) {
    mask |= 1<<i;
  }
  int field = (val << start_bit) & mask;


  // Loop over registers if a broadcast register:

  if (((addr & 0xFF00) == 0x8000) && (addr != 0x8000) && (addr != 0x8004) && (addr != 0x8008)) { // broadcast access to channel individual registers (loop over channels)
    for(int ch = 0; ch < m_info.Channels; ch++) {
      ret = CAEN_DGTZ_ReadRegister(m_handle, 0x1000 | (addr & 0xFF) | (ch << 8), &reg);
      reg = (reg & ~mask) | field;
      ret |= CAEN_DGTZ_WriteRegister(m_handle, 0x1000 | (addr & 0xFF) | (ch << 8), reg);   
    }
  } else {
    // Individual register:

    ret = CAEN_DGTZ_ReadRegister(m_handle, addr, &reg);
    reg = (reg & ~mask) | field;
    ret |= CAEN_DGTZ_WriteRegister(m_handle, addr, reg);  
  
  }

  if (ret != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("setRegisterBits failed: ", reg);
  }
}

/**
 * dataBuffered
 *    Determine if there's still data buffered.
 * @return bool -true if so.
 */
bool
CAENPha::dataBuffered()
{
  for (int i = 0; i < CAEN_DGTZ_MAX_CHANNEL; i++) {
    if ((m_nDppEvents[i] - m_nOffsets[i]) > 0)
      return true;                       // This channel has undelivered events.
  }
  return false;                          // All channels are empty.
}
/**
 * fillBuffers
 *   Read data from the digitizer into our internal buffers.
 */
void
CAENPha::fillBuffers()
{
  for (int i = 0; i < CAEN_DGTZ_MAX_CHANNEL; i++) {
    m_nDppEvents[i]  = 0;
    m_nOffsets[i]    = 0;
  }
  CAEN_DGTZ_ErrorCode status;
  uint32_t             nRead;
  status = CAEN_DGTZ_ReadData(
      m_handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, m_rawBuffer, &nRead
  );
  if (status == CAEN_DGTZ_CommError) {
     std::cout << "Comm error.. it's likely all over" << nRead << std::endl;
    return;
  }
  if (status != CAEN_DGTZ_Success) return;   // Unable to buffer for some reason.
  if (nRead == 0) return;                    // Nothing to read.
  
  status = CAEN_DGTZ_GetDPPEvents(
      m_handle, m_rawBuffer, nRead, (void**)(m_dppBuffer), (uint32_t*)m_nDppEvents
  );
  
  
}

/**
 * findEarliest
 *    Find the channel with the earliest timestamp (TimeTag).  This is part
 *    of ensuring that the data from the digitizer is delivered to the client
 *    in a time ordered fashion.
 *
 * @return channel - the channel whose first undelivered event has the lowest
 *                   timestamp.
 * @note The caller must make certain there's at least one channel with unread
 *       data.  This is done by ensuring dataBuffered() returns true.
 */
int
CAENPha::findEarliest()
{
  uint64_t lowestStamp = 0;
  lowestStamp--;                      // twos complements means this is umaxint64.
  int earliest = CAEN_DGTZ_MAX_CHANNEL; // Not allowed.
  for (int i =0; i < CAEN_DGTZ_MAX_CHANNEL; i++) {
    int offset = m_nOffsets[i];
    if (offset < m_nDppEvents[i]) {     // Channel has undelivered events.
      uint64_t tag = m_dppBuffer[i][offset].TimeTag;
      if(tag < lowestStamp) {
        lowestStamp = tag;
        earliest = i;
      }
    }
    
  }
  // It's a sorry state of affairs if earliest == CAEN_DGTZ_MAX_CHANNEL,
  // That means the caller broke the rules.
  
  if (earliest == CAEN_DGTZ_MAX_CHANNEL) {
    throw std::logic_error("CAENPha::findEarliest - no channels with data !!");
  }
  return earliest;
}
/**
 * Compute the fine gain register given:
 *
 * @param gain - the desired gain.
 * @param k    - The trapezoid rise time.
 * @param m    - The Decay time.
 *
 * See the FineGain register documentation in the CAEN document
 *   UM5678_725_730_DPP_PHA_Registers  (I'm using rev 1).
 */
uint16_t
CAENPha::fineGainRegister(double gain, int k, int m)
{
  // Figure out shf it's the integerized? rounded? log2(k*m)
  // I think that digitzer library effectively uses integerized.
  
  double shf = log(double(k*m))/log(2.0);      // identity: logn(x) = log(x)/log(n)
  double finegain = (log(gain)/log(2.0));

  //The following, works empirically. Unable to reason out why - it's likely to be the firmware version in use
   if(m_nsPerTick == 4)
	  return uint16_t(shf-finegain-8);
   else 
	  return uint16_t(shf-finegain-2*m_nsPerTick-2);

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
 */
void
CAENPha::processCheatFile()
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
