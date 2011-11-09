
#include <iostream>
#include <fstream>
#include <complex>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <cassert>
#include <cstdlib>

#include "particleType.h"
#include <HDDM/hddm_s.h>
#include "AMPTOOLS_DATAIO/ROOTDataWriter.h"
#include "AMPTOOLS_DATAIO/ASCIIDataWriter.h"

#include "AMPTOOLS_AMPS/b1piAngAmp.h"
#include "AMPTOOLS_AMPS/BreitWigner.h"

#include "AMPTOOLS_MCGEN/ProductionMechanism.h"
#include "AMPTOOLS_MCGEN/GammaPToNPartP.h"

#include "IUAmpTools/AmplitudeManager.h"
#include "IUAmpTools/ConfigFileParser.h"
#include "CLHEP/Vector/LorentzVector.h"
#include "CLHEP/Vector/LorentzRotation.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TTree.h"

#include "b1piAmpCheck.h"

using std::complex;
using namespace std;
using namespace CLHEP;



int main( int argc, char* argv[] ){
  
  string  configfile("gen_OmegaPiPi.cfg");
  string  outname("gen_test.ascii");
  b1piAmpCheck AmpCheck;
  bool diag = false, genFlat = false;
  
  // default upper and lower bounds 
  double lowMass = 0.7, highMass = 3.0, Mpipm,Mpi0;
  Mpipm=ParticleMass(PiPlus);
  Mpi0=ParticleMass(Pi0);
  
  double CustMaxInten=-1, maxInten, runs_maxInten=0.0;
  long int Seed=0;

  int nEvents = 30, batchSize = 200000;

  //Parse command line: -----------------------------------------------
  for (int i = 1; i < argc; i++){
    
    string arg(argv[i]);
    
    if (arg == "-c"){  
      if ((i+1 == argc) || (argv[i+1][0] == '-')) arg = "-h";
      else  configfile = argv[++i]; }
    if (arg == "-o"){  
      if ((i+1 == argc) || (argv[i+1][0] == '-')) arg = "-h";
      else  outname = argv[++i]; }
    if (arg == "-l"){  
      if ((i+1 == argc) || (argv[i+1][0] == '-')) arg = "-h";
      else  lowMass = atof( argv[++i] ); }
    if (arg == "-u"){  
      if ((i+1 == argc) || (argv[i+1][0] == '-')) arg = "-h";
      else  highMass = atof( argv[++i] ); }
    if (arg == "-n"){  
      if ((i+1 == argc) || (argv[i+1][0] == '-')) arg = "-h";
      else  nEvents = atoi( argv[++i] ); }
    if (arg == "-b"){  
      if ((i+1 == argc) || (argv[i+1][0] == '-')) arg = "-h";
      else  batchSize = atoi( argv[++i] ); }
    if (arg == "-s"){  
      if ((i+1 == argc) || (argv[i+1][0] == '-')) arg = "-h";
      else Seed = atoi( argv[++i] ); }
    if (arg == "-i"){  
      if ((i+1 == argc) || (argv[i+1][0] == '-')) arg = "-h";
      else CustMaxInten = runs_maxInten = atof( argv[++i] ); }
    
    if (arg == "-d"){  
      diag = true; }
    if (arg == "-f"){  
      genFlat = true; }
    if (arg == "-h"){
      cout << endl << " Usage for: " << argv[0] << endl << endl;
      cout << "\t -c <file>\t Config file" << endl;
      cout << "\t -o <name>\t Output name" << endl;
      cout << "\t -l <value>\t Low edge of mass range (GeV) [optional]" << endl;
      cout << "\t -u <value>\t Upper edge of mass range (GeV) [optional]" << endl;
      cout << "\t -n <value>\t Minimum number of events to generate [optional]" << endl;
      cout << "\t -f \t\t Generate flat in M(X) (no physics) [optional]" << endl;
      //cout << "\t -d \t\t Plot only diagnostic histograms [optional]" << endl ;
      cout << "\t -s <value> Specify random number generator seed [optional]" << endl;
      cout << "\t -b <value> Specify batch size for intensities in accept/reject alg. [optional]" << endl; 
      cout << "\t -i <value> Specify maximum intensity (accept/reject range)" << endl << endl;
      exit(1);
    }
  }
  
  if( configfile.size() == 0 || outname.size() == 0 ){
    cout << "No config file or output specificed:  run gen_5pi -h for help" << endl;
    exit(1);
  }
  // END OF ARGUMENT PARSING/CHECKING /////////////////////////////////////////

  
  srand48(Seed);

  string rootfname=outname + ".root";
  string asciifname=outname + ".ascii";
  //FILE *Ifid;

  // open config file
  ConfigFileParser parser( configfile );
  ConfigurationInfo* cfgInfo = parser.getConfigurationInfo();
  vector< ReactionInfo* > reactions = cfgInfo->reactionList();
  assert( reactions.size() == 1 );
  
  ReactionInfo* info = reactions[0];
  
  // setup amplitude manager
  AmplitudeManager ampManager( info->particleList(), info->reactionName() );	
  ampManager.registerAmplitudeFactor( b1piAngAmp() );
  ampManager.registerAmplitudeFactor( BreitWigner() );
  ampManager.setupFromConfigurationInfo( cfgInfo );  
  
  //Exprected particle list: 
  // pi- b1(pi+ omega(pi0 "rho"(pi- pi+)))
  //  2      3         4         5   6
  vector<int> part_types;
  part_types.push_back(1);  part_types.push_back(14);
  part_types.push_back(9);  part_types.push_back(8);
  part_types.push_back(7);
  part_types.push_back(9);  part_types.push_back(8);
  vector<float> part_masses;
  part_masses.push_back(Mpipm);  part_masses.push_back(Mpipm);
  part_masses.push_back(Mpi0);
  part_masses.push_back(Mpipm);  part_masses.push_back(Mpipm);

  
  ProductionMechanism::Type type =
    ( genFlat ? ProductionMechanism::kFlat : ProductionMechanism::kResonant );
   
  //generate over a range mass -- the daughters are pi-,pi+,omega
  GammaPToNPartP resProd( lowMass, highMass, part_masses,type );
  
  // seed the distribution with a sum of noninterfering Breit-Wigners
  // we can easily compute the PDF for this and divide by that when
  // doing accept/reject -- improves efficiency if seeds are picked well
  if( !genFlat ){
    // the lines below should be tailored by the user for the particular desired
    // set of amplitudes -- doing so will improve efficiency.  Leaving as is
    // won't make MC incorrect, it just won't be as fast as it could be
    resProd.addResonance( 1.89, 0.16,  1.0 );
    resProd.addResonance( 2.0, 0.25,  0.75 );
  }
  
  // open output file
  //HDDMDataWriter hddmOut( outname );
  ROOTDataWriter rootOut( rootfname);
  ASCIIDataWriter asciiOut( asciifname );

  TH1F* mass = new TH1F( "M", "Resonance Mass", 180, lowMass, highMass );
  TH1F* massW = new TH1F( "M_W", "Weighted Resonance Mass", 180, lowMass, highMass );
  massW->Sumw2();
  TH1F* intenW = new TH1F( "intenW", "True PDF / Gen. PDF", 1000, 0, 100 );
  TH2F* intenWVsM = new TH2F( "intenWVsM", "Ratio vs. M", 100, lowMass, highMass, 1000, 0, 10 );
  
  TH2F* dalitz = new TH2F( "dalitz", "Dalitz Plot", 100, 0, 3.0, 100, 0, 3.0 );

  TH1F* prod_ang = new TH1F( "alpha", "Production angle #alpha", 100, -PI, PI);

  TH1F* M_rho = new TH1F( "M_rho", "Omega Mass", 200, 0.27, .9 );
  TH1F* M_omega = new TH1F( "M_omega", "Omega Mass", 200, 0.6, 1.1 );
  TH1F* M_b1 = new TH1F( "M_b1", "Isobar Mass", 200, 0.5, 3.0 );
  
  TH2F *XAng = new TH2F("XAng","Angular distribution of X decay",
			50,-M_PI,M_PI,50,-1,1);
  TH2F *b1Ang = new TH2F("b1Ang","Angular distribution of b_{1} decay",
			 50,-M_PI,M_PI,50,-1,1);
  TH2F *OmegaAng = new TH2F("OmegaAng","Angular distribution of #omega decay",
			    50,-M_PI,M_PI,50,-1,1);
  TH2F *RhoAng = new TH2F("RhoAng","Angular distribution of #rho decay",
			  50,-M_PI,M_PI,50,-1,1);
  
  //Ifid=fopen("Idump.dat","w");

  int eventCounter = 0;
  while( eventCounter < nEvents ){
    
    if( batchSize < 1E4 ){  
      cout << "WARNING:  small batches could have batch-to-batch variations\n"
      << "          due to different maximum intensities!" << endl;
    }
    
    cout << "Number to generate: " << batchSize << endl;
    cout << "Generating four-vectors..." << endl;
    AmpVecs* aVecs = resProd.generateMany( batchSize );
    cout << "Calculating amplitudes..." << endl;
    aVecs->allocateAmps( ampManager, true );
    cout << "Calculating intensities..." << endl;


    maxInten=0;
    if(!genFlat)
      maxInten = ampManager.calcIntensities(*aVecs,true);
    else
      for(int i = 0; i < batchSize; ++i )
	if(aVecs->getEvent(i)->weight() > maxInten) 
	  maxInten = aVecs->getEvent(i)->weight();
    
    printf("MAXINTEN: %25.20f\n",maxInten);

    //override the max intensity found with that passed in through cmd line args
    if( CustMaxInten > 0 ){
      if(maxInten>CustMaxInten){
	printf("WARNING: Event found with intensity greater than custom-specified maximum\n");
	CustMaxInten=maxInten;
      }else maxInten=CustMaxInten;
    }else maxInten*=1.5;


    cout << "Processing events.." << endl;
    if(CustMaxInten < 0 && runs_maxInten < maxInten) runs_maxInten=maxInten;

    //double IbatchSum=0;
    for( int i = 0; i < batchSize; ++i ){
      
      Kinematics* evt = aVecs->getEvent( i );
      AmpCheck.SetEvent(*evt);

      double genWeight = evt->weight();
      double weightedInten = aVecs->m_pdIntensity[i];

      /*double realA=aVecs->m_pdAmps[2*aVecs->m_iNAmps*i];
      double imagA=aVecs->m_pdAmps[2*aVecs->m_iNAmps*i+1];
      double calcI=(realA*realA+imagA*imagA)*1.5*PI;
      printf("A=(%6.3f,%6.3f)->%7.4f\tI=%f\tI/calcI=%f\tmaxInten = %f \n", 
	realA, imagA, calcI,
	     weightedInten, weightedInten/calcI, maxInten);
	     IbatchSum+=weightedInten;
      */
      //fprintf(Ifid,"%25.20f\n",weightedInten);

      // obtain this by looking at the maximum value of intensity * genWeight
      if((!genFlat && weightedInten > drand48() * maxInten) ||
	 (genFlat && genWeight > drand48() * maxInten) ){
	
	double histWeight = 1.0;//genFlat ? genWeight : 1.0; 
	
	//Fill some useful histograms
	mass->Fill( AmpCheck.GetMX(), histWeight );
	massW->Fill( AmpCheck.GetMX(), genWeight );
        
	intenW->Fill( weightedInten, histWeight );
	intenWVsM->Fill( AmpCheck.GetMX(), weightedInten );
        
	dalitz->Fill( (AmpCheck.GetXsPi() + AmpCheck.Getb1sPi()).M2(),
		      (AmpCheck.Getb1sPi() + AmpCheck.GetOmega()).M2(),histWeight);
	M_rho->Fill(AmpCheck.GetMrho(), histWeight);
	M_omega->Fill(AmpCheck.GetMomega(), histWeight);
	M_b1->Fill(AmpCheck.GetMb1(), histWeight);
	
	// orientation of production plane in lab
	prod_ang->Fill(AmpCheck.GetAlpha(), histWeight);
	XAng->Fill(AmpCheck.GetXPhi(), AmpCheck.GetXCosTheta(), histWeight);
	b1Ang->Fill(AmpCheck.Getb1Phi(), AmpCheck.Getb1CosTheta(), histWeight);
	OmegaAng->Fill(AmpCheck.GetOmegaPhi(),AmpCheck.GetOmegaCosTheta(), histWeight);
	RhoAng->Fill(AmpCheck.GetRhoPhi(), AmpCheck.GetRhoCosTheta(), histWeight);
	
	// we want to save events with weight 1
	evt->setWeight( 1.0 );
	
	rootOut.writeEvent( *evt ); 	  
	asciiOut.writeEvent( *evt, part_types ); 	  
	++eventCounter;
      }
      
      
      delete evt;
    }
    //printf("BATCH AVERAGE:  %f\n",IbatchSum/batchSize);
    
    cout << eventCounter << " events were processed." << endl;
    delete aVecs;
  }

  if(!genFlat) printf("RUN_MAX_INTEN: %25.20f\n",runs_maxInten);
  
  mass->Write();
  massW->Write();
  dalitz->Write();
  intenW->Write();
  intenWVsM->Write();
  prod_ang->Write();
  M_b1->Write();
  M_omega->Write();
  M_rho->Write();
  XAng->Write();
  b1Ang->Write();
  OmegaAng->Write();
  RhoAng->Write();

  //fclose(Ifid);

  return 0;
}


