#include "SteppingAction.hh"

#include "EventAction.hh"

#include "GeometryLayout.hh"
#include "G4LogicalVolume.hh"
#include "DetectorConstruction.hh"
#include "G4GeometryTolerance.hh"
#include <cmath>
#include "G4OpticalPhoton.hh"
#include "G4PhysicalConstants.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VProcess.hh"

SteppingAction::SteppingAction(EventAction* eventAction, const DetectorConstruction* detector)
: fEventAction(eventAction), fDetector(detector)
{
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    if (!step || !fEventAction)
        return;

    const auto* preStepPoint = step->GetPreStepPoint();
    const auto* postStepPoint = step->GetPostStepPoint();
    auto* track = step->GetTrack();

    if (!preStepPoint || !postStepPoint || !track)
        return;

    const auto* preVolume = preStepPoint->GetPhysicalVolume();
    const G4String preLogicalName =
        preVolume ? preVolume->GetLogicalVolume()->GetName() : "";

    // Count all optical photons produced in each crystal, including scintillation.
    if (preLogicalName == "logicCrystal")
    {
        fEventAction->AddEnergyDeposit(preVolume->GetCopyNo(), step->GetTotalEnergyDeposit());
        const auto* secondaries = step->GetSecondaryInCurrentStep();

        if (secondaries)
        {
            for (const auto* secondary : *secondaries)
            {
                if (!secondary ||
                    secondary->GetDefinition() !=
                        G4OpticalPhoton::OpticalPhotonDefinition())
                {
                    continue;
                }

                fEventAction->AddGeneratedPhoton(preVolume->GetCopyNo());


            }
        }
    }

    // Virtual collector: without a guide, photons must traverse the end gap.
    // No physical SiPM, PDE, or Detection status is used.
    if (track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition() ||
        postStepPoint->GetStepStatus() != fGeomBoundary)
        return;

    const auto position = postStepPoint->GetPosition();
    const auto direction = preStepPoint->GetMomentumDirection();
    const auto tolerance = G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();
    if (!preVolume) return;
    const auto crystal = preVolume->GetCopyNo();
    if (crystal < 1 || crystal > DetectorConstruction::CrystalCount) return;
    const GeometryLayout layout(fDetector->GetConfiguration());
    const auto& config = fDetector->GetConfiguration();
    for (const int sign : {1, -1}) {
        const auto& area = sign > 0 ? config.s13 : config.s14;
        if (!area.enabled) continue;
        const G4String host = area.lightGuide
            ? (sign > 0 ? "logicS13Guide" : "logicS14Guide")
            : area.gap > 0 ? (sign > 0 ? "logicS13EndAir" : "logicS14EndAir")
                           : "logicCrystal";
        const auto planeZ = sign*(60*mm + (area.gap + (area.lightGuide ? area.guideLength : 0)));
        if (preLogicalName != host || sign*direction.z() <= 0 ||
            std::abs(position.z()-planeZ) > tolerance ||
            std::abs(position.x()-area.offsetX) > area.width/2 ||
            std::abs(position.y()-layout.CrystalY(crystal)-area.offsetY) > area.height/2)
            continue;
        fEventAction->RecordPhoton(crystal, sign, step);
        // Treat the aperture as a terminal collector: each photon is stored once.
        track->SetTrackStatus(fStopAndKill);
        return;
    }
}
