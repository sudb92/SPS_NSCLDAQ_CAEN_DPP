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
* @file     CAENPhaChannelParameters.cpp
* @brief    Converts parsed MCA2 XML channel  configuration file -> channel parameters
* @author   Ron Fox (rfoxkendo@gmail.com)
*
*  This class provides code that walks around the DOM tree created by
*  Parsing a PHA config file and stores the configuration, providing
*  accessors that can be used to drive the digitizer setup.
*/

#ifndef CAENPHACHANNELPARAMETERS_H
#define CAENPHACHANNELPARAMETERS_H

#include "pugixml.hpp"
#include <functional>

class CAENPhaChannelParameters
{
private:
  static pugi::xml_document empty;
  std::reference_wrapper<pugi::xml_document>  m_xml;

  // Parameters unpacked from the XML:

public:
  typedef enum _Polarity {positive, negative} Polarity;
  typedef enum _Coupling {ac, dc} Coupling;

  // Compass uses a sepasrate enable...
  // M2C just doesn't have us present if not enabled:
  
  bool enabled;
  
  // <InputSignal> parameters: 

  unsigned  dcOffset;
  unsigned  decimation;
  unsigned  digitalGain;
  Polarity  polarity;
  unsigned  range;

  // <EnergyFilter> parameters:

  double    decayTime;
  double   trapRiseTime;
  double   flattopDelay;
  double   trapFlatTop ;
  unsigned BLMean;
  unsigned trapGain;
  unsigned otReject;
  unsigned peakMean;
  double   baselineHoldoff;
  double   peakHoldoff;
  double   trapPeaking;
  
  // <Trigger> Parameters:

  unsigned threshold;
  unsigned rccr2smoothing;
  double   inputRiseTime;
  double   triggerHoldoff;
  double   triggerValidationWidth;    // in <RTDiscriminationWIndow> subtag.
  bool     coincidenceWindow;        //          ""     ""
  double   preTrigger;               // Add in MCA2

  // <TransistorReset> parameters:

  bool     trResetEnabled;
  unsigned trGain;
  unsigned trThreshold;
  unsigned trHoldoff;

  // <NewPHA> parameters:

  bool       energySkim;
  unsigned   lld;
  unsigned   uld;
  bool       fastTriggerCorrection;
  bool       baselineClip;
  unsigned   baselineAdjust;
  double     acPoleZero;
  Coupling   inputCoupling;
  double     fineGain;
  
  
  // New COMPASS parameters.
  
  bool psdCutEnable;
  double psdLowCut;
  double psdHighCut;
 
  //New parameters, v1.3.0
  bool fakeevt_ttroll_en;
  bool extras_enable;
  bool defaultPUREnable;
  




public:
  CAENPhaChannelParameters(pugi::xml_document& doc);
  CAENPhaChannelParameters() :  m_xml(empty) {}          // Default constructor
  CAENPhaChannelParameters(const CAENPhaChannelParameters& rhs);
  CAENPhaChannelParameters& operator=(const CAENPhaChannelParameters& rhs);


  void unpack();
  
private:
  void getInputSignalParams(pugi::xml_node iparams);
  void getEnergyFilterParams(pugi::xml_node eparams);
  void getTriggerParams(pugi::xml_node tparams);
  void getTransistorResetParams(pugi::xml_node trparams);
  void getNewPhaParams(pugi::xml_node newphanode);
  void getESkimParams(pugi::xml_node  skim);
};

#endif
