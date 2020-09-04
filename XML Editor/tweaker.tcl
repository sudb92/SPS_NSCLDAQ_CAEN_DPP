#!/bin/sh
# Start tclsh \
    exec /usr/bin/tclsh8.6 ${0} ${@}
#/*
#*-------------------------------------------------------------
# 
# CAEN SpA 
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#------------------------------------------------------------
#
#**************************************************************************
#* @note TERMS OF USE:
#* This program is free software; you can redistribute it and/or modify it under
#* the terms of the GNU General Public License as published by the Free Software
#* Foundation. This program is distributed in the hope that it will be useful, 
#* but WITHOUT ANY WARRANTY; without even the implied warranty of 
#* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the 
#* software, documentation and results solely at his own risk.
#*
#* @file      tweaker.tcl
#* @brief     Graphically tweak the parameters of a COMPASS XML file.
#* @author   Ron Fox
#*
#*/

##
# tweaker.tcl
#    This Tcl script allows some of the values for digitizers in the COMPASS
#    configuration file to be tweaked. 
#

#  Information about the program:

set programAuthors {Ron Fox}
set programVersion {v1.0-000}
set programReleaseDate {July 11, 2020}

package require Tk
package require tdom
package require snit

#----------------------- Global data -------------------------------

set xmlFile "";                      # COMPASSdom object -- see below
set title   "";                      # Title manager object.

set GUIColumns 3;                    # Number of controls per column of GUI.

##
# These lists are list of parameters that will be controlled by the
# app.  Each list composed c  pairs. The first element of each pair is
# the XML parameter name, the second the displayed parameter name on the GUI.


# All boards have these parameters.

#              Board level parameters

set commonBoardParameters [list                      \
    [list SRV_PARAM_START_DELAY {Start Delay}]       \
    [list SRV_PARAM_START_MODE  {Start Mode} ]       \
    [list SRV_PARAM_COINC_MODE  {Coincidence mode}]  \
    [list SRV_PARAM_COINC_TRGOUT {Coincidence window} ] \
]

set psdBoardParameters [list                         \
    [list SRV_PARAM_TRGOUT_MODE {Trigger output}]    \
];           # only on PSD boards.
set phaBoardParameters [list                 \
    [list SRV_PARAM_TRGOUT_MODE {Trigger output}] \
];           # only PHA boards have this.

#  Channel level parameters.

set commonChannelParameters [list                               \
    [list SRV_PARAM_CH_ENABLED  {Enabled}]                      \
    [list SRV_PARAM_CH_POLARITY {Input Polarity}]               \
    [list SRV_PARAM_CH_INDYN {Dynamic Range}]                   \
    [list SRV_PARAM_CH_BLINE_DCOFFSET {DC Offset}]              \
    [list SRV_PARAM_CH_THRESHOLD Threshold]                     \
    [list SRV_PARAM_CH_TRG_HOLDOFF {Trigger HOldoff}]           \
]

set psdChannelParameters [list                                 \
    [list SRV_PARAM_CH_CFD_DELAY {CFD Delay}]                   \
    [list SRV_PARAM_CH_CFD_FRACTION {CFD Fraction}]             \
    [list SRV_PARAM_CH_DISCR_MODE  {Discriminator Mode}]        \
    [list SRV_PARAM_CH_CFD_SMOOTHEXP {CFD Smoothing}]          \
    [list SRV_PARAM_CH_GATESHORT {Short Gate}]                  \
    [list SRV_PARAM_CH_GATE      {Long Gate}]                   \
    [list SRV_PARAM_CH_GATEPRE   {Pre-trigger}]                 \
    [list SRV_PARAM_CH_ENERGY_COARSE_GAIN {Coarse-gain}]        \
]

set phaChannelParameters [list                                  \
    [list SRV_PARAM_CH_TTF_DELAY {TTF Delay}]                   \
    [list SRV_PARAM_CH_TTF_SMOOTHING {TTF Smoothing}]           \
    [list SRV_PARAM_CH_TRAP_TFLAT {Trapezoid flattop}]          \
    [list SRV_PARAM_CH_PEAK_NSMEAN {Peak averaging}]            \
    [list SRV_PARAM_CH_TRAP_POLEZERO {Pole zero}]                   \
    [list SRV_PARAM_CH_PEAK_HOLDOFF {Peak holdoff}]            \
    [list SRV_PARAM_CH_TRAP_TRISE   {Rise-time}]                \
    [list SRV_PARAM_CH_TRAP_PEAKING {Peaking time}]             \
]

#
#  This array will be read in from the COMPASS properties file.
#   it will have property names as keys and values as values.
# 
array set properties [list]

# Candidate locations for Property filename:
#  First location is just in the same directory as the script (e.g. testing)
#  or just handed out.  Second location is ../etc relative to the script
#  that's where to expect it if it's installed as part of NSCLDAQ.
#
set propertyFile [list   \
    [file join [file dirname [info script]] i18n_en.properties]   \
    [file join [file dirname [info script]] .. etc i18n_en.properties] \
]
#----------------------- Property handling -------------------------
##
# readProperties
#   Reads the properties file into the properties array.  Property keys will be
#   the array indices.
#   We hunt for the properties file in the locations specified in ::propertyFile
#   If we can't find it, all the translations will just give the fallback values.
#   Which are not terrible for parameter names and are just the XML names of the
#   parameters for enumerated parameter values.
#
proc readProperties {} {
    foreach file $::propertyFile  {
        if {[file readable $file]} {
            set fd [open $file r]
            while {![eof $fd]} {
                set line [gets $fd]
                if {$line ne ""}  {
                    set parsed [split $line =]
                    set ::properties([lindex $parsed 0]) [lindex $parsed 1]
                }
            }
            close $fd            
            break
        }
    }
}
##
# getReadableParameterName
#    Translates a parameter name into a readable parameter name.
#    If there's a property named parameter.name.$paramName the value
#    of that property is used.  If not the fallback name provided is used.
#
# @param paramName - the parameter name to use.
# @param fallback  - The name to use if there's no property.
# @return string   - English name of the parameter
#
proc getReadableParameterName {paramName fallback} {
    set propname parameter.name.$paramName
    if {[array names ::properties $propname] eq ""} {
        return $fallback
    } else {
        return $::properties($propname)
    }
}
##
# getReadableValueName
#   Given a parameter value attempts to translate it into a human readable value.
#   If there's a translation property,  its key will be parameter.value.value-string
#   if there's no translation, we'll return the input value.
#
# @param compassValue - the compass XML value.
# @return string - the human readable parameter value.
#
proc getReadableValueName {compassValue} {
    set propname parameter.value.$compassValue
    if {[array names ::properties $propname] eq ""} {
        return  $compassValue
    } else {
        return $::properties($propname)
    }
}

