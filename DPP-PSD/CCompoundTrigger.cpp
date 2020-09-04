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
* @file     CCompoundTrigger.cpp
* @brief    Implementation of the compound trigger.
* @author   Ron FOx
*
*/
#include "CCompoundTrigger.h"

/**
 * setup
 *   just iterate overa all objects setting them up
 */
void
CCompoundTrigger::setup()
{
    for (int i = 0; i < m_triggers.size(); i++) {
        m_triggers[i]->setup();
    }
}
/**
 * teardown
 *   Just iterate over all objects tearing them down.
 */
void
CCompoundTrigger::teardown()
{
    for (int i =0; i < m_triggers.size(); i++) {
        m_triggers[i]->teardown();
    }
}
/**
 * operator()
 *    @return bool - true if any object has a trigger.
 */
bool
CCompoundTrigger::operator()()
{
    for (int i =0; i < m_triggers.size(); i++) {
        if ((*m_triggers[i])()) return true;
    }
    return false;
}
/**
 * addTrigger
 *    Add a new trigger modules to the set we monitor.
 *  @param pTrigger - trigger to add.. must remain in scope for the entire
 *                    program duration.
 */
void
CCompoundTrigger::addTrigger(CEventTrigger* pTrigger)
{
    m_triggers.push_back(pTrigger);
}