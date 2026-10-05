#include <TCanvas.h>
#include <TStyle.h>
#include <TFile.h>
#include <TF1.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TString.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>
#include <TPad.h>
#include <TPaveText.h>
#include <TText.h>
#include <algorithm>
#include <cmath>
#include <vector>

// Run: root PlotNPE.C
// Each event in the plotted NPE range contributes one count.
void PlotNPE(const char* filename = "../datas/output.root", int bins = 149)
{
    gStyle->SetOptStat(0);
    // Compact, transparent information box at the upper right.
    auto drawInfo = [](TH1D* hist, double efficiencyPercent, bool valid,
                       const TF1* fit) {
        hist->SetStats(false);
        gPad->Update();
        const TString lines[] = {
            valid ? Form("Efficiency = %.3g%%", efficiencyPercent) : "Efficiency = n/a",
            Form("NPE mean = %.4g", hist->GetMean()),
            fit ? Form("Fit mean = %.4g", fit->GetParameter(1)) : "Fit mean = n/a",
            Form("Entries = %.0f", hist->GetEntries())
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
        const double right = 1. - gPad->GetRightMargin() - 0.01;
        const double top = 1. - gPad->GetTopMargin() - 0.02;
        const double width = (maxWidth + 4.) / padWidth;
        const double height = (4. * (fontPixels + 2.) + 2.) / padHeight;
        auto* info = new TPaveText(right - width, top - height, right, top, "NDC");
        info->SetFillStyle(0);
        info->SetBorderSize(0);
        info->SetTextFont(43);
        info->SetTextSize(fontPixels);
        info->SetTextAlign(32);
        info->SetMargin(2. / (maxWidth + 4.));
        for (const auto& line : lines) info->AddText(line);
        info->Draw();
    };
    const int models[] = {1325, 1350, 1375};

    TFile file(filename, "READ");
    if (file.IsZombie()) return;
    TTree* trees[8] = {};
    for (int channel = 1; channel <= 8; ++channel) {
        trees[channel - 1] = file.Get<TTree>(Form("SiPM%d", channel));
        if (!trees[channel - 1] || !trees[channel - 1]->GetBranch("energy_eV") ||
            !trees[channel - 1]->GetBranch("Generated_photons")) return;
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

    TGraph efficiency[3];
    for (int row = 0; row < 3; ++row)
        efficiency[row] = TGraph(22, energy, pde[row]);
    std::vector<double> eventNPE[3][8];
    std::vector<int> eventGenerated[8];
    for (int channel = 1; channel <= 8; ++channel) {
        TTreeReader reader(trees[channel - 1]);
        TTreeReaderArray<double> photonEnergy(reader, "energy_eV");
        TTreeReaderValue<int> generated(reader, "Generated_photons");
        while (reader.Next()) {
            eventGenerated[channel - 1].push_back(*generated);
            double npe[3] = {};
            for (double photon : photonEnergy) {
                for (int row = 0; row < 3; ++row)
                    npe[row] += efficiency[row].Eval(std::clamp(photon, energy[0], energy[21])) / 100.;
            }
            for (int row = 0; row < 3; ++row) {
                eventNPE[row][channel - 1].push_back(npe[row]);
            }
        }
        if (reader.GetEntryStatus() != TTreeReader::kEntryBeyondEnd) return;
    }

    // Only events in [lowerNPE, upperNPE) enter the histogram and its statistics.
    const double lowerNPE = 1.;
    const double upperNPE = 150.;
    auto* canvas = new TCanvas("teflon_h1_canvas", "Teflon NPE: channels 1-8", 1600, 600);
    canvas->Divide(8, 3, 0.001, 0.001);
    TH1D* histograms[3][8] = {};
    double ratioSum[3][8] = {};
    size_t ratioEntries[3][8] = {};
    double commonMaximum = 0.;
    for (int row = 0; row < 3; ++row) {
        for (int channel = 1; channel <= 8; ++channel) {
            auto* hist = new TH1D(Form("h1_%d_ch%d", models[row], channel),
                                  Form("%d Channel %d;NPE;Counts", models[row], channel),
                                  bins, lowerNPE, upperNPE);
            hist->SetDirectory(nullptr);
            hist->SetStats(false);
            const auto& generated = eventGenerated[channel - 1];
            for (size_t event = 0; event < generated.size(); ++event) {
                const double npe = eventNPE[row][channel - 1][event];
                if (npe >= lowerNPE && npe < upperNPE) {
                    hist->Fill(npe);
                    // Event-wise mean percentage for the same NPE-selected events.
                    if (generated[event] > 0) {
                        ratioSum[row][channel - 1] += 100. * npe / generated[event];
                        ++ratioEntries[row][channel - 1];
                    }
                }
            }
            histograms[row][channel - 1] = hist;
            commonMaximum = std::max(commonMaximum, hist->GetMaximum());
        }
    }
    for (int row = 0; row < 3; ++row) {
        for (int channel = 1; channel <= 8; ++channel) {
            auto* hist = histograms[row][channel - 1];
            hist->SetMinimum(0.);
            hist->SetMaximum(std::max(1., commonMaximum*1.1));
            canvas->cd(row * 8 + channel);
            gPad->SetLeftMargin(0.18);
            gPad->SetRightMargin(0.12);
            gPad->SetBottomMargin(0.15);
            gPad->SetTopMargin(0.15);
            hist->SetLineColor(kBlue + row);
            hist->SetLineWidth(3);
            hist->Draw("HIST");
            const TF1* fittedGaussian = nullptr;
            if (hist->GetEntries() > 0 && hist->GetRMS() > 0.) {
                TF1 gaussian(Form("gaus_%d_ch%d", models[row], channel),
                             "gaus", lowerNPE, upperNPE);
                // Seed the mean from the histogram; all parameters remain free.
                gaussian.SetParameters(hist->GetMaximum(), hist->GetMean(), hist->GetRMS());
                gaussian.SetLineColor(kRed);
                gaussian.SetLineWidth(2);
                const int fitStatus = hist->Fit(&gaussian, "QRB0");
                if (auto* fit = hist->GetFunction(gaussian.GetName())) {
                    fit->Draw("SAME");
                    if (fitStatus == 0) fittedGaussian = fit;
                }
            }
            const size_t valid = ratioEntries[row][channel - 1];
            drawInfo(hist, valid ? ratioSum[row][channel - 1] / valid : 0., valid > 0,
                     fittedGaussian);
        }
    }
    canvas->Update();

}
