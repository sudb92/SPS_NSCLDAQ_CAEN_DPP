/*
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file CompassProject.cpp
# @brief Implement the CompassProject class.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#include "CompassProject.h"
#include "pugiutils.h"
#include "CAENPhaChannelParameters.h"
#include "CAENPhaParameters.h"

#include <stdexcept>
#include <iostream>
#include <stdio.h>
/**
 * constructor
 *    Read in the document.  Failures result in an exception.
 * @param file -name of the file to read.
 */
CompassProject::CompassProject(const char*name)
{
   pugi::xml_parse_result res = m_doc.load_file(name);
   if (!res) {
        throw std::invalid_argument(res.description());
   }
  // m_channelDefaults.inputRiseTime = .256;   // This seems compass's hard coded ussec value
   
}
/**
 destructor
*/
CompassProject::~CompassProject()
{
  m_connections.clear();
  for(int i = 0; i < m_boards.size(); i++) {
    delete m_boards[i];
  }
  m_boards.clear();
}

/**
 * operator()
 *    Invoked to process a successfully parsed XML description file.
 */
void
CompassProject::operator()()
{
    pugi::xml_node config = getNodeByName(m_doc, "configuration");
    if (config.type() == pugi::node_null) {
        throw std::invalid_argument("XML File is not a Compass config:  does not have a <configuration> tag");
    }
    std::vector<pugi::xml_node> boards = getAllByName(config, "board");
    
    // If there are no board configurations, that's bad too:
    
    if (boards.size() == 0) {
        throw std::invalid_argument("The Compass configuration file has no boards configured");
    }
    
    // Process the per channel parameters of each board:
    
    

    for (int i = 0; i < boards.size(); i++) {

        pugi::xml_node dppType =  getNodeByName(boards[i], "dppType");
	if(getStringContents(dppType)=="DPP_PHA"){


        CAENPhaParameters* board = new CAENPhaParameters();     
	
        ConnectionParameters connection;
        processBoardParameters(boards[i], *board, connection);               // Sets board and defaults.
        

        board->m_channelParameters = parseBoardChannelConfig(boards[i]);
        
        m_boards.push_back(board);
        m_connections.push_back(connection);
	}
    }
    
}
/**
 * parseBoardChannelConfigs
 *    Procdess the <channel> tags in a board configuration.
 *    These are of the form:
 *       <channel>
 *          <index>channel-number</index>
 *          <values>
 *             <entry><key>parameter-name</ke><value>parameter-value</value></entry>
 *             ...
 *          </values
 *
 *   See processChannelEntry for information about the entries and what they
 *   mean.
 *   This level we:
 *   1.  Iterate over channels in the board.
 *   2.  Figure out the channel number for a channel.
 *   3.  Create a channnel object
 *   4.  Load it with values from the entries in the chanel.
 *
 *   When all is said and done, return the set of channel configurations.
 *
 * @param board  - the node for the <board> tag.
 * @return std::vector<std::pair<unsigned, CAENPhaChannelParameters*> > -
 *             the unsigned is the channel number (<index> tag value).
 *             the CAENPhaChannelParameters* points to the processed parameters.
 */
std::vector<CompassProject::ChannelInfo>
CompassProject::parseBoardChannelConfig(pugi::xml_node board)
{
    // get the channels:
    
    std::vector<pugi::xml_node> channels = getAllByName(board, "channel");
    std::vector<std::pair<unsigned, CAENPhaChannelParameters*> > result;
    try {
        for (int c = 0; c < channels.size(); c++) {
            // There must be an index tag and its value is the
            
	  pugi::xml_node chindex = getNodeByNameOrThrow(channels[c], "index", "Missing <index> tag in <channel>");
            unsigned channelNumber = getUnsignedContents(chindex);
            
            // must have a <values> tag:
            
            pugi::xml_node values = getNodeByNameOrThrow(channels[c], "values", "Missing <values> tag");
            CAENPhaChannelParameters* params =
                new CAENPhaChannelParameters(m_channelDefaults);    // OK since we don't ask the doc to be processed.
            
            
            // Push back now so failure will clean up:
            
            result.push_back(
	        std::pair<unsigned, CAENPhaChannelParameters*>(channelNumber, params));
            processChannelParams(values, params);
            
            // We have to do some computation since DPP expects usec and some Compass params are % of others:
            
            params->flattopDelay = params->trapFlatTop * params->flattopDelay/100.0;   // % -> time.
            params->digitalGain = 0;        // Code for gain of 1.
        }
        
        
    }
    catch(...) {
        // Dispose of dynamic storage and re-throw:
        
        for (int i = 0; i < result.size(); i++) {
            delete result[i].second;
        }
        throw;
    }
    return result;
}
/**
 * processChannelParams
 *    Load a CAENPhaChannelParameters struct from the <values> tag
 *    of the <channel>  this consists of a sequence of
 *    <entry><key>parameter-name</key><value>parameter-value</value></key>
 *
 *  @param values - the <values> node of the <channel> being processed.
 *  @param params - Pointer to the class that has the parameter values for this
 *                  channel.
 */
void
CompassProject::processChannelParams(
        pugi::xml_node values, CAENPhaChannelParameters* params
)
{
    std::vector<pugi::xml_node> entries = getAllByName(values, "entry");  // Entries.
    for (int i = 0; i < entries.size(); i++) {
        processChannelEntry(entries[i], params);
    }
}

