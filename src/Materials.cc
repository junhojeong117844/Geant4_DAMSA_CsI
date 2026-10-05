#include "Materials.hh"

#include "G4Element.hh"
#include "G4NistManager.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

#include <algorithm>
#include <vector>

namespace
{
G4double InterpolateEmission(G4double wavelengthNm)
{
    // Approximate digitization of the supplied undoped-CsI X-ray emission
    // plot. The curve is normalized to 1 at about 300 nm.
    static const std::vector<G4double> wavelength = {
        250., 260., 270., 280., 290., 300., 310., 320., 330., 340.,
        350., 360., 380., 400., 450., 500., 550., 600., 650., 700.
    };
    static const std::vector<G4double> intensity = {
        0.035, 0.105, 0.280, 0.650, 0.910, 1.000, 0.900, 0.620,
        0.380, 0.220, 0.130, 0.090, 0.060, 0.045, 0.035, 0.030,
        0.020, 0.010, 0.004, 0.000
    };

    if (wavelengthNm <= wavelength.front())
        return intensity.front();
    if (wavelengthNm >= wavelength.back())
        return intensity.back();

    const auto upper = std::upper_bound(
        wavelength.begin(), wavelength.end(), wavelengthNm
    );
    const std::size_t i1 = static_cast<std::size_t>(
        std::distance(wavelength.begin(), upper)
    );
    const std::size_t i0 = i1 - 1;

    const G4double fraction =
        (wavelengthNm - wavelength[i0]) /
        (wavelength[i1] - wavelength[i0]);

    return intensity[i0] + fraction * (intensity[i1] - intensity[i0]);
}
}

Materials::Materials() = default;
Materials::~Materials() = default;

