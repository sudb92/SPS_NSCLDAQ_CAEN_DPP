/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2009.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Author:
             Ron Fox
	     NSCL
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

/**
* @file PSDParameters.cpp
* @brief  Implement compass file parsing for the DPP-PSD parameters.
*/
#include "PSDParameters.h"
#include "pugiutils.h"

#include <stdexcept>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>


/**
 * parseConfigurationFile
 *    Parse the configuration file and then process the specific
 *    parts of that file.
 * @param pFilename - the name of the file.
 */
void
PSDParameters::parseConfigurationFile(const char* pFilename)
{
    pugi::xml_document  doc;
    
    pugi::xml_parse_result res = doc.load_file(pFilename);
    if (!res) {
      throw std::invalid_argument(res.description());
    }
    parseConfiguration(doc);
    
}
/**
 * parseConfiguration
 *    Given that the XML configuration has been loaded into a
 *    document, that document is processed to build both device
 *    and channel configurations.  We care about the
 *    stuff that's in the DOM below the
 *    <configuration><board> nodes.  Note that there can be more than one
 *    board.  Each of these results in an elemnt in s_boardParams;
 */
void
PSDParameters::parseConfiguration(pugi::xml_document& doc)
{
    //  Need the <configuration> tag and then iterate over the
    //  <board> tags it contains.
    
    pugi::xml_node config = getNodeByName(doc, "configuration");
    if (config.type() == pugi::node_null) {
        throw std::invalid_argument(
            "XML file is not a compass config: Missing <configuration> tag"
        );
    }
    std::vector<pugi::xml_node> boards = getAllByName(config, "board");

    if (boards.size() == 0) {
        throw std::invalid_argument(
            "The compass configuration file has no board configurations."
        );
    }
    for (int i = 0; i < boards.size(); i++) {

        pugi::xml_node dppType =  getNodeByName(boards[i], "dppType");
	if(getStringContents(dppType)=="DPP_PSD")
	{
         s_boardParams.emplace(s_boardParams.end());
         PSDBoardParameters& board(s_boardParams.back());
         configureBoard(boards[i], board);
	}
    }
}
/**
 * configureBoard
 *    Does complete configuration of one board.  This means:
 *    - Pulling the board description from the tags that immediately
 *      follow the <board> tag and loading them into the board level
 *      configuration.
 *    - Loading default per-channel defaults into the channel configurations
 *      for the board from the <parameters> tags in the DOM>
 *    - Loading board level configuration parameters from the <parameters>
 *      tags.
 *    - Loading per channel overrides to dedfault configurations from the
 *      <channel> tags in the DOM.
 */
void PSDParameters::configureBoard(
    pugi::xml_node& boardNode, PSDBoardParameters& board
)
{
    configureBoardGlobalParameters(boardNode, board);
   
   // Get the <parameters> tag the default channel params are under that:
   
    pugi::xml_node paramsNode =
        getNodeByNameOrThrow(
            boardNode, "parameters", "Missing <parameters> node - no channel defaults"
        );
    setChannelDefaults(paramsNode, board);
    
    // Iterate over the channel tags and configure each channel's default
    // overrides.
    
    std::vector<pugi::xml_node> channelNodes =
        getAllByName(boardNode, "channel");
    for (int i =0; i < channelNodes.size(); i++) {
        configureChannel(channelNodes[i], &board);    
    }
}
/**
 * configureBoardGlobalParameters
 *    Configure the board from the individual parameters under the
 *    <board> tag.
 *
 *  @param boardNode - the <board> xml node in the DOM.
 *  @param board     - References the board to configure.
 *  @note this is really the board information not not the configuration.
 */
