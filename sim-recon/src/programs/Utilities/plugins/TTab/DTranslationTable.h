// $Id$
//
//    File: DTranslationTable.h
// Created: Thu Jun 27 15:33:38 EDT 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#ifndef _DTranslationTable_
#define _DTranslationTable_

#include <set>
#include <string>
using namespace std;


#include <JANA/JObject.h>
#include <JANA/JFactory.h>
#include <JANA/JEventLoop.h>
#include <JANA/JCalibration.h>
using namespace jana;

#include <DAQ/DModuleType.h>
#include <DAQ/Df250PulseIntegral.h>
#include <DAQ/Df250StreamingRawData.h>
#include <DAQ/Df250WindowSum.h>
#include <DAQ/Df250PulseRawData.h>
#include <DAQ/Df250TriggerTime.h>
#include <DAQ/Df250PulseTime.h>
#include <DAQ/Df250WindowRawData.h>
#include <DAQ/Df125PulseIntegral.h>
#include <DAQ/Df125PulseTime.h>
#include <DAQ/Df125TriggerTime.h>
#include <DAQ/DF1TDCHit.h>
#include <DAQ/DF1TDCTriggerTime.h>

#include <BCAL/DBCALDigiHit.h>
#include <BCAL/DBCALTDCDigiHit.h>
#include <CDC/DCDCDigiHit.h>
#include <FCAL/DFCALDigiHit.h>
#include <FDC/DFDCCathodeDigiHit.h>
#include <FDC/DFDCWireDigiHit.h>
#include <START_COUNTER/DSCDigiHit.h>
#include <START_COUNTER/DSCTDCDigiHit.h>
#include <TOF/DTOFDigiHit.h>
#include <TOF/DTOFTDCDigiHit.h>


class DTranslationTable:public jana::JObject{
	public:
		JOBJECT_PUBLIC(DTranslationTable);
		
		DTranslationTable(JEventLoop *loop);
		~DTranslationTable();
		
		// Each detector system has its own native indexing scheme.
		// Here, we define a class for each of them that has those
		// indexes. These are then used below in the DChannelInfo
		// class to relate them to the DAQ indexing scheme of
		// crate, slot, channel.
		typedef struct{
			uint32_t rocid;
			uint32_t slot;
			uint32_t channel;
		}csc_t;
		
		enum Detector_t{
			UNKNOWN_DETECTOR,
			BCAL,
			CDC,
			FCAL,
			FDC_CATHODES,
			FDC_WIRES,
			PS,
			PSC,
			SC,
			TAGH,
			TAGM,
			TOF
		};
		
		class BCALIndex_t{
			public:
			uint32_t module;
			uint32_t layer;
			uint32_t sector;
			uint32_t end;
		};
		
		class CDCIndex_t{
			public:
			uint32_t ring;
			uint32_t straw;
		};
		
		class FCALIndex_t{
			public:
			uint32_t row;
			uint32_t col;
		};

		class FDC_CathodesIndex_t{
			public:
			uint32_t package;
			uint32_t chamber;
			uint32_t view;
			uint32_t strip;
			uint32_t strip_type;
		};

		class FDC_WiresIndex_t{
			public:
			uint32_t package;
			uint32_t chamber;
			uint32_t wire;
		};

		class PSIndex_t{
			public:
			uint32_t side;
			uint32_t id;
		};

		class PSCIndex_t{
			public:
			uint32_t id;
		};

		class SCIndex_t{
			public:
			uint32_t sector;
		};

		class TAGHIndex_t{
			public:
			uint32_t id;
		};

		class TAGMIndex_t{
			public:
			uint32_t col;
			uint32_t row;
		};

		class TOFIndex_t{
			public:
			uint32_t plane;
			uint32_t bar;
			uint32_t end;
		};
		
		// DChannelInfo holds translation between indexing schemes
		// for one channel.
		class DChannelInfo{
			public:
				csc_t CSC;
				DModuleType::type_id_t module_type;
				Detector_t det_sys;
				union{
					BCALIndex_t bcal;
					CDCIndex_t cdc;
					FCALIndex_t fcal;
					FDC_CathodesIndex_t fdc_cathodes;
					FDC_WiresIndex_t fdc_wires;
					PSIndex_t ps;
					PSCIndex_t psc;
					SCIndex_t sc;
					TAGHIndex_t tagh;
					TAGMIndex_t tagm;
					TOFIndex_t tof;
				};
		};
		
		// Full translation table is collection of DChannelInfo objects
		//map<csc_t, DChannelInfo> TT;
		
