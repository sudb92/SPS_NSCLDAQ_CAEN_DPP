
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
 * @file DPPConfig.cpp 
 * @brief Provide DPP specific configuration getters.
 */

#include "DPPConfig.h"

#include <map>
#include <string>
#include <stdexcept>
#include "config.h"
#include "DPPConfig.h"

/*-----------------------------------------------------------------------
 *  Local data:
 */

typedef struct _NameValue {
  const char*  s_name;
  int          s_value;
} NameValue, *pNameValue;

// For getAcquisitionMode:

static std::map<std::string, int> AcqModeMap;
static const NameValue  AcquisitionModes[] = {
  {"OSCILLOSCOPE", CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope},
  {"LIST",         CAEN_DGTZ_DPP_ACQ_MODE_List},
  {"MIXED",        CAEN_DGTZ_DPP_ACQ_MODE_Mixed},
  {0, 0}
};

// For getListParams

static std::map<std::string, int> ListParamMap;
static const NameValue ListParams[] = {
  {"ENERGY_ONLY",     CAEN_DGTZ_DPP_SAVE_PARAM_EnergyOnly},
  {"TIME_ONLY",       CAEN_DGTZ_DPP_SAVE_PARAM_TimeOnly},
  {"ENERGY_AND_TIME", CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime},
  {"NONE",            CAEN_DGTZ_DPP_SAVE_PARAM_None},
  {0,0}
};

// For getExternalTriggerMode

static std::map<std::string, int> TriggerMap;
static const NameValue Triggers[] = {
  {"DISABLED",               CAEN_DGTZ_TRGMODE_DISABLED},
  {"TRGOUT_ONLY",            CAEN_DGTZ_TRGMODE_EXTOUT_ONLY},
  {"ACQUISITION_ONLY",       CAEN_DGTZ_TRGMODE_ACQ_ONLY},
  {"ACQUISITION_AND_TRGOUT", CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT},
  {0,0}
};

// For getFpioLevel:

static std::map<std::string, int> FpIoLevelMap;
static const NameValue FpioLevels[] = {
  {"NIM",   CAEN_DGTZ_IOLevel_NIM},
  {"TTL",   CAEN_DGTZ_IOLevel_TTL},
  {0,0}
};

// For enable/disable parameters:

static std::map<std::string, int> EnableMap;
static NameValue EnDis[] = {
  {"ENABLED",   1},
  {"DISABLED",  0},
  {0,0}
};

// for polarities:

static std::map<std::string, int> PolarityMap;
static NameValue Polarities[] = {
  {"POSITIVE",  CAEN_DGTZ_PulsePolarityPositive},
  {"NEGATIVE",  CAEN_DGTZ_PulsePolarityNegative},
  {0,0}
};


// Trigger configurations:

static std::map<std::string, int> TriggerConfigMap;
static NameValue TriggerConfigs[] = {
  {"THRESHOLD",    CAEN_DGTZ_DPP_TriggerConfig_Threshold},
  {"PEAK",         CAEN_DGTZ_DPP_TriggerConfig_Peak},
  {0,0}
};

// Trigger modes:

static std::map<std::string, int> TriggerModeMap;
static NameValue TriggerModes[] = {
  {"NORMAL",     CAEN_DGTZ_DPP_TriggerMode_Normal},
  {"COINCIDENCE", CAEN_DGTZ_DPP_TriggerMode_Coincidence},
  {0,0}
};

// Pile up Rejection/detection mode (PUR)

static std::map<std::string, int> PurMap;
static NameValue PurModes[] = {
  {"DETECT", CAEN_DGTZ_DPP_PSD_PUR_DetectOnly},
  {"ENABLED", CAEN_DGTZ_DPP_PSD_PUR_Enabled},
  {0,0}
};

// GPO mode:

static std::map<std::string, int> GPOMap;
static NameValue GPOValues[] = {
  {"S-IN", 0xc},
  {"RUN",  4},
  {"CLKOUT", 0x14},
  {"CLKPHASE", 0x24},
  {"BUSY",    0x34},
  {"TRUE",    3},
  {"FALSE",   2},
  {0,0}
  
};



