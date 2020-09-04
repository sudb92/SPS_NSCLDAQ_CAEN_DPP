#  SpecTclGUI save file created Tue Aug 25 14:12:50 CDT 2020
#  SpecTclGui Version: 1.0
#      Author: Ron Fox (fox@nscl.msu.edu)

#Tree params:


# Pseudo parameter definitions


# Tree variable definitions:

treevariable -set PSD_PHA_ts.00 0 ns
treevariable -set PSD_PHA_ts.01 0 ns
treevariable -set PSD_PHA_ts.02 0 ns
treevariable -set PSD_PHA_ts.03 0 ns
treevariable -set PSD_PHA_ts.04 0 ns
treevariable -set PSD_PHA_ts.05 0 ns
treevariable -set PSD_PHA_ts.06 0 ns
treevariable -set PSD_PHA_ts.07 0 ns
treevariable -set PSD_PHA_ts.08 0 ns
treevariable -set PSD_PHA_ts.09 0 ns
treevariable -set PSD_PHA_ts.10 0 ns
treevariable -set PSD_PHA_ts.11 0 ns
treevariable -set PSD_PHA_ts.12 0 ns
treevariable -set PSD_PHA_ts.13 0 ns
treevariable -set PSD_PHA_ts.14 0 ns
treevariable -set PSD_PHA_ts.15 0 ns
treevariable -set PSD_PHA_ts.16 0 ns
treevariable -set PSD_PHA_ts.17 0 ns
treevariable -set PSD_PHA_ts.18 0 ns
treevariable -set PSD_PHA_ts.19 0 ns
treevariable -set PSD_PHA_ts.20 0 ns
treevariable -set PSD_PHA_ts.21 0 ns
treevariable -set PSD_PHA_ts.22 0 ns
treevariable -set PSD_PHA_ts.23 0 ns
treevariable -set PSD_PHA_ts.24 0 ns
treevariable -set PSD_PHA_ts.25 0 ns
treevariable -set PSD_PHA_ts.26 0 ns
treevariable -set PSD_PHA_ts.27 0 ns
treevariable -set PSD_PHA_ts.28 0 ns
treevariable -set PSD_PHA_ts.29 0 ns
treevariable -set PSD_PHA_ts.30 0 ns
treevariable -set PSD_PHA_ts.31 0 ns

# Gate definitions in reverse dependency order
 

# Spectrum Definitions

catch {spectrum -delete 13}
spectrum 13 1 PSD_PHA_e.13 {{0.000000 16383.000000 16384}} long
catch {spectrum -delete 20}
spectrum 20 1 PSD_PHA_e.20 {{0.000000 16383.000000 16384}} long
catch {spectrum -delete dT}
spectrum dT 1 dTFrontBack {{-1000.000000 1000.000000 2001}} long

# Gate Applications: 


#  filter definitions: ALL FILTERS ARE DISABLED!!!!!!!


#  - Parameter tab layout: 

set parameter(select) 1
set parameter(Array)  false

#-- Variable tab layout

set variable(select) 1
set variable(Array)  0
