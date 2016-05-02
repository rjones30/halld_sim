// $Id$
//
//    File: DEventSourceEventStore.cc
// Creator: sdobbs 
//

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <stdlib.h>
using namespace std;

#include <DANA/DApplication.h>
#include <DANA/DStatusBits.h>

#include <TRandom3.h>

#include <HDDM/DEventSourceREST.h>

#include "DEventSourceEventStore.h"
#include "DESSkimData.h"

static string EventstoreQueryHelp() {
	string str = "For more information, go to https://github.com/JeffersonLab/HDEventStore/wiki";
	return str;
}

// forward declarations
//class DEventSourceREST;

// Various variables
static bool TEST_MODE = false;


//---------------------------------
// DEventSourceEventStore    (Constructor)
//---------------------------------
DEventSourceEventStore::DEventSourceEventStore(const char* source_name):JEventSource(source_name)
{
	// initialize data members
	es_data_loaded = false;
	event_source = NULL;
	min_run = 0;
	max_run = INT_MAX;   // default to something ridiculously large
	esdb_connection = "mysql://es_user@hallddb.jlab.org/EventStoreTMP";    // default to main JLab ES server
	BASE_SKIM_INDEX = 20;
	MAX_SKIM_INDEX = 64 - BASE_SKIM_INDEX;
	
	// eventstore parameters
	// for more information, see ...
	//grade = "physics";        // REST physics data
	//grade = "recon";        // REST physics data
	grade = "recon-unchecked";        // DEBUG!!
	timestamp = "21000501";   // use the latest data - this should never happen, anyway...
	load_all_skims = true;    // default to processing all skims
	run_period_set = false;
	run_range_set = false;
	
	// load run period mapping
	// hardcode for now, this info should move to RCDB...
	run_period_map["RunPeriod-2015-03"] = pair<int,int>(2607,3385);
	run_period_map["RunPeriod-2016-02"] = pair<int,int>(10000,20000);
		
	// read in configurations
	// priority:  JANA command line -> environment variable -> default
	if(getenv("EVENTSTORE_CONNECTION") != NULL)
		esdb_connection = getenv("EVENTSTORE_CONNECTION");
	gPARMS->SetDefaultParameter("ESDB:DB_CONNECTION", esdb_connection,
								"Specification of EventStore DB connection.");
	
	int test_mode_flag = 0;
	gPARMS->SetDefaultParameter("ESDB:TEST_MODE", test_mode_flag,
								"Toggle test mode features");
	if(test_mode_flag != 0) {
		TEST_MODE = true;
		if(gRandom == NULL)
			gRandom = new TRandom3(0);
	}
	
	// First, parse the eventsource query
	// For details of the query format, see: <...>
	
	// Tokenize the query string
	string es_query(source_name);
	istringstream iss(es_query);
	vector<string> tokens;
	copy(istream_iterator<string>(iss),
    	 istream_iterator<string>(),
     	 back_inserter(tokens));
	
	////////////////////////////////////////////////////////////

	// initialize database connection
	if(esdb_connection.substr(0,8) == "mysql://") {
		cout << "MySQL connection" << endl;
		esdb = static_cast<DESDBProvider *>(new DESDBProviderMySQL(esdb_connection));
	} else if(esdb_connection.substr(0,8) == "sqlite://") {
		cout << "SQLite connection" << endl;	
	} 
	
	// Connect to database
	esdb->Open();
	
	////////////////////////////////////////////////////////////
	// parse the ES command
	// Check query header
	if( (tokens[0] != "eventstore") || tokens.size() < 3)
		throw JException("Invalid ES query = " + es_query + "\n\n" + EventstoreQueryHelp() );
		
	if( (tokens[1] == "in") ) {   // read data from the data base
		timestamp = tokens[2];    // require a timestamp
		
		if(tokens.size() > 3) {
			// parse the rest
			int token_ind = 3;
			
			// the next argument is the data grade to use
			grade = tokens[token_ind++];
			
			while(token_ind < tokens.size()) {
				if(tokens[token_ind] == "runs") {
					if(run_period_set) 
						throw JException("Cannot set run range and run period in the same command!");
					
					// make sure there's enough args
					if(tokens.size() - token_ind < 3)
						throw JException("Invalid ES query = " + es_query + "\n\n" + EventstoreQueryHelp() );
						
					min_run = atoi(tokens[token_ind+1].c_str());   // ERROR CHECK!!
					max_run = atoi(tokens[token_ind+2].c_str());   // ERROR CHECK!!
	
					// sanity check
					if(max_run < min_run) {
						throw JException("Maximum run must be larger than minimum run!");
					}
					token_ind += 3;
				} else if(tokens[token_ind] == "run_period") {
					if(run_range_set) 
						throw JException("Cannot set run range and run period in the same command!");

					// make sure there's enough args
					if(tokens.size() - token_ind < 2)
						throw JException("Invalid ES query = " + es_query + "\n\n" + EventstoreQueryHelp() );
						
					map< string, pair<int,int> >::iterator run_period_itr = run_period_map.find(tokens[token_ind+1]);
					if(run_period_itr == run_period_map.end()) {
						// a bad run period was specified...
						PrintRunPeriods();
						throw JException("Invalid ES query = " + es_query + "\n");
					}
					min_run = run_period_itr->second.first;
					max_run = run_period_itr->second.second;
					token_ind += 2;
				} else if(tokens[token_ind] == "skims") {
					// for the skims command, assume the rest of the arguments are skim names
					while(token_ind++ < tokens.size()) {
						skim_list.push_back(tokens[token_ind]);
					}
					
					// sanity check - don't allow a million skims!
					if(MAX_SKIM_INDEX - BASE_SKIM_INDEX < skim_list.size()) {
						throw JException("Too many skims specified!!");
					}
				} else {
					// require a valid command
					throw JException("Invalid ES query = " + es_query + "\n\n" + EventstoreQueryHelp() );
				}
			}
			
			// sanity check - make sure the grade exists!
			vector<string> grades_in_db;
			esdb->GetGrades(grades_in_db);
			
			vector<string>::iterator grade_itr = find(grades_in_db.begin(), grades_in_db.end(), grade);
			if(grade_itr == grades_in_db.end()) {
				jerr << "Could not find grade \'" << grade << "\' in DB!" << endl;
				PrintGrades();
				throw JException("Invalid ES query = " + es_query + "\n");
			}
		}
		
		
		// debugging stuff
		if(TEST_MODE) {
			if(skim_list.size() == 0) {
				skim_list.push_back("pi0");
				skim_list.push_back("eta");
				skim_list.push_back("rho");
				skim_list.push_back("omega");				
			}
		}
	} else if( (tokens[1] == "info") ) {   // query information from the DB
		// runs <datestamp> [<grade> [<skim>] ] [runs <min> [<max>]] 
    	//    [<run info query>]    prints available runs in DB  that match criteria
        //PrintRuns();
                           
  		// grades                  prints available grades in DB
		if(tokens[2] == "grades") {
			PrintGrades();
                           
  		// run_periods                  prints available run periods
		} else if(tokens[2] == "run_periods") {
			PrintRunPeriods();

  		// skims <datestamp> <grade>  print skims available for the grade
        } else if(tokens[2] == "skims") {
        	if(tokens.size() < 5)
        		throw JException("Invalid query:  skims <datestamp> <grade>" );

        	PrintSkims(tokens[3], tokens[4]);    
		}
  		// actualDate <datestamp> <grade>    print actual internal date used
        //} else if(tokens[2] == "actualDate") {
        //	if(tokens.size() < 5)
        //		throw JException("Invalid query = actualDate <datestamp> <grade>" );
		//
        //	PrintActualDate();     
		//}
  		// versions   <datestamp> <grade> [-verbose]
       	//                   print run ranges versus versions
        //                use -verbose option for more details
	
		// We're just querying the database, so we can quit here
		exit(0);
	} else {
		throw JException("Invalid ES query = " + es_query + "\n\n" + EventstoreQueryHelp() );
	}

	////////////////////////////////////////////////////////////
	/*
	if(TEST_MODE)   // if we're testing, don't make any more checks 
		return;
		
	// load some data here
		
	// make sure we've found any files
	*/
}

