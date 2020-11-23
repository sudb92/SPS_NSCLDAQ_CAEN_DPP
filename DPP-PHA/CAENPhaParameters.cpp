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
* @file     CAENPhaParameters.cpp
* @brief    Converts parsed MCA2 XML configuration file -> PHA parameters
* @author   Ron Fox (rfoxkendo@gmail.com)
*
*  This class provides code that walks around the DOM tree created by
*  Parsing a PHA config file and stores the configuration, providing
*  accessors that can be used to drive the digitizer setup.
*/
#include "CAENPhaParameters.h"
#include "CAENPhaChannelParameters.h"
#include "pugiutils.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <sstream>



pugi::xml_document CAENPhaParameters::empty;
std::vector<std::pair<unsigned, pugi::xml_document*> > CAENPhaParameters::emptyDoms;


/*-------------------------------------------------------------------------
 *  Implementation of CAENPhaParameters class.
 */

/**
 * Constructor
 *   @param dom - the parsed dom.
 *   @param chDoms - vector of pairs of channel numbers and the doms that set them up.
 * 
 */
CAENPhaParameters::CAENPhaParameters(
    pugi::xml_document& dom, std::vector<std::pair<unsigned, pugi::xml_document*> >& chDoms) :
  m_dom(dom),
  m_channelDoms(chDoms), acqMode(1), ioctlmask(0),isExtTrgEnabled(false),isExtVetoEnabled(false)
{}
/**
 * Copy construction.
 *
 * @param rhs - the object that will be copy constructed into *this.
 */
CAENPhaParameters::CAENPhaParameters(const CAENPhaParameters& rhs) : 
  m_dom(rhs.m_dom), m_channelDoms(rhs.m_channelDoms)
{
  *this = rhs;
}
/**
 * Assignment
 *    copies the rhs into this -- if the rhs is distinct from this.
 *
 *  @param rhs - the guy to copy
 *  @return *this
 */
CAENPhaParameters&
CAENPhaParameters::operator=(const CAENPhaParameters& rhs)
{
  if (this != &rhs) {
    m_dom = rhs.m_dom;
    m_channelDoms = rhs.m_channelDoms;

    acqMode = rhs.acqMode;
    startDelay = rhs.startDelay;
    preTriggers = rhs.preTriggers;
    recordLength = rhs.recordLength;
    coincidenceSettings = rhs.coincidenceSettings;
    
    waveforms = rhs.waveforms;
    dualTrace = rhs.dualTrace;
    analogTrace1 = rhs.analogTrace1;
    analogTrace2 = rhs.analogTrace2;
    digitalTrace1 = rhs.digitalTrace2;

    maxEvtsBlt = rhs.maxEvtsBlt;
    saveMask   = rhs.saveMask;
    IOLevel    = rhs.IOLevel;

    triggerSource = rhs.triggerSource;
    gpioLogic = rhs. gpioLogic;
    transResetLength = rhs.transResetLength;
    transResetPeriod = rhs.transResetPeriod;
    OnboardCoinc = rhs.OnboardCoinc;

    groupconfigs[0] = rhs.groupconfigs[0];
    groupconfigs[1] = rhs.groupconfigs[1];

    s_startMode = rhs.s_startMode;
    shapTrgWidth = rhs.shapTrgWidth;
    trgoutmode = rhs.trgoutmode;
    m_channelParameters = rhs.m_channelParameters;
    ioctlmask = rhs.ioctlmask;

  }
  return  *this;
}

/**
 * Destructor:
 *    Kill off the channel parameter objects.
 */
CAENPhaParameters::~CAENPhaParameters()
{
  for (auto i = 0; i < m_channelParameters.size(); i++) {
    delete m_channelParameters[i].second;
  }
}

/**
 * unpack
 *   Process the dom tree placing the stuff we care about into
 *   appropriate member data.  This must be called prior to accessing
 *   the data.
 */
