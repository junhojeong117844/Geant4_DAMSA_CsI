#ifndef SteppingAction_h
#define SteppingAction_h 1

#include "G4UserSteppingAction.hh"

class EventAction;
class G4OpBoundaryProcess;
class G4Step;

class SteppingAction : public G4UserSteppingAction
{
public:
    explicit SteppingAction(EventAction* eventAction);
    ~SteppingAction() override = default;

    void UserSteppingAction(const G4Step*) override;

private:
    G4OpBoundaryProcess* GetBoundaryProcess();

    EventAction* fEventAction = nullptr;
    G4OpBoundaryProcess* fBoundaryProcess = nullptr;
};

#endif