#----------------------- COMPASSdom class --------------------------
##
# COMPASSdom
#   This class offers high level access to a DOM for a COMPASS XML file.
# Construction:
#   COMPASSdom name|%AUTO% filename.
#
# Mehods:
#   getBoards           - Returns a list of tokens that can be used
#                         to access each board.
#   getBoardDescription - Get a description of a board given a token
#                         from getBoards.
#   getChannels         - Returns a dict of all channel nodes.
#                         keys are channel numbers (indices)
#                         and values are nodes.
#   getChannelValue     - Returns the value of a channel parameter (e.g. SRV_PARAM_CH_ENABLED).
#   getBoardValue       - Get the value of a board level parameter.
#   setChannelValue     - Set the value of a channel parameter (e.g. SRV_PARAM_CH_ENABLED).
#   setBoardValue       - Set the value of a board level parameter
#   getParamDescription - Return information about a parameter.
#   getXML              - Returns the current XML of the dom.
#   wasModiied          - Returns true if the DOM has been changed.
#   getFilename         - Get current filename.
#   resetModified       - Reset the modified flag.
#  OPTIONS:
#   -modifycommand - Provides a script to execute when the DOM Is first modified.
#
snit::type COMPASSdom {
    variable dom;         # Tcl DOM
    variable root;        # document root element handle.
    variable modified 0
    variable filename;   # currrent filepath.
    
    option -modifycommand -default [list]
    
    constructor {args} {
        if {[llength $args] != 1} {
            error "COMPASSdom consructor requires a document file"
        }
        set filename [lindex $args 0]
        set fd [open $filename r]
        set xml [read $fd]
        close $fd
        set dom [dom parse $xml]
        
        #  The only verification we can make at at this time
        #  is that the dom root element is a
        #  <configuration> tag.
        
        set root [$dom documentElement]
        set tag [$root nodeName]
        if {$tag ne "configuration"} {
            error "$filename does not contain a COMPASS configuration file"
        }
    }
    #----------------------------- public methods -----------------------------
    ##
    # getBoards
    #    Gets handles to each board in the system.
    #
    # @return - list of board handles.
    # @note   - <board> tags are immediate children of the root (<configuration>)
    #           node.  We make no assumptions about whether or not there are
    #           other non <board> children:
    #
    method getBoards {} {
        set children [$root childNodes];    # Operations on the board affect underlying dom.
        set result [list]
        foreach child $children {
            if {[$child nodeName] eq "board"} {
                lappend result $child
            }
        }
        return $result
    }
    ##
    # getBoardDescription
    #   Returns a description of a board (e.g. one returned from getBoards)
    #  @param board - a board id e.g. an element of the list returned by getBoards.
    #  @return dict - with the following keys:
    #                 model  - model name of the digitizer.
    #                 serial - model serial number
    #                 dpptype - Type of dpp  (PHA or PSD or waveform).
    #                 connecton - OPTICAL or USB
    #                 link      - Connection link number.
    #                 node      - CONET node number.
    #                 address   - VME base address.
    #                 channels  - Number of channels.
    #        
    #
    method getBoardDescription {board} {
        set result [dict create]
        set children [$board childNodes]
        foreach child $children {
            set tag [$child nodeName]
            
            #  Only do the next bit if the tag is interesting to us:
            
            if {$tag in [list modelName serialNumber linkNum connectionType dppType channelCount address conetNode]} {
                set value [[$child firstChild] nodeValue]
                switch  -exact -- $tag {
                    modelName {
                        dict set result model $value
                    }
                    serialNumber {
                        dict set result serial $value
                    }
                    dppType {
                        if {$value eq "DPP_PSD"} {
                            dict set result dpptype PSD
                        } elseif {$value eq "DPP_PHA"} {
                            dict set result dpptype PHA
                        } else {
                            error "Unrecognized DPP type: $value"    
                        }
                        
                    }
                    connectionType {
                        dict set result connection $value
                    }
                    linkNum {
                        dict set result link $value
                    }
                    conetNode {
                        dict set result node $value
                    }
                    address {
                        dict set result address $value
                    }
                    channelCount {
                        dict set result channels $value
                    }
                }
            }
        }
        
        return $result
    }
    ##
    # getChannels
    #   @param board  The board to get the information from.
    #   @return dict of channel number/value pairs one for each channel in the
    #           digitizer.
    #   @note channel are top level relative to the board node.
    #
    method getChannels {board} {
        set chans [$board getElementsByTagName channel]
        set result [dict create]
        
        foreach channel $chans {
            set numbernode [$channel getElementsByTagName index]
            set number [[$numbernode firstChild] nodeValue]
            dict set result $number $channel
        }
        
        return $result
    }
    ##
    # getChannelValue
    #   Return the current value of a parameter (e.g. SRV_PARAM_CH_ENABLED)
    #   for a channel. This is a bit tricky.  The channel node may have an
    #   explicit setting for the parameter or it may just be defaulted.
    #   If the channel has a value, there will be a subtree in the channel
    #   node of the form:
    # \verbatim
    #    <values>
    #       <entry>
    #          <key>parameter-name (e.g. SRV_PARAM_CH_ENABLED)</key>
    #          <value bunc-o-attributes>parameter-value</value>
    #      ...
    # \endverbatim
    #
    #  If there isn't a node of that form under the channel tree, the value is
    #  gotten from a bunch of default parameter values that are in a subtree
    #  just below the board node of the form:
    #
    # \verbatim
    #     <parameters>
    #        <entry>
    #           <key>parameter-name</key>
    #           <value>
    #              <value bunch-o-attributes>default-value</value>
    #              <descriptor>
    #                 bunch-o-stuff describing min/max/step/units etc.
    #                 that may be useful in setting up GUI elements
    #                 that control that parameter.
    #              </descriptor>
    #           ...
    # @param board    - Board the channel is in (makes finding <parameters> easier/faster).
    # @param chan     - channel in the board.
    # @param name     - Parameter name.  If this does not exist in <parameters> an error is thrown.
    # @return string - parameter value - may have other representations (e.g. numeric)
    # @note it's up to the caller to ensure that the parameter is a channel and not a board
    #       parameter.  Channel parameters have names of the form SRV_PARAM_CH_*
    #       board level pameters will be of the form SRV_PARAM_* where * does not have
    #       CH_ as a substring.
    #
    method getChannelValue {board chan name} {
        set actual [$self _getChannelValue $chan $name]
        if {$actual eq ""} {
            set actual [$self _getParamValue $board $name]
        }
        
        #  If at the end of all of this, the result is nil, it's an error
        
        if {$actual eq ""} {
            error "There is no channel parameter named $name"
        } else {
            return $actual
        }
    }
    
    ##
    # getBoardValue
    #   Returns the value of a board level parameter.
    #   See the DOM tree described in getChannelParameter above.  The
    #   results of this are only gotten from the <parameters> tag subtree
    #
    # @param board - board dom tree root.
    # @param name -  name of the parameter.
    # @return string - value of parameter
    # 
    method getBoardValue {board name} {
        set actual [$self _getParamValue $board $name]
        set {$actual} {
            error "There's no parameter named $name"
        }
        return $actual
    }
    ##
    # setChannelValue
    #   Sets the value of a channel level parameter.  Note that this is a bit
    #   tricky.  If there's already a channel level subtree for this
    #   channel and this parameter it's <value> contents can just be modified.
    #   if not, however, a new dom tree must be created for that channel.
    #
    # @param board   - board to modify
    # @param channel - channel dom tree top to modify.
    # @param name    - name of the parameter to modify.
    # @param value   - New value of the parameter.
    # @note  The caller must ensure this is an actual channel parameter else
    #        an invalid parameter node in the <channel> subtree will get
    #        made/modified
    method setChannelValue {board channel name value} {
        
        #  If there's no channel level dom for this parameter
        #  we need to make a default one we can modify using the
        # board default to get a template.
            
        set valueTag [$self _getChannelParamDom  $channel $name]
        if {$valueTag eq ""} {
            set valueTag \
                [$self _createTemplateChannelParam $board $channel $name]
        }
        # If there's still no entry that means we could not find the parameter
        # at the board level..
            
        if {$valueTag eq "" } {
            error "There is no such parameter named $name"
        }
        #  Now that we have an actual entry for name we can set its value:
        
        set valueNode [$valueTag firstChild]
        $valueNode nodeValue $value
        
        $self _markModified
    }
    ##
    # setBoardValue
    #   Sets the value of a board level parameter.  Note that channel default
    #   values are also board level parameters.  This is the simplest of the
    #   setters, we just have to modify a dom subtree that's already there
    #   rather than making a new one.
    #
    # @param board - board dom tree root.
    # @param name - name of the parameter.
    # @param value - new parameter value.
    #
    method setBoardValue {board name value} {
        set descr [$self _getParamDom $board $name]
        if {$descr eq ""} {
            error "No such parameter description $name"
        }
        # Within the description there are nested <value> tags:
        
        set values [$descr getElementsByTagName value]
        set valuetag [lindex $values 1];              # The nexted tag.
        set valuenode [$valuetag firstChild]
        $valuenode nodeValue $value
        
        $self _markModified
    }
    ##
    # getParamDescription
    #    Get a description of a parameter for board.
    #
    # @param board    - board from which we get the parameter.
    # @param name     - name of the parameter.
    # @return dict    - Dictionary containing the following key/values:
    #                   * type  - Data type e.g. numeric, boolean, selection
    #                   * minimmum - for numeric the minimum allowed value.
    #                   * maximum  - in numeric the maxiumum allowed value.
    #                   * step     - Parameter granularity in numeric parameters.
    #                   * units    - Units of measure in numeric parameters.
    #                   * values   - Valid values for selection parameters.
    #                   * default  - default parameter value.
    # @note an error is thrown if there is no matching parameter defined in this
    #       board
    #   
    method getParamDescription {board name} {
        set descriptor [$self _findDescriptor $board $name]
        if {$descriptor eq ""} {
            error "There is no such parameter named $name"
        }
        #  ALl parameters have a default value:
        
        set dflttag [$descriptor getElementsByTagName defaultValue]
        set defnode [$dflttag firstChild]
        if {$defnode ne ""} {
            set default [$defnode nodeValue]
        } else {
            set default ""
        }
        
        set result [dict create default $default]
        
        # The remainder depends on the type of the parameter:
        
        set typetag [$descriptor getElementsByTagName type]
        set type    [[$typetag firstChild] nodeValue]
        if {$type eq "DOUBLE"} {
            #  Type is numeric. We have minimum, maximum step and units.
            
            set step [[[$descriptor getElementsByTagName step] firstChild] nodeValue]
            set minimum [[[$descriptor getElementsByTagName minimum] firstChild] nodeValue]
            set max [[[$descriptor getElementsByTagName maximum] firstChild] nodeValue]
            
            set unitstag [$descriptor getElementsByTagName udm]
            if {$unitstag ne ""} {
                set units [[$unitstag firstChild] nodeValue]
            } else {
                set units ""
            }
            
            dict set result type numeric
            dict set result step $step
            dict set result minimum $minimum
            dict set result maximum $max
            dict set result units $units
            
        } elseif {$type eq "TEXT"} {
            
            # TEXT elements are choices from a set of <values>.
            
            set choiceTags [$descriptor getElementsByTagName values]
            set choices [list]
            foreach choice $choiceTags {
                set choiceValue [[$choice firstChild] nodeValue]
                lappend choices $choiceValue
            }
            
            dict set result type selection
            dict set result values $choices
            
            
        } elseif {$type eq "BOOLEAN"} {
            # Booleans only have types.
            
            dict set result type boolean
            
        } else {
            error "Parameter $name has an unsupported datatype: $type"
        }
        
        return $result
        
    }
    ##
    # getXML
    #   Return the XML contents of the document.
    #
    # @return text - current xml
    #
    method getXML {} {
        return [$root asXML -indent 2 -xmlDeclaration 1 -encString UTF-8]
    }
    ##
    # wasModified
    #   @return bool - true if the dom has been modified.
    #   @note that this can give a false positive if a
    #                  value was changed and the restored to its original
    #                  value.
    #
    method wasModified {} {
        return $modified
    }
    ##
    # getFilename
    #     @return string - name of file the XML was read from.
    #
    method getFilename {} {
        return $filename
    }
    ##
    # resetModified
    #    Reset the modified flag.  This can be called when the DOM is saved
    #
    method resetModified {} {
        set modified 0
    }
    #--------------------------- private methods ------------------------------
    ##
    # _computeDppType
    #   Given the <amcFirmware> tag node, determines the firmware type.
    #   - Get the firmware major node.
    #   - if the value of that node is 139 -> PHA
    #   - if the value of that node is 136 -> PSD
    #   - Otherwise, at least for now, waveform.
    #
    # @param amcfw  - Amc firmware node tag.
    # @result the firmware type string
    #
    method _computeDppType {amcfw} {
        set major [$amcfw getElementsByTagName major]
        if {[llength $major] == 0} {
            error "<amcFirmware> tag does not have a <major> tag"
        }
        set result [[$major firstChild] nodeValue]
        if {$result eq 136} {
            set result PSD
        } elseif {$result eq 139} {
            set result PHA
        } else {
            set result waveform
        }
        
        return $result
    }
    ##
    # _getChannelValue
    #   Get the value of a parameter from a channel tag
    #
    # @param chan - <channel> tag node.
    # @param name - paramter name (e.g. SRV_PARAM_CH_ENABLED).
    # @return string - parameter value string.
    # @retval ""  - There is no parameter named $name
    #
    method _getChannelValue {chan name} {
        set valueTag [$self _getChannelParamDom $chan $name]
        set result ""
        if {$valueTag ne ""} {
            set result [[$valueTag firstChild] nodeValue]
        } 
        return $result
    }
    ##
    #  _getChannelParamDom
    #     Given the name of a parameter returns either the dom element of the
    #     <value> tag for that parameter or an empty string if there is not one.
    #
    #  @param chan - <channel> tag node.
    #  @param name - Name of the parameter to look for.
    #  @return string - dom node of the <value> subtag.
    #  @retval ""     - There's no matching entry.
    #
    method _getChannelParamDom {chan name} {
        set values [$chan getElementsByTagName values];   # there's always a <values> tag
        set entries [$values getElementsByTagName entry];  #this could be empty though.
        set result ""
        foreach entry $entries {
            #
            # Entries have keys which contain the parameter name:
            #  The value is the value of the parameter.
            
            set key [$entry getElementsByTagName key]
            set pname [[$key firstChild] nodeValue]
            if {$pname eq $name} {
                #
                #  The <value> tag contents are wht we need:
                
                set result [$entry getElementsByTagName value]
                break
            }
            
        }
        return $result
    }
    ##
    # _getParamValue
    #   Get the value of a parameter from the board <parameters>
    #   subtree of the dom.  This could be a board level parameter
    #   or it could be a default value for channel parameter.
    #
    # @param chan  - <board> tag node.
    # @param name   - Name of the parameter e.g. SRV_PARAM_CH_ENABLED
    # @return string - value parameter string.
    # @retval ""    - If there's no matching parameter name.
    #
    method _getParamValue {board name} {
        set result ""
        set entry [$self _getParamDom $board $name]
        if {$entry ne ""} {
            #
            #  getElemnentsByTagName the entire subtree, there'll be 2 matches:
            #  The first match is the outer <value> tag.
            #  The second match is the inner <value> tag which is what we want:
            
            set values [$entry getElementsByTagName value]
            set value [lindex $values 1]
            set result [[$value firstChild] nodeValue]
        }
        return $result
    }
    ##
    # _getParamDom
    #    Get the <entry> dom for a specific board parameter.
    #
    # @param board - board node.
    # @param name  - name fo the parameter string.
    # @return string - id of <entry> of the specified parameter.
    # @retval ""   - no matching parameter name.
    #
    method _getParamDom {board name} {
        set result ""
        set params [$board getElementsByTagName parameters]
        set entries [$params getElementsByTagName entry]
        
        foreach entry $entries {
            set key [$entry getElementsByTagName key]
            set pname [[$key firstChild] nodeValue]
            if {$pname eq $name} {
                return $entry
            }
        }
    }
    ##
    # _createTemplateChannelParam
    #    Creates a template channel value entry for the specified
    #    parmeter type.  The caller must have determined that this
    #    node does not yet exist.
    #    for the value of the parameter, we'll set the default value.
    #
    #    What we need to do is crate an XML fragment node that contains
    #    <entry>
    #      <key>name</key>
    #      <value> ...</value>  -- cloned from the default value node.
    #    </entry>
    #    And insert that as a child to the channel's <values> tag.
    #
    # @param board  - The board dom subtree.
    # @param channel - The channel dom subtree into which we're inserting
    # @param name    - name of the parameter.
    # @return string - <value> tag dom node.
    # @retval ""     - No such parameter defined.
    #
    method _createTemplateChannelParam {board channel name} {
        set boardLvl [$self _getParamDom $board $name]
        if {$boardLvl eq ""} {
            return "";            # no such parameter defined.
        }
        # Let's make the fragments we need:
        
        set entry [$dom createElement entry]
        set key   [$dom createElement key]
        set keytext [$dom createTextNode $name]
        set originalValues [$boardLvl getElementsByTagName value]
        set originalValue  [lindex $originalValues 1]
        set value  [$originalValue clone -deep];    #value attributes and contents.
        
        #  Let's put this all together as a fragment tree:
        
        $entry appendChild $key ;     #<entry<key /></entry>
        $key   appendChild $keytext;  #<entry><key>name</key></entry>
        $entry appendChild $value;     #<entry><key>name</key><value... /></entry
        
        # We have all the fragments we need, now let's get the channel's
        # <value> tag and insert what we need 
        
        set valuestag [$channel getElementsByTagName values]
        $valuestag appendChild $entry
        
        return $value;                # The value dom node as promised.
    }
    ##
    # _findDescriptor
    #    Locate the _descriptor tag that's associated with a specific
    #    board's parameter.  Each board has a <parameters> tag. Within
    #    those tags are <descriptor> tags which describe the
    #    parameters that are legal for *this* board.  This should
    #    not be confused with the set of <paramDescriptor> tags after all
    #   boards which describe the set of parameters that are the union of all
    #   parameters accepted by all boards.
    #
    # @param board - the board for which we're looking up the parameter.
    # @param name  - name of the parameter we're looking up (value of the
    #                <id> tag of the <descriptor>).
    # @return string - dom element handle for the requested parameter.
    # @retval ""     - If there's no match.
    #
    method _findDescriptor {board name} {
        set descriptors [$board getElementsByTagName descriptor]
        set result ""
        foreach descriptor $descriptors {
            set idtag [$descriptor getElementsByTagName id]
            set pname  [[$idtag firstChild] nodeValue]
            if {$pname eq $name} {
                set result $descriptor
                break
            }
        }
        return $result
    }
    ##
    # _markModified
    #    Mark the DOM modified:
    #    - Set the modified variable to 1.
    #    - If previously it was not 1, call the -modifycommand script.
    #
    method _markModified {} {
        set previously $modified
        set modified 1;            # In case script queries state.
        
        if {($previously == 0) && ($options(-modifycommand) ne "")} {
            # We have a transition to modified and a script was established:
            
            uplevel #0 $options(-modifycommand)
        }
    }
}
#--------------------------------Control widgets --------------------------

