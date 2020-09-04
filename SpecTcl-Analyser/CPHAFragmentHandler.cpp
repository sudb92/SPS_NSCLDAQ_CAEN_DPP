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

/** @file:  CPHAFragmentHandler.cpp
 *  @brief: Implement the fragment handler for s800 data.
 */

#include "CPHAFragmentHandler.h"
#include "FragmentIndex.h"
//#include "PacketUtils.h"

#include <iostream>
#include <string>
#include <stdexcept>
#include <iostream>


CPHAFragmentHandler::CPHAFragmentHandler()
{
}
/**
 * operator()
 *    Called to process the packet.
 *
 *  @param frag - reference to a fragment that describes the PHA event.
 */
void CPHAFragmentHandler::printEvent(FragmentInfo& frag)
{
	std::uint16_t* p = frag.s_itembody;
	std::cout << "\npsdHead "<< *p;
	std::cout << "\t" << event.size;
	std::cout << "\t" << event.s_data.first;
	std::cout << "\t" << event.timeStamp;
	std::cout << "\t" << event.EShort;
	std::cout << "\t" << event.s_data.second;
	std::cout << "\t" << event.Extras;
}


void
CPHAFragmentHandler::operator()(FragmentInfo& frag)
{
  std::uint16_t* p = frag.s_itembody;
  auto end = p+*p;
    

  uint16_t temp1, temp2, temp3, temp4; //Temporary words to store data
  uint32_t Energy, channelNum; //Temporary variables to be made into a std::pair for ease-of-access by CRawUnpacker.cpp
  auto iter = p; //Start with parsing        

  if (iter<end) {
	temp1 = *iter++;
	temp1 = *iter++;

	temp1 = *iter++; //Will be 001a typically - the size of event
	temp2 = *iter++; //Will be 0000 - the padded zeros usually
	event.size = temp1+0x10000*temp2; //Store it in the size variable
	//std::cout << "\t" << event.size;
	temp1 = *iter++; //Channel number
	temp2 = *iter++; // Padded zeros
	channelNum = temp1+0x10000*temp2  + 16*frag.s_sourceId; //Store channelnumber
	//std::cout << "\t" << channelNum;
	temp1 = *iter++; //Time stamp bits 0-3
	temp2 = *iter++; // TS 4-7
	temp3 = *iter++; // TS 8-11
	temp4 = *iter++; // TS 12-15
	event.timeStamp &= 0x0000000000000000;
	event.timeStamp |=  temp4 ; 
	event.timeStamp = event.timeStamp<<16;
	event.timeStamp |= temp3;
	event.timeStamp = event.timeStamp<<16;
	event.timeStamp |= temp2;
	event.timeStamp = event.timeStamp<<16;
	event.timeStamp |= temp1;
	//std::cout << "\t" << event.timeStamp;

	temp1 = *iter++; 
	temp2 = *iter++; 
	Energy = temp1+0x10000*temp2; //Store energy in temp variable
	//std::cout << "\t";
	//std::cout << "\t" << Energy;
	temp1 = *iter++; 
	temp2 = *iter++; 
	event.Extras = temp1+0x10000*temp2;
	//std::cout << "\t" << event.Extras;
	temp1 = *iter++;

	//Write the channel-number and data to the pair s_data
        event.s_data.first = channelNum;
        event.s_data.second = Energy;

	event.firmwareType = DppEvent::PHA;
  } else {
    std::string errmsg("CRawPHAUnpacker::parseEvent() ");
    errmsg += "Incomplete event found in buffer.";
    throw std::runtime_error(errmsg);
  }
  if((iter < end)||(iter>end)){
    std::string errmsg("CRawPHAUnpacker::parseEvent() ");
    errmsg += "Incorrect event size";
    throw std::runtime_error(errmsg);
	}


}
