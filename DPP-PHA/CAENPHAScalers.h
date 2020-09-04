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
* @file     CAENPHAScalers.h
* @brief    Scaler bank for scalers from CompassEventSegments for CAENPHA.
* @author   Ron Fox.
*
*/
#ifndef CAENPHASCALERS_H
#define CAENPHASCALERS_H
#include <CScaler.h>

class CompassEventSegment;

/**
 * @class CAENPHAScalers
 *   This class comprises a scaler module that fetches the counters
 *   from caen PHA event segments.  The underlying event segment
 *   must have enabled extra 2 to be th e trigger counters.  We'll produce
 *   16 trigger count 'scalers' followed by 16 missed triggers.
 *
 *   Note that the scalers have a granularity of at best 128 counts, probably
 *   because CAEN tosses them around as uin16_t's packed into a single
 *   uint32_t word.
 */
class CAENPHAScalers : public CScaler
{
private:
    CompassEventSegment*  m_pSegment;
    int                   m_nSourceId;
public:
    CAENPHAScalers(CompassEventSegment* pEventSegment, int srcid = 0);
    virtual std::vector<uint32_t> read();
    virtual int sourceId() {return m_nSourceId;}
};


#endif