##
# @class BoolChannelControl
#
#    Provides a control element for a boolean parameter
#
# OPTIONS:
#    -dom                    - The DOM Object containing the parsed COMPASS XML
#    -board                  - The board DOM subelement.
#    -channel                - The channel DOM subelement.
#    -name                 - The parameter name e.g. SRV_CH_PARAM_ENABLED
#
# @note - It is the creators responsibility to ensure the parameter
#         is boolean.  To ensure this use the createControl proc.
#
snit::widgetadaptor BoolChannelControl {
    option -dom -default "" -readonly 1
    option -board -default "" -readonly 1
    option -channel -default "" -readonly 1
    option -name -default "" -readonly 1

    
    delegate option * to hull
    delegate method * to hull
    
    variable value "false"
    
    constructor args {
        installhull using ttk::checkbutton -command [mymethod _toggle] \
            -onvalue true -offvalue false -variable [myvar value]
        
        $self configurelist $args
        
        #  All of the args must be present.
        
        if {$options(-dom) eq ""} {
            error "-dom required to construct BoolControl"
        }
        if {$options(-board) eq ""} {
            error "-board required to construct BoolControl"
        }
        if {$options(-channel) eq ""} {
            error "-channel required to construct BoolControl"
        }
        if {$options(-name) eq ""} {
            error "-name required to construct BoolControl"
        }
        
        #  Set the check button to the current value of the parameter:
        
        set value [$options(-dom) getChannelValue    \
            $options(-board) $options(-channel) $options(-name)]
        
    }
    #---------------------- private methods --------------------
    
    ##
    # _toggle
    #    Called when the checkbutton state changes.
    #    We get the current state and set the parameter accordingly.
    #
    method _toggle {} {
        $options(-dom) setChannelValue $options(-board) $options(-channel) $options(-name) $value
    }
    
}
##
# @class SelectChannelControl
#    Provides a combobox control that controls a channel parameter
#    that is a list of possible values.  A label to the right
#    of the box describes the parameter being controlled in human
#    readable terms.
#
# OPTIONS:
#    -dom   - Object encapsulating the dom.
#    -board - DOM subobject that is the board in the DOM.
#    -channel - DOM subobject that is the channe in the dom.
#    -name  - Name of the parameter in the XML.
#    -text  - Human readable value.
#
snit::widgetadaptor SelectChannelControl {
    component selector
    component title
    
    option -dom -default ""     -readonly 1
    option -board -default ""   -readonly 1
    option -channel -default "" -readonly 1
    option -name -default ""    -readonly 1
    
    delegate option -text to title
    delegate option *     to selector
    delegate method *     to selector
    
    variable value ""
    
    # Look up table from english names -> XML values.
    
    variable xmlValues -array [list] 
    
    constructor args {
        # Use a ttk::frame as the hull.
        
        installhull using ttk::frame
        
        # Create and layout the widgets.
        
        install title using ttk::label $win.title
        install selector using ttk::combobox  $win.selector -width 32 \
            -textvariable [myvar value]
        
        grid $selector $title -sticky w
        
        # Can't configure before installing title because
        # it's where the -text option is implemented.
        
        $self configurelist $args
        
        if {$options(-dom) eq ""} {
            error "-dom is a required option"
        }
        if {$options(-board) eq ""} {
            error "-board is a required option"
        }
        if {$options(-channel) eq ""} {
            error "-chan is a required option"
        }
        if {$options(-name) eq ""} {
            error "-name is a required option"
        }
        set info [$options(-dom) getParamDescription $options(-board) $options(-name)]
        if {[dict get $info type] ne "selection"} {
            error "-name value is not an enumerated parameter"
        }
        set xmlchoices [dict get $info values]
        set choices [list]
        foreach xml $xmlchoices {
            set choice [getReadableValueName $xml]
            lappend choices $choice
            set xmlValues($choice) $xml
        }
        
        $selector configure -values $choices
        
        
        # set the current value of the box.
        
        set xml \
            [$options(-dom) getChannelValue $options(-board) $options(-channel)\
             $options(-name)]
        set value [getReadableValueName $xml]
        $selector set $value
        
        # idiotic though it may seem, there's no -command option
        # for comboboxes so we'll set a write trace on [myvar value]
        # and use it's firing to update the value of the parameter.
        
        trace add variable [myvar value] write [mymethod _onChange]
        
    }
    destructor {
        # Need to kill off the trace -- if the var still exists...:
        # 
        catch {trace remove variable       \
            [myvar value] write [mymethod _onChange]}
        
    }
    #----------------------------  Private methods --------------
    
    ##
    # _onChange
    #   Trace handler for changes to the value variable.
    #   This causes the associated parameter to be modified.
    
    method _onChange {var1 index op} {
        set newValue [$selector get]
        set xml $xmlValues($newValue)
        
        $options(-dom) setChannelValue \
            $options(-board) $options(-channel) $options(-name) $xml
    }
}
    
