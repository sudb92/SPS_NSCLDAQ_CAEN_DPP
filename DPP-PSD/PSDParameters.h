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
* @file     PSDParameters.h
* @brief    PSD Parameter from Compass
* @author   Ron Fox
*
*/
#ifndef PSDPARAMETERS_H
#define PSDPARAMETERS_H
#include <string>
#include <vector>

namespace pugi {
    class xml_document;
    class xml_node;
};

/**
 * @class PSDChannelParameters
 *   Struct that contains per channel parameters.  see the per channel
 *   parameters in PSDParameters for a description of the XML parameter names
 *   that are used to fill this in.
 */
struct PSDChannelParameters {
    double s_channelTimeOffset;            // Ns time offset for timestamps.
    enum {positive, negative}
           s_polarity;                     // Input signal polarity.
    double s_threshold;                    // Trigger threshold in lsb.
    enum {
        fixed,
        samples16,
        samples64,
        samples256,
        samples1024,
        samples4096,
        samples16384
    }      s_blineNsMean;                  // samples to average for baseline:
    double s_cfdDelay;                     // CFD delay parameter.
    bool   s_enabled;                      // Channel is turned on.
    unsigned s_cfdSmoothing;                 // CFD interpolation samples.
    double s_purGap;                           // Pile up rejection gap.
    enum {
        vpp2V, vpphalfV
    }  s_dynamicRange;                    // Input dynamic range.
    double s_shortGate;                   // Short gate in ns.
    enum {
        frac25,
        frac50,
        frac75,
        frac100
    }  s_cfdFraction;                    // CFD Fraction.
    enum {
        led, cfd
    }  s_discriminatorType;             // Leading edge or cfd discriminator.
    double s_fixedBline;               // Baseline if fixed is chosen.
    double s_triggerHoldoff;           // ns of trigger holdoff.
    double s_gateLen;                  // Long gate length in ns.
    double s_dcOffset;                 // Baseline dc offset.
    enum {
        lsb2_5fc,
        lsb10fc,
        lsb40fc,
        lsb160fc,
        lsb640fc,
        lsb2560fc
    } s_coarseGain;                 // Coarse gain in femto coulombs/lsb.
    double s_gatePre;               // integration prior to gate(?).
    double s_preTrigger;            // Waveform capture pre-trigger.
};

struct PSDBoardParameters
{
    typedef enum _LinkType {
        usb, conet
    } LinkType;
    
    // Information about the board and how it's connected to us:
    
    std::string s_modelName;
    int         s_serialNumber;
    LinkType    s_linkType;
    int        s_linkNum;
    int        s_node;
    int        s_base;
    int        s_psPerSample;
    
    // Board level settings.
    
    bool       s_extClock;              // True if clock is external.
    bool       s_energy;                // True if include energy in datga.
    enum {
        disabled,
        couples,
        andOneToAll,
        oneChannelVeto,
	ExtTrgGate,
	ExtTrgVeto        
    }        s_coincidenceMode;      // Coincidence trigger mode.
    enum {
        software, sIn, firstTrigger
    }   s_startMode;                  // How data taking starts.
    double s_coincidenceTriggerOut;   // Coincidence trigger window(?)
    bool s_timeTag;                   // Tag data with timestamp.
    bool s_calibrateBeforeStart;     // Run calibration before starting.
    bool s_softwareTriggerAtStart;
    bool s_includeExtras;           // Include the extras packet.
    double s_recordLength;          // ns sampled for dpp.
    double s_startDelay;            // ns of start delay for the module (synch).
    bool   s_waveforms;             // Include waveforms in the event.
    enum {
        nim, ttl
    }     s_ioLevel;               // Logic signalling level for the I/Os.
    enum {
        level0,
        level1,
        softwareTrigger,
        externalTrigger,
        globalOrTrigger,
        running,
        delayedRunning,
        sampleClock,
        pllClock,
        busy,
        pllUnlocked,
        vProbe,
        syncIn
    } s_triggerOutputMode;        // What goes on the TRGOUT signal
    double s_eventAggregation;
    
