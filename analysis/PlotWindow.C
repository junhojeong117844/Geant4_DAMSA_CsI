#include <TCanvas.h>
#include <TStyle.h>
#include <TFile.h>
#include <TGraph.h>
#include <algorithm>
#include <TH2D.h>
#include <TString.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TPad.h>

void PlotWindow()
{
    gStyle->SetOptStat(110);
    const int models[] = {6025, 6050, 6075};
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
    TFile file("../datas/Teflon_8SiPM_0um_6mm.root", "READ");
    auto* canvas = new TCanvas("teflon_canvas", "Teflon: channels 1-8", 1600, 600);
    canvas->Divide(8, 3, 0.001, 0.001);
    TH2D* histograms[3][8] = {};
    double commonMaximum = 0.;

    for (int row = 0; row < 3; ++row) {
        TGraph efficiency(22, energy, pde[row]);
        for (int channel = 1; channel <= 8; ++channel) {
            auto* tree = file.Get<TTree>(Form("SiPM%d", channel));
            auto* hist = new TH2D(Form("light_%d_ch%d", models[row], channel),
                                 Form("%d Channel %d;x [mm];y [mm];", models[row], channel),
                                 1000, -5, 5, 1000, -5, 5);
            hist->SetDirectory(nullptr);
            hist->SetStats(false);

            TTreeReader reader(tree);
            TTreeReaderArray<double> x(reader, "x_mm");
            TTreeReaderArray<double> y(reader, "y_mm");
            TTreeReaderArray<double> photonEnergy(reader, "energy_eV");
            const double centerY = -(channel - 1) * 10.25;
            while (reader.Next()) {
                for (unsigned int i = 0; i < x.GetSize(); ++i)
                    hist->Fill(x[i], y[i] - centerY,
                               efficiency.Eval(std::clamp(photonEnergy[i], energy[0], energy[21])) / 100.);
            }

            histograms[row][channel - 1] = hist;
            commonMaximum = std::max(commonMaximum, hist->GetMaximum());
        }
    }

    // Use the same absolute light-yield color scale for all models and channels.
    if (commonMaximum <= 0.) commonMaximum = 1.;
    for (int row = 0; row < 3; ++row) {
        for (int channel = 1; channel <= 8; ++channel) {
            auto* hist = histograms[row][channel - 1];
            hist->SetMinimum(0.);
            hist->SetMaximum(commonMaximum);
            canvas->cd(row * 8 + channel);
            gPad->SetLeftMargin(0.18);
            gPad->SetRightMargin(0.12);
            gPad->SetBottomMargin(0.15);
            gPad->SetTopMargin(0.15);
            hist->Draw("COLZ");
        }
    }
    canvas->Update();
}