##
# @class NumericChannelControl
#
#   Provides a controller for a channel with a numeric value.
#   the control will take the form of a spinbox with from, to and increment
#   determined from the parameter descriptor.
#   There will be a fixed label displaying the units (again from the descriptor)
#   and a label that can be configured with human readable
#   names e.g. instead of SRV_PARAM_CH_TRAP_TRISE trap. rise-time.
#
# OPTIONS:
#   -dom    - COMPASSDom that contains the parsed XML>
#   -board  - DOM subelement that is the board.
#   -channel - DOM subelement that is the channel.
#   -name   - Parameter name e.g. SRV_PARAM_CH_TRAP_TRISE
#   -text   - Textual label (e.g. trap. rise-time).
#
snit::widgetadaptor NumericChannelControl {
    component selector
    component units
    component title
    
    option -dom -default ""     -readonly 1
    option -board -default ""   -readonly 1
    option -channel -default "" -readonly 1
    option -name -default ""    -readonly 1
    delegate option -text to title
    delegate method * to selector
    delegate option * to selector
    
    constructor args {
        installhull using ttk::frame
        
        # Create  the widgets.  We can't actually
        # fully configure the selector yet:
        
        install selector using \
            ttk::spinbox $win.select -command [mymethod _onChange]
        install units using ttk::label $win.units
        install title using ttk::label $win.title
        
        # Process the configuration:
        
        $self configurelist $args
        if {$options(-dom) eq ""} {
            error "-dom option is mandatory"
        }
        if {$options(-board) eq ""} {
            error "-board option is mandatory"
        }
        if {$options(-channel) eq ""} {
            error "-channel option is mandatory"
        }
        if {$options(-name) eq ""} {
            error "-name option is mandatory"
        }
        
        
        # Fully configure the units and spinbox based on the
        # parameter description.
        
        set info  [$options(-dom) getParamDescription \
            $options(-board)  $options(-name)]
        if {[dict get $info type] ne "numeric"} {
            error "$options(-name) is not a numeric parameter"
        }
        $selector configure -from [dict get $info minimum] \
            -to [dict get $info maximum] -increment [dict get $info step]
        $units configure -text [dict get $info units]
        
        $selector set [$options(-dom) getChannelValue \
            $options(-board) $options(-channel) $options(-name)]
        
        # Now that everything is configured do the layout.
        
        grid $selector $units $title -sticky w
        
    }
    
    
    #----------------------------- private methods ---------------------
    
    ##
    # _onChange
    #   Spinbox value changed so we need to change the value of the
    #   parameter to match
    #
    method _onChange {} {
        set newValue [$selector get]
        $options(-dom) setChannelValue \
            $options(-board) $options(-channel) $options(-name) $newValue
    }
}
##
# createChannelControl
#   Given a dom board, channel, parameter name/title and widget path,
#   create the appropriate widget type to control that parameter.
#
# @param dom     - COMPASSdom object holding the XML to edit.
# @param board   - the board subdom.
# @param channel - The channel subdom.
# @param param   - name of the parameter (e.g. SRV_PARAM_CH_POLARITY)
# @param title   - English name of the parameter (e.g. polarity)
# @param widget  - Desired widget path.
#
proc createChannelControl {dom board channel param title widget} {
    
   # Get a description of the paramter and based on the type figure out
   # the command we need to execute:
   
   set info [$dom getParamDescription $board $param]
   set type [dict get $info type]
   
   if {$type eq "numeric"} {
    set command NumericChannelControl
   } elseif {$type eq "boolean"} {
    set command BoolChannelControl
   } elseif {$type eq "selection"} {
    set command SelectChannelControl
   } else {
    error "Unrecognized parameter type for $param : $type"
   }
   
    return [$command $widget \
        -dom $dom -board $board -channel $channel -name $param -text $title]
}