void
PSDParameters::configureBoardGlobalParameters(
    pugi::xml_node& boardNode, PSDBoardParameters& board
)
{
    // <modelName>
    
    pugi::xml_node model =
        getNodeByNameOrThrow(boardNode, "modelName", "Missing <modelName>");
    board.s_modelName = getStringContents(model);
    
    // <serialNumber>
    
    pugi::xml_node serNum =
        getNodeByNameOrThrow(boardNode, "serialNumber", "Missing <serialNumber>");
    board.s_serialNumber = getUnsignedContents(serNum);
    
    // <connectionType>
    
    pugi::xml_node conType =
        getNodeByNameOrThrow(boardNode, "connectionType", "Missing <connectionType>");
    setLinkType(getStringContents(conType), board);
    
    // <linkNum> tgag
    
    pugi::xml_node linkNum =
        getNodeByNameOrThrow(boardNode, "linkNum", "Missing <linkNum> tag");
    board.s_linkNum = getUnsignedContents(linkNum);
    
    // <conetNode>
    
    pugi::xml_node nodeNum =
        getNodeByNameOrThrow(boardNode, "conetNode", "Missing <conetNode> tag");
    board.s_node = getUnsignedContents(nodeNum);
    
    // <address> tag.
    
    pugi::xml_node base =
        getNodeByNameOrThrow(boardNode, "address", "Missing <address> tag");
    board.s_base = getUnsignedContents(base);
    
    // <sampleTime> - picoseconds per channel
    
    pugi::xml_node sampling =
        getNodeByNameOrThrow(boardNode, "sampleTime", "Missing <sampleTime> tag");
    board.s_psPerSample = getUnsignedContents(sampling);   
}
/**
 * setChannelDefaults
 *    Iterate over the <entries> in the <parameters> tag.
 *    Many of these items are channel defaults, however others
 *    are board settings.
 * @param paramsNode - the <parameters> node DOM Object.
 * @param board      - The board object we're configuring.
 */
void
PSDParameters::setChannelDefaults(
    pugi::xml_node& parameters, PSDBoardParameters& board
)
{
    std::vector<pugi::xml_node> entries = getAllByName(parameters, "entry");
    for (int i = 0; i < entries.size(); i++) {
        setParameterValue(entries[i], board, -1);  // -1 means set all chans.
    }
}
/**
 *  configureChannel
 *    Called in a loop iterating over all <channel> tags.
 *     - Pick out the <index> for the channel number.
 *     - Pick out the <values> tag as the top of the
 *       DOM that has <entry> tags.
 *     - Use setParameterValues to set the configuration of the
 *       specified channel.
 *  @param chanNode - the <channel> tag node.
 *  @param board    - Reference to the board we're configuring.
 */
void
PSDParameters::configureChannel(
    pugi::xml_node& chanNode, PSDBoardParameters* board
)
{
    pugi::xml_node chanNumNode =
        getNodeByNameOrThrow(chanNode, "index",  "Missing <index> tag in <channel>");
    unsigned chanNum = getUnsignedContents(chanNumNode);

    // Now iterate over the <entry> tags for that node:
    
    std::vector<pugi::xml_node> entries = getAllByName2(chanNode, "entry","values");	//Edit by B.Sudarsan, July 2020

    for (int i =0; i < entries.size(); i++) {
        setParameterValue(entries[i], *board, chanNum);
    }
}
/**
 * setParameterValue
 *    Given an <entry> xml node, pull out the key and the value and
 *    set either the appropriate board or channel parameter.
 *
 *  @param entry - <entry> tag node.
 *  @param board - Board we're configuring.
 *  @param chan  - Number of the channel we're configuring -- -1 means
 *                 we're setting a channel default.
 */
