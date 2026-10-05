#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"
#include "globals.hh"
#include <array>
#include <vector>

class G4Run;
class DetectorConstruction;

class RunAction : public G4UserRunAction
{
public:
    struct PhotonData {
        std::vector<G4double> energy_eV, x_mm, y_mm, z_mm;
    };
    PhotonData& Photons(G4int channel) { return fPhotons.at(channel-1); }
    explicit RunAction(const DetectorConstruction* detector);
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;
    void EventCompleted();
private:
    const DetectorConstruction* fDetector;
    std::array<PhotonData, 16> fPhotons;
    bool fBooked = false;
};

#endif
