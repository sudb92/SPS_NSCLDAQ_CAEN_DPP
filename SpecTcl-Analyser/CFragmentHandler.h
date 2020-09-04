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

#ifndef CFRAGMENTHANDLER_H
#define CFRAGMENTHANDLER_H

// Forware definitions:
//
struct FragmentInfo;


/**
 * CFragmentHandler is the abstract base class for all concrete fragment handlers.
 *                  its purpose is to define the interface that must be
 *                  implemented by concrete fragment handlers.  This is done
 *                  by providing a pure virtual method, the function call operator.
 */

class CFragmentHandler {

public:
    virtual void operator()(FragmentInfo& frag) = 0;
};


#endif