#------------------------ board level GUI elements -----------------------

##
#  There's going to be a lot of similarity here between this and the
#  classes that provide encapsulated GUI elements for channel level
#  objects but I'm too stupid to know how to meaningfully know how to
#  factor those comonalities out into a base class.

##
# @class BoolBoardControl
#    Provides a controll element for a board level boolean parameter.
#
# OPTIONS:
#   -dom        - Dom object that holds the XML.
#   -board      - Board Dom identifier/command at the <board> tag.
#   -name       - Parameter name e.g. SRV_PARAM_TR_SW_OUT_PROPAGATE.
#   -text      - English name of the parameter (e.g. prop. trigger).
#
snit::widgetadaptor BoolBoardControl {
    option -dom -default ""   -readonly 1
    option -board -default "" -readonly 1
    option -name -default ""  -readonly 1
    
    delegate option * to hull;      # include -text.
    delegate method * to hull
    
    variable value "false"
    
    constructor args {
        installhull using ttk::checkbutton -command [mymethod _onToggle] \
            -onvalue true -offvalue false -variable [myvar value]
        
        
        $self configurelist $args
        
        if {$options(-dom) eq "" } {
            error "-dom is a required construction option"
        }
        if {$options(-board) eq ""} {
            error "-board is a required construction option"
        }
        if {$options(-name) eq ""} {
            error "-name is a required construction option"
        }
        set value [$options(-dom) getBoardValue $options(-board) $options(-name)]
        
    }
    #------------------------ private methods --------------------------
    
    ##
    # _onToggle
    #    Process clicks in the checkbutton.
    # @note value has the updated value of the checkbutton.
    #
    method _onToggle {} {
        $options(-dom) setBoardValue $options(-board) $options(-name) $value
    }
}
##
# @class SelectBoardControl
#     provides a combobox from which an enumerated parameter type
#     can be selected for a board parameter such as e.g. SRV_PARAM_COINC_MODE
#     (board level coincidence mode).
#
# OPTIONS:
#   -dom        - COMPASSdom object that's being configured.
#   -board      - board identifier within the -dom that we're configuring.
#   -name       - Parameter name (e.g. SRV_PARAM_COINC_MODE).
#   -text       - textual label e.g. "coinc. mode"
#     
snit::widgetadaptor SelectBoardControl {
    component selector
    component title
    
    option -dom -default   "" -readonly 1
    option -board -default "" -readonly 1
    option -name  -default "" -readonly 1
    
    delegate option -text to title
    delegate option *     to selector
    delegate method *     to selector
    
    variable value ""
    variable xmlValues -array [list]
    
    constructor args {
        installhull using ttk::frame
        
        install title using ttk::label $win.title
        install selector using ttk::combobox  $win.selector -width 32 \
            -textvariable [myvar value]
        
        $self configurelist $args
        
        if {$options(-dom) eq ""} {
            error "-dom is required at construction"
        }
        if {$options(-board) eq ""} {
            error "-board is required at construction"
        }
        if {$options(-name) eq ""} {
            error "-name is required at construction"
        }
        # Figure out and configure the selection choices:
        
        set info [$options(-dom) getParamDescription $options(-board) $options(-name)]
        if {[dict get $info type] ne "selection"} {
            error "$options(-name) is not an enumerated parameter"
        }
        
        set xmlchoices [dict get $info values]
        set choices [list]
        foreach xml $xmlchoices {
            set choice [getReadableValueName $xml]
            lappend choices $choice
            set xmlValues($choice) $xml
        }
        
        $selector configure -values $choices
        
        #  Set current value:
         # set the current value of the box.
        
        set xml \
            [$options(-dom) getBoardValue $options(-board) $options(-name)]
        set value [getReadableValueName $xml]
        $selector set $value
        
        # now we can do a meaningful layout:
        
        grid $selector $title -sticky w
        
        # Use a trace on value to know when things have changed:
        
        trace add variable [myvar value] write [mymethod _onChange]
    }
    destructor {
        # Need to kill off the trace -- if the var still exists...:
        # 
        catch {trace remove variable       \
            [myvar value] write [mymethod _onChange]}
        
    }
    #--------------- private methods ---------------------------------------
    
    ##
    # _onChange
    #   The value changed - set the parameter.
    #
    method _onChange {name index op} {
        set newValue [$selector get]
        set xml $xmlValues($newValue)
        
        $options(-dom) setBoardValue $options(-board) $options(-name) $xml
    }
}
##
# @class NumericBoardControl
#     Control a board level numeric parameter. e.g. SRV_PARAM_START_DELAY
#
# OPTIONS:
#  -dom      - COMPASSdom object containing the parsed XML
#  -board    - Board to be controlled in the -dom object.
#  -name     - parameter name e.g. SRV_PARAM_START_DELAY
#  -title    - Human readable parameter e.g. "start delay"
#
snit::widgetadaptor NumericBoardControl {
    component selector
    component units
    component title
    
    option -dom -default ""     -readonly 1
    option -board -default ""   -readonly 1
    option -name -default ""    -readonly 1
    
    delegate option -text to title
    delegate method * to selector
    delegate option * to selector
    
    constructor args {
        installhull using ttk::frame
        install selector using \
            ttk::spinbox $win.select -command [mymethod _onChange]
        install units using ttk::label $win.units
        install title using ttk::label $win.title
        
        # Process the configuration:
        
        $self configurelist $args
        if {$options(-dom) eq ""} {
            error "-dom option is mandatory"
        }
        if {$options(-board) eq ""} {
            error "-board option is mandatory"
        }
        
        if {$options(-name) eq ""} {
            error "-name option is mandatory"
        }
        
        # Fully configure the units and spinbox based on the
        # parameter description.
        
        set info  [$options(-dom) getParamDescription \
            $options(-board)  $options(-name)]
        if {[dict get $info type] ne "numeric"} {
            error "$options(-name) is not a numeric parameter"
        }
        $selector configure -from [dict get $info minimum] \
            -to [dict get $info maximum] -increment [dict get $info step]
        $units configure -text [dict get $info units]
        
        $selector set [$options(-dom) getBoardValue \
            $options(-board)  $options(-name)]
        
        # Now that everything is configured do the layout.
        
        grid $selector $units $title -sticky w
        
        
    }
    #----------------------------- private methods ---------------------
    
    ##
    # _onChange
    #   Spinbox value changed so we need to change the value of the
    #   parameter to match
    #
    method _onChange {} {
        set newValue [$selector get]
        $options(-dom) setBoardValue \
            $options(-board)  $options(-name) $newValue
    }
}

