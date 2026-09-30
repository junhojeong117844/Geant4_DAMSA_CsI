#include "DetectorConstruction.hh"
#include "Materials.hh"
#include "G4Box.hh"
#include "G4Exception.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4RotationMatrix.hh"
#include "G4Sphere.hh"
#include "G4SubtractionSolid.hh"
#include "G4SystemOfUnits.hh"
#include "G4Trd.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"
#include <algorithm>
#include <cmath>

DetectorConstruction::DetectorConstruction()
{
    // ================= USER CONFIGURATION =================
    // S13 is at +z, S14 at -z. Dimensions below are FULL sizes.
    fConfig.s13 = {false, 6*mm, 6*mm, 2*mm, 1*um, "air",
                   false, 10*mm, 1*um, "acrylic"};
    fConfig.s14 = {true, 1.3*mm, 1.3*mm, 0.3*mm, 1*um, "air",
                   false, 10*mm, 1*um, "acrylic"};
    fConfig.reflector = "none"; // none, aluminum, teflon
    fConfig.sideGap = 0.15*mm;
    fConfig.foilThickness = 0.04*mm;
    fConfig.collimator = false;
    fConfig.sourceBead = true;
    fConfig.checkOverlaps = true;
    fConfig.trigger = "self"; // none, self, external
    fConfig.selfChannel = "sum"; // sum, s13, s14, coincidence
    fConfig.selfThreshold = 13; // strict > threshold (Single_sipm: S14 > 13)
    fConfig.externalThreshold = 60.2*keV; // strict > threshold
    // PDE and all optical material properties stay in Materials.cc.
    // ======================================================

    fMaterials = new Materials();
    fMaterials->DefineMaterials();
}

DetectorConstruction::~DetectorConstruction()
{
    delete fMaterials;
}

G4String DetectorConstruction::SelectedTreeName() const
{
    if (fConfig.trigger == "external") return "ExternalTrigger";
    if (fConfig.trigger == "self") return "SelfTrigger";
    return "AllEvents";
}

G4bool DetectorConstruction::AcceptEvent(G4int s13, G4int s14, G4double psEdep) const
{
    if (fConfig.trigger == "none") return true;
    if (fConfig.trigger == "external") return psEdep > fConfig.externalThreshold;
    const auto threshold = fConfig.selfThreshold;
    if (fConfig.selfChannel == "s13") return s13 > threshold;
    if (fConfig.selfChannel == "s14") return s14 > threshold;
    if (fConfig.selfChannel == "coincidence") return s13 > threshold && s14 > threshold;
    return s13 + s14 > threshold;
}

