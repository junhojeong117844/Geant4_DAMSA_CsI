#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"

#include <atomic>

namespace
{
    std::atomic<G4long> completedEvents{0};
}

void EventAction::BeginOfEventAction(const G4Event*)
{
    fGeneratedPhotons = 0;
    fS13Photons = 0;
    fS14Photons = 0;

    fPSEdep = 0.;
    fCrystalEdep = 0.;
}

void EventAction::EndOfEventAction(const G4Event* event)
{
	const G4long completed = ++completedEvents;

    if (completed % 10000 == 0)
    {
        G4cout << "Completed events: "
               << completed
               << G4endl;
    }

    auto* analysisManager = G4AnalysisManager::Instance();
    const G4int eventID = event->GetEventID();

    auto record = [&](G4int tree) {
        G4int column = 0;
        analysisManager->FillNtupleIColumn(tree, column++, eventID);
        if (fDetector->HasExternalTrigger())
            analysisManager->FillNtupleDColumn(tree, column++, fPSEdep / keV);
        analysisManager->FillNtupleDColumn(tree, column++, fCrystalEdep / keV);
        analysisManager->FillNtupleIColumn(tree, column++, fGeneratedPhotons);
        const auto& c = fDetector->GetConfiguration();
        if (c.s13.enabled) analysisManager->FillNtupleIColumn(tree, column++, fS13Photons);
        if (c.s14.enabled) analysisManager->FillNtupleIColumn(tree, column++, fS14Photons);
        analysisManager->AddNtupleRow(tree);
    };
    if (fDetector->AcceptEvent(fS13Photons, fS14Photons, fPSEdep)) record(0);
    // Independent diagnostic tree, including events rejected by the trigger.
    if (fS13Photons > 0 || fS14Photons > 0) record(1);
}
