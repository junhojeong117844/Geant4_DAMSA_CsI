#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"

class G4Run;
class DetectorConstruction;

class RunAction : public G4UserRunAction
{
public:
    explicit RunAction(const DetectorConstruction* detector);
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;
private:
    const DetectorConstruction* fDetector;
    bool fBooked = false;
};

#endif
