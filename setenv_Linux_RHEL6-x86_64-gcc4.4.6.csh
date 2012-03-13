#!/bin/tcsh
 
# This file was generated by the script "mk_setenv.csh"
#
# Generation date: Tue Mar 13 15:46:56 EDT 2012
# User: gluex
# Host: roentgen.jlab.org
# Platform: Linux roentgen.jlab.org 2.6.32-220.4.2.el6.x86_64 #1 SMP Mon Feb 6 16:39:28 EST 2012 x86_64 x86_64 x86_64 GNU/Linux
# BMS_OSNAME: Linux_RHEL6-x86_64-gcc4.4.6
 
# Make sure LD_LIBRARY_PATH is set
if ( ! $?LD_LIBRARY_PATH ) then
   setenv LD_LIBRARY_PATH
endif
 
# HALLD
setenv HALLD_HOME /group/halld/Software/builds/sim-recon/sim-recon-2012-03-12
setenv HDDS_HOME /group/halld/Software/builds/hdds/hdds-1.2
setenv BMS_OSNAME Linux_RHEL6-x86_64-gcc4.4.6
setenv PATH ${HALLD_HOME}/bin/${BMS_OSNAME}:$PATH
 
# JANA
setenv JANA_HOME /group/12gev_phys/builds/jana_0.6.3/Linux_RHEL6-x86_64-gcc4.4.6
setenv JANA_CALIB_URL file:///group/halld/Software/calib/latest
setenv JANA_GEOMETRY_URL xmlfile://${HDDS_HOME}/main_HDDS.xml
setenv JANA_PLUGIN_PATH ${JANA_HOME}/lib
setenv PATH ${JANA_HOME}/bin:$PATH
 
# ROOT
setenv ROOTSYS /apps/root/5.30.00/root
setenv LD_LIBRARY_PATH ${ROOTSYS}/lib:$LD_LIBRARY_PATH
setenv PATH ${ROOTSYS}/bin:$PATH
 
# CERNLIB
setenv CERN /apps/cernlib/x86_64_rhel6
setenv CERN_LEVEL 2005
setenv LD_LIBRARY_PATH ${CERN}/${CERN_LEVEL}/lib:$LD_LIBRARY_PATH
setenv PATH ${CERN}/${CERN_LEVEL}/bin:$PATH
 
# Xerces
setenv XERCESCROOT /group/halld/Software/ExternalPackages/xerces-c-src_2_7_0.Linux_RHEL6-x86_64-gcc4.4.6
setenv LD_LIBRARY_PATH ${XERCESCROOT}/lib:$LD_LIBRARY_PATH
 
