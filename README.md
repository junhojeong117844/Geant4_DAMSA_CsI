# DAMSA_CsI

## ROOT output

### Histogram

- `GeneratedWavelength`
  - Wavelength distribution of every scintillation optical photon generated in `logicCrystal`
  - Unit: nm
  - Defined in `RunAction.cc`
  - Filled in `SteppingAction.cc`

### `Photosensor` ntuple

Stored only when S13 or S14 detects at least one optical photon.

- `eventID`
- `crystalEdep_keV`
- `Generated_photons`
- `S13`
- `S14`

### `Crystal` ntuple

Stored when the crystal has energy deposition or generates scintillation photons.

- `eventID`
- `crystalEdep_keV`
- `Generated_photons`

Source-energy recording and radioactive-decay secondary trees were removed.
No trigger-counter condition is used in event storage or stepping logic.

## Build

```bash
mkdir build
cd build
cmake ..
make -j
./sim batch.mac
```

CMake copies the `.mac` and beta-spectrum `.csv` files from `macros/` into the build directory.
# Geant4_DAMSA_CsI
