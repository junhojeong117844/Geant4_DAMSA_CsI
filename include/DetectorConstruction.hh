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
    struct RecordingArea {
        G4bool enabled;
        G4double width, height, offsetX, offsetY;
        G4double gap; // Crystal-to-guide gap, or recording gap when no guide is used.
        G4String gapMaterial;
        G4bool lightGuide;
        G4double guideLength, guideWidth, guideHeight;
        G4String guideMaterial;
    };
    struct Configuration {
        RecordingArea s13, s14; // +z and -z, respectively
        G4String reflector, trigger;
        G4double sideGap, foilThickness;
        G4bool collimator, sourceBead, checkOverlaps;
        G4int selfThreshold;
        G4String selfChannel;
        G4double externalThreshold;
    };

    static constexpr G4int CrystalCount = 8;
    G4int Channel(G4int crystal, G4int sign) const {
        return (crystal-1)*(fConfig.s13.enabled + fConfig.s14.enabled)
             + (sign > 0 || !fConfig.s13.enabled ? 1 : 2);
    }
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
