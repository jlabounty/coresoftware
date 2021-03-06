#ifndef PHG4VUserTrackingAction_h
#define PHG4VUserTrackingAction_h

#include <Geant4/G4UserTrackingAction.hh>
#include <list>

// Master UserTrackingAction: tracking actions can be registered with this class
// and they will be called in the order of registration.

class G4Track;
class PHG4TrackingAction;

class PHG4PhenixTrackingAction : public G4UserTrackingAction
{
public:
  PHG4PhenixTrackingAction( void ) : verbosity_(0) {}

  virtual ~PHG4PhenixTrackingAction() {}

  //! register an action. This is called in PHG4Reco::Init based on which actions are found on the tree
  void AddAction( PHG4TrackingAction* action ) { actions_.push_back( action ); }

  virtual void PreUserTrackingAction(const G4Track*);

  virtual void PostUserTrackingAction(const G4Track*);

  //! Get/Set verbosity level
  void Verbosity(int val) { verbosity_ = val; }
  int Verbosity() const { return verbosity_; }

private:

  //! list of subsystem specific Event actions
  typedef std::list<PHG4TrackingAction*> ActionList;
  ActionList actions_;
  int verbosity_;
};


#endif
