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

/** @file:  CRingItemDecoder.cpp 
 *  @brief: Implement the CRingItemDecoder class.
 */

#include "CRingItemDecoder.h"

/**
 *  The ring item factory is sort of like a smart upcast - given a ring item
 *  it creates the specific ring item object dynamically. This will be used
 *  to turn a CRingItem into a CPhysicsEventItem object..
*/
#include <CRingItemFactory.h>
#include <CRingItem.h>
#include <CPhysicsEventItem.h>
#include <DataFormat.h>           // Defines the ring item types.
// Fragment index provides iterator functionality on the fragments of a
// built event.
#include "FragmentIndex.h"         
#include <iostream>

/**
 *   constructor
 *       We just need to initialize the end handler pointer to null so that
 *       there's no intial end handler.  std::map constructs to empty so there's
 *       no need to do anything with it.
 */
CRingItemDecoder::CRingItemDecoder() : m_endHandler(0)
{    
}

/**
 *  registerFragmentHandler
 *     Register a handler for fragments from a specific event source id.
 *     If a duplicate registration is done, we just ovewrite the existing one.
 *     Other approaches include:
 *     -  Having a list of fragment handlers for each  map entry and appending.
 *     -  Throwing an error if a fragment handler is a duplicate.
 *     In this implementation, if you register a null fragment handler that's the
 *     same as not registering one.
 *
 * @param sourceId   - the source id that will be handled by the handler.
 * @param pHandler   - Pointer to the handler.  The caller is responsible for
 *                     storage management.
 */
void
CRingItemDecoder::registerFragmentHandler(std::uint32_t sourceId, CFragmentHandler* pHandler)
{
    m_fragmentHandlers[sourceId] = pHandler;    
}

/**
 * registerEndHandler
 *    Registers a handler that's invoked after all fragments of an event
 *    have been processed.
 *
 *  @param pHandler - pointer to the handler object.  The caller is
 *                    responsible for storage management.
 */
void
CRingItemDecoder::registerEndHandler(CEndOfEventHandler* pHandler)
{
    m_endHandler = pHandler;
}

/**
 * operator()
 *    Responsible for decoding a single ring item.
 *    - Extract the type.
 *    - If the type is a PHYSICS_EVENT convert the item to a PhysicsEventItem
 *      and dispatch to decodePhysicsEvent
 *    - If the type is anything else, dispatch to decodeOtherItems.
 *
 * @param pItem - pointer to the ring item.
 */
std::vector<DppEvent>
CRingItemDecoder::operator()(CRingItem* pItem)
{
    std::uint32_t itemType = pItem->type();
    CRingItem* pActualItem = CRingItemFactory::createRingItem(*pItem);
    
    if (itemType == PHYSICS_EVENT) {
        CPhysicsEventItem* pPhysics = dynamic_cast<CPhysicsEventItem*>(pActualItem);
        if (!pPhysics) {
            std::cerr << "Error item type was PHYSICS_EVENT but factory could not convert it";
            return std::vector<DppEvent>(0);
        }
        return decodePhysicsEvent(pPhysics);
    } else {
      decodeOtherItems(pActualItem);
      return std::vector<DppEvent>(0);
    }
    delete pActualItem;
}
/**
 * decodePhysicsEvent
 *    - extract the body header items just to show how that can be done.
 *    - Iterate over the fragments; for each fragment invoke
 *      any handler for its data source id.
 *    - If there's an end of event handler, invoke it after all fragments
 *      have been handed.
 *
 *  Note:  A warning messgae is emitted, and the fragment is not processed
 *         if it has no body header.
 *
 *   @param pItem - pointer to the physics even item  object.
 */
std::vector<DppEvent>
CRingItemDecoder::decodePhysicsEvent(CPhysicsEventItem* pItem)
{
    if (! pItem->hasBodyHeader()) {
        std::cerr << "Warning - an event has no body header - won't be processed\n";
        return std::vector<DppEvent>(0);;
    }
    
    // Pull out the body header information:   
    std::uint64_t timestamp = pItem->getEventTimestamp();
    std::uint32_t srcid     = pItem->getSourceId();   // the source id for next stage building.
    std::uint32_t btype     = pItem->getBarrierType();

    
    // Build the fragment iterator and ask it how many fragments the event has:
    FragmentIndex iterator(reinterpret_cast<std::uint16_t*>(pItem->getBodyPointer()));
    size_t  nFrags = iterator.getNumberFragments();
    std::cout << "\nnFrags:" << nFrags << " ";
   // std::vector<DppEvent> events;

    // Iterate over the fragments; There's a bit of trickeration here.
    // If we ask for the value of an element of a map that has not yet been set,
    // the map will create one with default construction.   For pointers this
    // results in a null pointer.
    
    for (size_t i = 0; i < nFrags; i++) {
        FragmentInfo f = iterator.getFragment(i);
        //std::cout << "\nns_size:" << f.s_size << " sid:" << f.s_sourceId;  
        CDppFragmentHandler *h = dynamic_cast<CDppFragmentHandler*>(m_fragmentHandlers[f.s_size]);
        if (h)
	{
	  (*h)(f); // Invoke the fragment handler if it exists.
	  events.push_back(h->getEvent());
	  if(runMode == DUMP)
		  h->printEvent();
	}
        
    }
    
    // Invoke any end of event handler:
    
    if (m_endHandler && runMode == DUMP) (*m_endHandler)(pItem);
   return events;
}

/**
 * decodeOtherItems
 *    Just dump the item type to stdout if it's not a physics item.
 *
 *  @param pItem - the non-physics event item.
 */
void
CRingItemDecoder::decodeOtherItems(CRingItem* pItem)
{
//    std::cout << pItem->toString() << std::endl;  // Ring items can stringify.
   if(runMode==DUMP) ; //std::cout << pItem->type() << std::endl;
}