void Materials::DefineMaterials()
{
    auto* nist = G4NistManager::Instance();

    auto* H = nist->FindOrBuildElement("H");
    auto* C = nist->FindOrBuildElement("C");
    auto* O = nist->FindOrBuildElement("O");
    auto* Si = nist->FindOrBuildElement("Si");
    auto* Cs = nist->FindOrBuildElement("Cs");
    auto* I = nist->FindOrBuildElement("I");
    auto* Na = nist->FindOrBuildElement("Na");
    auto* Fe = nist->FindOrBuildElement("Fe");

    colliMat = new G4Material("colliMat", 7.874*g/cm3, 1);
    colliMat->AddElement(Fe, 1);

    cookieMat = new G4Material("OpticalCookie", 1.2*g/cm3, 1);
    cookieMat->AddElement(Si, 1);

    greaseMat = new G4Material("OpticalGrease", 1.06*g/cm3, 4);
    greaseMat->AddElement(H, 8);
    greaseMat->AddElement(C, 2);
    greaseMat->AddElement(O, 1);
    greaseMat->AddElement(Si, 1);

    sodiumMat = new G4Material("Sodium", 0.97*g/cm3, 1);
    sodiumMat->AddElement(Na, 1);

    CsIMat = new G4Material("CsI", 4.51*g/cm3, 2);
    CsIMat->AddElement(Cs, 1);
    CsIMat->AddElement(I, 1);

    PSMat = new G4Material("PS", 1.032*g/cm3, 2);
    PSMat->AddElement(C, 9);
    PSMat->AddElement(H, 10);

    worldMat = nist->FindOrBuildMaterial("G4_Galactic");
    airMat = nist->FindOrBuildMaterial("G4_AIR");
    alMat = nist->FindOrBuildMaterial("G4_Al");
    teflonMat = nist->FindOrBuildMaterial("G4_TEFLON");
    auto* acrylicMat = nist->FindOrBuildMaterial("G4_PLEXIGLASS");

    constexpr G4int nEntries = 22;
    G4double photonEnergy[nEntries] = {
        1.80*eV, 1.85*eV, 1.91*eV, 1.97*eV, 2.03*eV,
        2.10*eV, 2.17*eV, 2.25*eV, 2.34*eV, 2.43*eV,
        2.53*eV, 2.64*eV, 2.75*eV, 2.88*eV, 3.02*eV,
        3.18*eV, 3.35*eV, 3.54*eV, 3.76*eV, 4.00*eV,
        4.28*eV, 4.59*eV
    };

    G4double csiRIndex[nEntries] = {
        1.7310, 1.7315, 1.7330, 1.7345, 1.7360,
        1.7380, 1.7395, 1.7420, 1.7460, 1.7500,
        1.7545, 1.7615, 1.7700, 1.7790, 1.7890,
        1.8000, 1.8140, 1.8335, 1.8560, 1.8845,
        1.9285, 1.9990
    };

    G4double csiAbsLength[nEntries];
    G4double airRIndex[nEntries];
    G4double airAbsLength[nEntries];
    G4double reflectorReflectivity[nEntries];
    G4double zeroEfficiency[nEntries];
    G4double cookieRindex[nEntries];
    G4double cookieAbs[nEntries];
    G4double greaseRindex[nEntries];
    G4double greaseAbs[nEntries];
    G4double acrylicRindex[nEntries];
    G4double acrylicAbs[nEntries];

	for (G4int i = 0; i < nEntries; ++i)
	{
		csiAbsLength[i] = 500.*mm;
		airRIndex[i] = 1.0;
		airAbsLength[i] = 10.*m;
		reflectorReflectivity[i] = 0.95;
		zeroEfficiency[i] = 0.0;
		cookieRindex[i] = 1.43;
		cookieAbs[i] = 10000.*mm;
		greaseRindex[i] = 1.465;
		greaseAbs[i] = 10000.*mm;
		// Placeholder PMMA optics, NOT a measured UV-transmission model.
		// T_bulk(d) = exp(-d/ABSLENGTH): 99.80% for a 20 mm path.
		// Fresnel losses are handled separately by optical boundaries.
		// Do not interpret the visual alpha or visible-light ~92% sheet
		// transmission as transmission at the CsI emission peak (~300 nm).
		// See ACRYLIC_OPTICS.md before substituting measured spectral data.
		acrylicRindex[i] = 1.49;
		acrylicAbs[i] = 10000.*mm;
	}

	auto* mptCookie = new G4MaterialPropertiesTable();
	mptCookie->AddProperty("RINDEX", photonEnergy, cookieRindex, nEntries);
	mptCookie->AddProperty("ABSLENGTH", photonEnergy, cookieAbs, nEntries);
	cookieMat->SetMaterialPropertiesTable(mptCookie);

	auto* mptGrease = new G4MaterialPropertiesTable();
	mptGrease->AddProperty("RINDEX", photonEnergy, greaseRindex, nEntries);
	mptGrease->AddProperty("ABSLENGTH", photonEnergy, greaseAbs, nEntries);
	greaseMat->SetMaterialPropertiesTable(mptGrease);

	auto* mptAcrylic = new G4MaterialPropertiesTable();
	mptAcrylic->AddProperty("RINDEX", photonEnergy, acrylicRindex, nEntries);
	mptAcrylic->AddProperty("ABSLENGTH", photonEnergy, acrylicAbs, nEntries);
	acrylicMat->SetMaterialPropertiesTable(mptAcrylic);

	G4double teflonRIndex[nEntries];
	G4double teflonAbsLength[nEntries];
	for (G4int i = 0; i < nEntries; ++i)
	{
		teflonRIndex[i] = 1.35;
		teflonAbsLength[i] = 1.*mm;
	}

	auto* mptTeflonMaterial = new G4MaterialPropertiesTable();
	mptTeflonMaterial->AddProperty(
			"RINDEX", photonEnergy, teflonRIndex, nEntries
			);
	mptTeflonMaterial->AddProperty(
			"ABSLENGTH", photonEnergy, teflonAbsLength, nEntries
			);
	teflonMat->SetMaterialPropertiesTable(mptTeflonMaterial);

	crystalSurf = new G4OpticalSurface("CrystalSurface");
	crystalSurf->SetModel(unified);
	crystalSurf->SetType(dielectric_dielectric);
	crystalSurf->SetFinish(polished);

	alReflectorSurf = new G4OpticalSurface("AlReflectorSurface");
	alReflectorSurf->SetModel(unified);
	alReflectorSurf->SetType(dielectric_metal);
	alReflectorSurf->SetFinish(polished);

	teflonReflectorSurf = new G4OpticalSurface("TeflonReflectorSurface");
	teflonReflectorSurf->SetModel(unified);
	teflonReflectorSurf->SetType(dielectric_dielectric);
	teflonReflectorSurf->SetFinish(groundbackpainted);

	auto* mptAlReflector = new G4MaterialPropertiesTable();
	mptAlReflector->AddProperty(
			"REFLECTIVITY", photonEnergy, reflectorReflectivity, nEntries
			);
	mptAlReflector->AddProperty(
			"EFFICIENCY", photonEnergy, zeroEfficiency, nEntries
			);
	alReflectorSurf->SetMaterialPropertiesTable(mptAlReflector);

	auto* mptTeflonReflector = new G4MaterialPropertiesTable();
	// Back-painted surfaces require the refractive index of the paint layer.
	mptTeflonReflector->AddProperty(
			"RINDEX", photonEnergy, teflonRIndex, nEntries
			);
	mptTeflonReflector->AddProperty(
			"REFLECTIVITY", photonEnergy, reflectorReflectivity, nEntries
			);
	mptTeflonReflector->AddProperty(
			"EFFICIENCY", photonEnergy, zeroEfficiency, nEntries
			);
	teflonReflectorSurf->SetMaterialPropertiesTable(mptTeflonReflector);

	// 2-nm sampling from 250 to 350 nm, followed by long-wavelength tail point
	std::vector<G4double> emissionWavelengthNm;
	for (G4int wavelengthNm = 250; wavelengthNm <= 350; wavelengthNm += 2)
		emissionWavelengthNm.push_back(static_cast<G4double>(wavelengthNm));

	const std::vector<G4double> tailWavelengthNm = {
		360., 370., 380., 390., 400., 425., 450., 475., 500.,
		525., 550., 575., 600., 625., 650., 675., 700.
	};
	emissionWavelengthNm.insert(
			emissionWavelengthNm.end(),
			tailWavelengthNm.begin(),
			tailWavelengthNm.end()
			);

	std::vector<G4double> emissionEnergy;
	std::vector<G4double> emissionIntensity;
	emissionEnergy.reserve(emissionWavelengthNm.size());
	emissionIntensity.reserve(emissionWavelengthNm.size());

	for (auto it = emissionWavelengthNm.rbegin();
			it != emissionWavelengthNm.rend(); ++it)
	{
		emissionEnergy.push_back((h_Planck * c_light) / (*it * nm));
		emissionIntensity.push_back(InterpolateEmission(*it));
	}

	auto* mptCsI = new G4MaterialPropertiesTable();
	mptCsI->AddProperty("RINDEX", photonEnergy, csiRIndex, nEntries);
	mptCsI->AddProperty("ABSLENGTH", photonEnergy, csiAbsLength, nEntries);
	mptCsI->AddProperty(
			"SCINTILLATIONCOMPONENT1",
			emissionEnergy.data(),
			emissionIntensity.data(),
			static_cast<G4int>(emissionEnergy.size())
			);
	mptCsI->AddConstProperty("SCINTILLATIONYIELD", 2100./MeV);
	mptCsI->AddConstProperty("RESOLUTIONSCALE", 1.0);
	mptCsI->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 16.*ns);
	CsIMat->SetMaterialPropertiesTable(mptCsI);

	auto* mptAir = new G4MaterialPropertiesTable();
	mptAir->AddProperty("RINDEX", photonEnergy, airRIndex, nEntries);
	mptAir->AddProperty("ABSLENGTH", photonEnergy, airAbsLength, nEntries);
	airMat->SetMaterialPropertiesTable(mptAir);

	auto* mptWorld = new G4MaterialPropertiesTable();
	mptWorld->AddProperty("RINDEX", photonEnergy, airRIndex, nEntries);
	mptWorld->AddProperty("ABSLENGTH", photonEnergy, airAbsLength, nEntries);
	worldMat->SetMaterialPropertiesTable(mptWorld);

}
