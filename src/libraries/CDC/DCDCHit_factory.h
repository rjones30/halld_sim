// $Id$
//
//    File: DCDCHit_factory.h
// Created: Tue Aug  6 11:29:56 EDT 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#ifndef _DCDCHit_factory_
#define _DCDCHit_factory_

#include <JANA/JFactory.h>
#include <DAQ/Df125CDCPulse.h>

#include "DCDCHit.h"

using namespace std;
using namespace jana;

class DCDCHit_factory: public jana::JFactory<DCDCHit>{
 public:
  DCDCHit_factory(){};
  ~DCDCHit_factory(){};

  int RemoveCorrelationHits;
  double CorrelationHitsCut;
  // timing cut limits
  double LowTCut;
  double HighTCut;
  int Dissable_CDC_TimingCuts;


 private:
  jerror_t init(void);						///< Called once at program start.
  jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
  jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber);	///< Called every event.
  jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
  jerror_t fini(void);						///< Called after last event of last event source has been processed.

  
};

#endif // _DCDCHit_factory_

