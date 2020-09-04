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
* @file     CPSDTrigger.cpp
* @brief    implementation of the PSD multimodule trigger.
* @author   Ron Fox
*
*/
#include "CPsdTrigger.h"
#include "CDPpPsdEventSegment.h"

/**
 * operator()
 *    Check at most all modules beginning with m_n NextModule and return true if
 *    at least one has a trigger.
 * @return bool
 */
bool
CPsdTrigger::operator()()
{
    for (int i =0; i < m_modules.size(); i++) {
        next();
        if (m_modules[m_nNextModule]->checkTrigger()) return true;
    }
    // Checked all of them.
    
    return false;
}
/**
 * addModule
 *    Add a module to the trigger check list
 * @param pModule - pointer to the module to add.
 */
void
CPsdTrigger::addModule(CDPpPsdEventSegment* pModule)
{
    m_modules.push_back(pModule);
}
////////////////////////////////////////////////////////////////////////////
// Utility methods;

/**
 * next
 *   Advances m_nNextModule to the next module (circularly).
 */
void
CPsdTrigger::next()
{
    m_nNextModule = (++m_nNextModule) % m_modules.size();
}