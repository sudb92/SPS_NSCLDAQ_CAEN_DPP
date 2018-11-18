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
 */
CAENPha::CAENPha(
    CAENPhaParameters& config, CAEN_DGTZ_ConnectionType linkType, int linknum,
    int node, uint32_t base,
    CAEN_DGTZ_AcqMode_t startMode, bool trgout, unsigned delay
  ) :
  m_configuration(config),
  m_startMode(startMode),
  m_trgout(trgout),
  m_startDelay(delay),
  m_rawBuffer(0),
  m_rawSize(0),
  m_dppSize(0),
  m_pWaveforms(0),
  m_wfSize(0)
  
{
  CAEN_DGTZ_ErrorCode status = CAEN_DGTZ_OpenDigitizer(linkType, linknum, node, base, &m_handle);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Open failed", status);
  }
  
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

  // We only support the 725 and 730 (250Mhz and 500Mhz).
  // set m_nsPerTick appropriately or complain about the digitizer type:

  if(boardInfo.FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE) {
    m_nsPerTick = 2;
  } else if (boardInfo.FamilyCode == CAEN_DGTZ_XX725_FAMILY_CODE) {
    m_nsPerTick = 4;
  } else {
    throw std::pair<std::string, int>("Un supported digitizer family", boardInfo.FamilyCode);
  }

  m_enableMask = setChannelMask();
  
  // Set individual trigger, mb1, propagate triggers, TRG validation(?)

  CAEN_DGTZ_WriteRegister(m_handle, 0x8008, 0xffffffff);    // Clear all bits.
  status = CAEN_DGTZ_WriteRegister(m_handle, 0x8000, 0x010000114);
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

  status = CAEN_DGTZ_WriteRegister(m_handle, 0x8004, 1);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Enable flush mode failed", status);
  }
 
  setTriggerAndSyncMode();
  
  sleep(1);

  setPerChannelParameters();
  
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
  
  setCoincidenceTriggers();
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
  status = CAEN_DGTZ_SWStartAcquisition(m_handle);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Failed to start or arm acquisition", status);
  }
  
  
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

  if (pData->TimeTag < m_nLastTimestamp[channel]) {
    m_nTimestampAdjusts[channel] += 0x100000000;	// Rolled over (hopefully only once).
  }
  m_nLastTimestamp[channel] = pData->TimeTag;
  pData->TimeTag |= m_nTimestampAdjusts[channel]; // Fold in the wraps.
  
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

  switch (m_startMode) {
  case CAEN_DGTZ_SW_CONTROLLED :
    status = CAEN_DGTZ_SetAcquisitionMode(m_handle, CAEN_DGTZ_SW_CONTROLLED);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set acq mode to sw controlled", status);
    }
    status = CAEN_DGTZ_SetRunSynchronizationMode(m_handle, CAEN_DGTZ_RUN_SYNC_Disabled);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set run sync mode to Disabled", status);
    }
    break;
  case CAEN_DGTZ_FIRST_TRG_CONTROLLED :
    status = CAEN_DGTZ_WriteRegister(m_handle, CAEN_DGTZ_ACQ_CONTROL_ADD,  0x02);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set run start on rising trigger edge", status);
    }
    status = CAEN_DGTZ_WriteRegister(m_handle, 0x8170, m_startDelay);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to set start delay", status);
    }
    break;
  case CAEN_DGTZ_S_IN_CONTROLLED:
    //   Software start will arm the board.
    break;
  default:
    throw std::pair<std::string, int>("Invalid start mode value", -1);
  }
  // Set internal/external trigger mode:

  switch (m_configuration.triggerSource) {
  case CAENPhaParameters::internal:
    status = CAEN_DGTZ_WriteRegister(m_handle, CAEN_DGTZ_TRIGGER_SRC_ENABLE_ADD, 0xffff);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Unable to enable internal trigger source", status);
    }
    break;
  case CAENPhaParameters::external:
    setRegisterBits(0x8080, 24, 24, 1);	// supposed to fall through. This disables per channel triggers.
  case CAENPhaParameters::both:
    status = CAEN_DGTZ_WriteRegister(m_handle, CAEN_DGTZ_TRIGGER_SRC_ENABLE_ADD, 0xc0000000); // internal/external.
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Unable to set external trigger", status);
    }
    break;
  default:
    throw std::pair<std::string, int>("Invalid trigger source", -1);
  }
  
  // Trigger output mode:
  
  if (m_trgout) {
    status = CAEN_DGTZ_WriteRegister(m_handle, 0x8110, m_enableMask);
  } else {
    setRegisterBits(0x811c, 16, 17, 0x3);     // Note status must be CAEN_DGTZ_Success from last.
  }
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int>("Unable to set GPO mode", status);
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
        
    } else if (m_configuration.coincidenceSettings[item].s_operation == CAENPhaParameters::And) {
      status = CAEN_DGTZ_WriteRegister(m_handle, 0x8180 + maskindex * 4 , 0x100 | ChTrgMask);
    }
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
 */