    // per channel parameters - 16 channels.
    
    PSDChannelParameters s_channelConfig[16];
    
};

/**
 * @class PSDParameters
 *    This struct defines the parameters that compass configuration files
 *    have for DPP-PSD board  configurations.  These consist
 *    of board level and per-channel configuration parameters.
 *    Board level parameters in this configuraiton consist, not  only
 *    of the definitions of the board but also board level parameters
 *    needed to define inputs, outputs and synchronization for multi-board
 *    setups.
 *    Note that there are tags we don't care about.  These are not listed.
 *
 *  Board Level parameters and their XML tags and parents:
 *
 *  | Tag           |   Parent   | Meaning                        |
 *  |---------------|------------|--------------------------------|
 *  |  modelName    | board      | Type of board (e.g V1730)       |
 *  |  serialNumber | board      | Identifies the exact specific board |
 *  |  connectionType | board    | Type of connection to the board |
 *  |  linkNum      |  board     | Which link of the connection type to use |
 *  |  conetNode    | board      | for connet which node on the link to use |
 *  |  address      | board      | For VME busses the base address |
 *  |  sampleTime   | board      | Picoseconds per sample        |
 *
 * The remaining parameters are key/value pairs in the <parameters> section.
 * They have the form
 *\verbatim
 * <entry>
 *   <key>key-name</key>
 *      <value>
 *         <value>param value</value>
 *         ...
 *      <value>
 *    </key>
 * \endverbatim
 *  | Key  name             |  Data Type  |   Meaning                      |
 *  |-----------------------|-------------|--------------------------------|
 *  |SRV_PARAM_DT_EXT_CLOCK | boolean     | Enable external clock          |
 *  |SRV_PARAM_ENERGY       | bool        | Include energyin data          |
 *  |SRV_PARAM_COINC_MODE   | text        | trigger coincidence mode       |
 *  |SRV_PARAM_START_MODE   | text        | Board start mode               |
 *  |SRV_PARAM_COINC_TRGOUT  | double ns   | ns of coincidence window for trg out |
 *  |SRV_PARAM_TIMEETAG      | bool        | include timestamp in data      |
 *  |SRV_PARAM_ADCCALIB_ONSTART_ENABLE | bool | Run calibration cycle on startup |
 *  |SRV_PARAM_SW_TRG_AT_START | bool     | Software trigger at start.  |
 *  |SRV_PARAM_EXTRAS        | bool      | include extras in the data   |
 *  | SRV_PARAM_RECLEN      | double ns  | Waveform capture window      |
 *  | SRV_PARAM_START_DELAY | double ns  | delay before starting to synch timestamps |
 *  | SRV_PARAM_ACQRUNNING | bool        | ??? active?                  |
 *  | SRV_PARAM _WAVEFORMS | bool        | Include waveforms in data    |
 *  | SRV_PARAM_IOLEVEL    | text        | nim/ttl I/O level            |
 *  | SRV_PARAM_TRGOUT_MODE | text enum  | Trigger output mode          |
 *  |SRV_PARAM_EVENTAGGR | double   | Event aggregations/transfer |
 *  
 *
 *  Channel level parameters and their XML tags (they liv under a <channel> tag).
 *  There are also board level tags for default parameter values to use if the tag
 *  is not specified.  Channel data is all of the form:
 *
 *  \verbatim
 *  <channel>
 *     <index>channel-number</index>
 *     <values>
 *        <entry>
 *          <key>parameter-name</key>
 *          <value>parameter-value</value>
 *        </entry>
 *        ...
 *     </values>
 *  </channel>
 *  \endverbatim
 *
 *  |   Key name             |data type   | Meaning                       |
 *  |SRV_PARAM_CH_TIME_OFFSET| double ns  | ns to add to ch   timestamps  |
 *  |SRV_PARAM_CH_POLARITY  | text       | Channel input signal polarity |
 *  |SRV_PARAM_CH_THRESHOLD | double     | Per channel trigger threshold. |
 *  |SRV_PARAM_CH_BLINE_NSMEAN | text    | no samples to average to get baseline |
 *  |SRV_PARAM_CH_CDF_DELAY | double ns  | CFD Delay in nanoseconds      |
 *  |SRV_PARAM_CH_ENABLED   | bool       | Is channel enabled?           |
 *  |SRV_PARAM_CH_CFD_SMOOTHEXP | text   | # samples between which to interpolate zero crossing |
 *  |SRV_PARAM_CH_PURGAP    | double lsb | Pile up rejection gap.        |
 *  |SRV_PARAM_CH_INDYN     | text       | input dynamic range           |
 *  |SRV_PARAM_CH_GATESHORT | double ns  | Length of short integration gate |
 *  |SRV_PARAM_CH_CFD_FRACTION | text    | CFD Fraction                 |
 *  |SRV_PARAM_CH_DISCR_MODE | text      | timing from LED or CFD zero crossing |
 *  |SRV_PARAM_CH_BLINE_FIXED | double   | Fixed baseline value.        |
 *  |SRV_PARAM_CH_TRG_HOLDOFF| double  ns  | trigger hold off           |
 *  |SRV_PARAM_CH_GATE       | double ns  | Integration gate.           |
 *  |SRV_PARAM_CH_BLINE_DCOFFSET | double % | Baseline dcoffset         |
 *  |SRV_PARAM_CH_ENERGY_COARSE_GAIN | text enum | Energy coarse gain value |
 *  |SRV_PARAM_CH_GATEPRE   | double ns   | Gate pre trigger |
 *  |SRV_PARAM_CH_PRETRG   | double ns   | Pre trigger value |
 * 
 */
