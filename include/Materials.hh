#ifndef Materials_h
#define Materials_h 1

#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4OpticalSurface.hh"

class Materials
{
public:
    Materials();
    ~Materials();

    void DefineMaterials();

    G4Material* GetWorldMat() { return worldMat; }
    G4Material* GetCsI() { return CsIMat; }
    G4Material* GetAir() { return airMat; }
    G4Material* GetAl() { return alMat; }
    G4Material* GetTeflon() { return teflonMat; }
    G4Material* GetCookie() { return cookieMat; }
    G4Material* GetGrease() { return greaseMat; }
    G4Material* GetSodium() { return sodiumMat; }
    G4Material* GetPs() { return PSMat; }
    G4Material* GetColli() { return colliMat; }

    G4OpticalSurface* GetReflectorSurf(G4Material* material)
    {
        return (material == alMat) ? alReflectorSurf : teflonReflectorSurf;
    }
    G4OpticalSurface* GetCrystalSurf() { return crystalSurf; }

private:
    G4Material* worldMat = nullptr;
    G4Material* CsIMat = nullptr;
    G4Material* airMat = nullptr;
    G4Material* alMat = nullptr;
    G4Material* teflonMat = nullptr;
    G4Material* cookieMat = nullptr;
    G4Material* sodiumMat = nullptr;
    G4Material* greaseMat = nullptr;
    G4Material* PSMat = nullptr;
    G4Material* colliMat = nullptr;

    G4OpticalSurface* alReflectorSurf = nullptr;
    G4OpticalSurface* teflonReflectorSurf = nullptr;
    G4OpticalSurface* crystalSurf = nullptr;
};

#endif
