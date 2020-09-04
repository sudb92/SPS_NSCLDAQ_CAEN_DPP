/*
Parameters.cpp

Method for computing new parameters for SpecTcl from raw data; specifically
for the SPS setup

Gordon M.
Nov. 2018

Current Edit: Jan 2019 
Changed adc1 to adc3 for post resoneut
*/
#include "Parameters.h"
#include <BufferDecoder.h>
#include <TCLAnalyzer.h>
#include <stdio.h>
#include "FP_kinematics.h"
#include "chanmap_calibration_sabre.h"
//#include <vector>
using namespace std;

Parameters::Parameters() :
  mtdc_values("tdc2", 65535,0.0, 65534.0, "channels", 32, 0),
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
  MaxEBack("MaxEBack",16384,0.0,16383.0,"Digitizer channel",5,0),
  dTFrontBack("dTFrontBack",-1000,1000,2001,"time units",5,0),
  dT_Tree_Digitizer("dT_Tree_Digitizer",16384,-50000,50000,"Time diff in ns",8,0)
{
}
Parameters::~Parameters() {
}

Bool_t Parameters::operator() (const Address_t pEvent,
                              CEvent& rEvent,
			      CAnalyzer& rAnalyzer,
                              CBufferDecoder& rDecoder)
{
  if(mtdc_values[1].isValid() && mtdc_values[2].isValid()) {
    fp1_tdiff = (mtdc_values[2]-mtdc_values[1])*0.0625/2*1/(1.83);
    fp1_tdiff_ns = (mtdc_values[2]-mtdc_values[1])*0.0625/2;
    fp1_tsum = (mtdc_values[2]+mtdc_values[1])*0.0625;
    if(mtdc_values[5].isValid()) {
      fp1_tcheck = ((mtdc_values[2]+mtdc_values[1])/2-mtdc_values[5])*0.0625;
    }
  }
  if(mtdc_values[3].isValid() && mtdc_values[4].isValid()) {
    fp2_tdiff = (mtdc_values[4]-mtdc_values[3])*0.0625/2*1/(1.969);
    fp2_tdiff_ns = (mtdc_values[4]-mtdc_values[3])*0.0625/2;
    fp2_tsum  = (mtdc_values[4]+mtdc_values[3])*0.0625;
    if(mtdc_values[6].isValid()) {
      fp2_tcheck = ((mtdc_values[3]+mtdc_values[4])/2-mtdc_values[6])*0.0625;
    }
  }
  if(mtdc_values[5].isValid()) {
    anode1_time = (mtdc_values[5])*0.0625;
    if(mtdc_values[8].isValid()) {
      fp1_y = mtdc_values[5] -mtdc_values[8];//Anode time minus cathode time
    }
  }
  if(mtdc_values[6].isValid()) {
    anode2_time = (mtdc_values[6])*0.0625;
    if(mtdc_values[8].isValid()) {
      fp2_y = mtdc_values[6] -mtdc_values[8];//Anode time minus cathode time
    }
  }
  if(fp1_tdiff.isValid() && fp2_tdiff.isValid()) {
    alpha = (Wire_Dist()/2.0 - Delta_Z(target_Z, target_A, beam_Z, beam_A, ejectile_Z, ejectile_A, beam_E, angle, B_field))/Wire_Dist();
    x_avg =  fp1_tdiff*(alpha) + fp2_tdiff*(1.0-alpha);
    theta = (fp2_tdiff - fp1_tdiff)/36.0;
  }
  if(fp1_y.isValid() && fp2_y.isValid()) {
    y_avg =  fp1_y*0.8 + fp2_y*0.2;
    phi = fp2_y - fp1_y;
  }
  if (adc3_values[1].isValid() && adc3_values[9].isValid())
  {
    plastic_esum = sqrt(adc3_values[1]*adc3_values[9]);
  }
  if(mtdc_values[7].isValid() && mtdc_values[9].isValid()) {
    rf_scint_time = fmod((mtdc_values[9]-mtdc_values[7])*0.0625,164.95);
  }

  /* Additions for SABRE coincidences made by B.Sudarsan: sbalak2@lsu.edu */

  /*Identify all active SABRE channels in present event*/
  //std::vector<int> active_channnels;
  int active_channels[128];
  for(int i=0; i<128; i++)  
        active_channels[i] = 0;

  
  for(int i=0; i<128; i++)
  {
	if(E_Ch_Tree_Digitizer[i].isValid())
	{
		active_channels[i] = -1; 
	}
	//active_channels.push_back(i);
  }

   int maxEfront[5] = {-1,-1,-1,-1,-1};
   int maxEback[5] = {-1,-1,-1,-1,-1};
   bool found_front_dgtz[5] = {false,false,false,false,false};
   bool found_back_dgtz[5] = {false,false,false,false,false};
   uint64_t maxFrontTimestamp[5]={0,0,0,0,0};
   uint64_t maxBackTimestamp[5]={0,0,0,0,0};
   bool digitizer_TS_update_array[8] = {false,false,false,false,false,false,false,false};
   int board_num = -1;
  
  //std::cout << "\n";
/*  for( int j=0; j<128; j++)
  if(active_channels[j]==-1)
	   std::cout<< j << ' ';
*/


  for( int j=0; j<128; j++)
  if(active_channels[j]==-1)
//for(auto& chan : active_channels)
{
   int chan = j;
//   std::cout<< chan << ' ';
   channelsabre result = return_channel(chan);
   if((result.detectorside == BackA)||(result.detectorside == BackB)||(result.detectorside == BackC)||(result.detectorside == BackD)||(result.detectorside == BackE))
   {
	switch(result.detectorside)
	{
		case BackA: found_back_dgtz[0] = true; 
				if(E_Ch_Tree_Digitizer[chan]>maxEback[0]) {
				maxEback[0] = E_Ch_Tree_Digitizer[chan];


				maxBackTimestamp[0]=digitizer_timestamps[chan];
				}
				board_num=0;
				break;
		case BackB: found_back_dgtz[1] = true; 
				if(E_Ch_Tree_Digitizer[chan]>maxEback[1]) {
				maxEback[1] = E_Ch_Tree_Digitizer[chan];


				maxBackTimestamp[1]=digitizer_timestamps[chan];
				}
				board_num=1;
				break;

		case BackC: found_back_dgtz[2] = true; 
				if(E_Ch_Tree_Digitizer[chan]>maxEback[2]) {
				maxEback[2] = E_Ch_Tree_Digitizer[chan];


				maxBackTimestamp[2]=digitizer_timestamps[chan];
				}
				board_num=2;
				break;

		case BackD: found_back_dgtz[3] = true;
				if(E_Ch_Tree_Digitizer[chan]>maxEback[3]) {
				maxEback[3] = E_Ch_Tree_Digitizer[chan];


				maxBackTimestamp[3]=digitizer_timestamps[chan];
				}
				board_num=3;
				break;

		case BackE: found_back_dgtz[4] = true; 
			     if(E_Ch_Tree_Digitizer[chan]>maxEback[4]) {
				maxEback[4] = E_Ch_Tree_Digitizer[chan];


				maxBackTimestamp[4]=digitizer_timestamps[chan];
				}break;

	}
   }
   if((result.detectorside == FrontA)||(result.detectorside == FrontB)||(result.detectorside == FrontC)||(result.detectorside == FrontD)||(result.detectorside == FrontE))
   {
	switch(result.detectorside)
	{
		case FrontA: found_front_dgtz[0] = true;
				if(E_Ch_Tree_Digitizer[chan]>maxEfront[0]) {
				maxEfront[0] = E_Ch_Tree_Digitizer[chan];


				maxFrontTimestamp[0]=digitizer_timestamps[chan];
				}break;

		case FrontB: found_front_dgtz[1] = true; 
				if(E_Ch_Tree_Digitizer[chan]>maxEfront[1]) {
				maxEfront[1] = E_Ch_Tree_Digitizer[chan];


				maxFrontTimestamp[1]=digitizer_timestamps[chan];
				}break;

		case FrontC: found_front_dgtz[2] = true; 
				if(E_Ch_Tree_Digitizer[chan]>maxEfront[2]) {
				maxEfront[2] = E_Ch_Tree_Digitizer[chan];


				maxFrontTimestamp[2]=digitizer_timestamps[chan];
				}break;

		case FrontD: found_front_dgtz[3] = true; 
				if(E_Ch_Tree_Digitizer[chan]>maxEfront[3]) {
				maxEfront[3] = E_Ch_Tree_Digitizer[chan];


				maxFrontTimestamp[3]=digitizer_timestamps[chan];
				}break;

		case FrontE: found_front_dgtz[4] = true; 
				if(E_Ch_Tree_Digitizer[chan]>maxEfront[4]) {
				maxEfront[4] = E_Ch_Tree_Digitizer[chan];


				maxFrontTimestamp[4]=digitizer_timestamps[chan];
				}break;

	}


	//bool is_test_channel = (chan==15)||(chan==31)||(chan==47)||(chan==53)||(chan==64)||(chan==80)||(chan==96)||(chan==112);
       //bool is_test_channel = (chan==0)||(chan==16)||(chan==32)||(chan==48)||(chan==64)||(chan==80)||(chan==96)||(chan==112);
       bool is_test_channel = true;
       if(coinc_tracker.isValid())
       {	
       if( is_test_channel && coinc_tracker.isValid() && coinc_tracker==1)// && E_Ch_Tree_Digitizer[chan] >= 9000 && E_Ch_Tree_Digitizer[chan] <=16384 && coinc_tracker.isValid() && coinc_tracker==2)
           {

		int boardid = -1;
                boardid = chan/16;
	        //std::cout << "\n-->" << boardid	 << " " << chan%16 << " " << chan;
/*		switch(chan)
                {
			case 15: boardid = 0; break;
			case 31: boardid = 1; break;
			case 47: boardid = 2; break;
			case 53: boardid = 3; break;
			case 64: boardid = 4; break;
			case 80: boardid = 5; break;
			case 96: boardid = 6; break;
			case 112: boardid = 7; break;
		}*/

/*		switch(chan)
                {
			case 0: boardid = 0; break;
			case 16: boardid = 1; break;
			case 32: boardid = 2; break;
			case 48: boardid = 3; break;
			case 64: boardid = 4; break;
			case 80: boardid = 5; break;
			case 96: boardid = 6; break;
			case 112: boardid = 7; break;
		}
*/
  	       if(boardid>-1)
		{
	       	  dT_Tree_Digitizer[boardid] = double((long int64_t)digitizer_timestamps[chan] - (long int64_t)vmusb_timestamp);	
		  std::cout << "\nChan: " << chan << "dt:" << dT_Tree_Digitizer[boardid];
		}
	   } 	

      }
   }


   }
     
  
   for(int i=0; i<5; i++)
   {
	if((maxEfront[i]!=-1)&&(maxEback[i]!=-1))
        {
		MaxEFront[i] = maxEfront[i];
		MaxEBack[i] = maxEback[i];
		dTFrontBack[i] = (double)((long int64_t)maxBackTimestamp[i] - (long int64_t)maxFrontTimestamp[i]);
	}
    }
  
  
  return kfTRUE;
}
