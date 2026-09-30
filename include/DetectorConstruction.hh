#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include "G4ThreeVector.hh"

class Materials;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    // All lengths are full sizes. Defaults are assigned in DetectorConstruction.cc.
    struct Sensor {
        G4bool enabled;
        G4double width, height, thickness;
        G4double gap;
        G4String gapMaterial;
        G4bool lightGuide;
        G4double guideLength, guideGap;
        G4String guideMaterial;
    };
    struct Configuration {
        Sensor s13, s14; // +z and -z, respectively
        G4String reflector, trigger;
        G4double sideGap, foilThickness;
        G4bool collimator, sourceBead, checkOverlaps;
        G4int selfThreshold;
        G4String selfChannel;
        G4double externalThreshold;
    };

    DetectorConstruction();
    ~DetectorConstruction() override;
    G4VPhysicalVolume* Construct() override;
    const Configuration& GetConfiguration() const { return fConfig; }
    G4bool HasExternalTrigger() const { return fConfig.trigger == "external"; }
    G4String SelectedTreeName() const;
    G4bool AcceptEvent(G4int s13, G4int s14, G4double psEdep) const;

private:
    Configuration fConfig;
    Materials* fMaterials = nullptr;
    void Validate() const;
};
#endif