// START_MODE

static std::map<std::string, int> StartModeMap;
static NameValue StartModeValues[] = {
  {"SOFTWARE", 4},
  {"S-IN",     0xd},
  {"TRIG1",    0xe},
  {"GPI",      0xd},
  {0,0}
};

static std::map<std::string, int> startModeCodeMap;
static NameValue StartModeCodeValues[]  = {
  {"SOFTWARE",  CAEN_DGTZ_SW_CONTROLLED},
  {"S_IN",      CAEN_DGTZ_S_IN_CONTROLLED},
  {"TRIG1",     CAEN_DGTZ_FIRST_TRG_CONTROLLED},
  {0,0}
};

/*--------------------------------------------------------------------------
 * Public methods:
 */

/**
 * getConnectionParams
 *    Obtains and parses/decodes the common.open parameter.
 *
 * @param dict - The dictionary containing the parsed configuration.
 * @return ConnectionParams - struct containing the parameters required for
 *                              CAEN_DGTZ_OpenDigitizer.
 * @throw std::runtime_error - if there's a problem.
 */
ConnectionParams
getConnectionParams(dictionary* dict)
{
  std::string connection = getParamString(dict, "open", -1, ""); 
  if (connection == "") {
    throw std::runtime_error("The configuration has no OPEN configuration in the [COMMON] settings");
  }

  char TypeString[100];
  int  linkNum;
  int  optNode;
  int  ba;

  int items = sscanf(connection.c_str(), "%s", TypeString);

  ConnectionParams result;

  // Item count and actual meaning depends on the typestring:
   
  if (std::string(TypeString) == "USB") {
    items = sscanf(connection.c_str(), "%s %d %x", TypeString, &linkNum, &ba);
    if (items != 3) {
      throw std::runtime_error("USB Connections are of the form USB linkum base-address");
    }
    result.linkType = CAEN_DGTZ_USB;
    result.linkNum  = linkNum;
    result.ConetNode = 0;
    result.VMEBaseAddress = ba;
      
  } else if (std::string(TypeString) == "PCI") {
    items = sscanf(connection.c_str(), "%s %d %d %x", TypeString, &linkNum, &optNode, &ba);
    if (items != 4) {
      throw std::runtime_error("OPTlink connections are of the form PCI link node base-address");
    }
    result.linkType = CAEN_DGTZ_OpticalLink;
    result.linkNum  = linkNum;
    result.ConetNode= optNode;
    result.VMEBaseAddress = ba;

  } else {
    std::string msg = "Invalid connection type string: ";
    msg += TypeString;
    msg += " Must be USB or PCI";
    throw std::runtime_error(msg);
  }

  return result;


}

/**
 * loadMap
 *   Loads a map if it's not yet loaded:
 * 
 * @param mapping     - Reference to the map to load.
 * @param nameValues  - array of name value pairs to load.
 */
static void loadMap(std::map<std::string, int>& mapping,const  NameValue* nameValues)
{
  // Only load empty maps.

  if (mapping.empty()) {
    while (nameValues->s_name) {
      mapping[std::string(nameValues->s_name)] = nameValues->s_value;
      nameValues++;
    }
  }
}
/**
 * getAcquisitionMode
 *   This is a global parameter.  The returned value can be directlyplugged into
 *   the mode param of CAEN_DGTZ_SetDPPAcquisitionMode().
 *
 * @param config   - The configuration dict.
 * @return CAEN_DGTZ_DPP_AcqMode_t as defined by the config file or LIST if not provided.
 */
CAEN_DGTZ_DPP_AcqMode_t
getAcquisitionMode(dictionary* config)
{
  loadMap(AcqModeMap, AcquisitionModes);

  return static_cast<CAEN_DGTZ_DPP_AcqMode_t>(
      getKeywordParam(config, "ACQUISITION_MODE", -1, "LIST", AcqModeMap)
  );
}

