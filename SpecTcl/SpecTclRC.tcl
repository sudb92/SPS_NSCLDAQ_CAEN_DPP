#    This software is Copyright by the Board of Trustees of Michigan
#    State University (c) Copyright 2005.
#
#    You may use this software under the terms of the GNU public license
#    (GPL).  The terms of this license are described at:
#
#     http://www.gnu.org/licenses/gpl.txt
#
#    Author:
#             Ron Fox
#	     NSCL
#	     Michigan State University
#	     East Lansing, MI 48824-1321


# (C) Copyright Michigan State University 2014, All rights reserved 
#
#
#  Setup the standard scripted commandsin SpecTcl.
#


#  Access SpecTcl Packages, 
#  Load splash and jpeg support:

lappend auto_path $SpecTclHome/TclLibs
package require splash
package require img::jpeg



set splash [splash::new -text 1 -imgfile $splashImage -progress 6 -hidemain 0]
splash::progress $splash {Loading button bar} 0

puts -nonewline "Loading SpecTcl gui..."
source $SpecTclHome/Script/gui.tcl
puts  "Done."

splash::progress $splash {Loading state I/O scripts} 1

puts -nonewline "Loading state I/O scripts..."
source $SpecTclHome/Script/fileall.tcl
puts "Done."

splash::progress $splash {Loading formatted listing scripts} 1

puts -nonewline "Loading formatted listing scripts..."
source $SpecTclHome/Script/listall.tcl
puts "Done."

splash::progress $splash {Loading gate copy scripts} 1

puts -nonewline "Loading gate copy script procs..."
source $SpecTclHome/Script/CopyGates.tcl
puts "Done."

splash::progress $splash {Loading tkcon console} 1

if {$tcl_platform(os) != "Windows NT"} {
	puts -nonewline "Loading TKCon console..."
	source $SpecTclHome/Script/tkcon.tcl
	puts "Done."
}

splash::progress $splash {Loading SpecTcl Tree Gui} 1

puts -nonewline "Starting treeparamgui..."
source $SpecTclHome/Script/SpecTclGui.tcl
puts " Done"


splash::progress $splash {SpecTcl ready for use} 1

splash::config $splash -delay 2000
