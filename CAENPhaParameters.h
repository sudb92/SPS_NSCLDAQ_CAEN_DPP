/*-------------------------------------------------------------
 
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
* @file     CAENPhaParameters.h
* @brief    Converts parsed MCA2 XML configuration file -> PHA parameters
* @author   Ron Fox (rfoxkendo@gmail.com)
*
*  This class provides code that walks around the DOM tree created by
*  Parsing a PHA config file and stores the configuration, providing
*  accessors that can be used to drive the digitizer setup.
*/

#ifndef CAENPHAPARAMETERS_H
#define CAENPHAPARAMETERS_H
#include "pugixml.hpp"
#include <vector>
#include <string>
#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

class CAENPhaChannelParameters;

/**
 * @class CAENPhaParameters
 *    Class to process the DOM struct of an MCA2 config file.
 */
class CAENPhaParameters {
private:
  pugi::xml_document& m_dom;
  std::vector<std::pair<unsigned, pugi::xml_document*> >& m_channelDoms;

  // Settings are public so we don't need getters:

public:
  typedef enum _CoincidenceOp   {
    Or, And, Majority
  } CoincidenceOp;
  typedef enum _TriggerControl {
    internal, external, both
  } TriggerControl;
  typedef enum _GPIOLogic {
    Logic_OR, Logic_AND
  } GPIOLogic;
  typedef enum _GPIODirection {
    input, output
  } GPIODirection;
  typedef enum _OutputSignal {
    On, Off
  } OutputSignal;
  

  typedef struct _ChannelCoincidenceSettings {
    unsigned            s_channel;
    bool                s_enabled;
    CoincidenceOp       s_operation;
    unsigned            s_mask;
    unsigned            s_majorityLevel;
    unsigned            s_window;
  } ChannelCoincidenceSettings, *pChannelCoincidenceSettings;


  int  acqMode;
  unsigned startDelay;
  double preTriggers;
  int    recordLength;
  std::vector <ChannelCoincidenceSettings> coincidenceSettings;

  vool         waveforms;
  bool         dualTrace;
  unsigned     analogTrace1;
  unsigned     analogTrace2;
  unsigned     digitalTrace1;
  unsigned     digitalTrace2;

  unsigned     maxEvtsBlt;
  unsigned     saveMask;
  unsigned     IOLevel;

  TriggerControl triggerSource;
  GPIOLogic      gpioLogic;
  int            transResetLength;
  int            transResetPeriod;
  
  typedef struct _GPIOGroupConfig {
    GPIODirection s_direction;
    OutputSignal  s_onoff;
    bool          s_dacinvert;
    int           s_dacoffset;
  } GPIOGroupConfig;

  GPIOGroupConfig groupconfigs[2];

  // With compass:
  
  CAEN_DGTZ_AcqMode_t s_startMode;
  
  // Decoded channel parameters.

  std::vector<std::pair<unsigned, CAENPhaChannelParameters*> > m_channelParameters;
  
  
public:
  CAENPhaParameters(pugi::xml_document& m_dom, std::vector<std::pair<unsigned, pugi::xml_document*> >& channelDoms);
  CAENPhaParameters() {}                // Default constructor
  ~CAENPhaParameters();


public:
  void unpack();

private:
  void processTopNode(pugi::xml_node aNode);
  void getAcquisitionSettings(pugi::xml_node acqsetnode);
  void processCoincidenceSettings(pugi::xml_node coincnode);
  void processWaveformSettings(pugi::xml_node wfnode);
  void processListParameters(pugi::xml_node lnode);
  void processGpioConfig(pugi::xml_node gpio);


  ChannelCoincidenceSettings getChannelCoincidenceSettings(unsigned chan, pugi::xml_node top);
  TriggerControl  decodeTriggerControl(pugi::xml_node node);
  GPIOLogic       decodeGPIOLogic(pugi::xml_node node);
  void            decodeGroupConfig(GPIOGroupConfig& group, pugi::xml_node node);


};

#endif