##
# createBoardControl
#    Given a dom, board and parameter name, creates the
#    appropriate control to manage that parameter.
#
# @param dom    - COMPASSdom object being edited.
# @param board  - Board in the DOM
# @param param  - name of the parameter (e.g. SRV_PARAM_START_DELAY).
# @param title  - Human readable title string (e.g. "start delay").
# @param widget - widget path to the GUI object to create
# @return string - widget name.
#
proc createBoardControl {dom board param title widget} {
   # Get a description of the paramter and based on the type figure out
   # the command we need to execute:
   
   set info [$dom getParamDescription $board $param]
   set type [dict get $info type]
   
   if {$type eq "numeric"} {
    set command NumericBoardControl
   } elseif {$type eq "boolean"} {
    set command BoolBoardControl
   } elseif {$type eq "selection"} {
    set command SelectBoardControl
   } else {
    error "Unrecognized parameter type for $param : $type"
   }
   
    return [$command $widget \
        -dom $dom -board $board  -name $param -text $title]
    
}

#----------------------- titleManager class ----------------------
##
# titleManager
#   Manages the title of a top level window. The title will be of the form:
#     application-name ?- filename ?[modified]??
#
#  Options:
#    -appname - (readonly) name of the application leftmost part of the file.
#    -widget  - (readonly) widget path of the top level (must be a toplevel.)
# Methods
#   openFile   - Add a filename to the title.
#   modifyFile - Add [modified] to the title.
#   saveFile   - Remove [modified] from the title if present.
#   closeFile  - Remove the filename (and [modified if present]) from the title.
#
snit::type titleManager {
    option -appname -default "" -readonly 1
    option -widget -default ""  -readonly 1
    
    variable  filename  "";      # non empty if file is open.
    variable  isModified 0;      # boolean true if the file is modified..
    
    constructor args {
        $self configurelist $args
        
        $self _updateTitle
    }
    #---------------------- public methods -----------------------
    ##
    # openFile
    #   - set the new filename.
    #   - clear the isModified flag.
    #   - update the title.
    #
    # @param newFile - filename being opened.
    # #
    method openFile {newFile} {
        set filename $newFile
        set isModified 0
        $self _updateTitle
    }
    ##
    # modifyFile
    #   Set the modify flag. This is not legal when there is no file.
    #
    method modifyFile {} {
        if {$filename eq ""} {
            error "Trying to set the modify flag but there's no file open"
        }
        set isModified 1
        $self _updateTitle
    }
    ##
    # saveFile
    #   clear the modify flag. This is not legal when there's no
    #   file defined.
    #
    method saveFile {} {
        if {$filename eq ""} {_
            error "There is no file to save!"
        }
        set isModified 0
        $self _updateTitle
    }
    ##
    # closeFile
    #   - clear the filename.
    #   - clear the modified flag.
    #   - update the title.
    #
    #  not legal if there's no file to close.
    #
    method closeFile {} {
        if {$filename eq ""} {
            error "There is no file to close"
        }
        set filename ""
        set isModified 0
        $self _updateTitle
    }
    #---------------------- private methods --------------------
    ##
    # _updateTitle
    #   Compute the title and set it in the window title.
    #
    method _updateTitle {} {
        set title $options(-appname)
        if {$filename ne ""} {
            append title " $filename"
            if {$isModified} {
                append  title {  [modified]}
            }
        }
        wm title $options(-widget) $title
    }
}
#------------------------- CONTROL GUI ------------------------------------

