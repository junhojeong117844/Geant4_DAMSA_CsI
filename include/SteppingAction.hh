#ifndef SteppingAction_h
#define SteppingAction_h 1

#include "G4UserSteppingAction.hh"

class EventAction;
class DetectorConstruction;
class G4Step;

class SteppingAction : public G4UserSteppingAction
{
public:
    SteppingAction(EventAction* eventAction, const DetectorConstruction* detector);
    ~SteppingAction() override = default;

    void UserSteppingAction(const G4Step*) override;

private:

    EventAction* fEventAction = nullptr;
    const DetectorConstruction* fDetector;
};

#endif