void
PSDParameters::setParameterValue(
    pugi::xml_node& entry, PSDBoardParameters& board, int chan
)
{

    

    pugi::xml_node keyNode = getNodeByNameOrThrow(entry, "key", "Missing <key> tag in <entry>");
    pugi::xml_node valNode = getNodeByNameOrThrow(entry, "value", "Missing <value> tag in <entry>");
    
    if(chan != -1)
	    valNode = entry;


    std::string key = getStringContents(keyNode);
    // What we actually do depends on the key tag contents, the parameter name.
    
    if (key == "SRV_PARAM_DT_EXT_CLOCK") {
        board.s_extClock = getBoolContents(valNode);
    } else if (key == "SRV_PARAM_CH_TIME_OFFSET") {
        double timeOffset = getDoubleValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_channelTimeOffset = timeOffset;
            }
        } else {
            board.s_channelConfig[chan].s_channelTimeOffset = timeOffset;
        }
    } else if (key == "SRV_PARAM_ENERGY") {
        board.s_energy = getBoolValue(valNode);
    } else if (key == "SRV_PARAM_CH_POLARITY") {
        std::string polarityString = getValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                setPolarity(board.s_channelConfig[i], polarityString);
            }
        } else {
            setPolarity(board.s_channelConfig[chan], polarityString);
        }
    } else if (key == "SRV_PARAM_COINC_MODE") {
        std::string cString = getValue(valNode);
        setCoincidenceMode(board, cString);
    } else if (key == "SRV_PARAM_CH_THRESHOLD") {
        double th = getDoubleValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_threshold = th;
            }
        } else {
            board.s_channelConfig[chan].s_threshold = th;
        }
    } else if (key == "SRV_PARAM_CH_BLINE_NSMEAN") {
        std::string nsMeanStr = getValue(valNode);
        if (chan == -1) {
	  for (int i =0; i < 16; i++) {
	    setChanNsMean(board.s_channelConfig[i], nsMeanStr);
	  }
	} else {
	  setChanNsMean(board.s_channelConfig[chan], nsMeanStr);

	}
    } else if (key == "SRV_PARAM_CH_CFD_DELAY") {
        if (chan == -1) {
	    for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_cfdDelay = getDoubleValue(valNode);
            }
        } else {
            board.s_channelConfig[chan].s_cfdDelay = getDoubleValue(valNode);
        }
    } else if (key =="SRV_PARAM_START_MODE") {
        setStartMode(board, getValue(valNode));
    } else if (key == "SRV_PARAM_CH_ENABLED") {
        bool enabled = getBoolValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_enabled = enabled;
            }
        } else {
            board.s_channelConfig[chan].s_enabled = enabled;
        }
    } else if (key == "SRV_PARAM_CH_CFD_SMOOTHEXP") {
        std::string smooth = getValue(valNode);
        if (chan == -1) {
           for (int i = 0; i < 16; i++) {
              setChannelCFDSmoothing(board.s_channelConfig[i], smooth);
	   }
        } else {
            setChannelCFDSmoothing(board.s_channelConfig[chan], smooth);
        }
    } else if (key == "SRV_PARAM_CH_PURGAP") {
        double gap = getDoubleValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_purGap = gap;
            }
        } else {
            board.s_channelConfig[chan].s_purGap = gap;
        }
    } else if (key == "SRV_PARAM_COINC_TRGOUT") {
        board.s_coincidenceTriggerOut = getDoubleValue(valNode);
    } else if (key == "SRV_PARAM_CH_INDYN") {
        std::string dynRange = getValue(valNode);
	

        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                setChannelDynamicRange(board.s_channelConfig[i], dynRange);
            }
        } else {
            setChannelDynamicRange(board.s_channelConfig[chan], dynRange);
        }
    } else if (key == "SRV_PARAM_TIMETAG") {
        board.s_timeTag = getBoolValue(valNode);
    } else if (key == "SRV_PARAM_ADCCALIB_ONSTART_ENABLE") {
        board.s_calibrateBeforeStart = getBoolValue(valNode);
    } else if (key == "SRV_PARAM_CH_GATESHORT") {
        double shortGate = getDoubleValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_shortGate = shortGate;
            }
        } else {
            board.s_channelConfig[chan].s_shortGate = shortGate;
        }
    } else if (key == "SRV_PARAM_CH_CFD_FRACTION") {
        std::string fraction = getValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                setCfdFraction(board.s_channelConfig[i], fraction);
            }
        } else {
            setCfdFraction(board.s_channelConfig[chan], fraction);
        }
    } else if (key == "SRV_PARAM_CH_DISCR_MODE") {
        std::string discMode = getValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                setDiscriminatorMode(board.s_channelConfig[i], discMode);
            }
        } else {
            setDiscriminatorMode(board.s_channelConfig[chan], discMode);
        }
    }  else if (key == "SRV_PARAM_CH_BLINE_FIXED") {
        double bline = getDoubleValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_fixedBline = bline;
            }
        } else {
            board.s_channelConfig[chan].s_fixedBline = bline;
        }
    } else if (key == "SRV_PARAM_SW_TRG_AT_START") {
        board.s_softwareTriggerAtStart = getBoolValue(valNode);
    } else if (key == "SRV_PARAM_CH_TRG_HOLDOFF") {
        double holdoff = getDoubleValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_triggerHoldoff = holdoff;
            }
        } else {
            board.s_channelConfig[chan].s_triggerHoldoff = holdoff;
        }
    } else if (key == "SRV_PARAM_CH_GATE") {
        double gate = getDoubleValue(valNode);
        if (chan == -1) {
            for (int i = 0; i < 16; i++) {
                board.s_channelConfig[i].s_gateLen = gate;
            }
        } else {
            board.s_channelConfig[chan].s_gateLen = gate;
        }
    } else if (key == "SRV_PARAM_EXTRAS") {
         board.s_includeExtras = getBoolValue(valNode);
    } else if (key == "SRV_PARAM_RECLEN") {
        board.s_recordLength = getDoubleValue(valNode);
    } else if (key == "SRV_PARAM_START_DELAY") {
        board.s_startDelay = getDoubleValue(valNode);
    } else if (key == "SRV_PARAM_WAVEFORMS") {
        board.s_waveforms = getBoolValue(valNode);
    } else if (key == "SRV_PARAM_CH_BLINE_DCOFFSET" ) {
        double dcOffset = getDoubleValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_dcOffset = dcOffset;
            }
        } else {
            board.s_channelConfig[chan].s_dcOffset = dcOffset;
        }
    } else if (key == "SRV_PARAM_IOLEVEL") {
        std::string sigLevels = getValue(valNode);
        setIoLevel(board, sigLevels);
    } else if (key == "SRV_PARAM_CH_ENERGY_COARSE_GAIN") {
        std::string coarseGain = getValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                setChannelCoarseGain(board.s_channelConfig[i], coarseGain);
            }
        } else {
            setChannelCoarseGain(board.s_channelConfig[chan], coarseGain);
        }
    } else if (key == "SRV_PARAM_TRGOUT_MODE") {
        std::string mode = getValue(valNode);
        setTriggerOutMode(board,  mode);
    } else if (key == "SRV_PARAM_CH_GATEPRE") {
        double preTrigger = getDoubleValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_gatePre = preTrigger;
            }
        } else {
            board.s_channelConfig[chan].s_gatePre = preTrigger;
        }
    } else if (key == "SRV_PARAM_EVENTAGGR") {
        board.s_eventAggregation = getDoubleValue(valNode);
    } else if (key == "SRV_PARAM_CH_PRETRG") {
        double pre = getDoubleValue(valNode);
        if (chan == -1) {
            for (int i =0; i < 16; i++) {
                board.s_channelConfig[i].s_preTrigger = pre;
            }
        } else {
            board.s_channelConfig[chan].s_preTrigger = pre;
        }
    } else {
        // Ignore all keys other than the ones above.
    }

}
/**
 * setLinkType
 *   Given link types of "USB", "CONET" or "OPTICAL"
 *   set a boards linktype to the appropriate value.
 *   Note that CONET and OPTICAl have the same meaning.
 *
 * @param value - the text string.
 * @param board - Board configuration.
 * @throw std::string - if the string isn't valid.
 */