##
# createBoardPage
#    Create and fill a frame with the board common controls.
#
# @param widget  - The name of the container widget that should be created.
# @param board   - The board DOM element in the ::xmlFile COMPASSdom
# @param params - The list of pairs of parameter,label to control
# @return string - widget path of the frame we're creating.
# 
proc createBoardPage {widget board params} {
    ttk::labelframe $widget -text {Board Parameters}
    
    set row 0
    set col 0
    set cidx 0
    foreach param $params {
        set xmlName [lindex $param 0]
        set label   [lindex $param 1]
        set label   [getReadableParameterName $xmlName $label]
        set wname   $widget.ctl[incr cidx]
        
        grid [createBoardControl $::xmlFile $board $xmlName $label $wname] \
            -row $row -column $col -sticky w -padx [list 0 5]
        
        if {[incr col] >= $::GUIColumns} {
            set col 0
            incr row
        }
    }    
    return $widget
}
##
# createChanPage
#  Create and fill a frame with the controls for single channel
#
# @param widget   - name of container widget that should be created.
# @param cno      - Channel number could be used to decorate the page.
# @param board    - ::xmlFile board DOM element in which the channel lives.
# @param chan     - ::xmlFile channel DOM element in which the channel lives.
# @param params   - the list of parameter,label pairs that indicate the
#                   things we're going to control and how to label them.
# @return string  - widget path of the frame we're creating ($widget).
#
proc createChanPage {widget cno board chan params} {

    set label "Channel $cno parameters"
    ttk::labelframe $widget -text $label
    
    set row 0
    set col 0
    set cidx 0
    foreach param $params {
        set xmlName [lindex $param 0]
        set label   [lindex $param 1]
        set label   [getReadableParameterName $xmlName $label]
        set wname   $widget.ctl[incr cidx]
        
        grid [createChannelControl $::xmlFile $board $chan $xmlName $label $wname]  \
            -row $row -column $col -sticky w -padx [list 0 5]
        
        if {[incr col] >= $::GUIColumns} {
            set col 0
            incr row
        }
        incr cidx
    }
    return $widget
}
##
# stockBoardNotebook
#
#   Create a tabbed notebook to control a board.
#   The notebook will have a tab that contains board parameters and
#   one tab for each channel in the board.
# @param widget - Name of the widget to create.
# @param board  - Board id in COMPASSdom.
# @return title - Title of the tab in which this notebook can be put.
#
proc stockBoardNotebook {widget board} {
    
    # get the board information and construct the tab title as well
    # as board identifying text at the top of each tab's window.
    
    set info [$::xmlFile getBoardDescription $board]
    set model [dict get $info model]
    set sn    [dict get $info serial]
    set tabTitle "$model SN $sn"
    
    ttk::notebook $widget
    
    #  Figure out the board level and channel level params to control:
    
    set type [dict get $info dpptype]
    set boardParams $::commonBoardParameters
    set chanParams  $::commonChannelParameters
    if {$type eq "PSD"} {
        lappend boardParams {*}$::psdBoardParameters
        lappend chanParams  {*}$::psdChannelParameters
    } elseif {$type eq "PHA"} {
        lappend boardParams {*}$::phaBoardParameters
        lappend chanParams  {*}$::phaChannelParameters
    } else {
        error "Unsupported board DPP type: $type"
    }
    
    $widget add [createBoardPage $widget.board $board $boardParams] -text board
    set chans [$::xmlFile getChannels $board]
    set chno 0
    foreach chan [dict key $chans] {
        set c [dict get $chans $chan]
        $widget add [createChanPage $widget.chan$chno $chno $board $c $chanParams] -text "Chan $chno"
        
        incr chno
    }
    
    return $tabTitle
    
}


