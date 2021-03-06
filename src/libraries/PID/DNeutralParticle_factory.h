// $Id$
//
//    File: DNeutralParticle_factory.h
// Created: Tue Aug  9 14:29:24 EST 2011
// Creator: pmatt (on Linux ifarml6 2.6.18-128.el5 x86_64)
//

#ifndef _DNeutralParticle_factory_
#define _DNeutralParticle_factory_

#include <JANA/JFactory.h>
#include <PID/DNeutralParticle.h>
#include <PID/DNeutralShower.h>

class DNeutralParticle_factory:public jana::JFactory<DNeutralParticle>{
	public:
		DNeutralParticle_factory(){};
		~DNeutralParticle_factory(){};

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *locEventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *locEventLoop, uint64_t eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.
};

#endif // _DNeutralParticle_factory_

