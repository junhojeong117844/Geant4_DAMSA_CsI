#ifndef DAMSA_GEOMETRY_LAYOUT_HH
#define DAMSA_GEOMETRY_LAYOUT_HH

#include "DetectorConstruction.hh"
#include "G4SystemOfUnits.hh"
#include <algorithm>
#include <cmath>

// Shared by geometry and primary generation so sources follow moved hardware.
struct GeometryLayout {
    G4double sideShift;
    G4double worldHalf;
    G4double crystalPitch;
    G4double CrystalY(G4int copy) const { return -(copy-1)*crystalPitch; }

    explicit GeometryLayout(const DetectorConstruction::Configuration& c)
    {
        const auto wrap = c.sideGap + (c.reflector == "none" ? 0 : c.foilThickness);
        // Preserve the original apparatus positions for thin wraps. For thicker
        // wraps retain at least the original 0.31 mm counter-to-envelope clearance.
        sideShift = std::max(0.0, wrap - 0.19*mm);
        G4double extent = std::max(60*mm + wrap, 41*mm + sideShift);
        for (const auto* s : {&c.s13, &c.s14}) {
            if (!s->enabled) continue;
            extent = std::max(extent, 60*mm + (s->gap + (s->lightGuide ? s->guideLength : 0)));
            if (s->lightGuide) {
                // The guide wrap thickness is measured normal to its tapered faces.
                const auto slope = std::max(std::abs(5*mm-s->guideWidth/2),
                                             std::abs(5*mm-s->guideHeight/2))/s->guideLength;
                extent = std::max(extent, 5*mm + (c.sideGap+c.foilThickness)
                    * std::sqrt(1+slope*slope));
            }
        }
        crystalPitch = 10.25*mm; // Fixed center-to-center spacing along -y.
        extent = std::max(extent, (DetectorConstruction::CrystalCount-1)*crystalPitch + 6*mm);
        worldHalf = std::max(250*mm, extent + 10*mm);
    }
};
#endif
