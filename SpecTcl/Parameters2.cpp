/*
Parameters.cpp

Method for computing new parameters for SpecTcl from raw data; specifically
for the SPS setup

Gordon M.
Nov. 2018

Current Edit: Jan 2019 
Changed adc1 to adc3 for post resoneut
*/
#include "Parameters2.h"
#include <BufferDecoder.h>
#include <TCLAnalyzer.h>
#include <stdio.h>
//#include "FP_kinematics.h"
//#include "chanmap_calibration_sabre.h"
//#include <vector>
using namespace std;

Parameters::Parameters() :
    m_values("PSD_PHA_e",16384,0.0,16383.0,"channels",32,0),
    m_timestamps("PSD_PHA_ts",0.0,"ns",32,0),
/*  mtdc_values("tdc2", 65535,0.0, 65534.0, "channels", 32, 0),
  tdc_values("tdc1", 4096,0.0, 4095.0, "channels", 32, 0),
  adc3_values("adc3", 4096, 0.0, 4095.0, "channels", 32, 0),
  adc2_values("adc2", 4096, 0.0, 4095.0, "channels", 32, 0),
  adc1_values("adc1", 4096, 0.0, 4095.0, "channels", 32, 0),
  fp1_tdiff("fp1_tdiff", 17596, -300.0, 300.0, "mm"), 
  fp1_tdiff_ns("fp1_tdiff_ns", 32000, 0.0, 2000.0, "ns"), 
  fp2_tdiff("fp2_tdiff", 17596, -300.0, 300.0, "mm"), 
  fp2_tdiff_ns("fp2_tdiff_ns", 32000, 0.0, 2000.0, "ns"), 
  fp1_tsum("fp1_tsum", 8192, 0.0, 8191.0, "ns"), 
  fp2_tsum("fp2_tsum", 8192, 0.0, 8191.0, "ns"),
  x_avg("x_avg", 17596, -300.0, 300.0, "mm"),
  theta("theta", 600, -3.0, 3.0, "radians"),
  plastic_esum("plastic_esum", 8192, 0.0, 8191.0, "channels"), 
  fp1_y("fp1_y", 65535, 0.0, 65534.0, "channels"), 
  fp2_y("fp2_y", 65535, 0.0, 65534.0, "channels"), 
  phi("phi", 65535, 0.0, 65534.0, "channels"), 
  y_avg("y_avg", 65535, 0.0, 65534.0, "channels"),
  alpha("alpha", 1000, 0.5, 15.0, ""),
  target_Z("target_Z",6,""),
  target_A("target_A",12,""),
  beam_A("beam_A",2,""),
  beam_Z("beam_Z",1,""),
  beam_E("beam_E",16.0,""),
  ejectile_A("ejectile_A",1,""),
  ejectile_Z("ejectile_Z",1,""),
  angle("angle",20.0,"degrees"),
  B_field("B_field",8840,"Gauss"),
  fp1_tcheck("fp1_tcheck", 8192, 0.0, 8191.0, "ns"),
  fp2_tcheck("fp2_tcheck", 8192, 0.0, 8191.0, "ns"),
  anode1_time("anode1_time", 8192, 0.0, 8191.0, "ns"),
  anode2_time("anode2_time", 8192, 0.0, 8191.0, "ns"),
  rf_scint_time("rf_scint_time",8192,-4095,4096,"ns"),

  vmusb_timestamp("vmusb_timestamp",0.0,"ns"),
  digitizer_timestamps("digitizer_timestamps",0.0,"ns",128,0),
  coinc_tracker("coinc_tracker", 3, 0, 2, "0-sabresingles, 1-coincident, 2-fpsingles"),
  E_Ch_Tree_Digitizer("dgtz_sabre",16384,0.0,16383.0,"Digitizer channel",128,0),
  MaxEFront("MaxEFront",16384,0.0,16383.0,"Digitizer channel",5,0),
  MaxEBack("MaxEBack",16384,0.0,16383.0,"Digitizer channel",5,0),*/
  dTFrontBack("dTFrontBack",2001,-4000,4000,"time units")//,5,0)
//  dT_Tree_Digitizer("dT_Tree_Digitizer",16384,-50000,50000,"Time diff in ns",8,0)
{
}
Parameters::~Parameters() {
}

Bool_t Parameters::operator() (const Address_t pEvent,
                              CEvent& rEvent,
			      CAnalyzer& rAnalyzer,
                              CBufferDecoder& rDecoder)
{
  if(m_values[13].isValid() && m_values[20].isValid()) {
    	dTFrontBack = double(m_timestamps[13] - m_timestamps[20]);	
//	dTFrontBack =static_cast<double>(m_timestamps[13]) -  static_cast<double>(m_timestamps[4]); 
//     std::cout << "\n dT: " << dTFrontBack ;//<< " " << m_timestamps[13] << " " << m_timestamps[20] << std::endl;  
//     std::cout << " PSD: " << m_timestamps[13] << std::endl;  

 }

 // if(m_values[20].isValid())
   //  std::cout << " PHA: " << m_timestamps[20] << std::endl;  
  //if(m_values[13].isValid())
    // std::cout << " PSD: " << m_timestamps[13] << std::endl;  
  
 
  
  return kfTRUE;
}
