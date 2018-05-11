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
 * @file config.h
 * @brief Functions to manage digitizer configuration.
 */
#ifndef CONFIG_H
#define CONFIG_H
extern "C" {
#include <iniparser.h>
}

#include <string>
#include <map>




extern dictionary* loadConfig(const char* filename);
extern std::string getParamString(dictionary* config, std::string key, int chan, std::string def);
extern int getIntParam(dictionary* config, std::string key, int chan, int def);
extern double getDoubleParam(dictionary* config, std::string key, int chan, double def);
extern int getKeywordParam(dictionary* config, std::string keyword, int chan, std::string defval, 
			   std::map<std::string, int> mapping);


#endif
