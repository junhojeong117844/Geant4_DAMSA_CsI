#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include <chrono>
#include <algorithm>
#include <mutex>
#include <iomanip>
#include <sstream>

namespace {
using Clock = std::chrono::steady_clock;
std::mutex progressMutex;
G4int completedEvents = 0;
G4int totalEvents = 0;
bool outputFailed = false;
G4int outputsFinished = 0;
Clock::time_point started, lastReport;
}

RunAction::RunAction(const DetectorConstruction* detector) : fDetector(detector)
{
    auto* a = G4AnalysisManager::Instance();
    a->SetNtupleMerging(true);
    a->SetFileName("../datas/output.root"); // Can be overridden with /analysis/setFileName.
}

void RunAction::BeginOfRunAction(const G4Run* run)
{
    if (IsMaster()) {
        std::lock_guard<std::mutex> lock(progressMutex);
        completedEvents = 0;
        outputFailed = false;
        outputsFinished = 0;
        totalEvents = run->GetNumberOfEventToBeProcessed();
        started = lastReport = Clock::now();
        G4cout << "[Events] 0/" << totalEvents << " events (0%)" << G4endl;
    }
    auto* a = G4AnalysisManager::Instance();
    // Book the configured output schema on every thread.
    if (!fBooked) {
        const auto& c = fDetector->GetConfiguration();
        for (G4int crystal = 1; crystal <= DetectorConstruction::CrystalCount; ++crystal) {
            for (int sign : {1, -1}) {
                if (!(sign > 0 ? c.s13.enabled : c.s14.enabled)) continue;
                const auto channel = fDetector->Channel(crystal, sign);
                auto& p = Photons(channel);
                a->CreateNtuple("SiPM" + std::to_string(channel), "SiPM event data; photon arrays share indices");
                for (const auto* name : {"eventID", "Generated_photons", "photon_count"})
                    a->CreateNtupleIColumn(name);
                a->CreateNtupleDColumn("energy_eV", p.energy_eV);
                a->CreateNtupleDColumn("x_mm", p.x_mm);
                a->CreateNtupleDColumn("y_mm", p.y_mm);
                a->CreateNtupleDColumn("z_mm", p.z_mm);
                a->CreateNtupleIColumn("crystalID");
                a->CreateNtupleDColumn("edep_MeV");
                a->FinishNtuple();
            }
        }
        fBooked = true;
    }
    if (!a->OpenFile()) {
        std::lock_guard<std::mutex> lock(progressMutex);
        outputFailed = true;
        G4cerr << "[Output] ERROR: could not open ROOT output." << G4endl;
    }

}

void RunAction::EventCompleted()
{
    std::lock_guard<std::mutex> lock(progressMutex);
    ++completedEvents;
    const auto now = Clock::now();
    if (completedEvents != totalEvents && now-lastReport < std::chrono::seconds(10))
        return;
    lastReport = now;
    if (completedEvents == totalEvents) {
        G4cout << "[Output] All " << totalEvents
               << " events processed. Finalizing ROOT output (merging/writing/closing)."
               << " Run NOT finished; do not stop the process. Output ETA unavailable."
               << G4endl;
        return;
    }
    const auto elapsed = std::chrono::duration<double>(now-started).count();
    const auto remaining = elapsed*(totalEvents-completedEvents)/completedEvents;
    const auto remainingSeconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::duration<double>(remaining)).count();
    std::ostringstream message;
    message << "[Events] " << completedEvents << '/' << totalEvents
            << " events (" << std::fixed << std::setprecision(1)
            << (totalEvents > 0 ? std::min(99.9, 100.0*completedEvents/totalEvents) : 0.0)
            << "%) | event-processing ETA " << remainingSeconds/3600 << "h "
            << (remainingSeconds%3600)/60 << "m " << remainingSeconds%60 << "s";
    G4cout << message.str() << G4endl;
}

void RunAction::EndOfRunAction(const G4Run*)
{
    auto* a = G4AnalysisManager::Instance();
    if (IsMaster())
        G4cout << "[Output] Writing final ROOT metadata..." << G4endl;
    const auto outputStarted = Clock::now();
    const bool written = a->Write();
    if (IsMaster())
        G4cout << "[Output] Closing ROOT file..." << G4endl;
    const bool closed = a->CloseFile();
    std::lock_guard<std::mutex> lock(progressMutex);
    outputFailed = outputFailed || !written || !closed;
    if (!IsMaster()) {
        ++outputsFinished;
        G4cout << "[Output] Worker output finalized: " << outputsFinished
               << " | write/close "
               << std::chrono::duration<double>(Clock::now()-outputStarted).count()
               << " s" << (written && closed ? "" : " (FAILED)") << G4endl;
        return;
    }
    if (outputFailed) {
        G4cerr << "[Run] FAILED: ROOT output reported an error; output may be incomplete."
               << G4endl;
    } else if (completedEvents != totalEvents) {
        G4cout << "[Run] Stopped early: " << completedEvents << '/' << totalEvents
               << " events. ROOT output closed." << G4endl;
    } else {
        G4cout << "[Run] 100% COMPLETE: ROOT output written and closed. Total elapsed "
               << std::chrono::duration<double>(Clock::now()-started).count()
               << " s. Safe to exit." << G4endl;
    }
}
