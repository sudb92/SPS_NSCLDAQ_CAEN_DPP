/**
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file CompassEventSegment.h
# @brief CAEN PHA event segment whose configuration comes from Compass.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#include "CompassEventSegment.h"
#include "CAENPha.h"
#include "CAENPhaParameters.h"
#include "CAENPhaChannelParameters.h"
#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>
#include "CompassProject.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdlib.h>

//static std::ofstream _rout[7]={NULL,NULL,NULL,NULL,NULL,NULL,NULL};
/*static std::ofstream _rout0("board0.out");
static std::ofstream _rout1("board1.out");
static std::ofstream _rout2("board2.out");
static std::ofstream _rout3("board3.out");
static std::ofstream _rout4("board4.out");
static std::ofstream _rout5("board5.out");
static std::ofstream _rout6("board6.out");*/
//static std::ofstream* _rout7=NULL;

bool fileSwitchOut = false;

/**
 * constructor
 *    For now just initialize the data.  The real action is in
 *    the initialize method.  Actually at this point we don't even
 *    require the existence or readability of the configuration file.
 *
 *  @param filename - Name of the compass configuration file.
 *  @param sourceId  - Event builder source id.
 *  @param linkType - Type of connection with the digitizer.
 *  @param linknum  - If CONNET, the link number of the interface.
 *  @param node     - If CONNET, the node on the daisy chain.
 *  @param base     - If necessary the module base addresss.
 *  @param pCheatFile - if not nullptr - register cheat file name.
 */
CompassEventSegment::CompassEventSegment(
        std::string filename, int sourceId,
        CAEN_DGTZ_ConnectionType linkType, int linkNum, int node, int base,
				const char* pCheatFile
	) : m_filename(filename), m_board(nullptr), m_id(sourceId),
    m_linkType(linkType), m_nLinkNum(linkNum), m_nNode(node), 
    m_nBase(base) , m_pCheatFile(pCheatFile)
{
    
}
/**
 * destructor
 */
CompassEventSegment::~CompassEventSegment()
{
    delete m_board;
}
/**
 * initialize
 *    Prepare the board for data taking.
 *    - Parse the Compass file.
 *    - Instantiate the m_board object
 *    - Setup the board from the parsed/processed configuration
 *      file.
 */
void
CompassEventSegment::initialize()
{
    try {
        CompassProject project(m_filename.c_str());
        project();
        
        // We need to locate the board that matches our parameters.
        
        CAENPhaParameters* ourBoard(nullptr);
        
        for (int i = 0; i < project.m_connections.size(); i++) {
            if (
                (m_linkType == project.m_connections[i].s_linkType)  &&
                (m_nLinkNum  == project.m_connections[i].s_linkNum)   &&
                (m_nNode    == project.m_connections[i].s_node)      &&
                (m_nBase    == project.m_connections[i].s_base) 
            ) {
                ourBoard = (project.m_boards[i]);
                break;
            }
        }
        if (!ourBoard) {                    // no matching board.
	  std::string msg = "No board in ";
	  msg +=  m_filename;
	  msg +=  " matches our connection parameters";
	  throw msg;   
        }
        // Now we can setup the board.

	//Initialize counters at zero
	for (int i =0; i < 16; i++) {
		m_triggerCount[i] = 0;
		m_missedTriggers[i] =0;
	        t[i] = clock();
	}
        
        setupBoard(*ourBoard);
    } catch (std::string msg) {
        std::cerr << "Initialization failed - " << msg << std::endl;
        throw;
    }
    catch (std::pair<std::string, int> phaErr) {
       std::cerr << "Initialization caught a error: "
        << phaErr.first << "(" << phaErr.second << ")\n";
       throw;
    }
}
/**
 * clear
 *    Clear the digitizer.
 */
void
CompassEventSegment::clear()
{
    // This is a no-op as reads are destructive.
}
/**
 *  disable()
 *     Diable the digitizer.  Our code assumes we have a valid
 *     digitizer driver.
 */
void
CompassEventSegment::disable()
{
    try {
        m_board->shutdown();
    }
    catch (std::pair<std::string, int> err) {
        std::cerr << "Board shutdown failed: "
            << err.first << " (" << err.second << ")\n";
        throw;
    }

 /* _rout0.close();
  _rout1.close();
  _rout2.close();
  _rout3.close();
  _rout4.close();
  _rout5.close();
  _rout6.close();*/

//  delete _rout[0];

}
/**
 * read
 *    Read an event from the digitizer and pass it up along the
 *    chain of control.
 *
 * @param pBufer - Pointer to the buffer into which to put the data.
 * @param maxwords - Largest event we can fit into th event buffer.
 * @return size_t  - Number of words read.
 */
