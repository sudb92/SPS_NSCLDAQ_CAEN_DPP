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
* @file     CAENPha.h
* @brief    Low level driver for CAEN Pha dpp digitizers for NSCLDAQ.
* @author   Ron Fox (rfoxkendo@gmail.com)
*
*
*/
#ifndef CAENPHA_H
#define CAENPHA_H
#include <cstddef>
#include <stdint.h>              // Types CAEN expects rather than <cstdint>
#include <tuple>
#include "CAENDigitizer.h"
#include "CAENPhaParameters.h"
#include "CAENPhaChannelParameters.h"




/**
 * @class CAENPha
 *    Driver for xx725 with PHA firmware.
 */

class CAENPha
{
private:
  CAENPhaParameters&  m_configuration;
  int                 m_handle;
  CAEN_DGTZ_BoardInfo_t m_info;

  // Synchronization stuff that the setup app doesn't understand:

  CAEN_DGTZ_AcqMode_t m_startMode;
  unsigned            m_startDelay;
  bool                m_trgout;
  char*               m_rawBuffer;
  uint32_t            m_rawSize;
  CAEN_DGTZ_DPP_PHA_Event_t* m_dppBuffer[CAEN_DGTZ_MAX_CHANNEL];
  uint32_t            m_dppSize;
  CAEN_DGTZ_DPP_PHA_Waveforms_t* m_pWaveforms;
  uint32_t            m_wfSize;
  int32_t            m_nDppEvents[CAEN_DGTZ_MAX_CHANNEL];
  int32_t            m_nOffsets[CAEN_DGTZ_MAX_CHANNEL];
  uint64_t           m_nTimestampAdjusts[CAEN_DGTZ_MAX_CHANNEL];
  uint64_t           m_nLastTimestamp[CAEN_DGTZ_MAX_CHANNEL];
  unsigned           m_nsPerTick; /* Nanoseconds per digitizer clock. */
  unsigned           m_nsPerTrigger;  // ns per trigger clock tick.
  const char*        m_pCheatFile;
  int conet_node;
  // Other data
  
  int m_enableMask;
public:
  CAENPha(CAENPhaParameters& config, CAEN_DGTZ_ConnectionType linkType, int linknum,
          int node, uint32_t base, CAEN_DGTZ_AcqMode_t startMode,
          bool trgout, unsigned delay, const char* pCheatFile=0);
  ~CAENPha();
  void setup();
  void shutdown();

  bool haveData();
  std::tuple<int, const CAEN_DGTZ_DPP_PHA_Event_t*, const CAEN_DGTZ_DPP_PHA_Waveforms_t*> Read();

  // Organizational methods
  
private:
  int setChannelMask();
  void setTriggerAndSyncMode();
  void setCoincidenceTriggers();
  void setPerChannelParameters();
  void calibrate();

  // Utility methods.
private:
  void setRegisterBits(uint16_t addr, int start_bit, int end_bit, int val);
  bool dataBuffered();
  void fillBuffers();
  int  findEarliest();
  uint16_t fineGainRegister(double value, int k, int m);
  void processCheatFile();
};
#endif
