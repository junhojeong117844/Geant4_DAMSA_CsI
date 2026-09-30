#include "G4RunManagerFactory.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4HadronicParameters.hh"
#include "G4SystemOfUnits.hh"

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "PhysicsList.hh"

#include <algorithm>
#include <thread>

int main(int argc, char** argv)
{
    // In an MT-enabled Geant4 build, Default creates an MT run manager.
    // The final argument requests one worker per available CPU core.
    const G4int numberOfThreads =
        static_cast<G4int>(std::max(1u, std::thread::hardware_concurrency()));

    auto* runManager = G4RunManagerFactory::CreateRunManager(
        G4RunManagerType::Default,
        nullptr,
        true,
        numberOfThreads 
    );

    // Apply the radioactive-decay lifetime threshold before initialization.
    G4HadronicParameters::Instance()
        ->SetTimeThresholdForRadioactiveDecay(1.0e60 * year);

    auto* detector = new DetectorConstruction();
    runManager->SetUserInitialization(detector);
    runManager->SetUserInitialization(new PhysicsList());
    runManager->SetUserInitialization(new ActionInitialization(detector));

    auto* uiManager = G4UImanager::GetUIpointer();

    if (argc == 1)
    {
        auto* ui = new G4UIExecutive(argc, argv);
        auto* visManager = new G4VisExecutive();
        visManager->Initialize();

        uiManager->ApplyCommand("/control/execute vis.mac");
        ui->SessionStart();

        delete visManager;
        delete ui;
    }
    else
    {
        uiManager->ApplyCommand(G4String("/control/execute ") + argv[1]);
    }

    delete runManager;
    return 0;
}
