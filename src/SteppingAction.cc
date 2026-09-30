#include "SteppingAction.hh"

#include "EventAction.hh"

#include "G4AnalysisManager.hh"
#include "G4LogicalVolume.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4OpticalPhoton.hh"
#include "G4PhysicalConstants.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VProcess.hh"

SteppingAction::SteppingAction(EventAction* eventAction)
: fEventAction(eventAction)
{
}

G4OpBoundaryProcess* SteppingAction::GetBoundaryProcess()
{
    if (fBoundaryProcess)
        return fBoundaryProcess;

    auto* processManager =
        G4OpticalPhoton::OpticalPhotonDefinition()->GetProcessManager();

    if (!processManager)
        return nullptr;

    auto* processList = processManager->GetProcessList();

    if (!processList)
        return nullptr;

    for (G4int i = 0; i < processList->size(); ++i)
    {
        auto* boundary =
            dynamic_cast<G4OpBoundaryProcess*>((*processList)[i]);

        if (boundary)
        {
            fBoundaryProcess = boundary;
            break;
        }
    }

    return fBoundaryProcess;
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

    // Energy deposited in the CsI crystal.
    const G4double edep = step->GetTotalEnergyDeposit();

	if (preLogicalName == "logicScintillator" && edep > 0.)
	{
		fEventAction->AddPSEdep(edep);
	}

    if (preLogicalName == "logicCrystal" && edep > 0.)
        fEventAction->AddCrystalEdep(edep);

    // Count and histogram each scintillation photon at the step where it is
    // generated in the CsI crystal. H1 itself is defined only in RunAction.
    if (preLogicalName == "logicCrystal")
    {
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

                const auto* creatorProcess = secondary->GetCreatorProcess();

                if (!creatorProcess ||
                    creatorProcess->GetProcessName() != "Scintillation")
                {
                    continue;
                }

                fEventAction->AddGeneratedPhoton();

                const G4double photonEnergy =
                    secondary->GetKineticEnergy();

                if (photonEnergy > 0.)
                {
                    const G4double wavelength =
                        h_Planck * c_light / photonEnergy;

                    // H1 ID 0: GeneratedWavelength
                    G4AnalysisManager::Instance()->FillH1(
                        0,
                        wavelength / nm
                    );
                }
            }
        }
    }

    // Only optical photons created by scintillation in the CsI can be counted
    // as S13/S14 detections.
    if (track->GetDefinition() !=
        G4OpticalPhoton::OpticalPhotonDefinition())
    {
        return;
    }

    const auto* creatorProcess = track->GetCreatorProcess();
    const auto* vertexVolume = track->GetLogicalVolumeAtVertex();

    if (!creatorProcess ||
        creatorProcess->GetProcessName() != "Scintillation" ||
        !vertexVolume ||
        vertexVolume->GetName() != "logicCrystal")
    {
        return;
    }

    if (postStepPoint->GetStepStatus() != fGeomBoundary)
        return;

    auto* boundaryProcess = GetBoundaryProcess();

    if (!boundaryProcess || boundaryProcess->GetStatus() != Detection)
        return;

    const auto* postVolume = postStepPoint->GetPhysicalVolume();

    if (!postVolume)
        return;

    const G4String& logicalName =
        postVolume->GetLogicalVolume()->GetName();

    if (logicalName == "logicS13")
        fEventAction->AddS13Photon();
    else if (logicalName == "logicS14")
        fEventAction->AddS14Photon();
}