void DetectorConstruction::Validate() const
{
    auto require = [](bool condition, const char* message) {
        if (!condition) G4Exception("DetectorConstruction", "InvalidConfiguration", FatalException, message);
    };
    require(fConfig.trigger == "none" || fConfig.trigger == "self" || fConfig.trigger == "external", "trigger: none, self, external");
    require(fConfig.reflector == "none" || fConfig.reflector == "aluminum" || fConfig.reflector == "teflon", "reflector: none, aluminum, teflon");
    require(fConfig.selfChannel == "sum" || fConfig.selfChannel == "s13" || fConfig.selfChannel == "s14" || fConfig.selfChannel == "coincidence", "selfChannel: sum, s13, s14, coincidence");
    require(fConfig.selfThreshold >= 0 && std::isfinite(fConfig.externalThreshold) && fConfig.externalThreshold >= 0, "Trigger thresholds must be nonnegative.");
    require(fConfig.sideGap > 0 && fConfig.sideGap < 0.25*mm && fConfig.foilThickness > 0 && fConfig.sideGap + fConfig.foilThickness < 0.5*mm, "Require sideGap > 0, foilThickness > 0, sideGap < 0.25 mm and their sum < 0.5 mm (counter clearance).");
    if (fConfig.trigger == "self") {
        require(fConfig.s13.enabled || fConfig.s14.enabled, "Self trigger requires an enabled SiPM.");
        require((fConfig.selfChannel != "s13" && fConfig.selfChannel != "coincidence") || fConfig.s13.enabled, "Selected selfChannel requires S13.");
        require((fConfig.selfChannel != "s14" && fConfig.selfChannel != "coincidence") || fConfig.s14.enabled, "Selected selfChannel requires S14.");
    }
    for (const auto* s : {&fConfig.s13, &fConfig.s14}) {
        auto materialOK = [](const G4String& name) { return name == "air" || name == "grease" || name == "cookie" || name == "acrylic"; };
        require(materialOK(s->gapMaterial) && materialOK(s->guideMaterial), "Coupling materials: air, grease, cookie, acrylic (optics in Materials.cc).");
        require(s->width > 0 && s->width <= 10*mm && s->height > 0 && s->height <= 10*mm && s->thickness > 0, "SiPM full width/height must be in (0, 10] mm; thickness > 0.");
        require(std::isfinite(s->gap) && s->gap >= 0 && std::isfinite(s->guideGap) && s->guideGap >= 0 && std::isfinite(s->guideLength) && s->guideLength > 0, "Coupling gaps must be finite and >= 0; guideLength must be finite and > 0.");
        require(60*mm + s->gap + s->thickness + (s->lightGuide ? s->guideLength + s->guideGap : 0) < 250*mm, "Readout assembly must fit inside the world.");
    }
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    Validate();
    const auto& c = fConfig;
    const G4double h = 5*mm, z = 60*mm;
    auto* world = new G4LogicalVolume(new G4Box("solidWorld", 250*mm, 250*mm, 250*mm), fMaterials->GetWorldMat(), "logicWorld");
    world->SetVisAttributes(G4VisAttributes::GetInvisible());
    auto* worldPV = new G4PVPlacement(nullptr, {}, world, "physWorld", nullptr, false, 0, c.checkOverlaps);
    auto place = [&](G4VSolid* solid, G4Material* mat, const G4String& name, const G4ThreeVector& pos, G4int copy = 0) -> G4VPhysicalVolume* {
        auto* lv = new G4LogicalVolume(solid, mat, "logic" + name);
        // Keep optical air volumes, but do not render them as solid wrapping.
        if (mat == fMaterials->GetAir()) lv->SetVisAttributes(G4VisAttributes::GetInvisible());
        return new G4PVPlacement(nullptr, pos, lv, "phys" + name, world, false, copy, c.checkOverlaps);
    };
    auto box = [&](const G4String& name, G4Material* mat, G4double xh, G4double yh, G4double zh, G4ThreeVector pos) {
        return place(new G4Box("solid" + name, xh, yh, zh), mat, name, pos);
    };
    auto* crystal = box("Crystal", fMaterials->GetCsI(), h, h, z, {});
    crystal->GetLogicalVolume()->SetVisAttributes(G4VisAttributes(G4Colour(0.45, 0.80, 1.00, 0.35)));
    auto* reflector = c.reflector == "none" ? nullptr : c.reflector == "aluminum" ? fMaterials->GetAl() : fMaterials->GetTeflon();
    // Reproduce Single_sipm's polished crystal/air boundaries and reflector surfaces.
    auto wrapFace = [&](const G4String& name, G4ThreeVector half, G4ThreeVector center, G4ThreeVector foilHalf, G4ThreeVector foilCenter) {
        auto* gap = box(name + "Air", fMaterials->GetAir(), half.x(), half.y(), half.z(), center);
        new G4LogicalBorderSurface(name + "CrystalToAir", crystal, gap, fMaterials->GetCrystalSurf());
        new G4LogicalBorderSurface(name + "AirToCrystal", gap, crystal, fMaterials->GetCrystalSurf());
        if (reflector) {
            auto* foil = box(name + "Reflector", reflector, foilHalf.x(), foilHalf.y(), foilHalf.z(), foilCenter);
            new G4LogicalBorderSurface(name + "AirToReflector", gap, foil, fMaterials->GetReflectorSurf(reflector));
        }
    };
    const auto a = c.sideGap, t = c.foilThickness;
    for (int sign : {-1, 1}) {
        const auto suffix = sign < 0 ? "Minus" : "Plus";
        wrapFace(G4String("X") + suffix, {a/2,h,z}, {sign*(h+a/2),0,0}, {t/2,h+a,z}, {sign*(h+a+t/2),0,0});
        wrapFace(G4String("Y") + suffix, {h+a,a/2,z}, {0,sign*(h+a/2),0}, {h+a+t,t/2,z}, {0,sign*(h+a+t/2),0});
    }
    auto couplingMaterial = [&](const G4String& name) {
        if (name == "grease") return fMaterials->GetGrease();
        if (name == "cookie") return fMaterials->GetCookie();
        if (name == "acrylic") return G4Material::GetMaterial("G4_PLEXIGLASS");
        return fMaterials->GetAir();
    };
    for (int sign : {1, -1}) {
        const auto& s = sign > 0 ? c.s13 : c.s14;
        const G4String name = sign > 0 ? "S13" : "S14";
        if (!s.enabled) {
            if (reflector) wrapFace(name + "End", {h+a+t,h+a+t,a/2}, {0,0,sign*(z+a/2)}, {h+a+t,h+a+t,t/2}, {0,0,sign*(z+a+t/2)});
            continue;
        }
        G4double end = z;
        // Without a guide: crystal -> gap -> SiPM.
        // With a guide: crystal -> gap -> tapered guide -> guideGap -> SiPM.
        if (s.gap > 0) box(name + "Gap", couplingMaterial(s.gapMaterial), h, h, s.gap/2, {0,0,sign*(end+s.gap/2)});
        end += s.gap;
        if (s.lightGuide) {
            // G4Trd half sizes are specified at -z then +z.
            auto* guide = new G4Trd("solid" + name + "Guide", sign > 0 ? h : s.width/2, sign > 0 ? s.width/2 : h,
                                   sign > 0 ? h : s.height/2, sign > 0 ? s.height/2 : h, s.guideLength/2);
            auto* guidePV = place(guide, couplingMaterial(s.guideMaterial), name + "Guide", {0,0,sign*(end+s.guideLength/2)});
            if (s.guideMaterial != "air") {
                // Alpha controls visualization opacity only (0 = transparent, 1 = opaque).
                guidePV->GetLogicalVolume()->SetVisAttributes(G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.2)));
            }
            // Open-ended tapered shells cover only the four lateral faces.
            // Use the existing aluminum reflector model (aluminized-Mylar approximation).
            const G4double halfLength = s.guideLength/2;
            const G4double xMinus = sign > 0 ? h : s.width/2;
            const G4double xPlus = sign > 0 ? s.width/2 : h;
            const G4double yMinus = sign > 0 ? h : s.height/2;
            const G4double yPlus = sign > 0 ? s.height/2 : h;
            const G4double slopeX = (xPlus-xMinus)/s.guideLength;
            const G4double slopeY = (yPlus-yMinus)/s.guideLength;
            const G4double normalX = std::sqrt(1+slopeX*slopeX);
            const G4double normalY = std::sqrt(1+slopeY*slopeY);
            auto envelope = [&](const G4String& label, G4double offset, G4double extension) {
                return new G4Trd("solid" + name + label,
                    xMinus + offset*normalX - slopeX*extension,
                    xPlus + offset*normalX + slopeX*extension,
                    yMinus + offset*normalY - slopeY*extension,
                    yPlus + offset*normalY + slopeY*extension,
                    halfLength + extension);
            };
            // Extend each subtraction cutter along the same taper to avoid end caps.
            const G4double cutterExtension = 0.1*um;
            auto* guideAirSolid = new G4SubtractionSolid("solid" + name + "GuideAir",
                envelope("GuideAirOuter", a, 0), envelope("GuideAirCut", 0, cutterExtension));
            auto* guideFoilSolid = new G4SubtractionSolid("solid" + name + "GuideReflector",
                envelope("GuideFoilOuter", a+t, 0), envelope("GuideFoilCut", a, cutterExtension));
            const G4ThreeVector guideCenter(0, 0, sign*(end+halfLength));
            auto* guideAir = place(guideAirSolid, fMaterials->GetAir(), name + "GuideAir", guideCenter);
            auto* guideFoil = place(guideFoilSolid, fMaterials->GetAl(), name + "GuideReflector", guideCenter);
            new G4LogicalBorderSurface(name + "GuideToAir", guidePV, guideAir, fMaterials->GetCrystalSurf());
            new G4LogicalBorderSurface(name + "AirToGuide", guideAir, guidePV, fMaterials->GetCrystalSurf());
            new G4LogicalBorderSurface(name + "GuideAirToReflector", guideAir, guideFoil,
                                       fMaterials->GetReflectorSurf(fMaterials->GetAl()));
            end += s.guideLength;
            if (s.guideGap > 0) box(name + "GuideGap", couplingMaterial(s.gapMaterial), s.width/2, s.height/2, s.guideGap/2, {0,0,sign*(end+s.guideGap/2)});
            end += s.guideGap;
        }
        auto* sensor = box(name, sign > 0 ? fMaterials->GetS13Mat() : fMaterials->GetS14Mat(), s.width/2, s.height/2, s.thickness/2, {0,0,sign*(end+s.thickness/2)});
        new G4LogicalSkinSurface(name + "_SkinSurface", sensor->GetLogicalVolume(), sign > 0 ? fMaterials->GetS13Surf() : fMaterials->GetS14Surf());
    }
    if (c.sourceBead) place(new G4Sphere("solidNa", 0, 0.1*mm, 0, 360*deg, 0, 180*deg), fMaterials->GetSodium(), "Na", {0,10.6*mm,-30.25*mm});
    if (HasExternalTrigger()) {
        auto* counter = box("Scintillator", fMaterials->GetPs(), 30*mm, 1*mm, 5*mm, {0,6.5*mm,0});
        counter->GetLogicalVolume()->SetVisAttributes(G4VisAttributes(G4Colour(0.65, 0.65, 0.65, 0.65)));
    }
    if (c.collimator) {
        auto* rotation = new G4RotationMatrix();
        rotation->rotateX(90*deg);
        auto* solid = new G4SubtractionSolid("Collimator", new G4Box("CollimatorBox", 20*mm,16.5*mm,15*mm),
                                            new G4Tubs("Hole",0,3.3*mm,16.6*mm,0,360*deg), rotation, {});
        auto* collimator = place(solid, fMaterials->GetColli(), "Collimator", {0,24.5*mm,0});
        collimator->GetLogicalVolume()->SetVisAttributes(G4VisAttributes(G4Colour(0.55, 0.55, 0.55, 0.70)));
    }
    G4cout << "DAMSA: " << SelectedTreeName() << ", S13=" << c.s13.enabled << ", S14=" << c.s14.enabled
           << ", reflector=" << c.reflector << ", collimator=" << c.collimator << G4endl;
    return worldPV;
}
