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

/** @file:  CPSDFragmentHandler.h
 *  @brief: Define the class that handles DPP-PSD fragments from CAEN Digitizers.
 */

#ifndef CPSDFRAGMENTHANDLER_H
#define CPSDFRAGMENTHANDLER_H

#include "CDppFragmentHandler.h"

#include <map>
#include <string>
#include <cstdint>

/**
 * CPSDFragmentHandler 
 */
class CPSDFragmentHandler : public CDppFragmentHandler
{

public:
    CPSDFragmentHandler();
    void operator()(FragmentInfo& frag);
    void printEvent(FragmentInfo& frag);
};

#endif