##
# teardownGUI
#   By destroying the top level notebook all children in the notebook
#   get destroyed as well.
#
proc teardownGUI {} {
    destroy .topnotebook
}
##
# loadGUI
#   - Create .topnotebook, the top level notebook.
#   - foreach board in the dom add a tab to that notebook that consists
#     of a tabbed notebook itself.
#   - in the board tabs add a tab 'board level' with the board level parameters
#   - and a tab for each channel in the board with channel level parameters.
#
proc loadGUI {} {
    ttk::notebook .topnotebook
    set boards [$::xmlFile getBoards]
    foreach board $boards {
        set bwidget [string tolower $board]
        set tabName [stockBoardNotebook .topnotebook.$bwidget $board]
        .topnotebook add .topnotebook.$bwidget -text $tabName
    }
    grid .topnotebook -sticky nswe
}

    


#-------------------------- menu processing -----------------------------------

##
# onDomModified
#    Called when the DOM is modified sets the modified flag in the title.
#
proc onDomModified {} {
    
     $::title modifyFile
    
}

##
# openXML
#   Open the XML file the caller must ensure there is no open XML file:
#   - The ::xmlFile is set with the object command for the new COMPASSdom
#   - Any standard callbacks are established by this proc.
#   - the title bar is updated with the new filename.
#
# @param filename - name of the XML file to open:
#
proc openXML {filename} {
    set ::xmlFile [COMPASSdom %AUTO% $filename]
    $::xmlFile configure -modifycommand onDomModified    
    $::title openFile $filename

}

##
# offerSave
#   Ask the user if they want to save the file and, if so,
#   invoke file->SaveAs  to prompt for a file to use to save
#   the XML.
#
proc offerSave {} {
    set fname [$xmlFile getFilename]
    set response [tk_messageBox                             \
        -title {Save modified file} -type yesno -icon question \
        -default yes -message "$fname has been modified Save?"
    ]
    if {$response eq "yes"} {
        file->SaveAs
    }
}

##
# help->About
#   Pop up the help about dialog.
#
proc help->About {} {
    set msg "tweaker version $::programVersion\n"
    append msg "Graphically edit CAEN Compass Configuration files\n"
    append msg "Author  : $::programAuthors\n"
    append msg "Released: $::programReleaseDate"
    
    tk_messageBox -icon info -type ok -title "Program information" -message $msg
}
##
# file->SaveAs
#   Ask for the name of a file.
#   and save the XML into that file if selected.
#   Title is told the file was saved.  The XML Is re-read so that
#   it's idea of the filename is updated.
#
proc file->SaveAs {} {
    set fname [tk_getSaveFile                                \
        -title {Save XML file to..} -defaultextension xml    \
        -confirmoverwrite 1                                  \
        -initialfile [$::xmlFile getFilename]                  \
        -filetypes {
            {{XML files}  {.xml}             }
            {{Configuration files} {.config} }
            {{Compass files} {.compass}      }
            {{All files}      *              }
        }                                                   \
    ]
    if {$fname ne ""} {
        
        # Save the file:
        
        set fd [open $fname w]
        set xml [$::xmlFile getXML]
        puts $fd $xml
        close $fd
        #
        #  Close the existing file:
        #
        $::title closeFile
        $::xmlFile destroy
        set $::xmlFile ""
        #
        # Open the saved file:
        #
        openXML $fname
        $::title openFile $fname
    }
}
##
# file->Save
#    Save the XML to the same file.
#    - The XML Is gotten and saved to the current file.
#    - The DOM's modify flag is cleared.
#    - The title is told the file has been saved
#
proc file->Save {} {
    set fname [$::xmlFile getFilename]
    set xml   [$::xmlFile getXML]
    set fd    [open $fname w]
    puts $fd $xml
    close $fd
    
    $::xmlFile resetModified
    $::title   saveFile
}


    


##
# file->Open
#    Open a file:
#    *   If there's a dom and it's been modified prompt user to save it first.
#    *   Prompt the file.
#    *   Save the file if given.
#
proc file->Open {} {
    if {$::xmlFile ne ""} {
        if {[$::xmlFile wasModified]} {
            offerSave
        }
        $::xmlFile destroy
        set ::xmlFile "";            # we no longer have an XML file.
        #
        #  There's no longer an open file - the user could decline to
        #  choose one.
        #
        
        $::title closeFile
            
        .appmenu.filemenu entryconfigure 2 -state disabled
        .appmenu.filemenu entryconfigure 3 -state disabled
    }

    #
    #  Prompt the file to read:
    #
    
    set filename [tk_getOpenFile                         \
        -title {Choose configuration file}               \
        -filetypes {
            {{XML Files}  {.xml }                    }
            {{Configuration files}  {.config}        }
            {{Compass files}        {.compass}       }
            {{ All Files}           *                }
        }                                                \
    ]
    if {$filename ne ""} {
        openXML $filename
        
        .appmenu.filemenu entryconfigure 2 -state normal
        .appmenu.filemenu entryconfigure 3 -state normal
    }
    
    #  Tear down any existing GUI and set up the GUI associated with the (new) DOM
    
    teardownGUI
    loadGUI
}
    
##
# file->Exit
#   If the there's a modified DOM, offer to save it.
#   Exit the program.
#
proc file->Exit {} {
    if {$::xmlFile ne ""} {
        if {[$::xmlFile wasModified]} {
            offerSave
        }
    }
    exit 0
}
##
# file->Quit
#    Exit without offering to save.
#
proc file->Quit {} {
    exit 0
}


##
# createAppMenu
#    Create the application menu. This consists of the following:
#
#  File menu:
#    -  Open    - Open a COMPASS configuration file
#    -  Save    - Save current configuration file back to original file.
#    -  Save As.. - Save current configuration file to a different file.
#    -  Exit    - Exit the document.  If modified prompt to save.
#    -  Quit    - Exit unconditionally without propmting.
#
# Help menu:
#    About - Current version of help menu.
#
proc createAppMenu {} {
    menu .appmenu

    menu .appmenu.filemenu -tearoff 0
    .appmenu add cascade -label File -menu .appmenu.filemenu
    .appmenu.filemenu add command -label {Open...} -accelerator Ctrl+O -command file->Open
    .appmenu.filemenu add separator
    .appmenu.filemenu add command -label {Save}    -accelerator   Ctrl+S -state disabled -command file->Save
    .appmenu.filemenu add command -label {Save As...} -accelerator Ctrl+Shift+S -state disabled -command file->SaveAs
    .appmenu.filemenu add separator
    .appmenu.filemenu add command -label {Exit}       -accelerator Ctrl+X -command file->Exit
    .appmenu.filemenu add command -label Quit -command file->Quit

    bind . <Control-Key-o> file->Open    
    bind . <Control-Key-x> file->Exit
    bind . <Control-Key-s> file->Save
    bind . <Control-Key-S> file->SaveAs

    menu .appmenu.helpmenu -tearoff 0
    .appmenu add cascade -label Help -menu .appmenu.helpmenu
    .appmenu.helpmenu add command -label About... -command help->About
    
    . configure -menu .appmenu
}



set title [titleManager %AUTO% -appname tweaker -widget .]
createAppMenu
readProperties

    

    

    

    
