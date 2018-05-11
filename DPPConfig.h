/******************************************************************************
*
* CAEN SpA - System integration division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************/
/**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the
* software, documentation and results solely at his own risk.
******************************************************************************/

/**
 * @file   DPPConfig.h
 * @brief Configuration functions for CAEN digitizers.
 * @note This uses and is built on top of the functions in config that provide a basic
 *       interface to iniparser
 */
#ifndef DPPCONFIG_H
#define DPPCONFIG_H
extern "C" {
#include <iniparser.h>
}
#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

typedef struct _ConnectionParams {
  CAEN_DGTZ_ConnectionType  linkType;
  int                       linkNum;
  int                       ConetNode;
  uint32_t                  VMEBaseAddress;
    
} ConnectionParams, *pConnectionParams;


extern ConnectionParams getConnectionParams(dictionary* pDict);
extern CAEN_DGTZ_DPP_AcqMode_t getAcquisitionMode(dictionary* config);
extern CAEN_DGTZ_DPP_SaveParam_t getListParameters(dictionary* config);
extern CAEN_DGTZ_TriggerMode_t getExternalTrigger(dictionary* config);
extern CAEN_DGTZ_IOLevel_t getFpLevel(dictionary* config);
extern int getChannelSelfTrigger(dictionary* config, int chan);
extern int getGatedStartMode(dictionary* config);
extern CAEN_DGTZ_PulsePolarity_t getChannelPulsePolarity(
  dictionary* config, int chan, dictionary* pOldConfig = 0
);
extern CAEN_DGTZ_DPP_TriggerConfig_t getModuleTriggerConfig(dictionary* config);
extern CAEN_DGTZ_DPP_TriggerMode_t getModuleTriggerMode(dictionary* config);
extern CAEN_DGTZ_DPP_PUR_t getPileupRejectionMode(dictionary* config);

extern uint32_t getGPOMode(dictionary* config, const char* dfltValue);
extern uint32_t getStartMode(dictionary* config, const char* dfltValue);
extern CAEN_DGTZ_AcqMode_t getStartModeCode(dictionary* config, const char* dfltValue);

#endif
