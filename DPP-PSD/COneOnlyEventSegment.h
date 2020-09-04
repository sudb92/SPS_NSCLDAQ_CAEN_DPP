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
* @file   COneOnlyEventSegment.h 
* @brief  Compound event segment that only allows one event segment to contribute.
* @author   Ron Fox
*
*/
#ifndef CONEONLYEVENTSEGMENT_H
#define CONEONLYEVENTSEGMENT_H
#include <CEventSegment.h>
#include <vector>


class COneOnlyEventSegment : public CEventSegment
{
private:
    std::vector<CEventSegment*>  m_segments;
    unsigned                     m_nextRead;
public:
    COneOnlyEventSegment();
    virtual ~COneOnlyEventSegment();
    
    virtual void initialize();
    virtual void clear();
    virtual void disable();
    virtual size_t read(void* pBuffer, size_t maxwords);
    
    void addModule(CEventSegment* p);
private:
    CEventSegment* nextToRead();
    void           nextSegment();
};


#endif