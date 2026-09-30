#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class G4Event;
class DetectorConstruction;

class EventAction : public G4UserEventAction
{
public:
    explicit EventAction(const DetectorConstruction* detector) : fDetector(detector) {}
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;

    void AddGeneratedPhoton() { ++fGeneratedPhotons; }
    void AddS13Photon() { ++fS13Photons; }
    void AddS14Photon() { ++fS14Photons; }
    void AddPSEdep(G4double edep) { fPSEdep += edep; }
    void AddCrystalEdep(G4double edep) { fCrystalEdep += edep; }

private:
    const DetectorConstruction* fDetector;
    G4int fGeneratedPhotons = 0;
    G4int fS13Photons = 0;
    G4int fS14Photons = 0;

    G4double fPSEdep = 0.;
    G4double fCrystalEdep = 0.;
};

#endif
