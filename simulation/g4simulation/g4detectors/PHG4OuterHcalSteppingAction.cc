#include "PHG4OuterHcalSteppingAction.h"
#include "PHG4OuterHcalDetector.h"

#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4Hitv1.h>

#include <g4main/PHG4TrackUserInfoV1.h>

#include <fun4all/getClass.h>

#include <Geant4/G4Step.hh>
#include <Geant4/G4MaterialCutsCouple.hh>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
// this is an ugly hack, the gcc optimizer has a bug which
// triggers the uninitialized variable warning which
// stops compilation because of our -Werror 
#include <boost/version.hpp> // to get BOOST_VERSION
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 4 && BOOST_VERSION == 105700 )
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma message "ignoring bogus gcc warning in boost header lexical_cast.hpp"
#include <boost/lexical_cast.hpp>
#pragma GCC diagnostic warning "-Wuninitialized"
#else
#include <boost/lexical_cast.hpp>
#endif

#include <iostream>

using namespace std;
//____________________________________________________________________________..
PHG4OuterHcalSteppingAction::PHG4OuterHcalSteppingAction( PHG4OuterHcalDetector* detector ):
  PHG4SteppingAction(NULL),
  detector_( detector ),
  hits_(NULL),
  absorberhits_(NULL),
  hit(NULL),
  light_scint_model_(true),
  light_balance_(false),
  light_balance_inner_radius_(0.0),
  light_balance_inner_corr_(1.0),
  light_balance_outer_radius_(10.0),
  light_balance_outer_corr_(1.0)  
{}

