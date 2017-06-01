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

class CAENPhaChannelParameters
{
private:
  pugi::xml_document&  m_xml;

  // Parameters unpacked from the XML:

public:
  typedef enum _Polarity {positive, negative} Polarity;
  typedef enum _Coupling {ac, dc} Coupling;

  // <InputSignal> parameters: 

  unsigned  dcOffset;
  unsigned  decimation;
  unsigned  digitalGain;
  Polarity  polarity;
  unsigned  range;

  // <EnergyFilter> parameters:

  unsigned decayTime;
  double   trapRiseTime;
  double   flattopDelay;
  double   trapFlatTop ;
  unsigned BLMean;
  unsigned trapGain;
  unsigned otReject;
  unsigned peakMean;
  double   baselineHoldoff;
  unsigned peakHoldoff;
  
  // <Trigger> Parameters:

  unsigned threshold;
  unsigned rccr2smoothing;
  double   inputRiseTime;
  double   triggerHoldoff;
  double   triggerValidationWidth;    // in <RTDiscriminationWIndow> subtag.
  bool     coincidenceWindow;        //          ""     ""

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
  unsigned   acPoleZero;
  Coupling   inputCoupling;
  

public:
  CAENPhaChannelParameters(pugi::xml_document& doc);

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
