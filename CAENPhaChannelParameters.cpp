#include "CAENPhaChannelParameters.h"
#include "pugiutils.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

pugi::xml_document  CAENPhaChannelParameters::empty;

/**
 * constructor
 * @param doc -the XML document parsed by pugixml.
 */
CAENPhaChannelParameters::CAENPhaChannelParameters(pugi::xml_document& doc) :
  m_xml(doc),
  enabled(true),           // For MCA2 where thre's no separate enabled.
  psdCutEnable(false)      // MCSA2 has no PDS low/high cuts.
  {}

CAENPhaChannelParameters::CAENPhaChannelParameters(const CAENPhaChannelParameters& rhs) : m_xml(rhs.m_xml)
{
  *this = rhs;
}

/**
 *  assignment
 *   @param rhs - the object we're being assigned from
 *   @return *this
 */
CAENPhaChannelParameters&
CAENPhaChannelParameters::operator=(const CAENPhaChannelParameters& rhs)
{
  if (this != &rhs) {               // Don't bother if not distinct.
    m_xml = rhs.m_xml;
    enabled = rhs.enabled;
    dcOffset = rhs.dcOffset;
    decimation = rhs.decimation;
    digitalGain = rhs.digitalGain;
    polarity  = rhs.polarity;
    range     = rhs.range;
    decayTime = rhs.decayTime;
    trapRiseTime = rhs.trapRiseTime;
    flattopDelay = rhs.flattopDelay;
    trapFlatTop  = rhs.trapFlatTop;
    BLMean = rhs.BLMean;
    trapGain = rhs.trapGain;
    otReject = rhs.otReject;
    peakMean = rhs.peakMean;
    baselineHoldoff = rhs.baselineHoldoff;
    peakHoldoff = rhs.peakHoldoff;
    threshold = rhs.threshold;
    rccr2smoothing =rhs.rccr2smoothing;
    inputRiseTime  = rhs.inputRiseTime;
    triggerHoldoff = rhs.triggerHoldoff;
    triggerValidationWidth = rhs.triggerValidationWidth;
    coincidenceWindow = rhs.coincidenceWindow;
    preTrigger = rhs.preTrigger;
    trResetEnabled = rhs.trResetEnabled;
    trGain = rhs.trGain;
    trThreshold = rhs.trThreshold;
    trHoldoff = rhs.trHoldoff;
    energySkim = rhs.energySkim;
    lld = rhs.lld;
    uld = rhs.uld;
    fastTriggerCorrection = rhs.fastTriggerCorrection;
    baselineClip  = rhs.baselineClip;
    baselineAdjust= rhs.baselineAdjust;
    acPoleZero   = rhs.acPoleZero;
    inputCoupling = rhs.inputCoupling;
    
    psdCutEnable = rhs.psdCutEnable;
    psdLowCut    = rhs.psdLowCut;
    psdHighCut   = rhs.psdHighCut;
  }
  return *this;
}
 
/**
 * unpack
 *   Travel the dom tree unpacking the parsed XML into parameters.
 */
void CAENPhaChannelParameters::unpack()
{
  pugi::xml_node top = m_xml.get().first_child();
  if (std::string("config") != top.name()) {
    std::string msg =  "Top node is not <config> in channel config file\n";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }

  // Iterate over the top level nodes and invoke the appropriate handlers:

  for (auto aNode = top.first_child(); aNode; aNode = aNode.next_sibling()) {
    std::string name = aNode.name();

    if (name == "InputSignal") {
      getInputSignalParams(aNode);
    } else if (name == "EnergyFilter") {
      getEnergyFilterParams(aNode);
    } else if (name == "Trigger") {
      getTriggerParams(aNode);
    } else if (name == "TransistorReset") {
      getTransistorResetParams(aNode);
    } else if (name == "NewPHA") {
      getNewPhaParams(aNode);
    }
  }

}


/**
 * getInputSIgnalParams
 *   Processes the <InputSIgnal> top level tag.  The contents of this tag define characteristics
 *   of the channel's input signal.  Note that <InputImpedance> is ignored since it is
 *   not evidently meaningful in the devices(?)
 *
 * @param iparams - pgui::xml_node for <InputSignal>
 *
 */
