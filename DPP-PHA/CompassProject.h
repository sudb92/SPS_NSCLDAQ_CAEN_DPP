/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file CompassProject.h
# @brief Class to parse projects.
# @author Ron Fox (rfoxkendo@gmail.com)

*/

#ifndef COMPASSPROJECT_H
#define COMPASSPROJECT_H

#include "pugiutils.h"
#include <vector>
#include <string>
#include "CAENPhaParameters.h"
#include "CAENPhaChannelParameters.h"
#include <CAENDigitizerType.h>

class CAENPhaChannelParameters;

class CompassProject {
public:
    typedef std::pair<unsigned, CAENPhaChannelParameters*>  ChannelInfo;
    typedef struct {
        CAEN_DGTZ_ConnectionType s_linkType;
        int                      s_linkNum;
        int                      s_node;
        int                      s_base;
    } ConnectionParameters, *pConnectionParameters;
private:
    std::string m_filename;
    pugi::xml_document m_doc;
    CAENPhaChannelParameters        m_channelDefaults;
public:
    std::vector<CAENPhaParameters*> m_boards;
    std::vector<ConnectionParameters> m_connections;
public:
    CompassProject(const char* file);
    virtual ~CompassProject();
    
    void operator()();                    // Parse the configuration.
    
protected:
    std::vector<ChannelInfo> parseBoardChannelConfig(pugi::xml_node board);
    void processChannelParams(pugi::xml_node values, CAENPhaChannelParameters* params);
    void processChannelEntry(pugi::xml_node entry, CAENPhaChannelParameters* params);
    
    void processBoardParameters(
        pugi::xml_node entry, CAENPhaParameters& board,
        ConnectionParameters& connection
    );
    void processABoardParameter(pugi::xml_node entry, CAENPhaParameters& board);
    int convertRccr2Smoothing(const std::string& code);
    unsigned convertBaselineMeanCode(const std::string& code);
    unsigned convertPeakMeanCode(const std::string& code);
    unsigned convertDCOffset(double pct);
    unsigned getDynamicRange(std::string keyword);
    unsigned nsToSamples(double value);
    CAEN_DGTZ_ConnectionType stringToLinkType(const std::string& strType);
    CAEN_DGTZ_AcqMode_t  getStartMode(std::string modeString);
    unsigned gainToCode(double value);
    uint32_t computeIoCtlMask(const std::string& enumText);
    void processTrgOutMode(CAENPhaParameters& board, const std::string& enumText);
};

#endif
