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
* @file     pugiutils.cpp
* @brief    utilitie methods for pugixml
* @author   Ron Fox (rfoxkendo@gmail.com)
*
*
*/
#include "pugiutils.h"
#include <iostream>
#include <stdexcept>

/*------------------------------------------------------------------------
 *  Useful predicate classes:
 */

class NameMatcher {
private:
  std::string m_name;
public:
  NameMatcher(const char*name) : m_name(name) {}
  bool operator()(pugi::xml_node node) {
    return m_name == node.name();
  }
};

/**
 * getNodeByName
 *   Finds a node by name.  This is done with a depth first search from a starting point.
 *
 * @param treetop - top of the subtree that will be searched.
 * @param name    - Name of the node to search for.
 * @return pugi::xml_node  - null node if not found else first match.
 */
pugi::xml_node
getNodeByName(pugi::xml_node treetop, const char* name)
{
  NameMatcher pred(name);
  return treetop.find_node(pred);
}
/**
 * getNodeByNameOrThrow
 *   Same as above but throws an error message if not found.
 *
 * @param treetop - top node under which the named node should be.
 * @param name    - tag name to search for.
 * @param msg     - Messsage to throw.
 */
pugi::xml_node
getNodeByNameOrThrow(pugi::xml_node treetop, const char* name, const char* msg)
{
  pugi::xml_node result = getNodeByName(treetop, name);
  if (result.type() == pugi::node_null) {
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }

  return result;
}

/**
 * getValue
 *   More than one node type has a <value> subtag and we're interested pcdata contents of that
 *   node:
 *   - Find the <value> child.
 *   - Get its contents (child) which must be a pcdata node.
 *   - Return the value of that node.
 *
 * @param node - the node that's supposed to have a value.
 * @return std::string
 *
 * @note an exception is thrown if:
 *   - The node has no <value>  child.
 *   - The <value> tag's first child is not  a node_pcdata node.
 */
std::string
getValue(pugi::xml_node node)
{
  pugi::xml_node value =  getNodeByName(node, "value");
  if (value.type() == pugi::node_null) {
        std::string msg("Node : ");
	msg += node.name();
	msg += " is supposed to have a <value> tag but does not";
	std::cerr << msg << std::endl;
	throw std::domain_error(msg);
  }
  return getStringContents(value);

    
}

/**
 * getUnsignedValue
 *
 *  Get <value> child contents converted to unsigned.
 *
 * @param node - node that has a <value> ultimate child>
 * @return unsigned - CTEXT in <value> converted to unsigned.
 */
unsigned
getUnsignedValue(pugi::xml_node node)
{
  std::string strValue = getValue(node);
  return std::stoul(strValue);
}
/**
 * getDoubleValue
 *    Get <value> contents as a double.
 *
 * @param node - a node that has a <value> ultimate child.
 * @return double
 */
double
getDoubleValue(pugi::xml_node node)
{
  std::string cstr = getValue(node);
  return std::stod(cstr);
}
/**
 * getBoolValue
 *   Get the boolean of a child <value> tag.
 *
 * @param node -parent of <value> node.
 * @return bool.
 */
bool
getBoolValue(pugi::xml_node node)
{ 
  std::string strValue = getValue(node);
  return ! ((strValue == "False") || (strValue == "false"));  // Compass uses lower cases
}
/**
 * getStringContents
 *   Given a tag that's supposed to contain a pc_data node, returns that
 *   data as an std::string.
 *
 * @param node - the node that contains data.
 * @return std::string - the string value of the pc_data 
 * @note It's an exception if the first child is not pcdata 
 * @note We assume there's only one child node.
 */
std::string
getStringContents(pugi::xml_node node)
{
  pugi::xml_node contents = node.first_child();
  if (contents.type() != pugi::node_pcdata) {
    std::string msg("Node : " );
    msg += node.name();
    msg += " is supposed to contain pcdata but does not.";
    std::cerr << msg << std::endl;
    throw std::domain_error(msg);
  }
  return std::string(contents.value());  
}


/**
 * getUnsignedContents
 *   Given a node that's supposed to contain pcdata that's an
 *   unsigned integer, returns those contents.
 *
 * @param node - The node to process.
 * @return unsigned
 */
unsigned
getUnsignedContents(pugi::xml_node node)
{
  std::string strData = getStringContents(node);

  return std::stoul(strData);
}
/**
 * getBoolContents
 *   Return the cdata contents of a node intepreted as a bool:
 *
 * @param node -the node.
 */
bool
getBoolContents(pugi::xml_node node)
{
  std::string strData = getStringContents(node);
  return (strData == "True") ? true : false;
}
/**
 * getAllByName
 *    Returns a vector of nodes whose names are given by the search string.
 *    It is not an error to fail to find any matches.  That just results
 *    in an empty vector.
 *
 *   @param parent  - node that is the parent of the search space.
 *   @param name    - Name of searched for node.
 *   @return std::vector<pugi::xml_node>  Vector of the nodes found.
 */
std::vector<pugi::xml_node>
getAllByName(pugi::xml_node parent, const char* name)
{
  std::string n(name);                 // For compares.
  std::vector<pugi::xml_node> result;
  
  pugi::xml_node child = parent.first_child();
  do {
    if(child.name() == n) {
      result.push_back(child);
    }
    
    child = child.next_sibling();
  } while (child.type() != pugi::node_null);
  
  return result;
}

/**
 * getAllByName2
 *    Returns a vector of nodes whose names are given by the search string.
      This function goes one level deeper, and looks for the child within a child given by name.
      We need this because the XML hierarchy is slightly different between global and per channel parameters

For channels:
	<channel>
		<values>
			<entry>
				<key>
				<value>
For globals:
	<parameters>
		<entry>
			<key>
			<value>
				<value>

 *    It is not an error to fail to find any matches.  That just results
 *    in an empty vector.
 *
 *   @param parent  - node that is the parent of the search space.
 *   @param name    - Name of searched for node.
 *   @return std::vector<pugi::xml_node>  Vector of the nodes found.
 */

//Edit by B.Sudarsan, July 2020
std::vector<pugi::xml_node>
getAllByName2(pugi::xml_node parent, const char* name, const char* first_child_name )
{
  std::string n(name);                 // For compares.
  std::vector<pugi::xml_node> result;
  
  pugi::xml_node child = parent.child(first_child_name).first_child();

  do {
    if(child.name() == n) {
      result.push_back(child);
    }
    
    child = child.next_sibling();
  } while (child.type() != pugi::node_null);
  
  return result;
}
