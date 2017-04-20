#include "ANALYSIS/DVertexCreator.h"

namespace DAnalysis
{

DVertexCreator::DVertexCreator(JEventLoop* locEventLoop)
{
	//INITIALIZE KINFITUTILS
	dKinFitUtils = new DKinFitUtils_GlueX(locEventLoop);
	dKinFitUtils->Set_IncludeBeamlineInVertexFitFlag(true);

	//CONTROL
	dUseSigmaForRFSelectionFlag = false;

	//GET THE GEOMETRY
	DApplication* locApplication = dynamic_cast<DApplication*>(locEventLoop->GetJApplication());
	DGeometry* locGeometry = locApplication->GetDGeometry(locEventLoop->GetJEvent().GetRunNumber());

	//TARGET INFORMATION
	locGeometry->GetTargetZ(dTargetCenterZ);
	double locTargetLength = 30.0;
	locGeometry->GetTargetLength(locTargetLength);

	//BEAM BUNCH PERIOD
	vector<double> locBeamPeriodVector;
	locEventLoop->GetCalib("PHOTON_BEAM/RF/beam_period", locBeamPeriodVector);
	dBeamBunchPeriod = locBeamPeriodVector[0];

	//GET TRACKING HYPOTHESES
	vector<int> locHypotheses = {PiPlus, KPlus, Proton, PiMinus, KMinus};
	ostringstream locMassStream;
	for(size_t loc_i = 0; loc_i < locHypotheses.size(); ++loc_i)
	{
		locMassStream << locHypotheses[loc_i];
		if(loc_i != (locHypotheses.size() - 1))
			locMassStream << ",";
	}
	string HYPOTHESES = locMassStream.str();
	gPARMS->SetDefaultParameter("TRKFIT:HYPOTHESES", HYPOTHESES);

	// Parse MASS_HYPOTHESES strings to make list of Particle_t's
	locHypotheses.clear();
	SplitString(HYPOTHESES, locHypotheses, ",");
	for(size_t loc_i = 0; loc_i < locHypotheses.size(); ++loc_i)
		dTrackingPIDs.push_back(Particle_t(locHypotheses[loc_i]));
	std::sort(dTrackingPIDs.begin(), dTrackingPIDs.end()); //so that can search later

	//INITIALIZE MISCELLANEOUS MEMBERS
	dESSkimData = nullptr;
	dEventRFBunch = nullptr;

	//SETUP dReactionVertexInfoMap
	vector<const DReactionVertexInfo*> locVertexInfos;
	locEventLoop->Get(locVertexInfos);
	for(auto locVertexInfo : locVertexInfos)
		dReactionVertexInfoMap.emplace(locVertexInfo->Get_Reaction(), locVertexInfo);
}

void DVertexCreator::Do_All(JEventLoop* locEventLoop, const vector<const DReaction*>& locReactions)
{
	/****************************************************** DESIGN MOTIVATION ******************************************************
	*
	* Creating all possible combos can be very time- and memory-intensive if not done properly.
	* For example, consider a 4pi0 analysis and 20 (N) reconstructed showers (it happens).
	* If you make all possible pairs of photons (for pi0's), you get 19 + 18 + 17 + ... 1 = (N - 1)*N/2 = 190 pi0 combos.
	* Now, consider that you have 4 pi0s: On the order of 190^4: On the order of a billion combos (although less once you guard against duplicates)
	*
	* So, we must do everything we can to reduce the # of possible combos in ADVANCE of actually attempting to make them.
	* And, we have to make sure we don't do anything twice (e.g. two different users have 4pi0s in their channel).
	* Therefore, to optimize the time and memory usage, we must do the following:
	*
	* 1) Re-use comboing results between DReactions.
	*    If working on each DReaction individually, it is difficult (takes time & memory) to figure out what has already been done, and what to share
	*    So instead, first break down the DReactions to their combo-building components, and share those components.
	*    Then build combos out of the components, and distribute the results for each DReaction.
	*
	* 2) Reduce the time spent trying combos that we can know in advance won't work.
	*    We can do this by placing cuts IMMEDIATELY on:
	*    a) Time difference between charged tracks
	*    b) Time difference between photons and possible RF bunches (discussed more below).
	*    c) Invariant mass cuts for various decaying particles (e.g. pi0, eta, omega, phi, lambda, etc.)
	*    Also, when building combos of charged tracks, we could only loop over PIDs of the right type, rather than all hypotheses
	*
	* 3) The only way to do both 1) and 2) is to make the loose time & mass cuts reaction-independent.
	*    Users can always specify reaction-dependent tighter cuts later, but they cannot specify looser ones.
	*    However, these cuts should be tweakable on the command line in case someone wants to change them.
	*
	*******************************************************************************************************************************/

	//Make initial delta-t cuts to select RF bunches for every neutral shower
	//Still need to place tighter cuts, calculate chisq on vert-by-vert basis


	//Building combos:
	//All combos: Decompose into vertices, group vertices by common final-state charged tracks
	//That way, the following exercises (for charged tracks) are not repeated for combos that have similar vertices but different #gammas

	//For each production vertex:
	//Make all combos of charged particles that are in time
	//Calculate vertex positions
	//Charged tracks vote on RF bunch: keep tally of votes/delta-t's, if they don't all match with at least one, cut

	//PICK RF_BUNCH / NEUTRALS FOR EACH PHOTOPRODUCTION VERTEX
	//For each photon at the production vertex, compute delta-t's, do PID cuts with all possible RF bunch choices: save N_shifts/delta-t's of those that pass cuts
	//Split each vertex: in terms of Nphotons at vertex
		//for each split: choose rf bunch, do PID cuts

	//Channel-dependent:
	//Fully-charged inv-mass cuts at that vertex
	//for each remaining N_shifts (and the photons for them): neutral inv mass cuts

	/**************************************************** COMBOING CHARGED TRACKS **************************************************
	*
	*******************************************************************************************************************************/


	/*
	 * Loop over DReactions:
	 * 1) Charged track comboing: All vertices
	 * 2) Production vertex calculation
	 * 3) Production vertex charged hypo RF bunch selection (time cuts) //as you go!
	 * 4) On-demand, vertex-z binned photon comboing
	 * 5) Production vertex neutral RF bunch selection (time cuts)
	 * 6) Final RF bunch vote
	 * 7)
	 */

	/**************************************************************** DETACHED VERTICES ***************************************************************/


	//First consider an OK scenario: g, p -> K+, Lambda
		//The K+ is not ideal, but is in theory sufficient to determine the photoproduction vertex
		//Problem: If K+ is low-theta (and it is), difficult to tell POCA to beamline: vertex-z could be way off
		//However, the K+ has beta = ~1, so even if the vertex-z is off, the RF - K+ time will be nearly the same regardless of vertex-z
	//Anyway, the lambda information is not needed to determine the production vertex, or vice versa, so the 2 vertices can be determined independently
		//Therefore, their linkage will NOT be recorded below, because it is not needed.
	//Why not record linkage anyway?  Because we are combining all channels together, and someone could also be studying:
		//g, p -> K+, Sigma0     and/or     g, p -> K0, pi+, Lambda
		//And both/one-of the vertices we calculated for g, p -> K+, Lambda can be re-used for these, as long as we don't force that linkage

	//Now consider: g, p -> (K+), Lambda
		//Now you must use decaying particles to define the production vertex

	//Now consider an ugly scenario: g, p -> K0, Sigma+
		//Both particles have detached vertices, and there are no charged tracks at the production vertex
		//To do this, you first have to find the K0 decay vertex, use the K0 trajectory to get the production vertex,
			//use missing mass to get the sigma+ trajectory, and then use the sigma+ & proton to get the sigma+ decay vertex
		//you can't get the sigma+ trajectory (and therefore vertex) without the beam photon!!
		//and you need the sigma+ decay vertex to compute the photon timing & p4

	//Now consider a nightmare scenario: g, p -> K0, pi0, Sigma+
		//Here, you can't even compute the Sigma+ missing mass without doing all possible photon combos

	//Now consider an impossible scenario: g, p -> K0, pi0, Sigma+ with missing pi-
		//Here, you can't even get a production vertex //just pick the center of the target

	//So, what to do?
		//Certainly the end results should be accurate
		//But, we also certainly can't afford to postpone neutral PID & invariant mass cuts until a beam photon is chosen: too many combos, too much memory & time
	//So, we assume that once we have the production vertex, that using it is sufficient for calculating temporary neutral p4 & times (for cuts)
		//This assumption works as long as the cuts are loose
		//Where the assumption is tested the most: when the detached vertex is significantly downstream in z from the production vertex, and photons are at 90 degrees
		//Say a loose timing cut of 1ns: this corresponds to 30cm ... and we can have detached vertices of 15cm+
	//So for neutrals at detached vertices:
		//Where the vertex position is unknown: evaluate as if they came from the production vertex, and increase the timing cut width by 0.5ns
		//Where the vertex position is known: propagate the RF time to the vertex-z of the detached vertex, and increase the timing cut width by 0.1ns
			//0.01*delta-z: The decaying particle does not have beta = 1, so propagating the RF time isn't perfect
				//In general, the momentum of the decaying particle may not be known without the neutrals or the beam photon, so cannot use it
	//For any particle where the production vertex is unknown (center of target):
		//Increase timing cut width by 0.5*targ_length/c (RF could be anywhere)







	//When to combo-in the detached vertices?
		//Once production vertex is chosen

	//When to create "combo steps"? //e.g. go to DReaction-basis
		//Just before kinfit

	//Then, with the vertices:
		//In general
	locEventLoop->Get(dChargedTracks, "Combo");
	locEventLoop->Get(dNeutralShowers, dShowerSelectionTag.c_str());
    locEventLoop->GetSingle(dEventRFBunch);

    vector<const DESSkimData*> locESSkimDataVector;
    locEventLoop->Get(locESSkimDataVector);
    dESSkimData = locESSkimDataVector.empty() ? NULL : locESSkimDataVector[0];

	Sort_ChargedTracks();
	Build_VertexInfos(locEventLoop, locReactions);
	Find_VertexCombos();
}


void DVertexCreator::Build_VertexInfos(JEventLoop* locEventLoop, const vector<const DReaction*>& locReactions)
{
	dAllVertexInfos_Set.clear();
	dAllVertexInfos_Vector.clear();
	for(auto& locReaction : locReactions)
	{
		if(!Check_NumParticles(locReaction))
			continue;
//Skim check commented until skims are ready!
//		if(!Check_Skims(locReaction))
//			continue;
		Build_VertexInfos(locReaction);
	}
}

void DVertexCreator::Sort_ChargedTracks(void)
{
	dPositiveChargedTracks.clear();
	dNegativeChargedTracks.clear();
	for(auto& locTrackIterator = dChargedTracks.begin(); locTrackIterator != dChargedTracks.end(); ++locTrackIterator)
	{
		//sort charged particles into +/-
		//Note that a DChargedTrack object can sometimes contain both positively and negatively charged hypotheses simultaneously:
		//sometimes the tracking flips the sign of the track
		const DChargedTrack* locChargedTrack = *locTrackIterator;
		if(locChargedTrack->Contains_Charge(1))
			dPositiveChargedTracks.push_back(locChargedTrack);
		if(locChargedTrack->Contains_Charge(-1))
			dNegativeChargedTracks.push_back(locChargedTrack);

		//save hypos by PID (with this, it's faster to query possible hypos later)
		for(auto& locChargedHypo : locChargedTrack->dChargedTrackHypotheses)
			dTrackMap_ByPID[locChargedHypo->PID()].push_back(locChargedHypo);
	}

	//sort tracks so can do faster set_intersection's later
	for(auto& locPIDTrackPair : dTrackMap_ByPID)
		std::sort(locPIDTrackPair.second.begin(), locPIDTrackPair.second.end());
	for(auto& locTrackDeltaTPair : dTrackMap_DeltaTMatch)
	{
		for(auto& locPIDPair : locTrackDeltaTPair.second)
			std::sort(locPIDPair.second.begin(), locPIDPair.second.end());
	}

	if(dDebugLevel > 0)
		cout << "#+, #-, #0 particles = " << dPositiveChargedTracks.size() << ", " << dNegativeChargedTracks.size() << ", " << dNeutralShowers.size() << endl;
}

bool DVertexCreator::Check_NumParticles(const DReaction* locReaction) const
{
	//see if enough particles were detected to build this reaction
	//locChargeFlag: 0/1/2/3/4 for all, charged, neutral, q+, q- particles
	deque<Particle_t> locDesiredDetectedPIDs;

	locReaction->Get_DetectedFinalPIDs(locDesiredDetectedPIDs, 3, true); //q+, include duplicate pids
	if(locDesiredDetectedPIDs.size() > dPositiveChargedTracks.size())
		return false; //not enough q+ tracks

	locReaction->Get_DetectedFinalPIDs(locDesiredDetectedPIDs, 4, true); //q-, include duplicate pids
	if(locDesiredDetectedPIDs.size() > dNegativeChargedTracks.size())
		return false; //not enough q- tracks

	locReaction->Get_DetectedFinalPIDs(locDesiredDetectedPIDs, 2, true); //neutrals, include duplicate pids
	return (locDesiredDetectedPIDs.size() <= dNeutralShowers.size())
}

bool DVertexCreator::Check_Skims(const DReaction* locReaction) const
{
	if(dESSkimData == nullptr)
		return true;

	string locReactionSkimString = locReaction->Get_EventStoreSkims();
	vector<string> locReactionSkimVector;
	SplitString(locReactionSkimString, locReactionSkimVector, ",");
	for(size_t loc_j = 0; loc_j < locReactionSkimVector.size(); ++loc_j)
	{
		if(!dESSkimData->Get_IsEventSkim(locReactionSkimVector[loc_j]))
			return false;
	}

	return true;
}

bool DVertexCreator::Cut_TrackDeltaT(const DChargedTrackHypothesis* locNewChargedHypo, const DChargedTrackHypothesis* locSavedChargedHypo)
{
	//ultimately, the PID delta-t cuts determine the range:
		//max delta-t = PID_cut_track1 + PID_cut_track2

	//first, get the PID timing cuts
	auto locCutIterator_SavedHypo = dPIDTimeCutMap.find(make_pair(locSavedChargedHypo->PID(), locSavedChargedHypo->t1_detector()));
	auto locCutIterator_NewHypo = dPIDTimeCutMap.find(make_pair(locNewChargedHypo->PID(), locNewChargedHypo->t1_detector()));
	if((locCutIterator_SavedHypo == dPIDTimeCutMap.end()) || (locCutIterator_NewHypo == dPIDTimeCutMap.end()))
		return true; //no delta-t cut: can't cut: true

	//evaluate delta-t
	double locDeltaT = locSavedChargedHypo->time() - locNewChargedHypo->time();
	double locMaxDeltaT = locCutIterator_NewHypo->second + locCutIterator_NewHypo->second;
	return (fabs(locDeltaT) <= locMaxDeltaT);
}




unordered_map<int, double> DVertexCreator::Calc_NeutralRFDeltaTs(const DNeutralShower* locNeutralShower, const TVector3& locVertex, double locRFTime) const
{
	//calc vertex time, get delta-t cut
	double locPathLength = (locNeutralShower->dSpacetimeVertex.Vect() - locVertex).Mag();
	double locVertexTime = locNeutralShower->dSpacetimeVertex.T() - locPathLength/29.9792458;
	double locVertexTimeVariance = dUseSigmaForRFSelectionFlag ? locNeutralShower->dCovarianceMatrix(4, 4) : 1.0;
	double locDeltaTCut = dPIDTimeCutMap[Gamma][locNeutralShower->dDetectorSystem];

	//loop over possible #-RF-shifts, computing delta-t's
	unordered_map<int, double> locRFDeltaTMap;

	//start with best-shift, then loop up in n-shifts
	int locOrigNumShifts = Calc_RFBunchShift(locRFTime, locVertexTime);
	int locNumShifts = locOrigNumShifts;
	double locDeltaT = locVertexTime - (locRFTime + locNumShifts*dBeamBunchPeriod);
	while(fabs(locDeltaT) < locDeltaTCut)
	{
		double locChiSq = locDeltaT*locDeltaT/locVertexTimeVariance;
		locRFDeltaTMap.emplace(locNumShifts, locChiSq);
		++locNumShifts;
		locDeltaT = locVertexTime - (locRFTime + locNumShifts*dBeamBunchPeriod);
	}

	//now loop down in n-shifts
	int locNumShifts = locOrigNumShifts - 1;
	double locDeltaT = locVertexTime - (locRFTime + locNumShifts*dBeamBunchPeriod);
	while(fabs(locDeltaT) < locDeltaTCut)
	{
		double locChiSq = locDeltaT*locDeltaT/locVertexTimeVariance;
		locRFDeltaTMap.emplace(locNumShifts, locChiSq);
		--locNumShifts;
		locDeltaT = locVertexTime - (locRFTime + locNumShifts*dBeamBunchPeriod);
	}

	return locRFDeltaTMap;
}






/********************************************************* MAKE INITIAL SPACETIME GUESSES **********************************************************/


unordered_map<const DReactionStepVertexInfo*, pair<DVector3, double>> DVertexCreator::Get_VertexTimeOffsets(JEventLoop* locEventLoop, const DReactionVertexInfo* locReactionVertexInfo, const DSourceCombo* locReactionCombo)
{
	//we can't calculate the time yet: we need the RF time.  however, we can compute the time offset FROM the RF time
	unordered_map<const DReactionStepVertexInfo*, pair<DVector3, double>> locVertexTimeOffsetMap; //double: time offset from the RF time
	auto locStepVertexInfos = locReactionVertexInfo->Get_StepVertexInfos();
	auto locReaction = locReactionVertexInfo->Get_Reaction();

	//loop through vertices, determining initial guesses
	map<pair<int, int>, const DKinematicData*> locReconDecayParticleMap; //decaying particle indices -> kinematic data //indices: when the decaying particle is in the INITIAL state
	for(const auto& locStepVertexInfo : locStepVertexInfos)
	{
		/***************************************** CHECK IF VERTEX POSITION IS INDETERMINATE A THIS STAGE ********************************************/

		bool locIsProductionVertexFlag = locStepVertexInfo->Get_ProductionVertexFlag();
		auto locSourceCombo = DSourceComboer::Get_StepPrimaryVertexCombo_Charged(locReactionCombo, locReactionVertexInfo, locStepVertexInfo);

		//get particles
		auto locSourceParticles = DAnalysis::Get_SourceParticles_ThisVertex(locSourceCombo);
		auto locDecayingParticles = Get_FullConstrainDecayingParticles(locStepVertexInfo, locReconDecayParticleMap);
		auto locNumConstrainingParticles = locSourceParticles.size() + locDecayingParticles.size();
		bool locVertexIndeterminateFlag = locIsProductionVertexFlag ? (locNumConstrainingParticles == 0) : (locNumConstrainingParticles < 2);

		if(locVertexIndeterminateFlag)
		{
			//can't determine it.  If this is the production vertex, choose the center of the target.  If not, choose the vertex where the decay parent was produced.
			auto locParentVertexInfo = locStepVertexInfo->Get_ParentVertexInfo();
			if(locParentVertexInfo == nullptr)
				locVertexTimeOffsetMap.emplace(locStepVertexInfo, std::make_pair(DVector3(0.0, 0.0, dTargetCenterZ), 0.0));
			else
				locVertexTimeOffsetMap.emplace(locStepVertexInfo, locVertexTimeOffsetMap[locParentVertexInfo]);
			continue;
		}

		/*********************************************************** INITIAL VERTEX GUESS ************************************************************/

		/************************************************** CALCULATING VERTEX POSITIONS ***********************************************
		*
		* Production vertex:
		* 1) If there is at least one charged track at the production vertex with a theta > 30 degrees:
		*    The production vertex is the POCA to the beamline of the track with the largest theta.
		* 2) If not, then for each detached vertex:
		*    a) If there are any neutral or missing particles, or there are < 2 detected charged tracks at the detached vertex: ignore it
		*    b) Otherwise:
		*       i) The detached vertex is at the center of the lines between the POCAs of the two closest tracks.
		*       ii) Calculate the p3 of the decaying particles at this point and then find the POCA of the decaying particle to the beamline.
		* 3) Now, the production vertex is the POCA to the beamline of the track/decaying-particle with the largest theta.
		* 4) Otherwise, the production vertex is the center of the target.
		*
		* Detached vertices:
		* 1) If at least 2 decay products have defined trajectories (detected & charged or decaying & reconstructed):
		*    The detached vertex is at the center of the lines between the POCAs of the two closest particles.
		* 2) If one decay product is detected & charged, and the decaying particle production vertex was well defined (i.e. not forced to be center of target):
		*    a) Determine the decaying particle trajectory from the missing mass of the system (must be done after beam photon selection!!)
		*    b) The detached vertex is at the POCA of the charged decay product and the decaying particle
		* 3) Otherwise use the decaying particle production vertex for its position.
		*
		*******************************************************************************************************************************/

		//Get detected charged track hypotheses at this vertex
		vector<const DKinematicData*> locVertexParticles;
		locVertexParticles.reserve(locSourceParticles.size());

		auto Get_Hypothesis = [](const pair<Particle_t, const JObject*>& locPair) -> bool
			{return static_cast<const DChargedTrack*>(locPair.second)->Get_Hypothesis(locPair.first);};
		std::transform(locVertexParticles.begin(), locVertexParticles.end(), std::back_inserter(locVertexParticles), Get_Hypothesis);


double dMinThetaForVertex = 30.0;
		DVector3 locVertexGuess;
		if(locIsProductionVertexFlag)
		{
			//use track with theta nearest 90 degrees
			auto locThetaNearest90Iterator = Get_ThetaNearest90Iterator(locVertexParticles);
			double locThetaNearest90 = (*locThetaNearest90Iterator)->momentum().Theta()*180.0/TMath::Pi();
			if(locThetaNearest90 < dMinThetaForVertex)
			{
				//try decaying particles instead
				auto locDecayingParticles = Get_FullConstrainDecayingParticles(locStepVertexInfo, locReconDecayParticleMap);
				auto locThetaNearest90Iterator_Decaying = Get_ThetaNearest90Iterator(locDecayingParticles);
				double locLargestTheta_Decaying = (*locThetaNearest90Iterator_Decaying)->momentum().Theta()*180.0/TMath::Pi();
				if(locLargestTheta_Decaying > locThetaNearest90)
					locThetaNearest90Iterator = locThetaNearest90Iterator_Decaying;
			}
			locVertexGuess = (*locThetaNearest90Iterator)->position();
		}
		else //detached vertex
		{
			locVertexParticles.insert(locVertexParticles.end(), locDecayingParticles.begin(), locDecayingParticles.end());
			locVertexGuess = dAnalysisUtilities->Calc_CrudeVertex(locVertexParticles);
		}

		/************************************************************ INITIAL TIME GUESS *************************************************************/

		double locTimeGuess = Calc_TimeOffset(locActiveSpacetimeConstraint, locVertexGuess);

		/************************************************************ UPDATE WITH RESULTS ************************************************************/

		DLorentzVector locSpacetimeVertex(locVertexGuess, locTimeGuess);
		locVertexTimeOffsetMap.emplace(locStepVertexInfo, locSpacetimeVertex);
		Construct_DecayingParticle(locStepVertexInfo, locSourceCombo, locSpacetimeVertex, locReconDecayParticleMap);
	}

	//do time offsets once all the vertices have been found
	Calc_TimeOffsets(locReactionCombo, locReactionVertexInfo, locVertexTimeOffsetMap);

	//free resources
	for(auto locDecayParticlePair : locReconDecayParticleMap)
		delete locDecayParticlePair.second;

	return locVertexTimeOffsetMap;
}

vector<const DKinematicData*>::const_iterator DVertexCreator::Get_ThetaNearest90Iterator(const vector<const DKinematicData*>& locParticles)
{
	auto Get_Nearer90Theta = [](const DKinematicData* lhs, const DKinematicData* rhs) -> bool
		{return fabs(lhs->momentum().Theta() - 0.5*TMath::Pi()) < fabs(rhs->momentum().Theta() - 0.5*TMath::Pi());};
	return std::max_element(locParticles.begin(), locParticles.end(), Get_Nearer90Theta);
}

vector<const DKinematicData*> DVertexCreator::Get_FullConstrainDecayingParticles(const DReactionStepVertexInfo* locStepVertexInfo, const map<pair<int, int>, const DKinematicData*>& locReconDecayParticleMap)
{
	auto locConstrainingDecayingParticles = locStepVertexInfo->Get_DecayingParticles_FullConstrain();
	vector<const DKinematicData*> locDecayingParticles;
	for(auto locDecayPair : locConstrainingDecayingParticles)
	{
		//the particle pair in the map is where the decaying particle was in the initial state
		//however, the pairs in locConstrainingDecayingParticles are when then the decaying particles were in the final state
		//so, we have to convert from final state to initial state to do the map lookup
		int locDecayStepIndex = DAnalysis::Get_DecayStepIndex(locStepVertexInfo->Get_Reaction(), locDecayPair.first.first, locDecayPair.first.second);
		auto locParticlePair = std::make_pair(locDecayStepIndex, DReactionStep::Get_ParticleIndex_Initial());

		auto locReconIterator = locReconDecayParticleMap.find(locParticlePair);
		if(locReconIterator != locReconDecayParticleMap.end())
			locDecayingParticles.push_back(locReconIterator->second);
	}
	return locDecayingParticles;
}

void DVertexCreator::Construct_DecayingParticle(const DReactionStepVertexInfo* locReactionStepVertexInfo, const DSourceCombo* locSourceCombo, DLorentzVector locSpacetimeVertex, map<pair<int, int>, const DKinematicData*>& locReconDecayParticleMap)
{
	//get "decaying" no-constrain decaying particles
	map<pair<int, int>, weak_ptr<DReactionStepVertexInfo>> locNoConstrainDecayingParticles = locReactionStepVertexInfo->Get_DecayingParticles_NoConstrain();
	auto locReaction = locReactionStepVertexInfo->Get_Reaction();
	for(const auto& locNoConstrainPair : locNoConstrainDecayingParticles)
	{
		//we cannot define decaying p4 via missing mass, because the beam is not chosen yet
		//therefore, if the no-constrain decaying particle is a final-state particle at this vertex, we cannot define its p4
		const auto& locParticlePair = locNoConstrainPair.first;
		if(locParticlePair.second >= 0)
			continue; //momentum must be determined by missing mass, cannot do yet! (or isn't detached, for which we don't care)

		//only create kinematic data for detached PIDs
		auto locDecayPID = locReaction->Get_ReactionStep(locParticlePair.first)->Get_PID(locParticlePair.second);
		if(!IsDetachedVertex(locDecayPID))
			continue; //not detached: we don't care!

		//create a new one
		auto locP4 = Calc_DecayingP4_ChargedOnly(locSourceCombo);
		auto locKinematicData = new DKinematicData(locDecayPID, locP4.Vect(), locSpacetimeVertex.Vect(), locSpacetimeVertex.T());
		//register it
		locReconDecayParticleMap[locParticlePair] = locKinematicData;
	}
}

void DVertexCreator::Calc_TimeOffsets(const DSourceCombo* locReactionCombo, const DReactionVertexInfo* locReactionVertexInfo, unordered_map<const DReactionStepVertexInfo*, pair<DVector3, double>>& locVertexTimeOffsetMap)
{
	for(auto& locVertexPair : locVertexTimeOffsetMap)
	{
		auto locStepVertexInfo = locVertexPair.first;
		if(locStepVertexInfo->Get_ProductionVertexFlag())
			continue; //offset is already 0 by default
		auto locVertex = locVertexPair.second.first;

		auto locParentVertexInfo = locStepVertexInfo->Get_ParentVertexInfo();
		auto locParentProductionVertex = locVertexTimeOffsetMap[locParentVertexInfo].first;
		auto locPathLength = (locParentProductionVertex - locVertex).Mag();

		auto locSourceCombo = DSourceComboer::Get_StepPrimaryVertexCombo_Charged(locReactionCombo, locReactionVertexInfo, locStepVertexInfo);
		auto locP4 = Calc_DecayingP4_ChargedOnly(locSourceCombo);

		locVertexPair.second.second = locPathLength/(locP4.Beta()*SPEED_OF_LIGHT); //time offset
	}
}

} //end DAnalysis namespace
