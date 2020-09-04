/*
Parameters.h

Method for computing new parameters for SpecTcl from raw data; specifically
for the SPS setup

Original program by:
Gordon M.
Nov. 2018  

SABRE array capabilities added by Sudarsan B.
Aug 2019
*/

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <config.h>
#include <EventProcessor.h>
#include <TreeParameter.h>
#include <stdio.h>
	


class Parameters : public CEventProcessor
{
  public:
    Parameters();
    virtual ~Parameters();

  virtual Bool_t operator() (const Address_t pEvent,
		 	      CEvent&         rEvent,
			      CAnalyzer&      rAnalyzer,
			      CBufferDecoder& rDecoder);
  
  private:
    CTreeParameterArray m_values;
    CTreeVariableArray m_timestamps;
/*    CTreeParameterArray adc3_values; 
    CTreeParameterArray adc2_values; 
    CTreeParameterArray adc1_values; 
    CTreeParameter fp1_tdiff; 
    CTreeParameter fp1_tdiff_ns; 
    CTreeParameter fp1_tsum;
    CTreeParameter fp2_tsum;
    CTreeParameter fp2_tdiff;
    CTreeParameter fp2_tdiff_ns;
    CTreeParameter x_avg;
    CTreeParameter theta;
    CTreeParameter plastic_esum;
    CTreeParameter fp1_y;
    CTreeParameter fp2_y;
    CTreeParameter phi;
    CTreeParameter y_avg;
    CTreeParameter fp1_tcheck;
    CTreeParameter fp2_tcheck;
    CTreeParameter w1_right_minus_tcheck1;
    CTreeParameter w1_left_minus_tcheck1;
    CTreeParameter anode1_time;
    CTreeParameter anode2_time;
    CTreeParameter rf_scint_time;
    CTreeParameter alpha;
    CTreeVariable target_Z;
    CTreeVariable target_A;
    CTreeVariable beam_A;
    CTreeVariable beam_Z;
    CTreeVariable beam_E;
    CTreeVariable ejectile_A;
    CTreeVariable ejectile_Z;
    CTreeVariable angle;
    CTreeVariable B_field;

    /*SABRE specific derived parameters go here
    CTreeParameterArray  E_Ch_Tree_Digitizer; //Convert the parsed digitizer E,Ch data into a tree node
    CTreeParameterArray MaxEFront; // Array of 5 storing max front signals seen on each detector if it exists
    CTreeParameterArray MaxEBack;  // Array of 5 storing max back signals seen on each detector if it exists
    CTreeParameterArray  dT_Tree_Digitizer; //Convert the parsed digitizer dT, T data into a tree node*/
    CTreeParameter dTFrontBack;
/*    CTreeParameter coinc_tracker;

    CTreeVariable vmusb_timestamp;
    CTreeVariableArray digitizer_timestamps;
*/ 

};

#endif