void
PSDParameters::setLinkType(
   const std::string& value, PSDBoardParameters& board
)
{
    if (value == "USB") {
        board.s_linkType = PSDBoardParameters::usb;
    } else if ((value == "CONET") || (value == "OPTICAL")) {
        board.s_linkType = PSDBoardParameters::conet;
    } else {
        std::string msg("Unrecognized link type string: ");
        msg += value;
        throw msg;
    }
}
/**
 * setPolarity
 *    Sets the polarity of a channel based on the value of the
 *    XML configuration file string.
 * @param chanParams - references a single channel parameter struct.
 * @param polString  - One of the valid polarity strings.
 * @throw std::string  - if an unrecotnzied polarity string is passed.
 */
void
PSDParameters::setPolarity(
    PSDChannelParameters& chanParams, const std::string& polString
)
{
    if (polString == "POLARITY_POSITIVE") {
        chanParams.s_polarity = PSDChannelParameters::positive;
    } else if (polString == "POLARITY_NEGATIVE") {
        chanParams.s_polarity = PSDChannelParameters::negative;
    } else {
        std::string msg("Unrecognized channel polarity value: ");
        msg += polString;
        throw polString;
    }
}
/**
 * setCoincMode
 *    Sets the board's coincidence moade configuration based on the
 *    value from the COMPASS XML file.
 *
 *  @param board   - the board configuration struct.
 *  @param coincString -Coincidence mode string from the XML file.
 *  @throw std::string - if the mode string is not recognized.
 */