/**
 * processChannelParams
 *    Process the parmaeters associated with each channel into a channel
 *    class.  The parameters are key/value pairs under an <entry> tag.
 *  TODO: Some of these are SRV_.... but shg9ould be SW_... indicating
 *        they're not hardware params but compass params.
 *  keys are:
 *     -  SRV_PARAM_CH_ENABLED - bool true if the channel is enabled.
 *     -  SRV_PARAM_CH_THRESHOLD - Channel trigger threshold mv???
 *     -  SRV_PARAM_CH_TRAP_TRISE - Trapezoid rise time nanoseconds
 *     -  SRV_PARAM_CH_ENERGYCUTENABLE - bool true if energy cut is turned on.
 *     -  SRV_PARAM_CH_PRETRG - nanoseconds(?)of pretrigger.
 *     -  SRV_PARAM_CH_ENERGYLOWCUT - Energy filter low level cutoff (units)?
 *     -  SRV_PARAM_CH_TRAP_TFLAT  - Trapezoid flattop tiem (ns).
 *     -  SRV_PARAM_CH_ENERGYHIGHCUT - Energy filter high level cutoff.
 *     -  SRV_PARAM_CH_POLARITY  - Channel input polarity either
 *                                * POLARITY_NEGATIVE
 *                                * POLARITY_POSITIVE
 *     -  SRV_PARAM_CH_TRG_HOLDOFF - Trigger holdoff (ns)
 *     -  SRV_PARAM_CH_TTF_SMOOTHING - RCCR2 smoothing.  One of:
 *                                * RCCR2_SMTH_2
 *                                * RCCR2_SMTH_4  - Look up legal values
 *                                * RCCR2_SMTH_8  - but just pull the number off
 *                                * RCCR2_SMTH_16 - the backend.
 *     - SRV_PARAM_CH_PSDLOWCUT - PSD filter low level cutoff (units?)
 *     - SRV_PARAM_CH_PSDHIGHCUT - PSD filter high level cuttof
 *     - SRV_PARAM_CH_TRAP_POLEZERO - Trapezoid filter pole zero decay time ns.
 *     - SRV_PARAM_CH_BLINE_NSMEAN - Baseline restoration - number of samples to average.
 *                                   form is BLINE_NSMEAN_nn where nn is our guy.
 *     - SRV_PARAM_CH_PSDCUTENABLE - boolean to turn on/off the PSD cut.
 *     - SRV_PARAM_CH_TTF_DELAY    - ttf delay ns(?)
 *     - SRV_PARAM_CH_BLINE_DCOFFSET -Baseline dc offset in %.
 *     - SRV_PARAM_CH_TRAP_PEAKING - Trapezoid peaking time % of TRAP_TFLAT
 *     - SRV_PARAM_CH_TIMECUTENABLE - boolean enable/disable time cut.
 *     - SRV_PARAM_CH_INDYN         - Input dynamic range
 *     - SRV_PARAM_CH_PEAK_NS_MEAN  - Peak smoothing # samples (PEAK_NSMEAN_nn samples)
 *     - SRV_PARAM_CH_TIMELOWCUT    - Time filter low cutoff.
 *     - SRV_PARAM_CH_TIMEHIGHCUT   - Time filter high cutoff.
 *     - SRV_PARAM_CH_PEAK_HOLDOFF  - Peak holdoff (ns).
 *     - SRV_PARAM_CH_SATURATION_REJECTION_ENABLE - filter out saturated events.
 *     - SRV_PARAM_CH_PUR_ENABLE    - Enable pile up rejection.
 *     - SRV_PARAM_CH_ENERGY_FINE_GAIN - Fine gain for the channel
 *
 *   @param entry - <entry> tag node.
 *   @param params - Pointer to the channel parameters.
 *(sorted below)
*	SRV_PARAM_CH_BLINE_DCOFFSET	Baseline	dc	offset	in	%.			
*	SRV_PARAM_CH_BLINE_NSMEAN	Baseline	restoration	number	of	samples	to	average.	
*	SRV_PARAM_CH_ENABLED	bool	TRUE	if	the	channel	is	enabled.	
*	SRV_PARAM_CH_ENERGY_FINE_GAIN	Fine	gain	for	the	channel			
*	SRV_PARAM_CH_ENERGYCUTENABLE	bool	TRUE	if	energy	cut	is	turned	on.
*	SRV_PARAM_CH_ENERGYHIGHCUT	Energy	filter	high	level	cutoff.			
*	SRV_PARAM_CH_ENERGYLOWCUT	Energy	filter	low	level	cutoff	(units)?		
*	SRV_PARAM_CH_INDYN	Input	dynamic	range					
*	SRV_PARAM_CH_PEAK_HOLDOFF	Peak	holdoff	(ns).					
*	SRV_PARAM_CH_PEAK_NS_MEAN	Peak	smoothing	#	samples	(PEAK_NSMEAN_nn	samples)		
	form	is	BLINE_NSMEAN_nn	where	nn	is	our	guy.	
*	SRV_PARAM_CH_POLARITY	Channel	input	polarity	either				
		POLARITY_NEGATIVE							
		POLARITY_POSITIVE							
*	SRV_PARAM_CH_PRETRG	nanoseconds(?)of	pretrigger.						
*	SRV_PARAM_CH_PSDCUTENABLE	boolean	to	turn	on/off	the	PSD	cut.	
*	SRV_PARAM_CH_PSDHIGHCUT	PSD	filter	high	level	cuttof			
*	SRV_PARAM_CH_PSDLOWCUT	PSD	filter	low	level	cutoff	(units?)		
*	SRV_PARAM_CH_PUR_ENABLE	Enable	pile	up	rejection.				
*	SRV_PARAM_CH_SATURATION_REJECTION_ENABLE	filter	out	saturated	events.				
*	SRV_PARAM_CH_THRESHOLD	Channel	trigger	threshold	mv???				
*	SRV_PARAM_CH_TIMECUTENABLE	boolean	enable/disable	time	cut.				
*	SRV_PARAM_CH_TIMEHIGHCUT	Time	filter	high	cutoff.				
*	SRV_PARAM_CH_TIMELOWCUT	Time	filter	low	cutoff.				
*	SRV_PARAM_CH_TRAP_PEAKING	Trapezoid	peaking	time	%	of	TRAP_TFLAT		
*	SRV_PARAM_CH_TRAP_POLEZERO	Trapezoid	filter	pole	zero	decay	time	ns.	
*	SRV_PARAM_CH_TRAP_TFLAT	Trapezoid	flattop	tiem	(ns).				
*	SRV_PARAM_CH_TRAP_TRISE	Trapezoid	rise	time	nanoseconds				
*	SRV_PARAM_CH_TRG_HOLDOFF	Trigger	holdoff	(ns)					
*	SRV_PARAM_CH_TTF_DELAY	ttf	delay	ns(?)					
*	SRV_PARAM_CH_TTF_SMOOTHING	RCCR2	smoothing.	One	of:				
	*	RCCR2_SMTH_2							
	*	RCCR2_SMTH_4	Look	up	legal	values			
	*	RCCR2_SMTH_8	but	just	pull	the	number	off	
	*	RCCR2_SMTH_16	the	backend.					

Pending:

SRV_PARAM_CH_FAKEEVT_TTROLL_EN
SRV_PARAM_CH_SELF_TRG_ENABLE
SRV_PARAM_CH_SPECTRUM_NBINS
SRV_PARAM_CH_TIME_OFFSET





 */
