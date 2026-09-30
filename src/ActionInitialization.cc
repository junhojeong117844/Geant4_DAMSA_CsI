#include "ActionInitialization.hh"

#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

void ActionInitialization::BuildForMaster() const
{
    SetUserAction(new RunAction(fDetector));
}

void ActionInitialization::Build() const
{
    SetUserAction(new PrimaryGeneratorAction());
    SetUserAction(new RunAction(fDetector));

    auto* eventAction = new EventAction(fDetector);
    SetUserAction(eventAction);
    SetUserAction(new SteppingAction(eventAction));
}
