// $Id$

#include "DEvent.h"
#include "DFactory_MCCheatHits.h"

static float BCAL_R=60.0;
static float TOF_Z=450.0;

static int qsort_mccheat_hits(const void* arg1, const void* arg2);


//------------------------------------------------------------------
// qsort_points_by_z
//------------------------------------------------------------------
static int qsort_mccheat_hits(const void* arg1, const void* arg2)
{
	MCCheatHit_t *a = (MCCheatHit_t*)arg1;
	MCCheatHit_t *b = (MCCheatHit_t*)arg2;
	
	// sort by track number first
	if(a->track != b->track)return a->track-b->track;
	
	// sort by z second
	if(a->z == b->z)return 0;
	return b->z < a->z ? 1:-1;
}

//-------------------
// evnt
//-------------------
derror_t DFactory_MCCheatHits::evnt(int eventnumber)
{
	GetCDCHits();
	GetFDCHits();
	GetBCALHits();
	GetTOFHits();
	GetCherenkovHits();
	GetFCALHits();
	GetUPVHits();
	
	// sort hits by track, then z
	qsort(_data->first(), _data->nrows, sizeof(MCCheatHit_t), qsort_mccheat_hits);

	return NOERROR;
}

//-------------------
// GetCDCHits
//-------------------
derror_t DFactory_MCCheatHits::GetCDCHits(void)
{
	// Loop over Physics Events
	s_PhysicsEvents_t* PE = hddm_s->physicsEvents;
	if(!PE) return NOERROR;
	
	for(int i=0; i<PE->mult; i++){
		// ------------ CdcPoints, Hits --------------
		s_Rings_t *rings=NULL;
		s_HitView_t *HV = PE->in[i].hitView;
		if(PE->in[i].hitView)
			if(PE->in[i].hitView->centralDC)
				rings = PE->in[i].hitView->centralDC->rings;
		if(rings){
			for(int j=0;j<rings->mult;j++){
				float radius = rings->in[j].radius;
				s_Straws_t *straws = rings->in[j].straws;
				if(straws){
					for(int k=0;k<straws->mult;k++){
						float phim = straws->in[k].phim;
						s_CdcPoints_t *cdcpoints = straws->in[k].cdcPoints;
						if(cdcpoints){
							for(int m=0;m<cdcpoints->mult;m++){
								MCCheatHit_t *mccheathit = (MCCheatHit_t*)_data->Add();
								mccheathit->r			= cdcpoints->in[m].r;
								mccheathit->phi		= cdcpoints->in[m].phi;
								mccheathit->z			= cdcpoints->in[m].z;
								mccheathit->track		= cdcpoints->in[m].track;
								mccheathit->system	= 1;
							}
						}
					}
				}
			}
		}
	}

	return NOERROR;
}

//-------------------
// GetFDCHits
//-------------------
derror_t DFactory_MCCheatHits::GetFDCHits(void)
{
	// Loop over Physics Events
	s_PhysicsEvents_t* PE = hddm_s->physicsEvents;
	if(!PE) return NOERROR;
	
	for(int i=0; i<PE->mult; i++){
		s_Chambers_t *chambers = NULL;
		s_HitView_t *HV = PE->in[i].hitView;
		if(PE->in[i].hitView)
			if(PE->in[i].hitView->forwardDC)
				chambers = PE->in[i].hitView->forwardDC->chambers;
		if(!chambers)continue;
		
		for(int j=0;j<chambers->mult;j++){

			s_AnodePlanes_t *anodeplanes = chambers->in[j].anodePlanes;
			if(anodeplanes){
			
				for(int k=0;k<anodeplanes->mult;k++){
					s_Wires_t *wires = anodeplanes->in[k].wires;
					if(!wires)continue;
				
					for(int m=0;m<wires->mult;m++){
						s_FdcPoints_t *fdcPoints = wires->in[k].fdcPoints;
						if(!fdcPoints)continue;
						for(int n=0;n<fdcPoints->mult;n++){
							MCCheatHit_t *mccheathit = (MCCheatHit_t*)_data->Add();
							float x = fdcPoints->in[n].x;
							float y = fdcPoints->in[n].y;
							mccheathit->r			= sqrt(x*x + y*y);
							mccheathit->phi		= atan2(y,x);
							if(mccheathit->phi<0.0)mccheathit->phi += 2.0*M_PI;
							mccheathit->z			= fdcPoints->in[n].z;
							mccheathit->track		= fdcPoints->in[n].track;
							mccheathit->system	= 2;
						}
					}
				}
			}

			s_CathodePlanes_t *cathodeplanes = chambers->in[j].cathodePlanes;
			if(cathodeplanes){
			
				for(int k=0;k<cathodeplanes->mult;k++){
					float tau = cathodeplanes->in[k].tau;
					float z = cathodeplanes->in[k].z;
					s_Strips_t *strips = cathodeplanes->in[k].strips;
					if(!strips)continue;
				
					for(int m=0;m<strips->mult;m++){
						// Just skip cathode hits
					}
				}
			}
		}
	}

	return NOERROR;
}

