#  SpecTclGUI save file created Fri Oct 09 16:31:21 CDT 2020
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
treevariable -set PSD_PHA_ts.32 0 ns
treevariable -set PSD_PHA_ts.33 0 ns
treevariable -set PSD_PHA_ts.34 0 ns
treevariable -set PSD_PHA_ts.35 0 ns
treevariable -set PSD_PHA_ts.36 0 ns
treevariable -set PSD_PHA_ts.37 0 ns
treevariable -set PSD_PHA_ts.38 0 ns
treevariable -set PSD_PHA_ts.39 0 ns
treevariable -set PSD_PHA_ts.40 0 ns
treevariable -set PSD_PHA_ts.41 0 ns
treevariable -set PSD_PHA_ts.42 0 ns
treevariable -set PSD_PHA_ts.43 0 ns
treevariable -set PSD_PHA_ts.44 0 ns
treevariable -set PSD_PHA_ts.45 0 ns
treevariable -set PSD_PHA_ts.46 0 ns
treevariable -set PSD_PHA_ts.47 0 ns
treevariable -set PSD_PHA_ts.48 0 ns
treevariable -set PSD_PHA_ts.49 0 ns
treevariable -set PSD_PHA_ts.50 0 ns
treevariable -set PSD_PHA_ts.51 0 ns
treevariable -set PSD_PHA_ts.52 0 ns
treevariable -set PSD_PHA_ts.53 0 ns
treevariable -set PSD_PHA_ts.54 0 ns
treevariable -set PSD_PHA_ts.55 0 ns
treevariable -set PSD_PHA_ts.56 0 ns
treevariable -set PSD_PHA_ts.57 0 ns
treevariable -set PSD_PHA_ts.58 0 ns
treevariable -set PSD_PHA_ts.59 0 ns
treevariable -set PSD_PHA_ts.60 0 ns
treevariable -set PSD_PHA_ts.61 0 ns
treevariable -set PSD_PHA_ts.62 0 ns
treevariable -set PSD_PHA_ts.63 0 ns

# Gate definitions in reverse dependency order
 

# Spectrum Definitions

catch {spectrum -delete 29}
spectrum 29 1 PSD_PHA_e.29 {{0.000000 16383.000000 16384}} long
catch {spectrum -delete 52}
spectrum 52 1 PSD_PHA_e.52 {{0.000000 16383.000000 16384}} long
catch {spectrum -delete dT}
spectrum dT 1 dTFrontBack {{-4000.000000 4000.000000 2001}} long

# Gate Applications: 


#  filter definitions: ALL FILTERS ARE DISABLED!!!!!!!


#  - Parameter tab layout: 

set parameter(select) 1
set parameter(Array)  false

#-- Variable tab layout

set variable(select) 1
set variable(Array)  0