/**
 * getListParameters
 *   Get the LIST_PARAMS parameter value.
 * @param config  - Configuration dict.
 * @return CAEN_DGTZ_DPP_SaveParam_t - suitable to be passed as teh param argument to
 *                  CAEN_DGTZ_SetDPPAcquisitionMode()
 */
CAEN_DGTZ_DPP_SaveParam_t
getListParameters(dictionary* config)
{
  loadMap(ListParamMap, ListParams);

  return static_cast<CAEN_DGTZ_DPP_SaveParam_t>(
     getKeywordParam(config, "LIST_PARAMS", -1, "ENERGY_AND_TIME", ListParamMap)
  );

}
/**
 * getExternalTrigger
 *   Returns the external trigger input mode setting. 
 * @param config - the configuration dictionary.
 * @return CAEN_DGTZ_TriggerMode_t - suitable for passing as the mode parameter to 
 *                                   CAEN_DGTZ_SetExtTriggerInputMode.
 */
CAEN_DGTZ_TriggerMode_t
getExternalTrigger(dictionary* config)
{
  loadMap(TriggerMap, Triggers);

  // This is a common only param.

  return static_cast<CAEN_DGTZ_TriggerMode_t>(
      getKeywordParam(config, "EXTERNAL_TRIGGER", -1, "ACQUISITION_ONLY", TriggerMap)
  );
}
/**
 * getFpLevel
 *    Returns the front panel level conventions.
 * @param config - The configuration dict.
 * @return CAEN_DGTZ_IOLevel_t - I/O level suitable to be passed to CAEN_DGTZ_SetIOLevel
 */
CAEN_DGTZ_IOLevel_t
getFpLevel(dictionary* config)
 {
   loadMap(FpIoLevelMap, FpioLevels);

   return static_cast<CAEN_DGTZ_IOLevel_t>(getKeywordParam(config, "FPIO_LEVEL", -1, "NIM", FpIoLevelMap));
 }
/**
 * getChannelSelfTrigger
 *   Gets the self trigger for a channel. 
 *  @param config - The configuration dict.
 *  @param chan   - Channel for which you want to get the self triggerstate.
 *  @return int - 1 for enable, 0 for disable.
 *  @note - The value is in decreasing order of precedence - the value in the channel's section,
 *          The value in the [COMMON[ section, the default we feed in.
 *  @note - While this is a per channel value, the [COMMON] value can be gotten by asking for
 *          channel -1.
 */
int
getChannelSelfTrigger(dictionary* config, int chan)
{
  loadMap(EnableMap, EnDis);

  return getKeywordParam(config, "SELF_TRIGGER", chan, "DISABLED", EnableMap);
}
/**
 * getGatedStartMode
 *   This is used to turn on external start for DAQ - used when synchronizing more than one
 *   module. 
 * @config - The configuration dictionary.
 * @return int - 1 for enabled 0 for disabled (note this is a [COMMON] only parameter.
 */
int
getGatedStartMode(dictionary* config)
{
  loadMap(EnableMap, EnDis);
  return getKeywordParam(config, "GATED_START", -1, "DISABLED", EnableMap);
}
/**
 * getChannelPulsePolarity
 *   Returns the channel pulse polarity as configured.
 * @param config - the configuration dictionary.
 * @param chan   - channel we want.
 * @param pOldCOnfig - If supplied the configuration from which the default comes.
 * @return CAEN_DGTZ_PulsePolarity_t
 */
CAEN_DGTZ_PulsePolarity_t
getChannelPulsePolarity(dictionary* config, int chan, dictionary* pOldConfig)
{
  loadMap(PolarityMap, Polarities);
  
  std::string defaultPolarity = "NEGATIVE";
  if (pOldConfig) {
    defaultPolarity = getParamString(
      pOldConfig, "PULSE_POLARITY", chan, "NEGATIVE"
    );
  }
  return static_cast<CAEN_DGTZ_PulsePolarity_t>(
     getKeywordParam(config, "PULSE_POLARITY", chan, defaultPolarity, PolarityMap)
  );
}