void
CAENPhaParameters::unpack()
{
  pugi::xml_document& dom(m_dom);
  
  pugi::xml_node top = dom.first_child();
  
  if(std::string(top.name()) !=  "config") {
    std::cerr << "Not a config file.. expected config element got " << top.name() << std::endl;
    throw std::domain_error("Invalid configuration file");
  }
  // Iterate over the top level tags and pass them off to the dispatcher:

  pugi::xml_node aTopNode;
  for (aTopNode = top.first_child(); aTopNode != top.last_child(); aTopNode = aTopNode.next_sibling()) {
    processTopNode(aTopNode);
  }
  processTopNode(aTopNode);        // Last child is an actual child.  Not null.

  // Process the channel parametrs to stock m_channelParameters.

  for (auto i = 0; i < m_channelDoms.get().size(); i++) {
    unsigned ch = m_channelDoms.get()[i].first;
    pugi::xml_document& chdoc( *(m_channelDoms.get()[i].second));
    CAENPhaChannelParameters* pChannel = new CAENPhaChannelParameters(chdoc);
    pChannel->unpack();
    m_channelParameters.push_back(std::pair<unsigned, CAENPhaChannelParameters*>(ch, pChannel));
  }
}
/*-------------------------------------------------------------------------------------
*  Utility methods:
*/

/**
 * processTopNode 
 *   Based on the name of a name (tag name) of a node, dispatch to a node processor to pull
 *   out the parameters we need for DAQ
 *
 * @param aNode - a node to process (passed by reference).
 */
void
CAENPhaParameters::processTopNode(pugi::xml_node aNode)
{
  const char* name = aNode.name();
  std::cout << "Processing node: " << name << std::endl;
  std::cout.flush();

  if (std::string("acquisitionsettings") == name) {
    getAcquisitionSettings(aNode);
  } else if (std::string("CoincidenceSettings") == name) {
    processCoincidenceSettings(aNode);
  } else if (std::string("WaveformSettings") == name) {
    processWaveformSettings(aNode);
  } else if (std::string("listparameters") == name) {
    processListParameters(aNode);
  } else if (std::string("GPIOConfig") == name) {
    processGpioConfig(aNode);
  } 

}/**
 * getAcquisitionSettings
 *   Given a top level tag that is <acquisitionsettings>:
 *    -  find the Pre-Triggers/value node, decode its contents as a double -> preTriggers
 *    -  find the RecordLength/value node, decode its contents as an int   -> recordLength
 *
 * @param aNode - xml node reference the caller assures is is for <acquisitionsettings>
 *
 * @note - if either of the nodes we need can't be found, we throw an error.
 *
 */
void
CAENPhaParameters::getAcquisitionSettings(pugi::xml_node aNode)
{
     // Note mispelled in the CAEN XML - may need to fix this if they do.
  pugi::xml_node modenode = getNodeByNameOrThrow(aNode, "AcquisitonMode", "No <AcquisitonMode> tag in <acquistionsettings>");
  acqMode = getUnsignedValue(modenode);

  pugi::xml_node pretrg = getNodeByName(aNode, "Pre-Triggers");
  pugi::xml_node reclen = getNodeByName(aNode, "RecordLength");

  if((pretrg.type() == pugi::node_null) || (reclen.type() == pugi::node_null)) {
    std::string msg( "Not a config file - <acquisitionsettings> is missing one of <Pre-Triggers> or <RecordLength>");
    std::cerr << msg << std::endl;
    throw:: std::domain_error(msg);
  }
  std::string preTrigString = getValue(pretrg);
  std::string reclenString  = getValue(reclen);

  preTriggers = std::stof(preTrigString.c_str());
  recordLength = std::stoul(reclenString);
}
  
/**
 * processCoincidenceSettings
 *    Process the contents of a <CoincidenceSettings> tag.  This is assumed to be a
 *    collection of <CoincidenceParametersChanneln> where n is a channel number.
 *    It is assumed that the first n that comes up missing is the end of it.
 *    That is no channels are skipped (too bad the XML designers just didn't do
 *    an attribute to select a channel number - would have been tres easy then.
 * 
 *    Each of these tags is assumed to contain:
 *    -   <CoincidenceLogic> whose contents are CAENDPP_CoincLogic_None  if coincidence
 *        triggering is not used else something else to turn  it on.
 *    -   <CoincidenceOperation> which can be CAENDPP_CoincOp_OR or CAENDPP_CoincOp_AND
 *    -   <MajorityLevel> - an integer.
 *    -   <TriggerWindow> - The coincidence overlap window.
 *
 * @param node - <CoincidencdSettings> node.
 */
