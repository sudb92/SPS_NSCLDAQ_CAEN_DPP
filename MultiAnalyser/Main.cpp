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

/** @file:  Main.cpp
 *  @brief: Main program to read data from a ring data source,.
 */

/**
    What this template does:  The main program can analyze data from either an
    online data source or from file.
    -  It reads ring items from the data source.
    -  It figures out the ring item type and creates the appropriate ring item
       type.
    -  Non PHYSICS_EVENT items are just dumped to stdout using the stringifiying
       method in the CRingItem class.
    -  Physics items:  The FragmentIndex class that Jeromy wrote is used
                       to iterate over all of the fragments in an event.
                       The framework allows you to register handlers for
                       data sources that are encountered in the  list of
                       fragments in an event.  Unhandled fragments are reported
                       but no error is thrown in consequence.
                       
    Note that heavy use is made of NSCLDAQ Ring item formatting classes in the
    input code.   I recommend that any NSCLDAQ analysis code do that too so you're
    not continuously reinventing wheels (or rings) that already have been
    written.
    
    To make use of this in e.g. Grutinizer
    -  Use whatever input methods you have to absorb ring items from the data
       source.
    -  Feed those ring items to a CRingItemDecoder class instance (see CRingItemDecoder.{h.cpp}).
    -  Note the in file documentation of the CFragmentHandler base class.
       You will need to have created fragment handlers for each data source type you
       are going to handle these are derived from CFragmentHandler and implement
       the function call operator.
    -  Register the the fragment handlers you need with the CRingItemDecoder
       instance so that it will call them for each fragment you can handle.
    -  If you want, you can register an "end of event" handler.  The reason
       you might want to do this is to create parameters that span the fragments
       the event has (e.g. a timestamp difference from one fragment type to another).
       
    The sample code:
       - Registers a fragment handler for S800 data.
       - Registers a fragment handler for the CAESAR data.
       
       These fragment handlers will just output  the body header info from
       each fragment and will output information about each packet the
       the event fragment has.  The end handler  is registered just to show
       that it works.
       - Assumes the endianess of this system is the same as the one that took
         the data in the first place.
       
       The code is heavily documented with comments.
*/
    
// CDataSource Factory creates a data source for file or online from a URI.
// CDataSource is the base class for data sources.
//  docs.nscl.msu.edu/daq/newsite/nscldaq-11.2/index.html has detailed
// documentation of all the classes we're going to use.

#include <CDataSourceFactory.h>
#include <CDataSource.h>
#include <DataFormat.h>                    // Defines ring item types inter alia.
#include <CRingItem.h>                     // Base class for all ring items.


#include "CRingItemDecoder.h"              // Sample code.
#include "CPHAFragmentHandler.h"          // Handle s800 fragments.
#include "CPSDFragmentHandler.h"        // Handle CAESAR fragments.
#include "CMyEndOfEventHandler.h"          // Handle end of event processing.
    
// Includes that are standard c++ things:

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <errno.h>

/* Note that not all errno.h's define ESUCCESS so:  */

#ifndef ESUCCESS
#define ESUCCESS 0
#endif

/**
 *  usage:
 *     Reports usage information for the program to stderr
 *     We accept a single parameter.  The URI of the data source.
 */
void
usage()
{
    std::cerr << "Usage\n";
    std::cerr << "    sampleunpacker  data-source-uri MODE\n";
    std::cerr << "Where:\n";
    std::cerr << "   data-source-uri is the URI for a file or ringbuffer that\n";
    std::cerr << "                   data will be read from\n";
    std::cerr << "   MODE can be one of: \n";
    std::cerr << "       ROOT - plot ROOT histograms from a given macro \n";
    std::cerr << "       DUMP - print the stream as a formatted output to stdout \n";
}

/**
 * main
 *    Entry point for the program -- the usual command parameters.
 */

int
main(int argc, char**argv)
{
    // We need to have a URI and only a URI:
    
    if (argc != 3) {
        usage();
        std::exit(EXIT_FAILURE);
    }
    
    /**
     *  Get the data source URI and use it to create a data source.
     *  our error handling is going to be a bit primitive.  We could catch the
     *  specific exception types and report something a bit more detailed on failure
     */
    std::string uri(argv[1]);
    std::string mode(argv[2]);


   // ModeEnum runMode;
    
    std::vector<uint16_t> sample = {PHYSICS_EVENT};   // means nothing from file.
    std::vector<uint16_t> exclude;                    // get all ring item types:
    
    CDataSource* pSource;
    try {
        pSource = CDataSourceFactory::makeSource(uri, sample, exclude);
    }
    catch (...) {
        std::cerr << "Failed to open the data source.  Check that your URI is valid and exists\n";
        std::exit(EXIT_FAILURE);
    }
    
    CDataSource& source(*pSource);
    
    CRingItemDecoder       decoder;
    CPSDFragmentHandler   psdhandler;
    CPHAFragmentHandler phahandler;
    CMyEndOfEventHandler     endhandler;
    
    decoder.registerFragmentHandler(62, &psdhandler);
    decoder.registerFragmentHandler(58, &phahandler);
    decoder.registerEndHandler(&endhandler);
    decoder.setOutputMode(mode);

    // Now we're ready to accept ring items from the data source.
    // Note that CDataSource::getItem will return a pointer to a
    // dynamically allocated ring item received from the source.
    // -  An int exception is thrown for most errors.
    // -  End of source (e.g. file) is indicated by a null pointer
    //    and errno of ESUCCESS;  Note that ESUCCESS  is not POSIX but
    //    is defined under Linux (it's 0).
    try {
        CRingItem* pItem;
        while (pItem = source.getItem()) {
            decoder(pItem);
            delete pItem;
        }
        // If errno is ESUCCESS we're done, otherwise, just throw the errno
        //  
        if (errno != ESUCCESS) throw errno;       
        std::exit(EXIT_SUCCESS);
    }
    catch (int errcode) {
        
        std::cerr << "Ring item read failed: " << std::strerror(errcode) << std::endl;
        std::exit(EXIT_FAILURE);
    }
    catch (std::string msg) {
        std::cout << msg << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // Code should not fall through to here so:
    
    std::cerr <<"BUG - control fell to the bottom of the main\n";
    std::exit(EXIT_FAILURE);
}
    
    