void
CAENPhaChannelParameters::getInputSignalParams(pugi::xml_node iparams)
{
  // <DCOffset>  - required:

  pugi::xml_node dcoffNode = getNodeByName(iparams, "DCOffset");
  if (dcoffNode.type() == pugi::node_null) {
    std::string msg("Unable to find <DCOFfset> inside <InputSignal> tag body");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);				          
  }
  dcOffset = getUnsignedValue(dcoffNode);


  // <InputDigitalGain>

  pugi::xml_node dgainNode = getNodeByName(iparams, "InputDigitalGain");
  if (dgainNode.type() == pugi::node_null) {
    std::string msg("Unable to find  <InputDigitalGain>  in <InpugtSignal> tag body");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  digitalGain = getUnsignedValue(dgainNode);

  // <Decimation>

  pugi::xml_node decimateNode = getNodeByName(iparams, "Decimation");
  if(decimateNode.type() == pugi::node_null) {
    std::string msg("Unable to find <Decimation> in <InputSignal> tag body");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  decimation = getUnsignedValue(decimateNode);

  // <PulsePolarity>
  
  pugi::xml_node polarityNode = getNodeByName(iparams, "PulsePolarity");
  if (polarityNode.type() == pugi::node_null) {
    std::string msg("Unable to find <PulsePolarity> in <InputSIgnal> tag body");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  polarity = getUnsignedValue(polarityNode) == 0 ? positive  : negative;

  // <InputRange> - this is an enumerated type.

  pugi::xml_node rangeNode = getNodeByName(iparams, "InputRange");
  if (rangeNode.type() == pugi::node_null) {
    std::string msg("Unable to find <InputRange> in <InputSignal> tag body");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  range = getUnsignedValue(rangeNode);

}

/**
 * getEnergyFilterParams
 *   Get the parameters of the energy filter(s).
 *
 * @param eparams - <EnergyFilter> tag node.
 */
void
CAENPhaChannelParameters::getEnergyFilterParams(pugi::xml_node eparams)
{
  // <DecayTime>

  pugi::xml_node dtimeNode = getNodeByName(eparams, "DecayTime");
  if (dtimeNode.type() == pugi::node_null) {
    std::string msg("Missing <DecayTime> tag in <EnergyFilter> body");
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  decayTime = getDoubleValue(dtimeNode);

  // <TrapRiseTime>

  pugi::xml_node trNode = getNodeByName(eparams, "TrapRiseTime");
  if (trNode.type() == pugi::node_null) {
    std::string msg = "Missing <TrapRiseTime> tag inside <EnergyFilter> body";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  trapRiseTime = getDoubleValue(trNode);

  // <FlatTopDelay>

  pugi::xml_node ftDelayNode = getNodeByName(eparams, "FlatTopDelay");
  if (ftDelayNode.type() == pugi::node_null) {
    std::string msg = "Missing <FlatTopDelay> tag in <EnergyFilter>";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  flattopDelay = getDoubleValue(ftDelayNode);

  // <TrapFlatTop>

  pugi::xml_node trFlatTopNode = getNodeByName(eparams, "TrapFlatTop");
  if (trFlatTopNode.type()  == pugi::node_null) {
    std::string msg = "Missing <TrapFlatTop> tag in <EnergyFilter>";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  trapFlatTop = getDoubleValue(trFlatTopNode);

  // <BLMean>

  pugi::xml_node blMeanNode = getNodeByName(eparams, "BLMean");
  if (blMeanNode.type()  == pugi::node_null) {
    std::string msg = "Missing <BLMean> tag in <EnergyFilter>";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
 } 
  BLMean = getUnsignedValue(blMeanNode);

  // <TrapezioddGain>  sic

  pugi::xml_node tGainNode = getNodeByName(eparams, "TrapeziodGain");
  if (tGainNode.type()  == pugi::node_null) {
    std::string msg = "Missing <TrapeziodGain> (sic) tag in <EnergyFilter>";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  } 
  trapGain = getUnsignedValue(tGainNode);
  
  // <OTREJ>

  pugi::xml_node otrejNode = getNodeByName(eparams, "OTREJ");
  if (otrejNode.type()  == pugi::node_null) {
    std::string msg = "Missing <OTREJ> tag in <EnergyFilter>";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  } 
  otReject = getUnsignedValue(otrejNode);

  // <PeakMean>

  auto peakMeanNode = getNodeByName(eparams, "PeakMean");
  
  if (peakMeanNode.type()  == pugi::node_null) {
    std::string msg = "Missing <PeakMean> tag in <EnergyFilter>";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  } 
  peakMean = getUnsignedValue(peakMeanNode);

  // <BLHoldoff>

  auto blHoldoffNode = getNodeByName(eparams, "BLHoldoff");
  if (blHoldoffNode.type()  == pugi::node_null) {
    std::string msg = "Missing <BLHoldoff> tag in <EnergyFilter>";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  } 
  baselineHoldoff = getDoubleValue(blHoldoffNode);

  // <PeakHoldoff>

  auto pkHoldoffNode = getNodeByName(eparams, "PeakHoldoff");
  if (pkHoldoffNode.type()  == pugi::node_null) {
    std::string msg = "Missing <PeakHoldoff> tag in <EnergyFilter>";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  } 
  peakHoldoff = getUnsignedValue(pkHoldoffNode);

}
/**
 * getTriggerParams
 *    Return parameters in the <Trigger> tag.
 *
 * @param tparams - <Trigger> node.
 */
void
CAENPhaChannelParameters::getTriggerParams(pugi::xml_node tparams)
{
  // <Threshold>

  pugi::xml_node threshNode = getNodeByNameOrThrow(tparams, "Threshold", 
						   "Missing <Threshold> tag in <Trigger>");
  threshold = getUnsignedValue(tparams);

  pugi::xml_node smoothingNode = getNodeByNameOrThrow(tparams, "RC-CR2Smoothing",
						      "Missing <RC-CR2Smoothing tag in <Trigger>");
  rccr2smoothing = getUnsignedValue(smoothingNode);

  pugi::xml_node riseTimeNode = getNodeByNameOrThrow(tparams, "InputRiseTime",
						    "Missing <InputRiseTime> tag in <Trigger>");
  inputRiseTime = getDoubleValue(riseTimeNode);

  pugi::xml_node tholdoffNode = getNodeByNameOrThrow(tparams, "TriggerHoldoff", 
						     "Missing <TriggerHoldoff> tag in <Trigger>");
  triggerHoldoff = getDoubleValue(tholdoffNode);

  pugi::xml_node vwidthNode = getNodeByNameOrThrow(tparams, "TrgValidationWidth",
						   "Missing <TriggerValidationWidth> tag in <Trigger>");
  triggerValidationWidth = getDoubleValue(vwidthNode);

  pugi::xml_node cwindowNode = getNodeByNameOrThrow(tparams, "CoincidenceWindow",
						    "Missing <CoincidenceWindow> tag in <Trigger>");
  coincidenceWindow = getUnsignedValue(cwindowNode) != 0;


}
/**
 * getTransistorResetParams
 *   Get parameters from <TransistorReset> tag.
 *
 * @param trparams - <TransistorReset> tag node.
 */
void
CAENPhaChannelParameters::getTransistorResetParams(pugi::xml_node trparams)
{
  trResetEnabled = getBoolValue(getNodeByNameOrThrow(trparams, "TREnabled",
						     "Missing <TREnabled> tag in <TransistorReset>"));

  trGain = getUnsignedValue(getNodeByNameOrThrow(trparams, "TRGain",
						 "Missing <TRGain> tag in <TransistorReset>"));
  
  trThreshold = getUnsignedValue(getNodeByNameOrThrow(trparams, "TRThreshold", 
						      "Missing <TRThreshold> Tag in <TransistorReset>"));

  trHoldoff = getUnsignedValue(getNodeByNameOrThrow(trparams, "TRHoldoff", 
						    "Missing <TRHoldoff> tag in <TransistorReset>"));

}

/**
 * getNewPhaParams
 *   Process the <NewPHA> tag which has the new pulse heigh analysis parameters.
 *
 * @param newphanode - the node for the <NewPHA> tag.
 *
 */
void
CAENPhaChannelParameters::getNewPhaParams(pugi::xml_node newphanode)
{
  pugi::xml_node eskim = getNodeByNameOrThrow(newphanode, "EnergySkim", 
					      "Missing <EnergySkim> tag in <NewPHA>");
  getESkimParams(eskim);

  fastTriggerCorrection = 
    getBoolValue(getNodeByNameOrThrow(newphanode, "FastTriggerCorrectionEnabled",
				      "Missing <FastTriggerCorrectionEnabled> tag in <NewPHA>"));

  baselineClip  = getBoolValue(getNodeByNameOrThrow(newphanode, "BaselineClipEnabled",
						   "Missing <BaselineClipEnabled> tag in <NewPHA>"));
  
  baselineAdjust = 
    getUnsignedValue(getNodeByNameOrThrow(newphanode, "BaselineAdjust",
					  "Missing <BaselineAdjust> tag in <NewPHA>"));

  acPoleZero = getUnsignedValue(getNodeByNameOrThrow(newphanode, "ACPoleZero",
						     "Missing <ACPoleZero> tag in <NewPHA>"));

  std::string couplingString = getValue(getNodeByNameOrThrow(newphanode, "InputCoupling",
							     "Missing <InputCoupling> tag in <NewPHA>"));
  if (couplingString == "CAENDPP_INCoupling_DC") {
    inputCoupling = dc;
  }  else {
    inputCoupling = ac;
  }
		   
}
/**
 * getESkimParams
 *   Unpack the parameters in the NewPha->EnergySkim tag.
 *
 * @param skim - the <EnergySkim> node.
 *
 */
void
CAENPhaChannelParameters::getESkimParams(pugi::xml_node skim)
{
  energySkim = getBoolContents(getNodeByNameOrThrow(skim, "enabled",
						    "No <enabled> tag inside <EnergySkim>"));
  lld = getUnsignedContents(getNodeByNameOrThrow(skim, "LLD",
						 "No <LLD> tag inside <EnergySkim>"));
  uld = getUnsignedContents(getNodeByNameOrThrow(skim, "ULD",
						 "No <ULD> tag inside <EnergySkim"));
}