void
CompassProject::processChannelEntry(pugi::xml_node entry, CAENPhaChannelParameters* param)
{
  pugi::xml_node keytag = getNodeByNameOrThrow(entry, "key", "Missing key tag in a channel <entry> ");
    // pugi::xml_node val = getNodeByNameOrThrow(entry, "value");
    
    
    // Meaning of value depends on the key so :
    
    std::string key = getStringContents(keytag);
    if (key == "SRV_PARAM_CH_ENABLED") {
        bool enabled = getBoolValue(entry);
        param->enabled = enabled;
    } else if (key == "SRV_PARAM_CH_THRESHOLD") {
        param->threshold = getDoubleValue(entry);
    } else if (key == "SRV_PARAM_CH_TRAP_TRISE") {
        param->trapRiseTime = getDoubleValue(entry)/1000.0; // usec expected
    } else if (key == "SW_PARAM_CH_ENERGYCUTENABLE") {
        param->energySkim = getBoolValue(entry);    
    } else if (key == "SRV_PARAM_CH_PRETRG" ) {
        param->preTrigger = getDoubleValue(entry);
    } else if (key =="SW_PARAM_CH_ENERGYLOWCUT") {

    } else if (key == "SRV_PARAM_CH_TRAP_TFLAT") {
        param->trapFlatTop = getDoubleValue(entry)/1000.0; // Expecting usec
    } else if (key == "SW_PARAM_CH_ENERGYHIGHCUT") {
    } else if (key == "SRV_PARAM_CH_POLARITY") {
        param->polarity = (getValue(entry) == "POLARITY_NEGATIVE") ?
            CAENPhaChannelParameters::negative :
            CAENPhaChannelParameters::positive;
            
    } else if (key == "SRV_PARAM_CH_TRG_HOLDOFF") {
        param->triggerHoldoff = getDoubleValue(entry);// * 8 / 1000.0;	// Seems a missing factor of 8 somewhere?
    } else if (key == "SRV_PARAM_CH_TTF_SMOOTHING") {
        param->rccr2smoothing = convertRccr2Smoothing(
            getValue(entry)
        );
    } else if (key == "SRV_PARAM_CH_PSDLOWCUT") {
        param->psdLowCut = getDoubleValue(entry);
    } else if (key == "SRV_PARAM_CH_PSDHIGHCUT") {
        param->psdHighCut = getDoubleValue(entry);
    } else if (key == "SRV_PARAM_CH_TRAP_POLEZERO") {
      param->decayTime = getDoubleValue(entry) / 1000.0;   // Expect usec.
    } else if (key == "SRV_PARAM_CH_BLINE_NSMEAN") {
        param->BLMean = convertBaselineMeanCode(getValue(entry));
    } else if (key == "SRV_PARAM_CH_PSDCUTENABLE") {
        param->psdCutEnable = getBoolValue(entry);
    } else if (key == "SRV_PARAM_CH_TTF_DELAY") {
        param->inputRiseTime = getDoubleValue(entry);
    } else if (key == "SRV_PARAM_CH_BLINE_DCOFFSET") {
        param->dcOffset = convertDCOffset(getDoubleValue(entry));  
    } else if (key == "SRV_PARAM_CH_TRAP_PEAKING") {
      param->flattopDelay = getDoubleValue(entry);   // % of TRAP_TFLAT so have to figure out later.
    } else if (key == "SW_PARAM_CH_TIMECUTENABLE") { // Compass Software param.
      
    } else if (key == "SRV_PARAM_CH_INDYN") {
                                                    // Input dynamic range.
        param->range = getDynamicRange(getValue(entry));
    } else if (key == "SRV_PARAM_CH_PEAK_NS_MEAN") {
        param->peakMean = convertPeakMeanCode(
	    getValue(entry)
	);
    } else if (key == "SRV_PARAM_CH_TIMELOWCUT") {
                                                   // << can we suppor this?
    } else if (key == "SRV_PARAM_CH_TIMEHIGHCUT") {
                                                   // can we support this?
    } else if (key == "SRV_PARAM_CH_SATURATION_REJECTION_ENABLE") {
        param->otReject = getBoolValue(entry);
    } else if (key == "SRV_PARAM_CH_PUR_ENABLE") {
                                                   // Can we support pilelup reject?
    } else if (key =="SRV_PARAM_CH_PEAK_HOLDOFF") {
        param->peakHoldoff = getDoubleValue(entry);// * 4 / 1000.0; // ???
    } else if (key == "SRV_PARAM_CH_ENERGY_FINE_GAIN") {
                                        // Default fine energy gain.
      param->fineGain = getDoubleValue(entry);

    } else if (key == "SRV_PARAM_CH_PEAK_NSMEAN") {
        param->peakMean = convertPeakMeanCode(getValue(entry));
      // Silently ignored parameters:
      }
    else if (key == "SW_PARAMETER_CH_LABEL"){
	;
    }


    //Extra parameters 
    else if (key == "SRV_PARAM_CH_FAKEEVT_TTROLL_EN"){
		param->fakeevt_ttroll_en = getBoolValue(entry);
    }
    else if(key == "SRV_PARAM_CH_SELF_TRG_ENABLE"){
	;
    }
    else if(key=="SRV_PARAM_CH_SPECTRUM_NBINS"){
	;
    }
    else if(key=="SRV_PARAM_CH_TIME_OFFSET"){
	;
    }

    /*Useless Software params, get rid of them so there aren't errors thrown*/
    else if(key=="SW_PARAM_CH_SATURATION_REJECTION_ENABLE"){
	;
    }
    else if(key=="SW_PARAMETER_CH_ENERGY_CALIBRATION_P1"){
	;
    }
    else if(key=="SW_PARAMETER_CH_ENERGY_CALIBRATION_P2"){
	;
    }
    else if(key=="SW_PARAMETER_CH_ENERGYHIGHCUT"){
	;
    }
    else if(key=="SW_PARAMETER_CH_TIMEHIGHCUT"){
	;
    }
    else if(key=="SW_PARAMETER_CH_ENERGY_CALIBRATION_P0"){
	;
    }
    else if(key=="SW_PARAM_CH_PUR_ENABLE"){
	;
    }
    else if(key=="SW_PARAMETER_CH_TIMELOWCUT"){
	;
    }
    else if(key=="SW_PARAMETER_CH_ENERGYLOWCUT"){
	;
    }
    else if(key=="SW_PARAM_CH_SATURATION_REJECTION_ENABLE"){
	;
    }
    else if(key=="SW_PARAMETER_CH_ENERGY_CALIBRATION_UDM"){
	;
    }
    else if(key=="SW_PARAMETER_CH_TIMECUTENABLE"){
	;
    }
    else if(key=="SW_PARAMETER_CH_ENERGYCUTENABLE"){
	;
    }


    else {
        std::cerr << "Unrecognized channel  parameter keyword in compass config file: "
		  << key << "  ignored\n";
    }

}

/**
 * processBoardParameters
 *   Parse and extract all of the board level parameters from the <board>
 *   children.  These are also <entry><key>name</key><value>value</value></entry>
 *   inside a <parameters> tag.
 *   They also may/do have <descriptor> tags which see like they are used
 *   by the Compass UI to figure out how to display/prompt for those params.
 *
 *   Each key, for reasons unclear to me has a value tag that's nested inside another
 *   <value> tag.
 * @param[in] pugi::xml_node - A <board> node.
 * @param[out] board - a reference to the board whose parameters we're figuring out.
 * @param[out] connection - how the board connects to the computer
 */
