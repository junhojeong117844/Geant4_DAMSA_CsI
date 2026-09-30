#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include <fstream>

RunAction::RunAction(const DetectorConstruction* detector) : fDetector(detector)
{
    auto* a = G4AnalysisManager::Instance();
    a->SetNtupleMerging(true);
    a->CreateH1("CsI_Wl", "CsI_Wl;Wavelength [nm];Counts", 400, 200., 600.);
    a->SetFileName("output.root"); // Can be overridden with /analysis/setFileName.
}

void RunAction::BeginOfRunAction(const G4Run* run)
{
    auto* a = G4AnalysisManager::Instance();
    // Book the configured output schema on every thread.
    if (!fBooked) {
        for (const auto& name : {fDetector->SelectedTreeName(), G4String("PhotoDetected")}) {
            a->CreateNtuple(name, name);
            a->CreateNtupleIColumn("eventID");
            if (fDetector->HasExternalTrigger()) a->CreateNtupleDColumn("PSEdep_keV");
            a->CreateNtupleDColumn("crystalEdep_keV");
            a->CreateNtupleIColumn("Generated_photons");
            const auto& c = fDetector->GetConfiguration();
            if (c.s13.enabled) a->CreateNtupleIColumn("S13");
            if (c.s14.enabled) a->CreateNtupleIColumn("S14");
            a->FinishNtuple();
        }
        fBooked = true;
    }
    a->OpenFile();
    if (!IsMaster()) return;
    // Keep the settings beside the output so different schemas are identifiable.
    std::ofstream out(a->GetFileName() + ".config.txt");
    const auto& c = fDetector->GetConfiguration();
    out << "runID=" << run->GetRunID() << "\ntrigger=" << c.trigger
        << "\nselfChannel=" << c.selfChannel << "\nselfThreshold_strict_gt=" << c.selfThreshold
        << "\nexternalThreshold_keV_strict_gt=" << c.externalThreshold/keV
        << "\nreflector=" << c.reflector << "\nsideGap_mm=" << c.sideGap/mm
        << "\nfoilThickness_mm=" << c.foilThickness/mm << "\ncollimator=" << c.collimator
        << "\nsourceBead=" << c.sourceBead << '\n';
    for (auto entry : {std::make_pair("s13", &c.s13), std::make_pair("s14", &c.s14)}) {
        const auto& s = *entry.second;
        out << entry.first << ": enabled=" << s.enabled << " width_mm=" << s.width/mm
            << " height_mm=" << s.height/mm << " thickness_mm=" << s.thickness/mm
            << " gap_mm=" << s.gap/mm << " gapMaterial=" << s.gapMaterial
            << " lightGuide=" << s.lightGuide << " guideLength_mm=" << s.guideLength/mm
            << " guideGap_mm=" << s.guideGap/mm << " guideMaterial=" << s.guideMaterial << '\n';
    }
}

void RunAction::EndOfRunAction(const G4Run*)
{
    auto* a = G4AnalysisManager::Instance();
    a->Write();
    a->CloseFile();
}