void
PSDParameters::setCoincidenceMode(
    PSDBoardParameters& board, const std::string& coincString
)
{
    if (coincString == "COINC_MODE_DISABLED") {
        board.s_coincidenceMode= PSDBoardParameters::disabled;
    } else if (coincString == "COINC_MODE_AND_COUPLES") {
        board.s_coincidenceMode = PSDBoardParameters::couples;
    } else if (coincString == "COINC_MODE_AND_ONETOALL") {
        board.s_coincidenceMode = PSDBoardParameters::andOneToAll;
    } else if (coincString == "COINC_MODE_ONECHANNEL_VETO") {
        board.s_coincidenceMode = PSDBoardParameters::oneChannelVeto;
    } else if (coincString == "COINC_MODE_EXT_GATE") {
        board.s_coincidenceMode = PSDBoardParameters::ExtTrgGate;
    } else if (coincString == "COINC_MODE_EXT_VETO") {
        board.s_coincidenceMode = PSDBoardParameters::ExtTrgVeto;
    } else {
        std::string msg("Unrecognized board coincidence mode: ");
        msg += coincString;
        throw msg;
    }
}
/**
 * setChannelNsMean
 *    Sets the number of samples used to average the baseline.
 *
 * @param chan   - channel parameters.
 * @param nsampMeanString - The selection string from the XML file.
 * @throw std::string - if the nsampMeanString is not recognized.
 */
void
PSDParameters::setChanNsMean(
    PSDChannelParameters& chan, const std::string& nsampMeanString
)
{
    if (nsampMeanString == "BLINE_NSMEAN_FIXED") {
        chan.s_blineNsMean = PSDChannelParameters::fixed;
    } else if (nsampMeanString == "BLINE_NSMEAN_16") {
        chan.s_blineNsMean = PSDChannelParameters::samples16;
    } else if (nsampMeanString == "BLINE_NSMEAN_64") {
        chan.s_blineNsMean = PSDChannelParameters::samples64;
    } else if (nsampMeanString == "BLINE_NSMEAN_256") {
        chan.s_blineNsMean = PSDChannelParameters::samples256;
    } else if (nsampMeanString == "BLINE_NSMEAN_1024") {
        chan.s_blineNsMean = PSDChannelParameters::samples1024;
    } else if (nsampMeanString == "BLINE_NSMEAN_4096") {
        chan.s_blineNsMean = PSDChannelParameters::samples4096;
    } else if (nsampMeanString == "BLINE_NSMEAN_16384") {
        chan.s_blineNsMean = PSDChannelParameters::samples16384;
    } else {
        std::string msg("Unrecognized baseline averaging sample key: ");
        msg += nsampMeanString;
        throw msg;
    }
}
/**
 * setStartMode
 *    Sets a board's start mode from the string mode in the
 *    COMPASS XML
 *  @param board - references the board to set.
 *  @param startModeString - the start mode string.
 *  @throw std::string -if the mode is not recognized.
 */
void
PSDParameters::setStartMode(
    PSDBoardParameters& board, std::string startModeString
)
{
    if (startModeString == "START_MODE_SW") {
        board.s_startMode = PSDBoardParameters::software;    
    } else if (startModeString == "START_MODE_S_IN") {
        board.s_startMode = PSDBoardParameters::sIn;
    } else if (startModeString == "START_MODE_FIRST_TRG") {
      board.s_startMode = PSDBoardParameters::firstTrigger;
    } else {
        std::string msg = "Unrecognized start mode string: ";
        msg += startModeString;
        throw msg;
    }
}
/**
 * setChannelCFDSmoothing
 *    Sets the number of samples to be used to interpolate the CFD
 *    zero crossing.  This must convert to an integer
 *    in the range 1-8.(?).
 *
 * @param chanParams - references channel parameters to set s_cfdSmoothing.
 * @param smooth     -  the compass string spec.
 * @throw std::string - if the resulting value is not valid.
 */