void
CompassProject::processBoardParameters(
    pugi::xml_node entry, CAENPhaParameters& board,
    ConnectionParameters& connection
)
{
  m_channelDefaults.baselineHoldoff = 2.0;       // This is obsolete since 11/22/2016
  m_channelDefaults.decimation      =   0;       // No compass config?
  m_channelDefaults.trapGain        =   3;       // ENERGY_COARSE_GAIN defined but not set.
  m_channelDefaults.triggerValidationWidth = 2;	// Again I don't see a COMPASS Parameter for this.
  m_channelDefaults.coincidenceWindow = false;
  m_channelDefaults.trResetEnabled    = false;
  m_channelDefaults.trGain            = 0;
  m_channelDefaults.trThreshold      = 30;
  m_channelDefaults.trHoldoff        = 1;
  m_channelDefaults.energySkim       = false;
  m_channelDefaults.lld = 0;
  m_channelDefaults.uld = 0;
  m_channelDefaults.baselineClip     = false;
  m_channelDefaults.fastTriggerCorrection = false;
  m_channelDefaults.baselineAdjust = 0;
  m_channelDefaults.digitalGain = 0;             // Gain code for 1 (decimation gain).
  m_channelDefaults.fineGain    = 1.0;           // Default fine gain.
  board.triggerSource= CAENPhaParameters::internal;

    // Load the connection parameters into connection.  Note that
    // if there's no base address, we load a zero...could be USB or CONET
    // connection -- only for VME adaptors does base address matter
    
    // <connectionType>
    
    pugi::xml_node connectionNode = getNodeByNameOrThrow(
      entry, "connectionType", "Missing required <connectionType> tag"
   ); 
   std::string connectionTypeString = getStringContents(connectionNode);
   connection.s_linkType = stringToLinkType(connectionTypeString);
    // <linkNum>
    
   pugi::xml_node linkNumNode = getNodeByNameOrThrow(
      entry,  "linkNum", "MIssing required <linkNum> tag"
   );
   connection.s_linkNum = getUnsignedContents(linkNumNode);
   
   // <address> (optional, defaults to zero)
   
   pugi::xml_node addressNode = getNodeByName(entry, "address");
   if (addressNode.type() == pugi::node_null) {
      connection.s_base = 0;
   } else {
      connection.s_base = getUnsignedContents(addressNode);
   }
   // Could be a connetNode -- if not, initialize that to zero.

   connection.s_node = 0;
   pugi::xml_node nodeNode = getNodeByName(entry, "conetNode");
   if (nodeNode.type() != pugi::node_null) {
     connection.s_node = getUnsignedContents(nodeNode);
   }
    
    // Locate the parametrs It's an error for there not to be one:
    
    pugi::xml_node params = getNodeByNameOrThrow(
        entry, "parameters",
        "Mandatory <parameters> tag missing for board"
    );
    // Now enumerate the <entry> tags:
    
    std::vector<pugi::xml_node> paramEntries =  getAllByName(params, "entry");
    
    // Process each parameter entry:
    

    //std::cout << "\nFoo!" <<  paramEntries.size();
    for (int i =0; i < paramEntries.size(); i++) {
        processABoardParameter(paramEntries[i], board);
    }
}

/**
 * processABoardParameter
 *     Given an <entry> tag process the <key>/<value> pair.
 *     The key contents are always  a text string while the
 *     <value> contents type depends on the key.
 *     Keys we know about:
 *
 *     SRV_PARAM_CH_TIME_OFFSET  - Time offset for zero in synch mode.
 *                                  double precision ns.
 *     SRV_PARAM_ENERGY          - Boolean I think we ignore this b/c it just
 *                                 tells Compass to appaly an energy calibration.
 *    SRV_PARAM_CH_POLARITY      - Default channel polarity  either
 *                                 POLARITY_POSITIVE or POLARITY_NEGATIVE
 *    SRV_PARAM_OUT_SELECTION    - Determinets what the LVDS outputs are:
 *                                   OUT_PROPAGATION_TRIGGER - propagates the trigger.
 *                                   OUT_PROPAGATION_TEST_0,
 *                                   OUT_PROPAGATION_TEST_1,
 *                                   OUT_PROPAGATION_ACQ_ON - True if data acquisition is on.
 *                                   OUT_PROPAGATION_SAMPLE_CLK - Output the sampling clock.
 *                                   OUT_PROPAGATION_PLL_CLK - Output is the PLL clock input.
 *                                   OUT_PROPAGATION_BUSY - output the digitizer busy.
 *                                   OUT_PROPAGATION_PLL_LOCK - State of the PLL Lock.
 *                                   OUT_PROPAGATION_VPROBE - Output a virtual probe.
 *                                   OUT_PROPAGATION_SYNCIN - Output the sync in pulse.
 *   SRV_PARAM_TRG_SW_ENABLE   - Boolean that is true if software triggers are allowed.
 *   SRV_PARAM_CH_TTF_DELAY    - Constant fraction default delay(?) ns
 *   SW_PARAMETER_DIFFERENCE_BINCOUNT - Compass - spectrum bin count.Z(?)?
 *   SW_PARAMETER_DISTRIBUTION_BINCOUNT - Compass spectrum bin count mapping(?)
 *   SRV_PARAM_CH_THRESHOLD   - Default trigger threshold for all channels.
 *   SW_PARAMETER_PSDBINCOUNT - Compass software not hardware.
 *   SW_PARAMETER_Y_BINCOUNT  - Compass software not hardware.
 *   SRV_PARAM_CH_BLINE_NSMEAN - Compass default baseline ns for mean.
 *   SRV_PARAM_CH_TRAP_TFLAT   - Default trapezoid flattop time.
 *   SW_PARAMETER_CH_TIMECUTENABLE - Default for enable time cut (Compass only).
 *   SRV_PARAM_START_MODE  - How acquisition starts:
 *                            START_MODE_SW Software start.
 *                            START_MODE_SIN - SIN after software start.
 *                            START_MODE_FIRST_TRG  First trigger after sw start.
 *                            
 *   SRV_PARAM_TIMEBOMBDOWNCOUNTER Firmware timebomb down counter is not used.
 *   SRV_PARAM_CH_ENABLED  - Defaul channel enable. (boolean)
 *   SRV_PARAM_CH_ENERGY_FINE_GAIN Default energy fine gain.
 *   SW_PARAM_CH_SATURATION_REJECTION_ENABLE - Compass software only.
 *   SW_PARAMETER_CH_ENERGYLOWCUT Compass energy cut.
 *   SW_PARAMETER_CH_PSDHIGHCUT  Compass PSD high cut.
 *   SRV_PARAM_CH_INDYN - Input dynamic range.
 *                        INDYN_2_0_VPP
 *                        INDYN_0_5_VPP
 *   SRV_PARAM_TIMETAG - Time tag should/should not appear in event.
 *   SRV_PARAM_ADCCALIB_ONSTART_ENABLE - Do calibration on start.
 *   SRV_PARAM_CH_PEAK_NSMEAN Samples to compute peak mean.
 *   SRV_PARAM_CH_TRGOUT  By default channel contributes or not to trigger output.
 *   SW_PARAMETER_CH_ENERGY_CALIBRATION_P0 - Compass energy calibration parameter.
 *   SW_PARAMETER_ENERGYBINCOUNT Compass energy calibration parameter.
 *   SW_PARAMETER_CH_ENERGY_CALIBRATION_P2 Compass energy calibration parameter.
 *   SW_PARAMETER_CH_ENERGY_CALIBRATION_P1 Compass energy calibration parameter.
 *   SW_PARAMETER_CH_TIMEHIGHCUT Compass time cut high level.
 *   SRV_PARAM_CH_TRAP_POLEZERO - Trapezoid pole zero (don't see how to program this).
 *   SW_PARAMETER_X_BINCOUNT - Compass software parameter, ignore.
 *   SW_PARAMETER_CH_ENERGYCUTENABLE - Compass software energy cut enable.
 *   SRV_PARAM_CH_TRG_HOLDOFF - Default trigger holdoff.
 *   SW_PARAMETER_CH_PSDCUTENABLE- Compass software - enable PSD cut.
 *   SRV_PARAM_EXTRAS - Select event extra data (enable).
 *   SRV_PARAM_TRG_EXT_OUT_PROPAGATE Enable propagation of trigger to EXTOUT.
 *   SRV_PARAM_RECLENWave form capture length in ns.
 *   SRV_PARAM_START_DELAY Nanoseconds of start delay.  NOte the timestamp zeroes on
 *                     the start.
 *   
 *   SRV_PARAM_TRG_EXT_ENABLE  Enable external trigger.
 *   SW_PARAMETER_CH_PSDLOWCUT PSD Low cut for COMPASS software.
 *   SRV_PARAM_WAVEFORMS Put waveforms in event.
 *   SRV_PARAM_CH_PEAK_HOLDOFF Peak holdoff.
 *   SW_PARAMETER_CH_LABEL COmpass software label (name) for the channel.
 *   SRV_PARAM_CH_TRAP_TRISE Trapezoid rise time.
 *   SW_PARAMETER_CH_ENERGY_CALIBRATION_UDM Compass calibrated energy units.
 *   SW_PARAMETER_TIME_DIFFERENCE_CH_T0 Compass coincidence timing param(?)
 *   SRV_PARAM_CH_BLINE_DCOFFSET - DC OFfset in percent of full scale.
 *   SW_PARAMETER_TIME_DIFFERENCE_CH_T1 COmpass time parameter.
 *   SRV_PARAM_CH_OUT_PROPAGATE - propagate channel ??
 *   SRV_PARAM_IOLEVEL Front panel IO singal level:
 *                      FPIOTYPE_NIM, FPIOTYPE_TTL allowed values.
 *   SW_PARAMETER_CH_DC_OFFSET_CALIBRATION_P1 COmpass dc offset calibration p1.
 *   SRV_PARAM_TRGVAL_PROPAGATE  Trigger propagation boolean.  Propagate hw triggers
 *                   to front panel TRGOUT
 *   SW_PARAMETER_CH_DC_OFFSET_CALIBRATION_P0 - DC offset calibration parameter p0.
 *   SW_PARAMETER_CH_TIMELOWCUT - low cut on time (coincidence sw) Compass only.
 *   SRV_PARAM_CH_TTF_SMOOTHING -  RCCR2 smoothing factor, one of:
 *                             RCCR2_SMTH_1, RCCR2_SMTH_2, RCCR2_SMTH_4,
 *                             RCCR2_SMTH_8, RCCR2_SMTH_16, RCCR2_SMTH_32,
 *                             RCCR2_SMTH_64
 *   SW_PARAMETER_TIME_DISTRIBUTION_CH_T0 - COMPASS only time distribution. param T0
 *   SW_PARAMETER_TIME_DISTRIBUTION_CH_T1 - COMPASS only time distribution. param T1
 *   SW_PARAM_CH_PUR_ENABLE - Pile up rejection enable.
 *   SRV_PARAM_EVENTAGGR   - Event aggregation
 *   SRV_PARAM_CH_TRAP_PEAKING - Trapezoid peaking in % of TRAP_TFLAT
 *   SRV_PARAM_CH_PRETRG   - Pretrigger (samples?  ns?)
 *   SW_PARAMETER_CH_ENERGYHIGHCUT - energy high cut for the Compass gate.
 *   SRV_PARAM_TRG_SW_OUT_PROPAGATE - Propagate sw triggers to TRGOUT
 *   
 *
 *  @param param - an <entry> tag node.  We're interested in key/value nodes
 *                 within that node.
 *  @param board - Reference to a board parameter block.

Pending:

SRV_PARAM_ACQRUNNING
SRV_PARAM_COINC_MODE
SRV_PARAM_COINC_TRGOUT
SRV_PARAM_SW_TRG_AT_START
SRV_PARAM_TRGOUT_MODE



 */
