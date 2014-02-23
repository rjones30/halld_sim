#ifndef _DCutActions_
#define _DCutActions_

#include <string>
#include <iostream>
#include <deque>

#include "TRandom3.h"
#include "TMath.h"

#include "JANA/JEventLoop.h"

#include "particleType.h"
#include "TRACKING/DMCThrown.h"
#include "PID/DChargedTrackHypothesis.h"
#include "PID/DNeutralParticleHypothesis.h"

#include "ANALYSIS/DAnalysisAction.h"
#include "ANALYSIS/DParticleCombo.h"
#include "ANALYSIS/DAnalysisUtilities.h"
#include "ANALYSIS/DMCThrownMatching.h"

#include "ANALYSIS/DParticleComboBlueprint_factory.h"

using namespace jana;
using namespace std;

/*
//CLASSES DEFINED BELOW:
DCutAction_MaxNumParticleCombos

DCutAction_PIDFOM
DCutAction_CombinedPIDFOM
DCutAction_TruePID
DCutAction_AllTruePID

DCutAction_AllVertexZ
DCutAction_MaxTrackDOCA
DCutAction_KinFitFOM

DCutAction_MissingMass
DCutAction_MissingMassSquared
DCutAction_InvariantMass
*/

class DCutAction_MaxNumParticleCombos : public DAnalysisAction
{
	public:
		DCutAction_MaxNumParticleCombos(const DReaction* locReaction, unsigned int locMaxNumParticleCombos, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_MaxNumParticleCombos", false, locActionUniqueString), 
		dMaxNumParticleCombos(locMaxNumParticleCombos){}

		string Get_ActionName(void) const;

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);
		inline void Initialize(JEventLoop* locEventLoop){}

		unsigned int dMaxNumParticleCombos;
};

class DCutAction_PIDFOM : public DAnalysisAction
{
	public:
		DCutAction_PIDFOM(const DReaction* locReaction, Particle_t locStepPID, Particle_t locParticleID, double locMinimumConfidenceLevel, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_PIDFOM", false, locActionUniqueString), 
		dStepPID(locStepPID), dParticleID(locParticleID), dMinimumConfidenceLevel(locMinimumConfidenceLevel){}

		string Get_ActionName(void) const;
		inline void Initialize(JEventLoop* locEventLoop){}

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);

		Particle_t dStepPID;
		Particle_t dParticleID;
		double dMinimumConfidenceLevel;
};

class DCutAction_CombinedPIDFOM : public DAnalysisAction
{
	public:
		DCutAction_CombinedPIDFOM(const DReaction* locReaction, double locMinimumConfidenceLevel, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_CombinedPIDFOM", false, locActionUniqueString), dMinimumConfidenceLevel(locMinimumConfidenceLevel){}

		string Get_ActionName(void) const;
		inline void Initialize(JEventLoop* locEventLoop){}

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);

		double dMinimumConfidenceLevel;
};

class DCutAction_TruePID : public DAnalysisAction
{
	public:
		DCutAction_TruePID(const DReaction* locReaction, Particle_t locTruePID, Particle_t locInitialPID, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_TruePID", false, locActionUniqueString), dTruePID(locTruePID), dInitialPID(locInitialPID){}

		inline void Initialize(JEventLoop* locEventLoop){}

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);

		Particle_t dTruePID;
		Particle_t dInitialPID;
};

class DCutAction_AllTruePID : public DAnalysisAction
{
	public:
		DCutAction_AllTruePID(const DReaction* locReaction, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_AllTruePID", false, locActionUniqueString){}

		inline void Initialize(JEventLoop* locEventLoop){}

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);
};

class DCutAction_AllVertexZ : public DAnalysisAction
{
	public:
		DCutAction_AllVertexZ(const DReaction* locReaction, double locMinVertexZ, double locMaxVertexZ, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_AllVertexZ", false, locActionUniqueString), dMinVertexZ(locMinVertexZ), dMaxVertexZ(locMaxVertexZ){}

		string Get_ActionName(void) const;
		inline void Initialize(JEventLoop* locEventLoop){}

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);