void
CAENPha::setPerChannelParameters()
{
  std::vector<std::pair<unsigned, CAENPhaChannelParameters*> >&
    chParams(m_configuration.m_channelParameters);
    
  int preTrigSamples = 2040; //m_configuration.preTriggers/5;  // Just samples?!??
  CAEN_DGTZ_ErrorCode status = CAEN_DGTZ_SetDPPPreTriggerSize(m_handle, -1, preTrigSamples);
  if (status != CAEN_DGTZ_Success) {
    throw std::pair<std::string, int> ("Failed to set DPP Pre trigger size", status);
  }
  // Now the DPP Parameters:
  
  CAEN_DGTZ_DPP_PHA_Params_t dppParams;
  memset(&dppParams, 0, sizeof(CAEN_DGTZ_DPP_PHA_Params_t)); // Probably not needed.
  
  // Fill in the elements present in chParams:
  
  for (unsigned i = 0; i < chParams.size(); i++) {
    int ch = chParams[i].first;
    CAENPhaChannelParameters& params(*(chParams[i].second));
    
    status = CAEN_DGTZ_SetChannelDCOffset(m_handle, ch, params.dcOffset*65535/16383);  // ?? in MCA2.
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
    dppParams.a[ch]   = params.rccr2smoothing;
    dppParams.b[ch]   = params.inputRiseTime * 1000;            
    dppParams.thr[ch] = params.threshold;
    dppParams.nsbl[ch] = params.BLMean;
    dppParams.nspk[ch] = params.peakMean;
    dppParams.pkho[ch] = params.peakHoldoff * 1000;          
    dppParams.blho[ch] = params.baselineHoldoff * 1000;     
    dppParams.trgho[ch] = params.triggerHoldoff * 1000;     
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

    // NewPHA parameters

    // What to do with transistor reset params?  There are no elements in the CAEN_DGTZ_DPP_PHA_Params_t struct?
    

    uint32_t rangeReg(0);
    if (params.range == 10) {
      rangeReg = 1;
    } else if (params.range == 9) {
      rangeReg  = 0;
    } else {
      throw std::pair<std::string, int>("Input range value not compatible with supported digitizers", params.range);
    }
    uint32_t rangeAddr = 0x1028 | (ch << 8);
    status = CAEN_DGTZ_WriteRegister(m_handle, rangeAddr, rangeReg);
    if (status != CAEN_DGTZ_Success) {
      throw std::pair<std::string, int>("Failed to write range register", status);
    }
    status = CAEN_DGTZ_WriteRegister(m_handle, 0x10a0 + (ch << 8), 0);
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
  // For input range, for now only support 730/725, values 10, and 9 for 0.5 and 2.0Vpp respectively

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
  
  int shf = log(double(k*m))/log(2.0);      // identity: logn(x) = log(x)/log(n)
  double result = 65535.0 * gain * (1 << shf)/double(k*m);
  return uint16_t(shf);
}