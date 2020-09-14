/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2017.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Authors:
             Ron Fox
             Jeromy Tompkins 
	     NSCL
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

/** @file:  CPHAFragmentHandler.h
 *  @brief: Define the class that handles PHA fragments arising from PHA digitizers.
 */

#ifndef CPHAFRAGMENTHANDLER_H
#define CPHAFRAGMENTHANDLER_H

#include "CDppFragmentHandler.h"
#include <map>
#include <string>
#include <cstdint>

/**
 * CPHAFragmentHandler
 */
class CPHAFragmentHandler : public CDppFragmentHandler
{

public:
    CPHAFragmentHandler();
    void operator()(FragmentInfo& frag);
    void printEvent(FragmentInfo& frag);
};

#endif