		double dMinVertexZ;
		double dMaxVertexZ;
};

class DCutAction_MaxTrackDOCA : public DAnalysisAction
{
	public:
		DCutAction_MaxTrackDOCA(const DReaction* locReaction, Particle_t locInitialPID, double locMaxTrackDOCA, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_MaxTrackDOCA", false, locActionUniqueString), dInitialPID(locInitialPID), dMaxTrackDOCA(locMaxTrackDOCA){}

		string Get_ActionName(void) const;
		void Initialize(JEventLoop* locEventLoop);

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);

		Particle_t dInitialPID;
		double dMaxTrackDOCA;
		const DAnalysisUtilities* dAnalysisUtilities;
};

class DCutAction_KinFitFOM : public DAnalysisAction
{
	public:
		DCutAction_KinFitFOM(const DReaction* locReaction, double locMinimumConfidenceLevel, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_KinFitFOM", true, locActionUniqueString), dMinimumConfidenceLevel(locMinimumConfidenceLevel){}

		string Get_ActionName(void) const;
		inline void Initialize(JEventLoop* locEventLoop){}

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);

		const string dKinFitName;
		double dMinimumConfidenceLevel;
};

class DCutAction_MissingMass : public DAnalysisAction
{
	public:
		DCutAction_MissingMass(const DReaction* locReaction, bool locUseKinFitResultsFlag, double locMinimumMissingMass, double locMaximumMissingMass, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_MissingMass", locUseKinFitResultsFlag, locActionUniqueString), 
		dMinimumMissingMass(locMinimumMissingMass), dMaximumMissingMass(locMaximumMissingMass){}

		string Get_ActionName(void) const;
		void Initialize(JEventLoop* locEventLoop);

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);

		double dMinimumMissingMass;
		double dMaximumMissingMass;
		const DAnalysisUtilities* dAnalysisUtilities;
};


class DCutAction_MissingMassSquared : public DAnalysisAction
{
	public:
		DCutAction_MissingMassSquared(const DReaction* locReaction, bool locUseKinFitResultsFlag, double locMinimumMissingMassSq, double locMaximumMissingMassSq, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_MissingMassSq", locUseKinFitResultsFlag, locActionUniqueString), 
		dMinimumMissingMassSq(locMinimumMissingMassSq), dMaximumMissingMassSq(locMaximumMissingMassSq){}

		string Get_ActionName(void) const;
		void Initialize(JEventLoop* locEventLoop);

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);

		double dMinimumMissingMassSq;
		double dMaximumMissingMassSq;
		const DAnalysisUtilities* dAnalysisUtilities;
};

class DCutAction_InvariantMass : public DAnalysisAction
{
	public:
		DCutAction_InvariantMass(const DReaction* locReaction, Particle_t locInitialPID, bool locUseKinFitResultsFlag, double locMinimumInvariantMass, double locMaximumInvariantMass, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_InvariantMass", locUseKinFitResultsFlag, locActionUniqueString), 
		dInitialPID(locInitialPID), dMinimumInvariantMass(locMinimumInvariantMass), dMaximumInvariantMass(locMaximumInvariantMass){}

		string Get_ActionName(void) const;
		void Initialize(JEventLoop* locEventLoop);

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);

		Particle_t dInitialPID;
		double dMinimumInvariantMass;
		double dMaximumInvariantMass;
		const DAnalysisUtilities* dAnalysisUtilities;
};

class DCutAction_GoodEventRFBunch : public DAnalysisAction
{
	public:
		DCutAction_GoodEventRFBunch(const DReaction* locReaction, bool locCutIfBadRFBunchFlag, string locActionUniqueString = "") : 
		DAnalysisAction(locReaction, "Cut_GoodEventRFBunch", false, locActionUniqueString), 
		dCutIfBadRFBunchFlag(locCutIfBadRFBunchFlag){}

		string Get_ActionName(void) const;
		inline void Initialize(JEventLoop* locEventLoop){}

	private:
		bool Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo);

		bool dCutIfBadRFBunchFlag; //if false, will cut if good rf bunch
};

#endif // _DCutActions_