struct PSDParameters {
public:
    std::vector<PSDBoardParameters> s_boardParams;
public:
    
    // Top level parsing.
    
    void parseConfigurationFile(const char* pFilename);
    void parseConfiguration(pugi::xml_document& doc);
    void configureBoard(pugi::xml_node& boardNode, PSDBoardParameters& board);
    
    void configureBoardGlobalParameters(
        pugi::xml_node& boardNode, PSDBoardParameters& board
    );
    void setChannelDefaults(
        pugi::xml_node& paramsNode, PSDBoardParameters& board
    );
    void configureChannel(
        pugi::xml_node& chanNode, PSDBoardParameters* board
    );
 
 
    // Utilities:
    
    void setParameterValue(
	pugi::xml_node& entry, PSDBoardParameters& board, int chanNum
    );
    void setLinkType(const std::string& value, PSDBoardParameters& board);
    void setPolarity(
        PSDChannelParameters& chanParams, const std::string& polString
    );
    void setCoincidenceMode(
        PSDBoardParameters& board, const std::string& coincString
    );
    void setChanNsMean(
        PSDChannelParameters& c, const std::string& nsampMeanString
    );
    void setStartMode(
        PSDBoardParameters& board, std::string startModeString
    );
    void setChannelCFDSmoothing(
        PSDChannelParameters& chanParams, const std::string& smooth
    );
    void setChannelDynamicRange(
        PSDChannelParameters& chanParams, const std::string& dynRange
    );
    void setCfdFraction(
        PSDChannelParameters& chanParams, const std::string& frac
    );
    void setDiscriminatorMode(
        PSDChannelParameters& chanParams, const std::string& mode
    );
    void setIoLevel (
        PSDBoardParameters& board, const std::string& signalling
    );
    void setChannelCoarseGain(
        PSDChannelParameters& chanParams, const std::string& gain    
    );
    void setTriggerOutMode(
        PSDBoardParameters& board, const std::string& mode
    );
};



#endif