void
CompassProject::processABoardParameter(pugi::xml_node param, CAENPhaParameters& board)
{


  pugi::xml_node keyNode = getNodeByNameOrThrow(
        param, "key", "Missing <key> tag in global parameters <entry>"
    );
    std::string key = getStringContents(keyNode);    // Name of parameter.
    param = getNodeByNameOrThrow(param, "value", "Missing outer <value> tag in board parameter");    // Value is nested in value.sheesh.
    
    // Figure out how to decode each parameter:
    //std::cerr << "\n" << key;    
    if (key == "SRV_PARAM_CH_TIME_OFFSET") {   // Time offset for sync (ignored now).
            
    } else if (key == "SRV_PARAM_ENERGY")   {  // Compass apply energy calibration(?)
        
    } else if (key == "SRV_PARAM_CH_POLARITY") { // Default channel polarity
        std::string polString = getValue(param);
        CAENPhaChannelParameters::Polarity pol;
        if (polString == "POLARITY_POSITIVE") {
            pol = CAENPhaChannelParameters::positive;
        } else if (polString == "POLARITY_NEGATIVE") {
            pol = CAENPhaChannelParameters::negative;
        } else {
            throw std::string("Invalid channel polarity string");
        }
        m_channelDefaults.polarity = pol; // Default polarity.
        
    } else if (key == "SRV_PARAM_OUT_SELECTION") {   // OUT/Sum output selection(?)
      board.ioctlmask = computeIoCtlMask(getValue(param)); //Deprecated
        
                                           // Can be: 
        /*                                   OUT_PROPAGATION_TRIGGER - propagates the trigger.
        *                                   OUT_PROPAGATION_TEST_0,
        *                                   OUT_PROPAGATION_TEST_1,
        *                                   OUT_PROPAGATION_ACQ_ON - True if data acquisition is on.
        *                                   OUT_PROPAGATION_SAMPLE_CLK - Output the sampling clock.
        *                                   OUT_PROPAGATION_PLL_CLK - Output is the PLL clock input.
        *                                   OUT_PROPAGATION_BUSY - output the digitizer busy.
        *                                   OUT_PROPAGATION_PLL_LOCK - State of the PLL Lock.
        *                                   OUT_PROPAGATION_VPROBE - Output a virtual probe.
        *                                   OUT_PROPAGATION_SYNCIN - Output the sync in pulse. */
    } else if (key == "SRV_PARAM_TRG_SW_ENABLE") {
      bool val = getBoolValue(param);               // For now ignored.
    
    } else if (key == "SRV_PARAM_CH_TTF_DELAY") {
      m_channelDefaults.inputRiseTime = static_cast<unsigned>(getDoubleValue(param));
    } else if (key == "SW_PARAMETER_DIFFERENCE_BINCOUNT") {
                                         // Compass software parameter ignored.
    } else if (key == "SW_PARAMETER_DIFFERENCE_BINCOUNT") {
                                         // Compass software parameter ignored.
    } else if (key == "SW_PARAMETER_DISTRIBUTION_BINCOUNT") {
                                        // COmpass software parameter ignored.
    } else if (key == "SRV_PARAM_CH_THRESHOLD") {  // default threshold value
        unsigned defaultThreshold = static_cast<unsigned>(getDoubleValue(param));
        m_channelDefaults.threshold = defaultThreshold;
    } else if (key == "SW_PARAMETER_PSDBINCOUNT") {
                                        // COMPASS Software parameter.
    } else if (key == "SW_PARAMETER_Y_BINCOUNT") {
                                        // Compass software parameter
    } else if (key == "SRV_PARAM_CH_BLINE_NSMEAN") {
                                        // Default samples in baseline mean.
        unsigned defaultBlMean = convertBaselineMeanCode(getValue(param));
        m_channelDefaults.BLMean = defaultBlMean;
    } else if (key == "SRV_PARAM_CH_TRAP_TFLAT") {
                                        // Default trapezoid flat top time.
      double defaultFlattopTime = getDoubleValue(param)/1000.0; // expecting usec.
        m_channelDefaults.trapFlatTop = defaultFlattopTime;
    } else if (key == "SW_PARAMETER_CH_TIMECUTENABLE") {
                                        // Compass software parameter.
    } else if (key == "SRV_PARAM_START_MODE") {
                                        // Digitizer start mode.
      board.s_startMode = getStartMode(getValue(param));
      if( getValue(param) == "START_MODE_FIRST_TRG")
	  board.triggerSource= CAENPhaParameters::internal;
                                         
    } else if (key == "SRV_PARAM_TIMEBOMBDOWNCOUNTER") {
                                        // Unused - timebomb counter(?)
    } else if (key == "SRV_PARAM_CH_ENABLED") { 
                                        // Default channel enable.
        bool defaultEnabled = getBoolValue(param);
        m_channelDefaults.enabled = defaultEnabled;
        
    } else if (key == "SRV_PARAM_CH_ENERGY_FINE_GAIN") {
                                        // Default fine energy gain.
        m_channelDefaults.fineGain = getDoubleValue(param);
    } else if (key == "SW_PARAM_CH_SATURATION_REJECTION_ENABLE") {
                                        // Compass software parameter.
    } else if (key == "SW_PARAMETER_CH_ENERGYLOWCUT") {
                                        // COmpass software parameter.
    } else if (key == "SW_PARAMETER_CH_PSDHIGHCUT") {
                                        // COmpass software parameter.
    } else if (key == "SRV_PARAM_CH_INDYN") {
                                        // Input dynamic range.
        unsigned defaultDynamicRange = getDynamicRange(getValue(param));
        m_channelDefaults.range = defaultDynamicRange;
        
    } else if (key == "SRV_PARAM_TIMETAG") {
                                        // timetag -> event ignored unconditionally on.
    } else if (key == "SRV_PARAM_ADCCALIB_ONSTART_ENABLE") {
                                        // calibrate on start - ignored on unconditionally.
    } else if (key == "SRV_PARAM_CH_PEAK_NSMEAN") {
        unsigned defaultNsPeakMean = convertPeakMeanCode(getValue(param));
        m_channelDefaults.peakMean = defaultNsPeakMean;
    } else if (key == "SRV_PARAM_CH_TRGOUT") {
                                        // Trigger output for hw channels (ignored).
        
    } else if (key == "SW_PARAMETER_CH_ENERGY_CALIBRATION_P0") {
                                        // COmpass energy calibration software param
    } else if (key == "SW_PARAMETER_ENERGYBINCOUNT") {
                                        // COmpass energy calibration sw param.
    } else if (key == "SW_PARAMETER_CH_ENERGY_CALIBRATION_P2") {
                                        // Compass energy calibration sw param.
    } else if (key == "SW_PARAMETER_CH_ENERGY_CALIBRATION_P1") {
                                        // Compass energy calibration sw param.
    } else if (key == "SW_PARAMETER_CH_TIMEHIGHCUT") {
                                        // Compass time sw param value.
    } else if (key == "SRV_PARAM_CH_TRAP_POLEZERO") {
        double defaultPolezeroNs = getDoubleValue(param);
        m_channelDefaults.decayTime = defaultPolezeroNs /1000.0; // Expecting usec
    } else if (key == "SW_PARAMETER_X_BINCOUNT") {
                                        // Compass sw parameter.
    } else if (key == "SW_PARAMETER_CH_ENERGYCUTENABLE") {
                                        // Compass sw parameter
    } else if (key == "SRV_PARAM_CH_TRG_HOLDOFF") {
                                        // Default trigger holdoff.
        double defaultTriggerHoldoff = getDoubleValue(param);// *8 / 1000.0; // expects usec. seems a missing x8 somewhere.
        m_channelDefaults.triggerHoldoff = defaultTriggerHoldoff;
        
    } else if (key == "SW_PARAMETER_CH_PSDCUTENABLE") {
                                        // Compass sw parameter.
    } else if (key == "SRV_PARAM_EXTRAS") {
//        m_channelDefaults.extras_enable = getBoolValue(param);                        // Enable extra recording (always true).  
    } else if (key == "SRV_PARAM_TRG_EXT_OUT_PROPAGATE") {
                                       // External trigger -> trgout (ignored for now)
    } else if (key == "SRV_PARAM_RECLEN") { 
                                    // Length of recorded waveform.
      board.recordLength = getDoubleValue(param);

    } else if (key == "SRV_PARAM_START_DELAY") {
      unsigned startDelay = getDoubleValue(param);  // start delay in ns
      board.startDelay = startDelay;
        
    } else if (key == "SRV_PARAM_TRG_EXT_ENABLE" ) {
        bool extTriggerEnable = getBoolValue(param);   // Compute trigger Control
    } else if (key == "SW_PARAMETER_CH_PSDLOWCUT") {
                                    // Compass software parameter.
    } else if (key == "SRV_PARAM_WAVEFORMS") {
        board.waveforms = getBoolValue(param);
    } else if (key == "SRV_PARAM_CH_PEAK_HOLDOFF") {
        unsigned defaultPeakHoldoff = getDoubleValue(param);// * 4 / 1000.0;  //??
        m_channelDefaults.peakHoldoff = defaultPeakHoldoff;
    } else if (key == "SW_PARAMETER_CH_LABEL") {
                                        // Compass software parameter name.
    } else if (key == "SRV_PARAM_CH_TRAP_TRISE") {
        double defaultTrapRiseTime = getDoubleValue(param)/1000.0; // usec expected.
        m_channelDefaults.trapRiseTime = defaultTrapRiseTime;
    } else if (key == "SW_PARAMETER_CH_ENERGY_CALIBRATION_UDM") {
                                        // COmpass Sw e calibration param.
    } else if (key == "SW_PARAMETER_TIME_DIFFERENCE_CH_T0") {
                                        // COmpass sw parameter.
    } else if (key == "SRV_PARAM_CH_BLINE_DCOFFSET") {
                                // Default channel dc offset (baselinhe offset).
        unsigned defaultDCOffset = convertDCOffset(getDoubleValue(param));
        m_channelDefaults.dcOffset = defaultDCOffset;
    } else if (key == "SW_PARAMETER_TIME_DIFFERENCE_CH_T1") {
                                // COmpass softweare parameter.
    } else if (key == "SRV_PARAM_CH_OUT_PROPAGATE") {
                                // ?? ignored for now.
    } else if (key == "SRV_PARAM_IOLEVEL") {
	if(getValue(param)=="FPIOTYPE_NIM")
		board.IOLevel = 0;
	else
		board.IOLevel = 1;
      // Unused - hard coded to NIM now IIR.
    } else if (key == "SW_PARAMETER_CH_DC_OFFSET_CALIBRATION_P1") {
                                // Compass param for calibrating DC offsets.
    } else if (key == "SRV_PARAM_TRGVAL_PROPAGATE") {
        bool propagateTrigger  = getBoolValue(param);   // Triggers -> TRGO
    } else if (key == "SW_PARAMETER_CH_DC_OFFSET_CALIBRATION_P0") {
                                // Compass param for calibrating DC Offsets.
    } else if (key == "SW_PARAMETER_CH_TIMELOWCUT") {
                                // COmpass software time cut parameter.
    } else if (key == "SRV_PARAM_CH_TTF_SMOOTHING") {
        int defaultRCCR2Smoothing = convertRccr2Smoothing(getValue(param));
        m_channelDefaults.rccr2smoothing = defaultRCCR2Smoothing;
        
    } else if (key == "SW_PARAMETER_TIME_DISTRIBUTION_CH_T0") {
                                // COmpass time distrib calibration param 
    } else if (key == "SW_PARAMETER_TIME_DISTRIBUTION_CH_T1") {
                                // Compass tgime distrib calibration param.
    } else if (key == "SW_PARAM_CH_PUR_ENABLE") {     // Need to add support here.
        m_channelDefaults.defaultPUREnable = getBoolValue(param);  // enable pileup rejection
        
    } else if (key == "SRV_PARAM_EVENTAGGR") {
        unsigned eventAggregation =                  // need to add support here.
            static_cast<unsigned>(getDoubleValue(param));
    } else if (key == "SRV_PARAM_CH_TRAP_PEAKING") {  // Trapezoid peaking-- flat top delay
      double defaultTrapPeaking = getDoubleValue(param); // Expected in used
        m_channelDefaults.flattopDelay = defaultTrapPeaking;
        
    } else if (key == "SRV_PARAM_CH_PRETRG") {
        //unsigned defaultPretrig = nsToSamples(getDoubleValue(param));
        m_channelDefaults.preTrigger = getDoubleValue(param);
    } else if (key == "SW_PARAMETER_CH_ENERGYHIGHCUT" ) {
                                    // Compass software parameter for e cut.
    } else if (key == "SRV_PARAM_TRG_SW_OUT_PROPAGATE") {
        bool propagateSwTrigger = getBoolValue(param);   // show sw triggers at TRGO
    } else if (key == "SRV_PARAM_ACQRUNNING") {
    } 
//SRV_PARAM_ACQRUNNING
//SRV_PARAM_COINC_MODE
//SRV_PARAM_COINC_TRGOUT
//SRV_PARAM_SW_TRG_AT_START
//SRV_PARAM_TRGOUT_MODE
//New parameters
    else if (key == "SRV_PARAM_COINC_MODE"){
        std::string coincString = getValue(param);
	//std::cout << key << " " << coincString;
	if(coincString == "COINC_MODE_EXT_GATE")
	{
		board.OnboardCoinc = CAENPhaParameters::TrgInGated;
 	        board.isExtTrgEnabled = true;
 	        board.isExtVetoEnabled = false;
		std::cout << "\nCoinc. mode : Trg-in Gate" << std::flush;
	}
	else if(coincString == "COINC_MODE_EXT_VETO")
	{
		board.OnboardCoinc = CAENPhaParameters::TrgInVeto;
 	        board.isExtVetoEnabled = true;
 	        board.isExtTrgEnabled = false;
		std::cout << "\nCoinc. mode : Trg-in Veto" << std::flush;
	}
	else 
	{
		board.OnboardCoinc = CAENPhaParameters::None;
 	        board.isExtVetoEnabled = false;
 	        board.isExtTrgEnabled = false;
		std::cout << "\nCoinc. mode : None" << std::flush;		
	}

    }
    else if (key == "SRV_PARAM_COINC_TRGOUT"){
	board.shapTrgWidth = getDoubleValue(param);
	std::cout << "\nShaped Trigger Width : "  << board.shapTrgWidth;
    }
    else if (key == "SRV_PARAM_SW_TRG_AT_START"){
    }
    else if (key == "SRV_PARAM_TRGOUT_MODE"){
	processTrgOutMode(board, getValue(param)); 

    }
    else if (key == "SW_PARAMETER_CH_LABEL"){
	;
    }


    //Extra parameters 
    else if (key == "SRV_PARAM_CH_FAKEEVT_TTROLL_EN"){
		;
    }
    else if(key == "SRV_PARAM_CH_SELF_TRG_ENABLE"){
	;
    }
    else if(key=="SRV_PARAM_CH_SPECTRUM_NBINS"){
	;
    }
    else if(key=="SRV_PARAM_CH_TIME_OFFSET"){
	;
    }






     else {
        std::cerr << "Unrecognized key tag: " << key << " ignored in board/default param processing"<<std::endl;
    }

 
}

