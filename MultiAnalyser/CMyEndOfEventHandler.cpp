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

/** @file:  CMyEndOfEventHandler.cpp
 *  @brief: Implementation of the end of event handler.
 */

#include "CMyEndOfEventHandler.h"
#include <CRingItem.h>

#include <iostream>

/**
 * operator()
 *     just output a marker to stdout indicatig the end of event:
 *     
 * @param pItem - the entire event ring item.
 */
void
CMyEndOfEventHandler::operator()(CRingItem* pItem)
{
  std::cout << "\n--------------------End of Event -----------------------\n";
}
