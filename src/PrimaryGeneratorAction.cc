#include "PrimaryGeneratorAction.hh"
#include "GeometryLayout.hh"
#include "G4RunManager.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

thread_local std::vector<G4double> srEnergy;
thread_local std::vector<G4double> srRatio;
thread_local std::vector<G4double> yEnergy;
thread_local std::vector<G4double> yRatio;

void ReadSpectrum(const G4String& fileName,
                  std::vector<G4double>& energy,
                  std::vector<G4double>& ratio)
{
    std::ifstream file(fileName);
    G4String line;

    std::getline(file, line);  // 첫 번째 제목 줄 제외

    while (std::getline(file, line))
    {
        std::stringstream ss(line);

        G4String indexText;
        G4String energyText;
        G4String ratioText;

        std::getline(ss, indexText, ',');
        std::getline(ss, energyText, ',');
        std::getline(ss, ratioText, ',');

        energy.push_back(std::stod(energyText));
        ratio.push_back(std::stod(ratioText));
    }
}

G4double SampleEnergy(const std::vector<G4double>& energy,
                      const std::vector<G4double>& ratio)
{
    G4double totalRatio = 0.0;

    for (G4double value : ratio)
        totalRatio += value;

    G4double randomRatio = G4UniformRand() * totalRatio;
    G4double sum = 0.0;

    for (G4int i = 0; i < ratio.size(); i++)
    {
       /* sum += ratio[i];

        if (randomRatio <= sum)
            return energy[i];
*/
		sum += ratio[i];

        if (randomRatio <= sum)
        {
            G4double lowEdge;
            G4double highEdge;

            if (i == 0)
                lowEdge = energy[i];
            else
                lowEdge = 0.5 * (energy[i - 1] + energy[i]);

            if (i == energy.size() - 1)
                highEdge = energy[i];
            else
                highEdge = 0.5 * (energy[i] + energy[i + 1]);

            return lowEdge
                 + G4UniformRand() * (highEdge - lowEdge);
        }
    }

    return energy.back();
}

PrimaryGeneratorAction::PrimaryGeneratorAction()
: G4VUserPrimaryGeneratorAction()
{
    fParticleGun = new G4ParticleGun();

    ReadSpectrum("../datas/Sr-90 Beta Spectrum.csv", srEnergy, srRatio);
    ReadSpectrum("../datas/Y-90 Beta Spectrum.csv", yEnergy, yRatio);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
    G4ParticleTable* particleTable =
        G4ParticleTable::GetParticleTable();

    const auto* detector = static_cast<const DetectorConstruction*>(
        G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    const GeometryLayout layout(detector->GetConfiguration());

    G4String particleName = "beam"; // Sr90_collimator, ion, beam usable

    if (particleName == "Sr90_collimator")
    {
        G4double beamEnergy;

        if (G4UniformRand() < 0.5)
            beamEnergy = SampleEnergy(srEnergy, srRatio);
        else
            beamEnergy = SampleEnergy(yEnergy, yRatio);

        G4ParticleDefinition* electron =
            particleTable->FindParticle("e-");

        fParticleGun->SetParticleDefinition(electron);
        fParticleGun->SetParticleEnergy(beamEnergy * MeV);

        G4double collimatorLength = 33.0 * mm;
        G4double collimatorRadius = 3.0 * mm;

        G4double r = collimatorRadius * std::sqrt(G4UniformRand());
        G4double phi = 2.0 * CLHEP::pi * G4UniformRand();

        G4double x = r * std::cos(phi);
        G4double z = r * std::sin(phi);

        fParticleGun->SetParticlePosition(
            G4ThreeVector(0.0, 41.0 * mm + layout.sideShift, 0.0)
        );

        fParticleGun->SetParticleMomentumDirection(
            G4ThreeVector(x, -collimatorLength, z).unit()
        );
    }
    else if (particleName == "beam")
    {
        G4ParticleDefinition* gamma =
            particleTable->FindParticle("proton");

        fParticleGun->SetParticleDefinition(gamma);
        fParticleGun->SetParticleEnergy(480.0 * MeV);
        fParticleGun->SetParticlePosition(
            G4ThreeVector(0.0, 6.0 * mm + layout.sideShift, -0.0)
        );
        fParticleGun->SetParticleMomentumDirection(
            G4ThreeVector(0.0, -1.0, 0.0)
        );
    }
    else if (particleName == "ion")
    {
        // Cs-137
        //const G4int Z = 55;
        //const G4int A = 137;

		// Sr-90
		const G4int Z = 38;
		const G4int A = 90;

        G4ParticleDefinition* ion =
            particleTable->GetIonTable()->GetIon(Z, A, 0.0 * keV);

        fParticleGun->SetParticleDefinition(ion);
        fParticleGun->SetParticleCharge(0.0);
        fParticleGun->SetParticleEnergy(0.0 * keV);
        fParticleGun->SetParticlePosition(
            G4ThreeVector(0.0, 7.0 * mm + layout.sideShift, -55.0 * mm)
        );
    }

    fParticleGun->GeneratePrimaryVertex(anEvent);
}
