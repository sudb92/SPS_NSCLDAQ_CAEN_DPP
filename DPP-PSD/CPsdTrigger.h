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
* @file     CPsdTrigger.h
* @brief    Header for trigger module for PSD trigger.
* @author   Ron Fox
*
*/
#ifndef CPSDTRIGGER_H
#define CPSDTRIGGER_H
#include <CEventTrigger.h>
#include <vector>

class CDPpPsdEventSegment;

/**
 * @class CPsdTrigger
 *    Provides a trigger checker for a set of PSD modules - the modules need not
 *    all be synchronized though it hardly makes sense to run them asynch unless you're
 *    doing a singles experiment.
 */
class CPsdTrigger : public CEventTrigger {
private:
    std::vector<CDPpPsdEventSegment*> m_modules;
    unsigned                          m_nNextModule;
public:
    virtual bool operator()();
    void addModule(CDPpPsdEventSegment* p);
private:
    void next();                    // module iteration.
};

#endif