/*--------------------------------------------------------------------------
 *  Data conversion convenience methods:
 */

/**
 * convertRccr2Smoothing
 *    Convert a string of the form RCCR2_SMTH_nn into nn.
 *
 * @param code - the string code for the smoothing factor.
 * @return int - The nn from the string.
 */
int
CompassProject::convertRccr2Smoothing(const std::string& code)
{
   if (code == "RCCR2_SMTH_1") return 0; 
   if (code == "RCCR2_SMTH_2") return 1;
   if (code == "RCCR2_SMTH_4") return 2;
   if (code == "RCCR2_SMTH_8") return 4;
   if (code == "RCCR2_SMTH_16") return 8;
   if (code == "RCCR2_SMTH_32") return 0x10;
   if (code == "RCCR2_SMTH_64") return 0x20;
   if (code == "RCCR2_SMTH_128") return 0x3f;  // not yet defined but it's a value.
   
   std::string msg = "Unrecognized RCCR2 smoothing value: ";
   msg += code;
   throw msg;
}
/**
 * convertBaselineMeanCode
 *    Convert a string of the form BLINE_NSMEAN_nn to nn
 *
 *  @param code - String of the fomr BLINE_NSMEAN_nn
 *  @return unsigned the nn of the code.
 */
unsigned
CompassProject::convertBaselineMeanCode(const std::string& code)
{
  if (code == "BLINE_NSMEAN_NONE") return 0;
  if (code == "BLINE_NSMEAN_FIXED") return 0;
  if (code == "BLINE_NSMEAN_16")    return 1;
  if (code == "BLINE_NSMEAN_64")    return 2;
  if (code == "BLINE_NSMEAN_256")   return 3;
  if (code == "BLINE_NSMEAN_1024")  return 4;
  if (code == "BLINE_NSMEAN_4096")  return 5;
  if (code == "BLINE_NSMEAN_16384") return 6;

  throw std::string("Invalid baseline mean code");
}
/**
 * convertPeakMeanCode
 *    Convert a string of the form PEAK_NSMEAN_nn
 *    to nn.
 *
 *    @param code - the string to convert.
 *    @return unsigned - Resulting integer value.
 */