void
PSDParameters::setChannelCFDSmoothing(
    PSDChannelParameters& chanParams, const std::string& smooth
)
{
    int value = -1;                           // Assume failure
    if (smooth == "CFD_SMOOTH_EXP_1") {
        value = 1;
    } else if (smooth == "CFD_SMOOTH_EXP_2") {
        value = 2;
    } else if (smooth == "CFD_SMOOTH_EXP_4") {
        value = 4;
    } else if (smooth == "CFD_SMOOTH_EXP_8") {
        value = 8;
    } else if (smooth == "CFD_SMOOTH_EXP_16") {
        value = 16;
    }
    
    if ((value <= 0) || (value > 16)) {
        std::string msg = "Invalid CFD smoothing string: ";
        msg += smooth;
        throw msg;
    }
    chanParams.s_cfdSmoothing = value;
}
/**
 * setChannelDynamicRange
 *    Sets a channel input range from the compass string.
 *
 * @param chanParams - reference to the channel whose s_dynamicRange
 *                     we're setting.
 * @param dynRange - The compass XML range string.
 * @throw std::string - if the string is not recognized.
 */
void
PSDParameters::setChannelDynamicRange(
    PSDChannelParameters& chanParams, const std::string& dynRange
)
{
    if (dynRange == "INDYN_2_0_VPP") {
        chanParams.s_dynamicRange = PSDChannelParameters::vpp2V;    // 2 V peak to peak.
    } else if (dynRange == "INDYN_0_5_VPP") {
        chanParams.s_dynamicRange = PSDChannelParameters::vpphalfV; // 0.5 V peak to peak.
    } else {
        std::string msg = "Invalid input channel dynamic range string: ";
        msg += dynRange;
        throw msg;
    }
}
/**
 * setCfdFraction
 *    Sets the CFD fraction from the enumerated string value in the
 *    COMPASS XML configuration file.
 *
 *  @param chanParams - reference to channel parameters.
 *  @param frac       - Fraction string.
 *  @throw std::string - If the fraction string is not recognized.
 */
void
PSDParameters::setCfdFraction(
    PSDChannelParameters& chanParams, const std::string& frac
)
{
    if (frac == "CFD_FRACTLIST_25") {
        chanParams.s_cfdFraction = PSDChannelParameters::frac25;
    } else if (frac == "CFD_FRACTLIST_50") {
        chanParams.s_cfdFraction = PSDChannelParameters::frac50;
    } else if (frac == "CFD_FRACTLIST_75") {
        chanParams.s_cfdFraction = PSDChannelParameters::frac75;
    } else if (frac == "CFD_FRACTLIST_100") {
        chanParams.s_cfdFraction = PSDChannelParameters::frac100;
    } else {
        std::string msg = "Unrecognized CFD fraction specification: ";
        msg += frac;
        throw msg;
    }
}
/**
 * setDiscriminatorMode
 *    Determines if the discriminator for a channel is LED or CFD
 *
 * @param chanParams - references a channel parameter struct.
 * @param mode       - Compass XML file string used to specify the mode.
 * @throw std::string - if the mode string is not recognized.
 */
void
PSDParameters::setDiscriminatorMode(
    PSDChannelParameters& chanParams, const std::string& mode
)
{
    if (mode == "DISCR_MODE_LED") {
        chanParams.s_discriminatorType = PSDChannelParameters::led;
    } else if (mode == "DISCR_MODE_CFD") {
        chanParams.s_discriminatorType = PSDChannelParameters::cfd;
    } else {
        std::string msg = "Unrecognized Discriminator mode string: ";
        msg += mode;
        throw msg;
    }
}
/**
 * setIoLevel
 *    Sets the front panel I/O signalling levels.
 *  @param board - references the board configuration struct.
 *  @param signalling - The string from the COMPASS XML selecting the
 *                front panel signalling mode.
 *  @throw std::string - If the signalling specifier is not recognized.
 */
