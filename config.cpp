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
 * @file config.cpp
 * @brief Functions to manage digitizer configuration.
 */

#include "config.h"

#include <algorithm>
#include <stdexcept>
#include <ctype.h>

#include <stdio.h>


/*-------------------------------------------------------------------------
 *  local functions:
 */

/**
 * strtolower
 *   Convert an std::string to lower case
 * @param in - the string to convert.
 * @return std::string converted string.
 */
static std::string
strtolower(std::string in)
{
  std::transform(in.begin(), in.end(), in.begin(), ::tolower);
  return in;
}

/*------------------------------------------------------------------------
 * Public methods:
 */

/**
* loadConfig
*    Load a configuration file
* @param filename - Name of the config file.
* @return dictionary* Pointer to an inifile dictionary that contains the loaded configuration.
*                     dictionary is dynamically allocated storage that must be freed via
*                     iniparse_freedict().
* @throw runtime_error in case of errors.
*
*/

dictionary* loadConfig(const char* filename)
{
  dictionary* pDict = iniparser_load(filename);
  if (!pDict) {
    throw std::runtime_error("Config file parse failed");
  }
  return pDict;
}

/**
 * getParamString
 *   Return a parameter string value.  First the parameter is looked up in the common
 *   section.  If it is also in the specific channel's section that value is used instead.
 *   See the parameter defs below however.
 *
 * @param config - a configuration dictionary.
 * @param key - The keyword to get (e.g. TRG_THRESHOLD) - this is case insensitive.
 * @param chan - The channel for which we want the value,  -1 means global only.
 * @param def  - Default value if the string does not exist 
 *               in either [GLOBAL] or per chan sections
 * @return std::string - value of the parameter.
 *
 */
std::string
getParamString(dictionary* config, std::string key, int chan, std::string def)
{
  key = strtolower(key);
  std::string global="common:";
  global += key;
  std::string result = iniparser_getstring(config, global.c_str(), const_cast<char*>(def.c_str()));
  if (chan >= 0) {
    char szChannel[200];
    sprintf(szChannel, "%d:%s", chan, key.c_str());
    if (iniparser_find_entry(config, szChannel)) {
      result = iniparser_getstring(config, szChannel, const_cast<char*>(def.c_str()));
    }
  }

  return result;
    
}
/**
 * getIntParam
 *   Same as getParamString but an integer parameter is returned.  Note that we are going
 *   to use getParamString rather than iniparse_getint so that we don't need
 *   to reproduce the logic of that method.
 *
 * @param config  - Configuration dictionary.
 * @param key     - Key within common or a channel.
 * @param chan    - a channel number; -1 means only look in [common].
 * @param def     - Default value.
 * @return int    - Value of the parameter.
 * @throw std::runtime_error - if the value is not a valid integer.
 */
int 
getIntParam(dictionary* config, std::string key, int chan, int def)
{
  int value = def;

  std::string stringValue = getParamString(config, key, chan, "");
  if (stringValue != "") {
    int n = sscanf(stringValue.c_str(), "%d", &value);
    if (n == 0) {
      std::string msg = "Key value for : ";
      msg += key;
      msg += " must be an integer but was : ";
      msg += stringValue;
      throw std::runtime_error(msg);
    }

  }
  return value;
}
/**
 * getDoubleParam
 *   Same as getIntParam but the value returned is a double precision value.
 * @param config  - Configuration dictionary.
 * @param key     - Key within common or a channel.
 * @param chan    - a channel number; -1 means only look in [common].
 * @param def     - Default value.
 * @return int    - Value of the parameter.
 * @throw std::runtime_error - if the value is not a valid integer.
 */
double
getDoubleParam(dictionary* config, std::string key, int chan, double def)
{
  double value = def;

  std::string stringValue = getParamString(config, key, chan, "");
  if (stringValue != "") {
    int n = sscanf(stringValue.c_str(), "%lg", &value);
    if (n == 0) {
      std::string msg = "Key value for : ";
      msg += key;
      msg += " must be a floating point value but was : ";
      msg += stringValue;
      throw std::runtime_error(msg);
    }

  }
  return value;
}
/**
 * getKeywordParam
 *   Lookup an integer value associated with a keyword parameter.
 *
 * @param config - dictionary containing the configuration.
 * @param keyword - The keyword.
 * @param chan    - As in the other getter a channel that can override the [common] setting.
 * @param default - Default _string_ (not converted value).
 * @param mapping - std::map<std::string, int> mapping between strings and return values.
 * @return int    - Mapped keword value.
 * @throw std::runtime_error - keyword value doesn't correspond to a map entry e.g.
 */
int
getKeywordParam(dictionary* config, std::string keyword, int chan, std::string defval, 
		std::map<std::string, int> mapping)
{
  std::string stringVal = getParamString(config, keyword, chan, defval);

  if (mapping.find(stringVal) == mapping.end()) {
    throw std::runtime_error("Invalid keyword value for keyword parameter");
  }
  return mapping[stringVal];
}




