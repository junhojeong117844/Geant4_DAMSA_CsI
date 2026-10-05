#include <TCanvas.h>
#include <TStyle.h>
#include <TFile.h>
#include <TH2D.h>
#include <TGraph.h>
#include <TF1.h>
#include <TString.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>
#include <TPad.h>
#include <TPaveText.h>
#include <TText.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

void PlotEcorr(const char* filename = "../datas/output.root")
{
    const int models[] = {1325, 1350, 1375};
    // Edit bin counts and ranges here. Upper limits of 0 use the data maximum.
    const int energyBins = 220, npeBins = 220;
    const double lowerEnergy = 0., upperEnergy = 22.;
    const double lowerNPE = 0., upperNPE = 110.;
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kViridis);
    TFile file(filename, "READ");
    if (file.IsZombie()) return;
    TTree* trees[8] = {};
    for (int crystal = 1; crystal <= 8; ++crystal) {
        trees[crystal - 1] = file.Get<TTree>(Form("SiPM%d", crystal));
        if (!trees[crystal - 1] || !trees[crystal - 1]->GetBranch("edep_MeV") ||
            !trees[crystal - 1]->GetBranch("energy_eV")) {
            std::cerr << "PlotEcorr: SiPM" << crystal << " requires edep_MeV and energy_eV\n";
            return;
        }
    }

    const double energy[] = {
        1.80, 1.85, 1.91, 1.97, 2.03, 2.10, 2.17, 2.25, 2.34, 2.43,
        2.53, 2.64, 2.75, 2.88, 3.02, 3.18, 3.35, 3.54, 3.76, 4.00, 4.28, 4.59
    };
    // Original S1325CSPDE, S1350CSPDE, S1375CSPDE tables from Materials.cc (%).
    const double pde[3][22] = {
        {9.9, 11.3, 12.8, 14.2, 15.8, 17.2, 18.8, 20.3, 22.2, 23.6,
         24.6, 25.2, 25.1, 24.9, 23.7, 22.1, 19.1, 18.0, 17.1, 15.1, 9.5, 1.5},
        {15.9, 17.8, 20.4, 22.8, 25.1, 27.9, 29.1, 32.6, 35.1, 37.1,
         39.3, 40.0, 39.9, 39.1, 37.4, 34.4, 30.2, 28.4, 26.8, 23.6, 14.0, 2.5},
        {20.0, 22.5, 25.5, 29.2, 31.8, 34.3, 37.3, 41.2, 44.0, 46.7,
         49.1, 49.9, 49.9, 48.6, 46.5, 42.8, 37.2, 35.5, 33.2, 29.2, 16.5, 3.0}
    };

    std::vector<std::pair<double, double>> events[3][8];
    for (int crystal = 0; crystal < 8; ++crystal) {
        for (int row = 0; row < 3; ++row)
            events[row][crystal].reserve(trees[crystal]->GetEntries());
        TTreeReader reader(trees[crystal]);
        TTreeReaderValue<double> edep(reader, "edep_MeV");
        TTreeReaderArray<double> photonEnergy(reader, "energy_eV");
        while (reader.Next()) {
            if (!std::isfinite(*edep) || *edep <= 0.) continue;
            double npe[3] = {};
            for (double photon : photonEnergy) {
                // Find the interpolation interval once for all three PDE models.
                const double e = std::clamp(photon, energy[0], energy[21]);
                const int i = std::clamp(int(std::upper_bound(energy, energy + 22, e) - energy) - 1, 0, 20);
                const double t = (e - energy[i]) / (energy[i + 1] - energy[i]);
                for (int row = 0; row < 3; ++row)
                    npe[row] += (pde[row][i] + t * (pde[row][i + 1] - pde[row][i])) / 100.;
            }
            for (int row = 0; row < 3; ++row) {
                if (std::isfinite(npe[row])) events[row][crystal].emplace_back(*edep, npe[row]);
            }
        }
        if (reader.GetEntryStatus() != TTreeReader::kEntryBeyondEnd) {
            std::cerr << "PlotEcorr: failed to read SiPM" << crystal + 1 << '\n';
            return;
        }
    }

    auto* canvas = new TCanvas("ecorr_canvas", "Energy deposit vs NPE", 1600, 600);
    canvas->Divide(8, 3, 0.001, 0.001);
    for (int row = 0; row < 3; ++row) {
        for (int crystal = 1; crystal <= 8; ++crystal) {
            const auto& sample = events[row][crystal - 1];
            double maxEnergy = 0., maxNPE = 0.;
            for (const auto& event : sample) {
                maxEnergy = std::max(maxEnergy, event.first);
                maxNPE = std::max(maxNPE, event.second);
            }
            const double energyMax = upperEnergy > lowerEnergy ? upperEnergy : std::max(lowerEnergy + 0.001, 1.05 * maxEnergy);
            const double npeMax = upperNPE > lowerNPE ? upperNPE : std::max(lowerNPE + 1., 1.05 * maxNPE);
            auto* hist = new TH2D(Form("ecorr_%d_crystal%d", models[row], crystal),
                                 Form("%d Crystal %d;edep [MeV];NPE;", models[row], crystal),
                                 energyBins, lowerEnergy, energyMax,
                                 npeBins, lowerNPE, npeMax);
            hist->SetDirectory(nullptr);
            hist->SetStats(false);
            // Means and the linear fit use events inside the selected axis ranges.
            TGraph fitPoints;
            for (const auto& event : sample)
                if (event.first >= lowerEnergy && event.first < energyMax &&
                    event.second >= lowerNPE && event.second < npeMax) {
                    hist->Fill(event.first, event.second);
                    fitPoints.SetPoint(fitPoints.GetN(), event.first, event.second);
                }

            TF1 linear(Form("ecorr_fit_%d_crystal%d", models[row], crystal),
                       "pol1", lowerEnergy, energyMax);
            const bool fitOK = fitPoints.GetN() >= 3 && fitPoints.GetRMS(1) > 0. &&
                               int(fitPoints.Fit(&linear, "QSN")) == 0;

            canvas->cd(row * 8 + crystal);
            gPad->SetLeftMargin(0.18);
            gPad->SetRightMargin(0.12);
            gPad->SetBottomMargin(0.15);
            gPad->SetTopMargin(0.15);
            hist->Draw("COLZ");
            if (fitOK) {
                linear.SetLineColor(kRed + 1);
                linear.SetLineWidth(2);
                //linear.DrawCopy("SAME");
            }
            gPad->Update();
            const TString lines[] = {
                Form("NPE mean = %.4g", hist->GetMean(2)),
                Form("Edep mean = %.4g MeV", hist->GetMean(1)),
                fitOK ? Form("Light/Energy = %.4g NPE/MeV", linear.GetParameter(1))
                      : "Light/Energy = n/a"
            };
            const double padWidth = gPad->GetWw() * gPad->GetAbsWNDC();
            const double padHeight = gPad->GetWh() * gPad->GetAbsHNDC();
            const double fontPixels = 0.85 * std::max(8, int(0.035 * std::min(padWidth, padHeight)));
            UInt_t maxWidth = 0;
            for (const auto& line : lines) {
                TText text(0., 0., line);
                text.SetTextFont(43);
                text.SetTextSize(fontPixels);
                UInt_t width = 0, height = 0;
                text.GetBoundingBox(width, height);
                maxWidth = std::max(maxWidth, width);
            }
            const double left = gPad->GetLeftMargin();
            const double top = 1. - gPad->GetTopMargin() - 0.02;
            const double width = (maxWidth + 4.) / padWidth;
            const double height = (3. * (fontPixels + 2.) + 2.) / padHeight;
            auto* info = new TPaveText(left, top - height, left + width, top, "NDC");
            info->SetFillStyle(0);
            info->SetBorderSize(0);
            info->SetTextAlign(14);
            info->SetTextFont(43);
            info->SetTextSize(fontPixels);
            info->SetMargin(2. / (maxWidth + 4.));
            //for (const auto& line : lines) info->AddText(line);
            info->Draw();
        }
    }
    canvas->Update();
}