//-------------------
// GetBCALHits
//-------------------
derror_t DFactory_MCCheatHits::GetBCALHits(void)
{
	// Loop over Physics Events
	s_PhysicsEvents_t* PE = hddm_s->physicsEvents;
	if(!PE) return NOERROR;
	
	for(int i=0; i<PE->mult; i++){
		s_BarrelShowers_t *barrelShowers = NULL;
		s_HitView_t *HV = PE->in[i].hitView;
		if(PE->in[i].hitView)
			if(PE->in[i].hitView->forwardTOF)
				barrelShowers = PE->in[i].hitView->barrelEMcal->barrelShowers;
		if(!barrelShowers)continue;
		
		for(int j=0;j<barrelShowers->mult;j++){
			MCCheatHit_t *mccheathit = (MCCheatHit_t*)_data->Add();
			mccheathit->r			= BCAL_R;
			mccheathit->phi		= barrelShowers->in[j].phi;
			mccheathit->z			= barrelShowers->in[j].z;
			mccheathit->track		= barrelShowers->in[j].track;
			mccheathit->system	= 3;
		}
	}

	return NOERROR;
}

//-------------------
// GetTOFHits
//-------------------
derror_t DFactory_MCCheatHits::GetTOFHits(void)
{
	// Loop over Physics Events
	s_PhysicsEvents_t* PE = hddm_s->physicsEvents;
	if(!PE) return NOERROR;
	
	for(int i=0; i<PE->mult; i++){
		s_TofPoints_t *tofPoints = NULL;
		s_HitView_t *HV = PE->in[i].hitView;
		if(PE->in[i].hitView)
			if(PE->in[i].hitView->forwardTOF)
				tofPoints = PE->in[i].hitView->forwardTOF->tofPoints;
		if(!tofPoints)continue;
		
		for(int j=0;j<tofPoints->mult;j++){
			MCCheatHit_t *mccheathit = (MCCheatHit_t*)_data->Add();
			float x = tofPoints->in[j].x;
			float y = tofPoints->in[j].y;
			mccheathit->r			= sqrt(x*x + y*y);
			mccheathit->phi		= atan2(y,x);
			if(mccheathit->phi<0.0)mccheathit->phi += 2.0*M_PI;
			mccheathit->z			= TOF_Z;
			mccheathit->track		= tofPoints->in[j].track;
			mccheathit->system	= 4;
		}
	}

	return NOERROR;
}

//-------------------
// GetCherenkovHits
//-------------------
derror_t DFactory_MCCheatHits::GetCherenkovHits(void)
{

	return NOERROR;
}

//-------------------
// GetFCALHits
//-------------------
derror_t DFactory_MCCheatHits::GetFCALHits(void)
{

	return NOERROR;
}

//-------------------
// GetUPVHits
//-------------------
derror_t DFactory_MCCheatHits::GetUPVHits(void)
{

	return NOERROR;
}

//------------
// Print
//------------
derror_t DFactory_MCCheatHits::Print(void)
{
	// Ensure our Get method has been called so _data is up to date
	Get();
	if(!_data)return NOERROR;
	if(_data->nrows<=0)return NOERROR; // don't print anything if we have no data!

	cout<<name<<endl;
	cout<<"---------------------------------------"<<endl;
	cout<<"row:   r(cm): phi(rad):  z(cm): track:    system:"<<endl;
	cout<<endl;
	
	MCCheatHit_t *mccheathit = (MCCheatHit_t*)_data->first();
	for(int i=0; i<_data->nrows; i++, mccheathit++){
		char str[80];
		memset(str,' ',80);
		str[79] = 0;

		char num[32];
		sprintf(num, "%d", i);
		strncpy(&str[3-strlen(num)], num, strlen(num));

		sprintf(num, "%3.1f", mccheathit->r);
		strncpy(&str[12-strlen(num)], num, strlen(num));
		sprintf(num, "%1.3f", mccheathit->phi);
		strncpy(&str[22-strlen(num)], num, strlen(num));
		sprintf(num, "%3.1f", mccheathit->z);
		strncpy(&str[30-strlen(num)], num, strlen(num));
		sprintf(num, "%d", mccheathit->track);
		strncpy(&str[37-strlen(num)], num, strlen(num));
		char *system = "<unknown>";
		switch(mccheathit->system){
			case 1: system = "CDC";				break;
			case 2: system = "FDC";				break;
			case 3: system = "BCAL";			break;
			case 4: system = "TOF";				break;
			case 5: system = "Cherenkov";		break;
			case 6: system = "FCAL";			break;
			case 7: system = "UPV";				break;
		}
		sprintf(num, "%s", system);
		strncpy(&str[48-strlen(num)], num, strlen(num));

		cout<<str<<endl;
	}
	cout<<endl;
}

