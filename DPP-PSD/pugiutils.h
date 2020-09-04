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
* @file     pugiutils.h
* @brief    utility methods for pugixml
* @author   Ron Fox (rfoxkendo@gmail.com)
*
*
*/

#ifndef PUGIUTILS_H
#define PUGIUTILS_H

#include "pugixml.hpp"
#include <string>
#include <vector>

pugi::xml_node getNodeByName(pugi::xml_node treetop, const char* name);
pugi::xml_node getNodeByNameOrThrow(pugi::xml_node treetop, const char* name, const char* msg);
std::string getValue(pugi::xml_node node);
unsigned    getUnsignedValue(pugi::xml_node node);
double      getDoubleValue(pugi::xml_node node);
bool        getBoolValue(pugi::xml_node node);
std::string getStringContents(pugi::xml_node node);
unsigned    getUnsignedContents(pugi::xml_node node);
bool        getBoolContents(pugi::xml_node node);
std::vector<pugi::xml_node> getAllByName(pugi::xml_node parent, const char* name);
std::vector<pugi::xml_node> getAllByName2(pugi::xml_node parent, const char* name,const char* first_child_name);


#endif
