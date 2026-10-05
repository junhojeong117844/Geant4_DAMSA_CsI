#include "EventAction.hh"
#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4Step.hh"

void EventAction::BeginOfEventAction(const G4Event*)
{
    fGeneratedByCrystal.fill(0);
    fEdepByCrystal.fill(0.);
    // Clear contents without replacing the vectors bound to the analysis manager.
    for (G4int channel = 1; channel <= 16; ++channel) {
        auto& p = fRun->Photons(channel);
        p.energy_eV.clear();
        p.x_mm.clear();
        p.y_mm.clear();
        p.z_mm.clear();
    }
}

void EventAction::RecordPhoton(G4int crystal, G4int sign, const G4Step* step)
{
    auto& p = fRun->Photons(fDetector->Channel(crystal, sign));
    const auto* pre = step->GetPreStepPoint();
    const auto* post = step->GetPostStepPoint();
    const auto position = post->GetPosition();
    const auto energy = pre->GetKineticEnergy();
    p.energy_eV.push_back(energy/eV);
    p.x_mm.push_back(position.x()/mm);
    p.y_mm.push_back(position.y()/mm);
    p.z_mm.push_back(position.z()/mm);
}

void EventAction::EndOfEventAction(const G4Event* event)
{
    const auto& c = fDetector->GetConfiguration();
    auto* a = G4AnalysisManager::Instance();
    for (G4int crystal = 1; crystal <= DetectorConstruction::CrystalCount; ++crystal) {
        for (int sign : {1, -1}) {
            const auto& area = sign > 0 ? c.s13 : c.s14;
            if (!area.enabled) continue;
            const auto channel = fDetector->Channel(crystal, sign);
            const auto tree = channel-1;
            a->FillNtupleIColumn(tree, 0, event->GetEventID());
            a->FillNtupleIColumn(tree, 1, fGeneratedByCrystal.at(crystal-1));
            a->FillNtupleIColumn(tree, 2, fRun->Photons(channel).energy_eV.size());
            a->FillNtupleIColumn(tree, 7, crystal);
            a->FillNtupleDColumn(tree, 8, fEdepByCrystal.at(crystal-1)/MeV);
            a->AddNtupleRow(tree);
        }
    }
    fRun->EventCompleted();
}
