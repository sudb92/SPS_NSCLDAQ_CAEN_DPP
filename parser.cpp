/*-------------------------------------------------------------
 
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
* @file     parser.cpp
* @brief    Testing pugixml parser on configuration files from mca2
* @author   Ron Fox (rfoxkendo@gmail.com)
*
*  This file contains code that provides a parser for MCA2 XML setup files
*  using the pugixml parser.  The stuff we care about is extracted from
*  The parsed XML DOM tree into a useful object where it can more easily
*  be used to set up the digitizer for acquisition.
*/

#include <iostream>
#include <cstdlib>
#include "pugixml.hpp"
#include "CAENPhaParameters.h"


/**
 * Main just gets the name of the parser from the command line
 * Construct the DOM tree and pass it on to the parser object.
 *
 * @param argc - number of command line words (must be 2).
 * @param argv - pointer to the list of command line word pointers.
 *               argv[1] is a pointer to the filename.
 */
int
main (int argc, char** argv)
{
  if (argc <  2) {
    std::cerr << "Invalid number of command line parameters\n";
    std::cerr << "Usage: \n";
    std::cerr << "   " << argv[0] << " xml-file [channel0xml...]\n";
    std::cerr << "Where\n    xml-file is an xml digitizer configuration file\n";
    std::exit(EXIT_FAILURE);
  }
  // Parse the digitizer document into a DOM tree:

  pugi::xml_document dom;
  pugi::xml_parse_result result = dom.load_file(argv[1]);

  // Report errors on DOM parse phase and exit if there are some:

  if (!result) {
    std::cerr << "Failed XML -> DOM parse of " << argv[1] << std::endl;
    std::cerr << "Reason: " << result.description() << std::endl;
    std::cerr << "Offset  " << result.offset <<  std::endl;
    std::cerr << dom.child("node").attribute("attr").value() << std::endl;
    std::exit(EXIT_FAILURE);
  }

  //  parse the per channel doms (if there are any).. the assumption is that
  //  these are sequential channels numbered from 0.

  std::vector<std::pair<unsigned, pugi::xml_document*> > chdoms;
  unsigned chan(0);

  for (int i = 2; i < argc; i++) {
    std::pair<unsigned, pugi::xml_document*> achDom;
    achDom.first = chan;
    achDom.second = new pugi::xml_document;
    result = achDom.second->load_file(argv[i]);
    if(!result) {
      std::cerr << "Failed to parse channel dom for channel " << chan 
		<< " from " << argv[i] << std::endl;
      std::cerr << "Reason:   " << result.description() << std::endl;
      std::cerr << "Offset    " << result.offset        << std::endl;
      std::cerr << achDom.second->child("node").attribute("attr").value() << std::endl;
      exit(EXIT_FAILURE);
    }
    chdoms.push_back(achDom);

    chan++;
  }

  // Construct the result on the dom and unpack.

  CAENPhaParameters params(dom, chdoms);
  params.unpack();

  std::cout << "Pre trigger at " << params.preTriggers << std::endl;
  std::cout << "Record Length  " << params.recordLength <<  std::endl;

}

