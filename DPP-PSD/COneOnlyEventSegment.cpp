/*
*-------------------------------------------------------------
 
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
* @file     COneOnlyEventSegment.cpp
* @brief    Implement the one only event segment.
* @author   Ron Fox
*
*/
#include "COneOnlyEventSegment.h"

/**
 * constructor
 */
COneOnlyEventSegment::COneOnlyEventSegment() : m_nextRead(0)
{
    
}
/**
 * destructor  -- probably never gets called anyway.
 */
COneOnlyEventSegment::~COneOnlyEventSegment()
{
    
}
/**
 *  initialize
 *     Just let each segment intialize itself.
 */
void
COneOnlyEventSegment::initialize()
{
    for (int i =0; i < m_segments.size(); i++) {
        m_segments[i]->initialize();
    }
}
/**
 * clear
 *   Let each segment clear itself.
 */
void
COneOnlyEventSegment::clear()
{
    for (int i =0; i < m_segments.size(); i++) {
        m_segments[i]->clear();
    }
}
/**
 * disable
 *    Each segment can take care of itself.
 */
void
COneOnlyEventSegment::disable()
{
    for (int i =0; i < m_segments.size(); i++) {
        m_segments[i]->disable();
    }
}
/**
 * read
 *    - Start the reads in a round robbin sequence.
 *    - Once a child segment provides data we're done.
 * @param pBuffer - pointer to where the data gets read.
 * @param maxwords - Maximum number of words that can be read without overflowing the buffer.
 * @return size_t  - number of words read - 0 means no segment had data.
 */
size_t
COneOnlyEventSegment::read(void* pBuffer, size_t maxwords)
{
    for (int i =0; i < m_segments.size(); i++) {
        CEventSegment* p = nextToRead();
        size_t result = p->read(pBuffer, maxwords);
        if (result) return result;
    }
    return 0;
}
/**
 * addModule
 *    Adds a new event segment to the compound.
 *
 * @param p - pointer to the event segment to add.
 */
void
COneOnlyEventSegment::addModule(CEventSegment* p)
{
    m_segments.push_back(p);
}
/////////////////////////////////////////////////////////////////////////
// Local utility methods.

/**
 * nextToRead
 *
 * @return CEventSegment* - pointer to the next segment to read - fair round robbin.
 */
CEventSegment*
COneOnlyEventSegment::nextToRead()
{
    nextSegment();
    return m_segments[m_nextRead];
}
/**
 * nextSegment
 *    Modulo increment of m_nextRead.
 */
void
COneOnlyEventSegment::nextSegment()
{
    m_nextRead++;
    m_nextRead = m_nextRead % m_segments.size();
}