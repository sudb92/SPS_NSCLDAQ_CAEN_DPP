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

/** @file:  CFragmentHandler.h
 *  @brief: Abstract base class that defines fragment handler mandatory interface.
 */
/* Adapted for DPP-PHA/PSD by Sudarsan B
 sbalak2@lsu.edu, Aug-Sep 2020 */

#ifndef CDPPFRAGMENTHANDLER_H
#define CDPPFRAGMENTHANDLER_H

// Forward definitions:
//
struct FragmentInfo;
#include <utility>
#include <vector>
#include <iostream>
#include "CFragmentHandler.h"
struct DppEvent
{
  typedef enum _type { PHA, PSD } type;
  type firmwareType;
  uint32_t size;
  uint64_t timeStamp;
  uint32_t Extras;
  uint32_t EShort;
  uint32_t Extras2;
  uint32_t Board;		
  std::pair<std::uint32_t,uint32_t> s_data; //First element channel number, second element data
};

/**
 * CFragmentHandler is the abstract base class for all concrete fragment handlers.
 *                  its purpose is to define the interface that must be
 *                  implemented by concrete fragment handlers.  This is done
 *                  by providing a pure virtual method, the function call operator.
 */

class CDppFragmentHandler : public CFragmentHandler{
protected:
   DppEvent event;
public:
    DppEvent getEvent() { return event; }
    void printEvent()
	{
	if(event.firmwareType == DppEvent::PHA)
		std::cout << "\nphaHead ";
	else if (event.firmwareType == DppEvent::PSD)
		std::cout << "\npsdHead ";

	std::cout << "\t" << event.size;
	std::cout << "\t" << event.s_data.first;
	std::cout << "\t" << event.timeStamp;
	std::cout << "\t" << event.EShort;
	std::cout << "\t" << event.s_data.second;
	std::cout << "\t" << event.Extras;
	}
};


#endif
