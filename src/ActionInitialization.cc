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
    auto* runAction = new RunAction(fDetector);
    SetUserAction(runAction);

    auto* eventAction = new EventAction(fDetector, runAction);
    SetUserAction(eventAction);
    SetUserAction(new SteppingAction(eventAction, fDetector));
}