void
CAENPhaParameters::processCoincidenceSettings(pugi::xml_node node)
{
  unsigned channel= 0;
  ChannelCoincidenceSettings settings;
  bool done = false;

  while (! done) {
    std::stringstream chan;
    chan << "CoincidenceParametersChannel" << channel;
    pugi::xml_node channelNode = getNodeByName(node, chan.str().c_str());
    if (channelNode.type() != pugi::node_null) {
      settings = getChannelCoincidenceSettings(channel, channelNode);
      coincidenceSettings.push_back(settings);
      
    } else {
      done = true;
    }
    channel++;
  }
  
}
/**
 * processWaveformSettings
 * 
 *   This proceses the settings that determine which waveforms are acquired in 
 *   addition to the PHA data.  We don't apply any computed transformation so
 *   the only things we carea bout are:
 *   - <DualTrace>   value 0 for no and 1 for yes.
 *   - <AnalogTrace1>  first analog trace.
 *   - <AnalogTrace2>  second analog trace (if dual trace on).
 *   - <DigitalTrace1> First digital trace
 *   - <DigitalTrace2> Second digital trace.
 * 
 * @param wfnode - The node for the <WaveformSettings> tag.
 */
void
CAENPhaParameters::processWaveformSettings(pugi::xml_node wfnode)
{
  // All of these nodes have a <value> child tag.

  pugi::xml_node dual = getNodeByName(wfnode, "DualTrace");
  if (dual.type() == pugi::node_null) {
    std::string msg("Unable to locate <DualTrace> tag");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  std::string dtString = getValue(dual);
  dualTrace =  std::stoul(dtString) != 0;

  pugi::xml_node at1 = getNodeByName(wfnode, "AnalogTrace1");
  if (at1.type() == pugi::node_null) {
    std::string msg("Unable to locate <AnalogTrace1> tag");
    std::cerr << msg <<  std::endl;
    throw std::domain_error(msg);
  }
  std::string at1str = getValue(at1);
  analogTrace1 = std::stoul(at1str);

  pugi::xml_node at2 = getNodeByName(wfnode, "AnalogTrace2");
  if (at2.type() == pugi::node_null) {
    std::string msg("Unable to locate <AnalogTrace2> tag");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  std::string at2str = getValue(at2);
  analogTrace2 = std::stoul(at2str);

  pugi::xml_node dt1 = getNodeByName(wfnode, "DigitalTrace1");
  if (dt1.type() == pugi::node_null) {
    std::string msg("Unable to locate <DigitalTrace1>");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  std::string dt1str = getValue(dt1);
  digitalTrace1 = stoul(dt1str);

  pugi::xml_node dt2 = getNodeByName(wfnode, "DigitalTrace2");
  if (dt2.type() == pugi::node_null) {
    std::string msg("Unable to locate <DigitalTrace2>");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  std::string dt2str = getValue(dt2);
  digitalTrace2 = std::stoul(dt2str);
}
/**
 * processListParameters
 *   Process the list mode parameters.  There's much here to ignore as these
 *   parameters also control wheter MCA2 records data and where e.g.
 *   That's left to the underlying data acquisition system.
 *   What we do care about is:
 *  - <MaxNumEvtsBLT>  - maximum number of events in a BLT.
 *  - <SaveMask>       - provisionally as I'm not sure what this is.
 *  - <IOLevel>        - Select NIM or TTL for front panel LEMO I/Os.
 *
 * @param lnode - The <listparameters> node
 */
void
CAENPhaParameters::processListParameters(pugi::xml_node lnode)
{
  pugi::xml_node maxblt = getNodeByName(lnode, "MaxNumEvtsBLT");
  if (maxblt.type() == pugi::node_null) {
    std::string msg("Unable to locate <MaxNumEvtsBLT> tag");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  std::string maxbltstr = getValue(maxblt);
  maxEvtsBlt =  stoul(maxbltstr);

  pugi::xml_node savemask = getNodeByName(lnode, "SaveMask");
  if (savemask.type() == pugi::node_null) {
    std::string msg("Unable to locate <SaveMask> tag");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  std::string maskStr = getValue(savemask);
  saveMask =  stoul(maskStr);

  pugi::xml_node iolevel = getNodeByName(lnode, "IOLevel");
  if (iolevel.type() == pugi::node_null) {
    std::string msg("Unable to locate <IOLevel> tag");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  std::string levelstr = getValue(iolevel);
  IOLevel = stoul(levelstr);

}
/**
 * processGpioConfig
 *    Decode the configuration of the GPIO front panel I/O:
 *    <TriggerCOntrol>  - internal/external.
 *    <GPIOLogic>  - and or or.
 *     <TransResetLength> integer.
 *     <TransResetPeriod> integer
 *     <GPIO> - group one configuration.
 *     <GPIO> - group two configuration.
 *
 * @param gpio - <GPIOConfig> node.
 */
void
CAENPhaParameters::processGpioConfig(pugi::xml_node gpio)
{
  pugi::xml_node gpioNode;
  int group (0);
  for (gpioNode = gpio.first_child(); gpioNode; gpioNode = gpioNode.next_sibling())  {
    std::cout << "GPIO:  " << gpioNode.name() << std::endl;
    if (std::string("TriggerControl") == gpioNode.name()) {
      triggerSource = decodeTriggerControl(gpioNode);  // Decode the ctext.
    } else if(std::string("GPIOLogic") == gpioNode.name()) {
      gpioLogic = decodeGPIOLogic(gpioNode);
    } else if (std::string("TransResetLength") == gpioNode.name()) {
      transResetLength = getUnsignedContents(gpioNode);
    } else if (std::string("TransResetPeriod") == gpioNode.name()) {
      transResetPeriod = getUnsignedContents(gpioNode);
    } else if (std::string("GPIO")             == gpioNode.name()) {
      decodeGroupConfig(groupconfigs[group], gpioNode);
      group++;
    }
  }
}  


/**
 * getChannelCoincidenceSettings
 *   Given the top node of a channel coincidence setting block pulls out the
 *   Items we need.
 * 
 * @param ch  - Channel number.
 * @param top - a <coincidenceParametrsChanneln> tag.
 * @return ChannelCoincidenceSettings struct.
 */
CAENPhaParameters::ChannelCoincidenceSettings
CAENPhaParameters::getChannelCoincidenceSettings(unsigned ch, pugi::xml_node top)
{
  ChannelCoincidenceSettings result;
  result.s_channel = ch;

  // Logic enabled?
  
  ::  pugi::xml_node logic = getNodeByName(top, "CoincidenceLogic");
  if (logic.type() == pugi::node_null) {
    std::stringstream msg;
    msg << "Unable to find the <CoincidenceLogic> tag for " << top.name();
    std::cerr << msg.str() << std::endl;
    throw std::domain_error(msg.str());
  }

  std::string logicstr =  getStringContents(logic);
  result.s_enabled = logicstr != "CAENDPP_CoincLogic_None";

  pugi::xml_node op = getNodeByName(top, "CoincidenceOperation");
  if (op.type() == pugi::node_null) {
    std::stringstream msg;
    msg << "Unable to find the <CoincidencdeOperation> tag for " << top.name();
    std::cerr << msg.str() << std::endl;
    throw std::domain_error(msg.str());
  }
  std::string opString = getStringContents(op);
  if (opString == "CAENDPP_CoincOp_OR") {
    result.s_operation = Or;
  } else if (opString == "CAENDPP_CoincOp_AND") {
    result.s_operation = And;
  } else {
    result.s_operation = Majority;
  }

  pugi::xml_node mask = getNodeByName(top, "CoincidenceMask");
  if (mask.type() == pugi::node_null) {
    std::stringstream msg;
    msg << "Unable to find the <CoincidenceMask> tag for " << top.name();
    std::cerr << msg.str() << std::endl;
    throw std::domain_error(msg.str());
  }
  result.s_mask = getUnsignedContents(mask);

  pugi::xml_node majority = getNodeByName(top, "MajorityLevel");
  if(majority.type() == pugi::node_null) {
    std::stringstream msg;
    msg << "Unable to find the <MajorityLevel> tag for " << top.name();
    std::cerr << msg.str() << std::endl;
    throw std::domain_error(msg.str());
  }
  result.s_majorityLevel = getUnsignedContents(majority);

  pugi::xml_node win = getNodeByName(top, "TriggerWindow");
  if (majority.type() == pugi::node_null) {
    std::stringstream msg;
    msg << "Unable to find the <TriggerWindow> tag for " << top.name();
    std::cerr << msg.str() << std::endl;
    throw std::domain_error(msg.str());
  }
  result.s_window = getUnsignedContents(win);

  return result;
  
}


/**
 * decodeTriggerControl
 *  Given the trigger control decodes the textual value into a TriggerControl enum.
 *
 * @param node -node that contains the trigger control text.
 * @return TriggerControl
 */
CAENPhaParameters::TriggerControl
CAENPhaParameters::decodeTriggerControl(pugi::xml_node node)
{
  std::string strValue = getStringContents(node);
  if (strValue == "CAENDPP_TriggerControl_Internal") {
    return internal;
  } else if (strValue == "CAENDPP_TriggerControl_External")  {
    return external;
  } else {
    return both;
  }
}

/**
 * decodeGPIOLogic
 *   Given the <GPIOLogic> node decodes its cdata contents to a GPIOLogic enum.
 * 
 * @param node -the node to decode.
 * @return GPIOLogic.
 */
CAENPhaParameters::GPIOLogic
CAENPhaParameters::decodeGPIOLogic(pugi::xml_node node)
{
  std::string strValue = getStringContents(node);
  if (strValue == "CAENDPP_GPIOLogic_OR") {
    return Logic_OR;
  } else {
    return Logic_AND;
  }
}

/**
 * decodeGroupConfig
 *   Given a reference to a GPIOGroupConfig struct and the <GPIO> node that
 *   describes it fills in the values of that struct.
 *
 * @param group - group configuration struct.
 * @param node  - <GPIO> node.
 */

void
CAENPhaParameters::decodeGroupConfig(GPIOGroupConfig& group, pugi::xml_node node)
{
  pugi::xml_node modeNode = getNodeByName(node, "GPIOMode");
  if(modeNode.type() == pugi::node_null) {
    std::string msg = "Unable to find <GPIOMode> tag inside  <GPIO> tag group";
    std::cerr << msg << std::endl;
    throw std::logic_error(msg);
  }
  std::string modeStr = getStringContents(modeNode);
  if (modeStr == "CAENDPP_GPIOMode_OUTSignal") {
    group.s_direction  = output;
  } else {
    group.s_direction = input;
  }

  pugi::xml_node onoffNode =  getNodeByName(node, "SignalOut");
  if(onoffNode.type() == pugi::node_null) {
    std::string msg= "Unable to find <SignalOut> tag in side of <GPIO>";
    std::cerr << msg << std::endl;
    throw std::logic_error(msg);
  }
  std::string onoffStr = getStringContents(onoffNode);
  if (onoffStr == "CAENDPP_OUTSignal_OFF") {
    group.s_onoff = Off;
  } else {
    group.s_onoff = On;
  }

  pugi::xml_node invertNode = getNodeByName(node, "DACInvert");
  if (invertNode.type() == pugi::node_null) {
    std::string msg = "Unable to locate <DACInvert> tag in <GPIO> tag group";
    std::cerr << msg << std::endl;
    throw std::logic_error(msg);
  }
  group.s_dacinvert = getBoolContents(invertNode);



  pugi::xml_node offsetNode = getNodeByName(node, "DACOffset");
  if (offsetNode.type() == pugi::node_null) {
    std::string msg = "Unable to locate <DACOffset> tag in <GPIO> tagset";
    std::cerr << msg << std::endl;
  }
  group.s_dacoffset = getUnsignedContents(offsetNode);

}


