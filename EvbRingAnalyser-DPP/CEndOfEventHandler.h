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

/** @file:  CEndOfEventHandler.h
 *  @brief: Provide an abstract base class that defines the interface for end handlers.
 */
/* Adapted for DPP-PHA/PSD by Sudarsan B
 sbalak2@lsu.edu, Aug-Sep 2020 */

#ifndef CENDOFEVENTHANDLER_H
#define CENDOFEVENTHANDLER_H

// Forward definitions.

class CRingItem;

/**
 *  CEndOfEventHandler - This is an abstract base class that defines the
 *                       interface an end of event handler must implement.
 *                       End of event handlers can be registered to be called
 *                       after all fragments in an event are processed. This
 *                       allows parameters to be produced that require information
 *                       from more than one fragment.
 */
class CEndOfEventHandler {
  
public:
    virtual void operator()(CRingItem* pItem) = 0;
};


#endif