//---------------------------------
// ~DEventSourceEventStore    (Destructor)
//---------------------------------
DEventSourceEventStore::~DEventSourceEventStore()
{
	if (event_source != NULL)
		delete event_source;
}

//---------------------------------
// GetEvent
//---------------------------------
jerror_t DEventSourceEventStore::GetEvent(JEvent &event)
{
	// FOR DEBUGGING - EMIT EVENTS FOREVER
	if(TEST_MODE) {
		// output some fake event with skim information
    	event.SetEventNumber(1);
    	event.SetRunNumber(10000);
    	event.SetJEventSource(this);
   		event.SetRef(NULL);
    	event.SetStatusBit(kSTATUS_FROM_FILE);
    	event.SetStatusBit(kSTATUS_PHYSICS_EVENT);

		for(int i=0; i<4; i++)
			if(gRandom->Uniform() < 0.5)
				event.SetStatusBit(BASE_SKIM_INDEX+i);

		return NOERROR;
	}
	
	// make sure the file is open
	while(event_source == NULL) {
		while(OpenNextFile() != NOERROR) {}  // keep trying to open files until none are left
		if(event_source == NULL)
			return NO_MORE_EVENTS_IN_SOURCE;
	
		// skip to next event
		jerror_t retval;
		if( (retval = MoveToNextEvent()) != NOERROR)
			return retval;   // if we can't get to another event, then we're done
		
		// read the next event in
		retval = event_source->GetEvent(event);
		if(retval == NOERROR) {
			// tag event
			;
		} else if(retval == NO_MORE_EVENTS_IN_SOURCE) {   
			// if the source is empty, close the current one, then move to the next
			delete event_source;
			event_source = NULL;
		} else {   // if there'a another error, then pass it on...
			return retval;
		}
	}
	
	return NOERROR;
}

