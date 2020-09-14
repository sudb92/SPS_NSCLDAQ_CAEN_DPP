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

/** @file:  CMyEndOfEventHandler.h
 *  @brief: define the class that implements the analysis specific end of
 *          event handler.
 */
/* Adapted for DPP-PHA/PSD by Sudarsan B
 sbalak2@lsu.edu, Aug-Sep 2020 */

#ifndef CMYENDOFEVENTHANDLER_H
#define CMYENDOFEVENTHANDLER_H

#include "CEndOfEventHandler.h"              // Base class definition.

class CMyEndOfEventHandler : public CEndOfEventHandler
{
public:
    void operator()(CRingItem* pItem);
};


#endif