		// Methods
		bool IsSuppliedType(string dataClassName) const;
		void ApplyTranslationTable(jana::JEventLoop *loop) const;
		
		// fADC250
		DBCALDigiHit* MakeBCALDigiHit(const BCALIndex_t &idx, const Df250PulseIntegral *pi, const Df250PulseTime *pt) const;
		DFCALDigiHit* MakeFCALDigiHit(const FCALIndex_t &idx, const Df250PulseIntegral *pi, const Df250PulseTime *pt) const;
		DSCDigiHit*   MakeSCDigiHit(  const SCIndex_t &idx,   const Df250PulseIntegral *pi, const Df250PulseTime *pt) const;
		DTOFDigiHit*  MakeTOFDigiHit( const TOFIndex_t &idx,  const Df250PulseIntegral *pi, const Df250PulseTime *pt) const;

		// fADC125
		DCDCDigiHit* MakeCDCDigiHit(const CDCIndex_t &idx, const Df125PulseIntegral *pi, const Df125PulseTime *pt) const;
		DFDCCathodeDigiHit* MakeFDCCathodeDigiHit(const FDC_CathodesIndex_t &idx, const Df125PulseIntegral *pi, const Df125PulseTime *pt) const;

		// F1TDC
		DBCALTDCDigiHit* MakeBCALTDCDigiHit(const BCALIndex_t &idx,      const DF1TDCHit *hit) const;
		DFDCWireDigiHit* MakeFDCWireDigiHit(const FDC_WiresIndex_t &idx, const DF1TDCHit *hit) const;
		DSCTDCDigiHit*   MakeSCTDCDigiHit(  const SCIndex_t &idx,        const DF1TDCHit *hit) const;
		DTOFTDCDigiHit*  MakeTOFTDCDigiHit( const TOFIndex_t &idx,       const DF1TDCHit *hit) const;
		


		void ReadTranslationTable(JCalibration *jcalib=NULL);
		
		template<class T> void CopyToFactory(JEventLoop *loop, vector<T*> &v) const;
		template<class T> void CopyDf250Info(T *h, const Df250PulseIntegral *pi, const Df250PulseTime *pt) const;
		template<class T> void CopyDf125Info(T *h, const Df125PulseIntegral *pi, const Df125PulseTime *pt) const;
		template<class T> void CopyDF1TDCInfo(T *h, const DF1TDCHit *hit) const;

	protected:
		string XML_FILENAME;
		bool NO_CCDB;
		set<string> supplied_data_types;
};

//---------------------------------
// CopyToFactory
//---------------------------------
template<class T>
void DTranslationTable::CopyToFactory(JEventLoop *loop, vector<T*> &v) const
{
	/// Template method for copying values from local containers into
	/// factories. This is done in a template since the type appears
	/// in at least 3 places below. It makes the code calling this
	/// more succinct and therefore easier to add new types.

	// It would be a little safer to use a dynamic_cast here, but
	// all documentation seems to discourage using that as it is
	// inefficient.
	JFactory<T> *fac = (JFactory<T> *)loop->GetFactory(T::static_className());
	if(fac) fac->CopyTo(v);
}

//---------------------------------
// CopyDf250Info
//---------------------------------
template<class T>
void DTranslationTable::CopyDf250Info(T *h, const Df250PulseIntegral *pi, const Df250PulseTime *pt) const
{
	/// Copy info from the fADC250 into a hit object.
	h->pulse_integral = pi->integral;
	h->pulse_time     = pt==NULL ? 0:pt->time;
	h->pedestal       = 0; // for future development
	h->QF             = pi->quality_factor;
	
	h->AddAssociatedObject(pi);
	if(pt) h->AddAssociatedObject(pt);
}

//---------------------------------
// CopyDf125Info
//---------------------------------
template<class T>
void DTranslationTable::CopyDf125Info(T *h, const Df125PulseIntegral *pi, const Df125PulseTime *pt) const
{
	/// Copy info from the fADC125 into a hit object.
	h->pulse_integral = pi->integral;
	h->pulse_time     = pt==NULL ? 0:pt->time;
	h->pedestal       = 0; // for future development
	h->QF             = pi->quality_factor;
	
	h->AddAssociatedObject(pi);
	if(pt) h->AddAssociatedObject(pt);
}

//---------------------------------
// CopyDF1TDCInfo
//---------------------------------
template<class T>
void DTranslationTable::CopyDF1TDCInfo(T *h, const DF1TDCHit *hit) const
{
	/// Copy info from the fADC125 into a hit object.
	h->time = hit->time;
	
	h->AddAssociatedObject(hit);
}


#endif // _DTranslationTable_