//---------------------------------
// FreeEvent
//---------------------------------
void DEventSourceEventStore::FreeEvent(JEvent &event)
{
	if(event_source != NULL)
		event_source->FreeEvent(event);
}

//---------------------------------
// GetObjects
//---------------------------------
jerror_t DEventSourceEventStore::GetObjects(JEvent &event, JFactory_base *factory)
{
	/// This gets called through the virtual method of the
	/// JEventSource base class. It creates the objects of the type
	/// on which factory is based.

	// We must have a factory to hold the data
	if(!factory) throw RESOURCE_UNAVAILABLE;

	// return meta-EventStore information
	JEventLoop* locEventLoop = event.GetJEventLoop();
    string dataClassName = factory->GetDataClassName();
	
	if (dataClassName =="DESSkimData") {
		JFactory<DESSkimData> *essd_factory = dynamic_cast<JFactory<DESSkimData>*>(factory);
		
		LockRead();			// LOCK class data
		vector<DESSkimData*> skim_data_vec;
		DESSkimData *skim_data = new DESSkimData(event, skim_list, BASE_SKIM_INDEX);
		skim_data_vec.push_back(skim_data);
		UnlockRead();       // UNLOCK

		essd_factory->CopyTo(skim_data_vec);
		return NOERROR;
	}
	
	if(!event_source) throw RESOURCE_UNAVAILABLE;
	
	return event_source->GetObjects(event, factory);
}

//---------------------------------
// MoveToNextEvent
//---------------------------------
jerror_t DEventSourceEventStore::MoveToNextEvent()
{
	// if we're loading all of the skims, then we don't need to skip any events
	if(load_all_skims)
		return NOERROR;
		
	
}

//---------------------------------
// OpenNextFile
//---------------------------------
jerror_t DEventSourceEventStore::OpenNextFile()
{
	if(!es_data_loaded) {
		// LoadESData();
		
		// sanity check
		if(data_files.size() == 0) {
			jerr << "Could not load any files from EventStore!" << endl;
			return NO_MORE_EVENT_SOURCES;
		}
		
		// keep a pointer to the current file
		current_file_itr = data_files.begin();
		
		es_data_loaded = true;   
	}
	
	// if there's a current file open, close it so that we don't leak memory
	if(event_source != NULL) {
		delete event_source;
		event_source = NULL;
	}
	
	while( (event_source == NULL) && (current_file_itr != data_files.end()) ) {
		// TO FIX: properly handle multiple data types
		try {
			event_source = static_cast<JEventSource*>(new DEventSourceREST(current_file_itr->c_str()));
		} catch (...) {
			event_source = NULL;
		}
		
		current_file_itr++;
	}
	
	// error check
	if(event_source == NULL)
		return NO_MORE_EVENT_SOURCES;
	else
		return NOERROR;
}

//---------------------------------
// PrintGrades
//---------------------------------
void DEventSourceEventStore::PrintGrades()
{
	vector<string> grades;
	esdb->GetGrades(grades);
	
	// print out information
	cout << endl << "Available grades:" << endl;
	for(vector<string>::iterator it = grades.begin();
		it != grades.end(); it++)
			cout << "  " << *it << endl;
	cout << endl;
}

//---------------------------------
// PrintRunPeriods
//---------------------------------
void DEventSourceEventStore::PrintRunPeriods()
{
	vector<string> grades;
	esdb->GetGrades(grades);
	
	// print out information
	cout << endl << "Available Run Periods:" << endl;
	for(map< string, pair<int,int> >::iterator it = run_period_map.begin();
		it != run_period_map.end(); it++) {
			pair<int,int> &the_run_range = it->second;
			cout << "  " << it->first << ":  " 
				 << the_run_range.first << " - " << the_run_range.second << endl;
	}
	cout << endl;
}

//---------------------------------
// PrintSkims
//---------------------------------
void DEventSourceEventStore::PrintSkims(string datestamp, string grade)
{
	vector<string> skims;
	esdb->GetSkims(skims, datestamp, grade);
	
	// print out information
	cout << endl << "Available skims for grade " << grade << ":" << endl;
	for(vector<string>::iterator it = skims.begin();
		it != skims.end(); it++)
			cout << "  " << *it << endl;
	cout << endl;
}