size_t
CompassEventSegment::read(void* pBuffer, size_t maxwords)
{
  std::tuple<int, const CAEN_DGTZ_DPP_PHA_Event_t*, const CAEN_DGTZ_DPP_PHA_Waveforms_t*>
  event = m_board->Read();
  
  // Figure out the event size.  Note that since CAEN_DGTZ_DPP_PHA_Event_t*
  // and CAEN_)DGTZ_DPP_PHA_Waveforms_t almost certainly have pads,
  // we're going to write the data an item at a time into the buffer.
  
  int chan = std::get<0>(event);
  const CAEN_DGTZ_DPP_PHA_Event_t* dppData = std::get<1>(event);
  const CAEN_DGTZ_DPP_PHA_Waveforms_t* wfData = std::get<2>(event);

  if (!(dppData || wfData)) {
    reject();
    //clear();
    return 0;                            // No event.
  }

 // if(!(dppData->Extras == 10) )
//     std::cout <<std::dec<< "\nsid:"<< m_id + 1 << "\tCh:" << chan <<"\tEn:" << dppData->Energy<<"\tTs:"<<dppData->TimeTag<<"\tExtr:"<<dppData->Extras<<"\tExtr2:"<<dppData->Extras2;
//if(!(dppData->Extras == 10 || dppData->Extras == 0 ))
  if((dppData->Extras&64)>>6 != 0)
   {
	//std::cout <<std::endl<<"Extr:"<<dppData->Extras<<"\tExtr2:"<<dppData->Extras2<<"\t"<<128/(double(clock() - t[chan]));
	//m_triggerCount[chan] += 1024;	
	m_triggerCount[chan] += 128.;
	t[chan] = clock();
   }
//if(!(dppData->Extras2 == 0))
//	std::cout <<std::endl<<"Extr:"<<dppData->Extras<<"\tExtr2:"<<dppData->Extras2;

if((dppData->Extras == 10)||dppData->TimeTag==0)
  {
      reject();
      clear();
      return 0; //Ignore if it's the 'fake event'
  }


  
  // Note that both the 730 and 725 have a timestamp in 8ns granularity.
  // We store the timestamp in ns in the body header:
  //int64_t offsetsubtract[8] = {-6,54,12,8,0,-12,0,0};
  int64_t offsetsubtract[8] = {0,0,0,0,0,0,0,0};
//-48.000000	-92.000000	-48.000000	-28.000000	-4.000000	-4.000000	0.000000	

  setSourceId(m_id);                     // Source id from member data.         
  setTimestamp(dppData->TimeTag+(offsetsubtract[(int)m_id]));        // Event timestamp - in ns (CAENPha did that).
  size_t eventSize = computeEventSize(*dppData, *wfData);
  
  if ((eventSize / sizeof(uint16_t)) > maxwords) {
    throw std::string("PHAEventSegment size exceeds maxwords - expand max event size");
  }


  // Header consists of the total inclusive size in bytes and the channel #
 /* std::ofstream* outstr=NULL;
  if(fileSwitchOut==true){
	switch((int)m_id){
			   case 0 : outstr = &_rout0; break;
			   case 1 : outstr = &_rout1; break;
			   case 2 : outstr = &_rout2; break;
			   case 3 : outstr = &_rout3; break;
			   case 4 : outstr = &_rout4; break;
			   case 5 : outstr = &_rout5; break;
			   case 6 : outstr = &_rout6; break;
			}

    (*outstr) << std::dec<<"\n"<<dppData->TimeTag+(offsetsubtract[(int)m_id]) <<";"<< dppData->Energy<<";"<< m_id;
   }*/



  pBuffer = putLong(pBuffer, eventSize);
  pBuffer = putLong(pBuffer, chan);
  
  // Body is dpp data followed by wf data:
  pBuffer = putDppData(pBuffer, *dppData);
  pBuffer = putWfData(pBuffer, *wfData);
  
  return (eventSize / sizeof(uint16_t));     
}
/**
 * checkTrigger
 *    Return true if our board has data.. used for the compound trigger
 *    event struct.
 * @return bool - true if the board has data.
 */
bool
CompassEventSegment::checkTrigger()
{
    return m_board->haveData();
}
/*-------------------------------------------------------------------
 * Private member functions.
 */

/**
 * setupBoard
 *    Setup the board:
 *    - If there's a Pha object, delete it and set it's pointer to null.
 *    - Create a new board object save it as m_pBoard.
 *    - Invoke the board object's setup mode
 *    @param board - Reference to our board configuration object.
 */
void
CompassEventSegment::setupBoard(CAENPhaParameters& board)
{
    delete m_board;                // Get rid of any prior board driver.
    m_board = nullptr;
    m_board = new CAENPha(
        board, m_linkType, m_nLinkNum,
				m_nNode, m_nBase,
        board.s_startMode, true, 
        board.startDelay,
				m_pCheatFile
    );
    m_board->setup();
    
}



