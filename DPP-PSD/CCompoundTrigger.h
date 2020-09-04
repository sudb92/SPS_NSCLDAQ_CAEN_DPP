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
* @file     CCompoundTrigger.h
* @brief    Define a generic compound trigger.
* @author   Ron Fox
*
*/
#ifndef CCOMPOUNDTRIGGER_H
#define CCOMPOUNDTRIGGER_H
#include <CEventTrigger.h>
#include <vector>

/**
 * @class CCompoundTrigger
 *    Supports getting an event trigger from the logical or of several sources.
 *    For exmample, DPP-PSD and DPP-PHA modules or those modules in conjunction
 *    with some other trigger source.
 */
class CCompoundTrigger : public CEventTrigger
{
private:
    std::vector<CEventTrigger*> m_triggers;
public:

    virtual void setup();
    virtual void teardown();
    virtual bool operator()();
    
    void addTrigger(CEventTrigger* pTrigger);

};

#endif