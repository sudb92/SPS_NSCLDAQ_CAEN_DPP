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
* @file CPsdCompoundEventSegment.h
* @brief Class to run several event segments as a synchronized chain.
*/

#ifndef CPSDCOMPOUNDEVENTSEGMENT_H
#define CPSDCOMPOUNDEVENTSEGMENT_H
#include <CEventSegment.h>
#include <vector>
#include <memory>

class CDPpPsdEventSegment;


class CPsdCompoundEventSegment  : public CEventSegment
{
private:
    std::vector<CDPpPsdEventSegment*> m_modules;
    unsigned                          m_nextRead;
public:
    CPsdCompoundEventSegment();
    virtual ~CPsdCompoundEventSegment();
    virtual void initialize();
    virtual void clear();
    virtual void disable();
    virtual size_t read(void* pBuffer, size_t maxwords);
    
    void addModule(CDPpPsdEventSegment* p);
private:
    CDPpPsdEventSegment* nextToRead();


    void nextModule();
};

#endif
