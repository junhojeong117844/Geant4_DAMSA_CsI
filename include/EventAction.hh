#ifndef EventAction_h
#define EventAction_h 1
#include "G4UserEventAction.hh"
#include "globals.hh"
#include <array>
class G4Step;
class DetectorConstruction;
class RunAction;
class EventAction : public G4UserEventAction {
public:
    EventAction(const DetectorConstruction* detector, RunAction* run) : fDetector(detector), fRun(run) {}
    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;
    void AddGeneratedPhoton(G4int crystal) { ++fGeneratedByCrystal.at(crystal-1); }
    void AddEnergyDeposit(G4int crystal, G4double energy) { fEdepByCrystal.at(crystal-1) += energy; }
    void RecordPhoton(G4int crystal, G4int sign, const G4Step* step);
private:
    const DetectorConstruction* fDetector;
    RunAction* fRun;
    std::array<G4double, 8> fEdepByCrystal{};
    std::array<G4int, 8> fGeneratedByCrystal{};
};
#endif
