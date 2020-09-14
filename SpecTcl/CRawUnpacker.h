/*
CRawUnpacker.h

Code borrowed from http://docs.nscl.msu.edu/daq/newsite/nscldaq-11.2/x5013.html
Raw unpacker class that serves to parse the physics event sent by v1730 into a CTreeParameterArray object readable by SpecTcl

B.Sudarsan
sbalak2@lsu.edu
28 August 2018
*/



#ifndef CRAWUNPACKER_H
#define CRAWUNPACKER_H

#include <config.h>
//#include <DataFormat.h>
#include <EventProcessor.h>
#include <TreeParameter.h>
#include <cstdint> 
#include <cstddef> 

class CEvent;
class CAnalyzer;
class CBufferDecoder;

class CRawUnpacker : public CEventProcessor
{
  private:
    CTreeParameterArray  m_values; //Convert the parsed eventdata into a tree node
    CTreeVariableArray  m_timestamps;
  public:
    CRawUnpacker();
    virtual ~CRawUnpacker();
    virtual Bool_t operator()(const Address_t pEvent,
                              CEvent& rEvent,
                              CAnalyzer& rAnalyzer,
                              CBufferDecoder& rDecoder);
};

#endif 