//____________________________________________________________________________..
bool PHG4OuterHcalSteppingAction::UserSteppingAction( const G4Step* aStep, bool )
{

  G4TouchableHandle touch = aStep->GetPreStepPoint()->GetTouchableHandle();
  // get volume of the current step
  G4VPhysicalVolume* volume = touch->GetVolume();

  // detector_->IsInOuterHcal(volume)
  // returns 
  //  0 is outside of OuterHcal
  //  1 is inside scintillator
  // -1 is steel absorber

  int whichactive = detector_->IsInOuterHcal(volume);

  if (!whichactive)
    {
      return false;
    }
  unsigned int motherid = ~0x0; // initialize to 0xFFFFFF using the correct bitness
  int tower_id = -1;
  if (whichactive > 0) // scintillator
    {
// G4AssemblyVolumes naming convention:
//     av_WWW_impr_XXX_YYY_ZZZ
// where:

//     WWW - assembly volume instance number
//     XXX - assembly volume imprint number
//     YYY - the name of the placed logical volume
//     ZZZ - the logical volume index inside the assembly volume
// e.g. av_1_impr_82_HcalOuterScinti_11_pv_11
// 82 the number of the scintillator mother volume
// HcalOuterScinti_11: name of scintillator slat
// 11: number of scintillator slat logical volume
// use boost tokenizer to separate the _, then take value
// after "impr" for mother volume and after "pv" for scintillator slat
// use boost lexical cast for string -> int conversion
      boost::char_separator<char> sep("_");
      boost::tokenizer<boost::char_separator<char> > tok(volume->GetName(), sep);
      boost::tokenizer<boost::char_separator<char> >::const_iterator tokeniter;
      for (tokeniter = tok.begin(); tokeniter != tok.end(); ++tokeniter)
	{
	  if (*tokeniter == "impr")
	    {
	      ++tokeniter;
	      motherid = boost::lexical_cast<int>(*tokeniter);
	    }
	  else if (*tokeniter == "pv")
	    {
	      ++tokeniter;
	      tower_id = boost::lexical_cast<int>(*tokeniter);
	    }
	}
    }
  else
    {
      tower_id = touch->GetCopyNumber(); // steel plate id
    }
  // collect energy and track length step by step
  G4double edep = aStep->GetTotalEnergyDeposit() / GeV;
  G4double eion = (aStep->GetTotalEnergyDeposit() - aStep->GetNonIonizingEnergyDeposit()) / GeV;
  G4double light_yield = 0;

  const G4Track* aTrack = aStep->GetTrack();

  // if this block stops everything, just put all kinetic energy into edep
  if (detector_->IsBlackHole())
    {
      edep = aTrack->GetKineticEnergy() / GeV;
      G4Track* killtrack = const_cast<G4Track *> (aTrack);
      killtrack->SetTrackStatus(fStopAndKill);
    }
  int layer_id = detector_->get_Layer();

  // make sure we are in a volume
  if ( detector_->IsActive() )
    {
      bool geantino = false;

      // the check for the pdg code speeds things up, I do not want to make
      // an expensive string compare for every track when we know
      // geantino or chargedgeantino has pid=0
      if (aTrack->GetParticleDefinition()->GetPDGEncoding() == 0 &&
	  aTrack->GetParticleDefinition()->GetParticleName().find("geantino") != string::npos)
	{
	  geantino = true;
	}
      G4StepPoint * prePoint = aStep->GetPreStepPoint();
      G4StepPoint * postPoint = aStep->GetPostStepPoint();
      //       cout << "track id " << aTrack->GetTrackID() << endl;
      //       cout << "time prepoint: " << prePoint->GetGlobalTime() << endl;
      //       cout << "time postpoint: " << postPoint->GetGlobalTime() << endl;
      switch (prePoint->GetStepStatus())
	{
	case fGeomBoundary:
	case fUndefined:
    hit = new PHG4Hitv1();
	  hit->set_layer(motherid);
	  hit->set_scint_id(tower_id); // the slat id (or steel plate id)
	  //here we set the entrance values in cm
	  hit->set_x( 0, prePoint->GetPosition().x() / cm);
	  hit->set_y( 0, prePoint->GetPosition().y() / cm );
	  hit->set_z( 0, prePoint->GetPosition().z() / cm );
	  hit->set_px( 0, prePoint->GetMomentum().x() / GeV );
	  hit->set_py( 0, prePoint->GetMomentum().y() / GeV );
	  hit->set_pz( 0, prePoint->GetMomentum().z() / GeV );
	  // time in ns
	  hit->set_t( 0, prePoint->GetGlobalTime() / nanosecond );
	  //set the track ID
	  {
	    int trkoffset = 0;
	    if ( G4VUserTrackInformation* p = aTrack->GetUserInformation() )
	      {
		if ( PHG4TrackUserInfoV1* pp = dynamic_cast<PHG4TrackUserInfoV1*>(p) )
		  {
		    trkoffset = pp->GetTrackIdOffset();
		  }
	      }
	    hit->set_trkid(aTrack->GetTrackID() + trkoffset);
	  }

	  //set the initial energy deposit
	  hit->set_edep(0);
	  hit->set_eion(0); // only implemented for v5 otherwise empty
    hit->set_light_yield(0);
	  if (whichactive > 0) // return of IsInOuterHcalDetector, > 0 hit in scintillator, < 0 hit in absorber
	    {
	      // Now add the hit
	      hits_->AddHit(layer_id, hit);
	    }
	  else
	    {
	      absorberhits_->AddHit(layer_id, hit);
	    }
	  break;
	default:
	  break;
	}
      // here we just update the exit values, it will be overwritten
      // for every step until we leave the volume or the particle
      // ceases to exist
      hit->set_x( 1, postPoint->GetPosition().x() / cm );
      hit->set_y( 1, postPoint->GetPosition().y() / cm );
      hit->set_z( 1, postPoint->GetPosition().z() / cm );

      hit->set_px(1, postPoint->GetMomentum().x() / GeV );
      hit->set_py(1, postPoint->GetMomentum().y() / GeV );
      hit->set_pz(1, postPoint->GetMomentum().z() / GeV );

      hit->set_t( 1, postPoint->GetGlobalTime() / nanosecond );

      if (whichactive > 0)
        {

          if (light_scint_model_)
            {
              light_yield = GetVisibleEnergyDeposition(aStep);

              static bool once = true;
              if (once && edep>0)
                {
                  once = false;

                  cout << "PHG4OuterHcalSteppingAction::UserSteppingAction::"
                      //
                      << detector_->GetName() << " - "
                      << " use scintillating light model at each Geant4 steps. "
                      <<"First step: "
                      <<"Material = "<<aTrack->GetMaterialCutsCouple()->GetMaterial()->GetName()<<", "
                      <<"Birk Constant = "<<aTrack->GetMaterialCutsCouple()->GetMaterial()->GetIonisation()->GetBirksConstant()<<","
                      <<"edep = " <<edep<<", "
                      <<"eion = " <<eion<<", "
                      <<"light_yield = " <<light_yield
                      << endl;
                }

            }
          else
            {
              light_yield = eion;
            }

          if (light_balance_)
            {
              float r = sqrt(
                  pow(postPoint->GetPosition().x() / cm, 2)
                      + pow(postPoint->GetPosition().y() / cm, 2));
              const float cor = GetLightCorrection(r);
              light_yield = light_yield * cor;

              static bool once = true;
              if (once && light_yield>0)
                {
                  once = false;

                  cout << "PHG4OuterHcalSteppingAction::UserSteppingAction::"
                      //
                      << detector_->GetName() << " - "
                      << " use a simple light collection model with linear radial dependence. "
                      <<"First step: "
                      <<"r = " <<r<<", "
                      <<"correction ratio = " <<cor<<", "
                      <<"light_yield after cor. = " <<light_yield
                      << endl;
                }

            }
        }

      //sum up the energy to get total deposited
      hit->set_edep(hit->get_edep() + edep);
      hit->set_eion(hit->get_eion() + eion);
      hit->set_light_yield(hit->get_light_yield() + light_yield);
      hit->set_path_length(aTrack->GetTrackLength() / cm);
      if (geantino)
	{
	  hit->set_edep(-1); // only energy=0 g4hits get dropped, this way geantinos survive the g4hit compression
          hit->set_eion(-1);
	}
      if (edep > 0)
	{
	  if ( G4VUserTrackInformation* p = aTrack->GetUserInformation() )
	    {
	      if ( PHG4TrackUserInfoV1* pp = dynamic_cast<PHG4TrackUserInfoV1*>(p) )
		{
		  pp->SetKeep(1); // we want to keep the track
		}


	    }
	}

      //       hit->identify();
      // return true to indicate the hit was used
      return true;

    }
  else
    {
      return false;
    }
}