/**
 * getModuleTriggerConfig
 *   Gets the trigger configuration...this is given as obsolete in the digitizer software 
 *   manual..presumably because the CFD make peak triggering obsolete?
 * @param config - The configuration dict.
 * @return CAEN_DGTZ_DPP_TriggerConfig_t ready to stuff in the trgc element of the
 *               CAEN_DGTZ_DPP_TrigggerConfig_t
 */
CAEN_DGTZ_DPP_TriggerConfig_t
getModuleTriggerConfig(dictionary* config)
{
  loadMap(TriggerConfigMap, TriggerConfigs);

  return static_cast<CAEN_DGTZ_DPP_TriggerConfig_t>(
      getKeywordParam(config, "TRIGGER_CONFIG", -1, "THRESHOLD", TriggerModeMap)
  );
}
/**
 * getModuleTriggerMode
 *   Gets the trigger mode (Normal or Coincidence).
 * @param config - Configuration dictionary.
 * @return CAEN_DGTZ_DPP_TriggerMode_t - trigger mode that can be directly sent to
 *           CAEN_DGTZ_SetDPPTriggerMode.
 */
CAEN_DGTZ_DPP_TriggerMode_t
getModuleTriggerMode(dictionary* config)
{
  loadMap(TriggerModeMap, TriggerModes);
  return static_cast<CAEN_DGTZ_DPP_TriggerMode_t>(
      getKeywordParam(config, "TRIGGER_MODE", -1, "NORMAL", TriggerModeMap)
  );
}
/**
 *  getPilupRejectionMode
 *    Returns the desired pileup rejection mode for the board.
 * @param config - The configuration dictionary.
 * @return CAEN_DGTZ_DPP_PUR_t - the pileup rejection setting to store in the
 *           purh field of the CAEN_DGTZ_DPP_PSD_Params_t struct sent to 
 *           CAEN_DGTZ_SetDPPParameters as the params struct.
 */
CAEN_DGTZ_DPP_PUR_t
getPileupRejectionMode(dictionary* config)
{
  loadMap(PurMap, PurModes);
  return static_cast<CAEN_DGTZ_DPP_PUR_t>(
    getKeywordParam(config, "PUR_MODE", -1, "DETECT", PurMap)
  );
}

/**
 * getGPOMode
 *   Returns the correct values for bits 17-18 in the Front panel I/O
 *   register for the selected GPO mode.
 *
 * @param config - Configuration dictionary.
 * @param dfltValue- If the value is not provided this is the default value.
 * @return uint32_t
 */
 uint32_t
 getGPOMode(dictionary* config, const char* dfltValue)
 {
  loadMap(GPOMap, GPOValues);
  return static_cast<uint32_t>(getKeywordParam(config, "GPO", -1, dfltValue, GPOMap));
 }
 /**
  * getStartMode
  *    Returns the correct values for bitrs 0-1 of the Acquisition control
  *    register for the start mode.
  *
  * @param config -Configuration dictionary.
  * @param dfltValue - Default value.
  * @return uint32_t
  */
uint32_t getStartMode(dictionary* config, const char* dfltValue)
{
  loadMap(StartModeMap, StartModeValues);
  return static_cast<uint32_t>(
    getKeywordParam(config, "START_MODE", -1, dfltValue, StartModeMap)
  );
}

/**
 *  Return the value of the start mode enum given the start mode configuration option.
 *
 * @param config - parsed configuration dictionary.
 * @param dfltVal - Default value for the START_MODE parameter if not given.
 * @return CAEN_DGTZ_StartMode_t
 */
CAEN_DGTZ_AcqMode_t
getStartModeCode(dictionary* config, const char* dfltVal)
{
  loadMap(startModeCodeMap, StartModeCodeValues);

  return static_cast<CAEN_DGTZ_AcqMode_t>(
     getKeywordParam(config, "START_MODE", -1,  dfltVal, startModeCodeMap)
  );
}