unsigned
CompassProject::convertPeakMeanCode(const std::string& code)
{
  if (code == "PEAK_NSMEAN_1") return 0;
  if (code == "PEAK_NSMEAN_4") return 1;
  if (code == "PEAK_NSMEAN_16") return 2;
  if (code == "PEAK_NSMEAN_64") return 3;

  throw std::string("Invalide code for SRV_PARAM_CH_PEAK_NSMEAN");
}
/**
 * convertDCOffset
 *    Convert a DC offset into the value used by MCA2 that's
 *    a range 0-16383
 *
 *  @param pct - percent of full scale (signed)?
 *  @param unsigned - converted value.
 */
unsigned
CompassProject::convertDCOffset(double pct)
{
//BAD:
// Assuming this is a signed DC offset percentage:
//   double value = 16383.0*(1-pct/100.0);
//   return static_cast<int>(value);

//unsigned DC offset percentage, NOT signed according to Compass 1.3.0
//Also, use the DAC full range not the ADC full range. Confusing, yes but this works. 
   double value = 65535.0*(pct/100.0);
   return static_cast<int>(value);

}
/**
 * getDynamicRange
 *     Return a dynamic range selector value for the input dynamic range
 *    value for CAENPha::setupChannelParameters.
 *
 * @param keyword - dynamic range keyword, e.g. INDYN_2_0_VPP
 * @return unsigned.
 * @retval 10 - 2 vPP
 * @retval  9 - 0.5 vPP
 * 
 */