//____________________________________________________________________________..
void PHG4OuterHcalSteppingAction::SetInterfacePointers( PHCompositeNode* topNode )
{

  string hitnodename;
  string absorbernodename;
  if (detector_->SuperDetector() != "NONE")
    {
      hitnodename = "G4HIT_" + detector_->SuperDetector();
      absorbernodename =  "G4HIT_ABSORBER_" + detector_->SuperDetector();
    }
  else
    {
      hitnodename = "G4HIT_" + detector_->GetName();
      absorbernodename =  "G4HIT_ABSORBER_" + detector_->GetName();
    }

  //now look for the map and grab a pointer to it.
  hits_ =  findNode::getClass<PHG4HitContainer>( topNode , hitnodename.c_str() );
  absorberhits_ =  findNode::getClass<PHG4HitContainer>( topNode , absorbernodename.c_str() );

  // if we do not find the node it's messed up.
  if ( ! hits_ )
    {
      std::cout << "PHG4OuterHcalSteppingAction::SetTopNode - unable to find " << hitnodename << std::endl;
    }
  if ( ! absorberhits_)
    {
      if (verbosity > 0)
	{
	  cout << "PHG4HcalSteppingAction::SetTopNode - unable to find " << absorbernodename << endl;
	}
    }
}

float PHG4OuterHcalSteppingAction::GetLightCorrection(float r) {
  float m = (light_balance_outer_corr_ - light_balance_inner_corr_)/(light_balance_outer_radius_ - light_balance_inner_radius_);
  float b = light_balance_inner_corr_ - m*light_balance_inner_radius_;
  float value = m*r+b;  
  if (value > 1.0) return 1.0;
  if (value < 0.0) return 0.0;

  return value;
}