void
PSDParameters::setIoLevel(
    PSDBoardParameters& board, const std::string& signalling
)
{
    if (signalling == "FPIOTYPE_NIM") {
        board.s_ioLevel = PSDBoardParameters::nim;
    } else if (signalling == "FPIOTYPE_TTL") {
        board.s_ioLevel = PSDBoardParameters::ttl;
    } else {
        std::string msg = "I/O signalling specification not recognized: ";
        msg += signalling;
        throw msg;
    }
}
/**
 * setChannelCoarseGain
 *    Sets the coarse gain for a channel.  This specifies the number of
 *    femto-coulombs per LSB value
 *
 *    @param chanParams - refereces the board parameters struct.
 *    @param gain       - the coarse gain selection string from the
 *                         COMPASS configuration file.
 *   @throw std::string - If gain is not a recognized value.
 */
void
PSDParameters::setChannelCoarseGain(
    PSDChannelParameters& chanParams, const std::string& gain
)
{
    if (gain == "CHARGESENS_2.5_FC_LSB_VPP") {
        chanParams.s_coarseGain = PSDChannelParameters::lsb2_5fc;
    } else if (gain == "CHARGESENS_10_FC_LSB_VPP") {
        chanParams.s_coarseGain = PSDChannelParameters::lsb10fc;
    } else if (gain == "CHARGESENS_40_FC_LSB_VPP") {
        chanParams.s_coarseGain = PSDChannelParameters::lsb40fc;
    } else if (gain == "CHARGESENS_160_FC_LSB_VPP") {
        chanParams.s_coarseGain = PSDChannelParameters::lsb160fc;
    } else if (gain == "CHARGESENS_640_FC_LSB_VPP") {
        chanParams.s_coarseGain = PSDChannelParameters::lsb640fc;
    } else if (gain == "CHARGESENS_2560_FC_LSB_VPP") {
        chanParams.s_coarseGain = PSDChannelParameters::lsb2560fc;
    } else {
        std::string msg = "Channel coarse gain selector not recognized: ";
        msg += gain;
        throw msg;
    }
}
/**
 * setTriggerOutMode
 *    Selects the signal that will be output on TRGO
 *
 *  @param board - references the board parameters.
 *  @param mode - COMPASS TRG0 output mode string.
 *  @throw std::string - if the mode string is not recognized.
 */
void
PSDParameters::setTriggerOutMode(
    PSDBoardParameters& board, const std::string& mode
)
{
    if (mode == "TRGOUT_MODE_LEVEL0") {
        board.s_triggerOutputMode = PSDBoardParameters::level0;
    } else if (mode == "TRGOUT_MODE_LEVEL1") {
        board.s_triggerOutputMode = PSDBoardParameters::level1;
    } else if (mode == "TRGOUT_MODE_SW_TRG") {
        board.s_triggerOutputMode = PSDBoardParameters::softwareTrigger;
    } else if (mode == "TRGOUT_MODE_EXT_TRG") {
        board.s_triggerOutputMode = PSDBoardParameters::externalTrigger;
    } else if (mode == "TRGOUT_MODE_GLOBAL_OR_TRG") {
        board.s_triggerOutputMode = PSDBoardParameters::globalOrTrigger;
    } else if (mode == "TRGOUT_MODE_RUN") {
        board.s_triggerOutputMode = PSDBoardParameters::running;
    } else if (mode == "TRGOUT_MODE_DELAYED_RUN") {
        board.s_triggerOutputMode = PSDBoardParameters::delayedRunning;
    } else if (mode == "TRGOUT_MODE_SAMPLE_CLK") {
        board.s_triggerOutputMode = PSDBoardParameters::sampleClock;
    } else if (mode == "TRGOUT_MODE_PLL_CLK") {
        board.s_triggerOutputMode = PSDBoardParameters::pllClock;
    } else if (mode =="TRGOUT_MODE_BUSY") {
        board.s_triggerOutputMode = PSDBoardParameters::busy;
    } else if (mode == "TRGOUT_MODE_PLL_UNLOCK") {
        board.s_triggerOutputMode = PSDBoardParameters::pllUnlocked;
    } else if (mode == "TRGOUT_MODE_VPROBE") {
        board.s_triggerOutputMode = PSDBoardParameters::vProbe;
    } else if (mode == "TRGOUT_MODE_SYNCIN") {
        board.s_triggerOutputMode = PSDBoardParameters::syncIn;
    } else {
        std::string msg = "Trigger output mode selector not recognized: ";
        msg += mode;
        throw msg;
    }
}