unsigned
CompassProject::getDynamicRange(std::string keyword)
{
    if (keyword == "INDYN_2_0_VPP") return 10;
    if (keyword == "INDYN_0_5_VPP") return 9;
    
    // Illegal value
    
    throw std::string("Invalid dynamic range value");
}
/**
 * nsToSamples
 *    Coverts nanoseconds to samples -- assumes a 725 (250Mhz).
 *
 * @param ns - nanoseconds to convert.
 * @return unsigned - equivalent number of samples.
 */
unsigned
CompassProject::nsToSamples(double ns)
{
    // each sample is 4ns for the 730:
    
    return static_cast<unsigned>(ns/4.0);
}
/**
 *  stringToLinkeType
 *     Converts a string to a connection type enum value:
 *
 *  @param strType -stringified type.
 *  @return CAEN_DGTZ_ConnectionType
 */
CAEN_DGTZ_ConnectionType
CompassProject::stringToLinkType(const std::string& strType)
{
   if (strType == "USB") {
      return CAEN_DGTZ_USB;
   } else if (strType == "OpticalLink") {  // Guess might be CONET
      return CAEN_DGTZ_OpticalLink;
   } else if (strType == "CONET") {
      return CAEN_DGTZ_OpticalLink;
   } else if (strType == "OPTICAL") {
      return CAEN_DGTZ_OpticalLink;
   } else {
      std::string msg = "Invalid link type string: " ;
      msg  += strType;
      throw(msg);
   }
}
/**
 * getStartMode
 *   Given a string encoding of the start mode, returnn the corresponding
 *   acquisition mode value.
 *
 * @param modeString - stringified mode.
 * @return CAEN_DGTZ_AcqMode_t
 */
CAEN_DGTZ_AcqMode_t
CompassProject::getStartMode(std::string modeString)
{
  if (modeString == "START_MODE_SW" ) {
    return CAEN_DGTZ_SW_CONTROLLED;
  } else if (modeString == "START_MODE_S_IN") {
    return CAEN_DGTZ_S_IN_CONTROLLED;
  } else if (modeString == "START_MODE_FIRST_TRG") {
    return CAEN_DGTZ_FIRST_TRG_CONTROLLED;
  } else {
    throw std::string("Invalid SRV_PARAM_START_MODE value");
  }
}
/**
 *  gainToCode
 *    Convert a gain value to a code:
 *
 *    - 1.0 -> 0
 *    - 2.0 -> 1
 *    - 4.0 -> 2
 *    - 8.0 -> 3
 *
 * @param value - value to convert.
 * @return unsigned code or throws an std::string if the value is invalid.
 */
unsigned
CompassProject::gainToCode(double value)
{

  if (value == 1.0) {
    return 0;
  } else if (value == 2.0) {
    return 1;
  } else if (value == 4.0) {
    return 2;
  } else if (value == 8.0) {
    return 3;
  } else {
    throw std::string("Invalid value for FINEGAIN  must be 1, 2, 4, or 8");
  }
}
/**
* computeIoCtlMask
* Convert the string value of the I/O OUT_SELECTion to a mask of the bits
* in positions 16-19 of the I/O control register.
*
*  @param enumValue - the string value that describes what to do:
*
* Valid values are:
* OUT_PROPAGATION_TRIGGER - propagates the trigger.
* OUT_PROPAGATION_TEST_0,
* OUT_PROPAGATION_TEST_1,
* OUT_PROPAGATION_ACQ_ON - True if data acquisition is on.
* OUT_PROPAGATION_SAMPLE_CLK - Output the sampling clock.
* OUT_PROPAGATION_PLL_CLK - Output is the PLL clock input.
* OUT_PROPAGATION_BUSY - output the digitizer busy.
* OUT_PROPAGATION_PLL_LOCK - State of the PLL Lock.
* OUT_PROPAGATION_VPROBE - Output a virtual probe.
* OUT_PROPAGATION_SYNCIN - Output the sync in pulse.
*
*  @return uint32_t  - value to plug into the I/O Control register.
*/
uint32_t
CompassProject::computeIoCtlMask(const std::string& enumValue)
{
   uint32_t result;
   if (enumValue == "OUT_PROPAGATION_TRIGGER") {
      result = 0;
   } else if (enumValue == "OUT_PROPAGATION_TEST_0") { // Not certain how
      result = 0x10000;                                // test map..
   } else if (enumValue == "OUT_PROPAGATION_TEST_1") { // but hopefully we're
      result = 0x20000;                                // not in test mode.
   } else if (enumValue == "OUT_PROPAGATION_ACQ_ON") {
      result = 0x10000;
   } else if (enumValue == "OUT_PROPAGATION_SAMPLE_CLK") {
      result = 0x50000;
   } else if (enumValue == "OUT_PROPAGATION_PLL_CLK") {
      result = 0x90000;
   } else if (enumValue == "OUT_PROPAGATION_BUSY") {
      result = 0xd0000;
   } else if (enumValue == "OUT_PROPAGATION_PLL_LOCK") {
      result = 0xd0000;            // I think these bits depend on fw version.
   } else if (enumValue == "OUT_PROPAGATION_VPROBE") {
      result = 0x20000;             // Virtual probe.
   } else if (enumValue == "OUT_PROPAGATION_SYNCIN") {
      result = 0x30000;     
   } else {
      throw std::string("Unrecognized value for SRV_PARAM_OUT_SELECTION ");
   }
   
   return result;
}

void CompassProject::processTrgOutMode(CAENPhaParameters & board, const std::string& enumText)
{
	
	if(enumText == "TRGOUT_MODE_LEVEL0"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_LEVEL0;
	} else 	if(enumText == "TRGOUT_MODE_LEVEL1"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_LEVEL1;
	}else 	if(enumText == "TRGOUT_MODE_SW_TRG"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_SW_TRG;
	}else 	if(enumText == "TRGOUT_MODE_EXT_TRG"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_EXT_TRG;
	}else 	if(enumText == "TRGOUT_MODE_GLOBAL_OR_TRG"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_GLOBAL_OR_TRG;
	}else 	if(enumText == "TRGOUT_MODE_RUN"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_RUN;
	}else 	if(enumText == "TRGOUT_MODE_DELAYED_RUN"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_DELAYED_RUN;
	}else 	if(enumText == "TRGOUT_MODE_SAMPLE_CLK"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_SAMPLE_CLK;
	}else 	if(enumText == "TRGOUT_MODE_PLL_CLK"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_PLL_CLK;
	}else 	if(enumText == "TRGOUT_MODE_BUSY"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_BUSY;
	}else 	if(enumText == "TRGOUT_MODE_PLL_UNLOCK"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_PLL_UNLOCK;
	}else 	if(enumText == "TRGOUT_MODE_VPROBE"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_VPROBE;
	}else 	if(enumText == "TRGOUT_MODE_SYNCIN"){
		board.trgoutmode = CAENPhaParameters::TRGOUT_MODE_SYNCIN;
	}else {
      throw std::string("Unrecognized value for SRV_PARAM_TRGOUT_MODE ");
   }
   

}