/**
 * putWord
 *    Put a word into the buffer.
 *
 * @param pDest - pointer to where the word goes
 * @param data  -uint16_t to put.
 * @return void* Pointer to the next free slot in the buffer.
 */
void*
CompassEventSegment::putWord(void* pDest, uint16_t data)
{
    uint16_t* p = reinterpret_cast<uint16_t*>(pDest);
    *p++ = data;
    
    return p;
}
/**
 * putLong
 *  Put a 32 bit long into the buffer.
 *
 * @param pDest - pointer to where the data goes.
 * @param data  - uint32_t to put.
 * @return void* - pointer to the next free slot in the buffer.
 */
void*
CompassEventSegment::putLong(void* pDest, uint32_t data)
{
    uint32_t* p = reinterpret_cast<uint32_t*>(pDest);
    *p++ = data;
    return p;
}

/**
 * putQuad
 *    Put a 64 bit quadword into the buffer.
 *
 * @param pDest - pointer to where the data goes.
 * @param data  - the data.
 * @return void* - points to the next free slot in the buffer.
 */
void*
CompassEventSegment::putQuad(void* pDest, uint64_t data)
{
    uint64_t* p = reinterpret_cast<uint64_t*>(pDest);
    *p++ = data;
    return p;
}

/**
 * computeEventSize
 *    Figure out how big the event is, in bytes, including a 32 bit event size.
 *
 *  @param dppInfo - reference to the CAEN_DGTZ_PHA_Event_t containing the event.
 *  @param wfInfo  - reference to the CAEN_DGTZ_PHA_Waveforms_t containing decoded waveforms.
 *  @return size_t - Number of bytes the event will require in the data stream.
 */
size_t
CompassEventSegment::computeEventSize(
    const CAEN_DGTZ_DPP_PHA_Event_t& dppInfo, const CAEN_DGTZ_DPP_PHA_Waveforms_t& wfInfo
)
{
    size_t result = sizeof(uint32_t);               // Size of event
    
    // The dpp info is a time tag (uint64_t), E, extras, (16 bits )and
    // extras2 (32 bits)):
    
    result += sizeof(uint64_t)   +                    // Time tag.
              2*sizeof(uint16_t) +                    // E, Extras.
              sizeof(uint32_t);                       // Extras2.
              
    // Waveform data size is a bit more complex.  See the putWfData
    // method for considerations.
    
    result += sizeof(uint32_t) + sizeof(uint16_t);  // Ns, and dualtrace flag.
    if (wfInfo.Ns) {
        // If there are samples, there's always at least one trace of 16 bit words:
        
      size_t nBytes = wfInfo.Ns * sizeof(uint16_t);
      result += nBytes;
      
      //  If dual trace there's a second one:
      
      if (wfInfo.DualTrace) {
	result += nBytes;
      }
    }
    return result;
}

/**
 * putDppData
 *    Puts the DPP data into the event buffer.
 * @param pDest - where to put the DPP Data.
 * @param dpp   - Reference to the dpp data.
 * @return - pointer to the next free slot in the buffer.
 */
void*
CompassEventSegment::putDppData(void* pDest, const CAEN_DGTZ_DPP_PHA_Event_t& dpp)
{
    pDest = putQuad(pDest, (dpp.TimeTag));
    pDest = putWord(pDest, (dpp.Energy));
    pDest = putWord(pDest, (dpp.Extras));
    pDest = putLong(pDest, (dpp.Extras2));
    
    return pDest;
}

/**
 * putWfData
 *    Put the waveform data to the output buffer.
 *    We put the following information:
 *    -   ns - number of samples (32 bits)
 *    -   dt - Non zero if ther are two traces (16 bits).
 *             note that we widen this in order to make alignment better.
 *    -   First trace if ns > 0 ns*16 bits.
 *    -   Second trace if ns > 0 && dual trace  ns * 16 bits.
 *
 * @param pDest - pointer to where the data must go
 * @param wf    - Reference to CAEN_DGTZ_PHA_Waveforms_t - the decoded waveforms.
 * @return void* - Pointer to the next free memory of the buffer (where next data goes).
 */
void*
CompassEventSegment::putWfData(void* pDest, const CAEN_DGTZ_DPP_PHA_Waveforms_t& wf)
{
  pDest = putLong(pDest, wf.Ns);                            // # samples.
  pDest = putWord(pDest, wf.DualTrace);
    size_t nBytes = wf.Ns * sizeof(uint16_t);
    if (nBytes > 0) {                                   // There are waveforms:
      uint8_t* p = reinterpret_cast<uint8_t*>(pDest);
        memcpy(p, wf.Trace1, nBytes);               // Write trace 1...
        p +=  nBytes;
        
        if (wf.DualTrace) {                            // Write second trace too:
            memcpy(p, wf.Trace2, nBytes);
            p += nBytes;
        }
	pDest = p;
    }
    return pDest;
}


