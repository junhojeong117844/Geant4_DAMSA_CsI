#include <TCanvas.h>
#include <TStyle.h>
#include <TFile.h>
#include <TH1D.h>
#include <TString.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TPad.h>
#include <algorithm>

// Run: root PlotEdep.C
void PlotEdep()
{
    gStyle->SetOptStat(110);
    TFile file("../datas/output.root", "READ");
    if (file.IsZombie()) return;
    TTree* trees[8] = {};
    double upperEnergy = 100;
    for (int crystal = 1; crystal <= 8; ++crystal) {
        trees[crystal - 1] = file.Get<TTree>(Form("SiPM%d", crystal));
        if (!trees[crystal - 1] || !trees[crystal - 1]->GetBranch("edep_MeV")) return;
    }

    auto* canvas = new TCanvas("edep_canvas", "Energy deposit: crystals 1-8", 1600, 200);
    canvas->Divide(8, 1, 0.001, 0.001);
    for (int crystal = 1; crystal <= 8; ++crystal) {
        auto* hist = new TH1D(Form("edep_crystal%d", crystal),
                             Form("Crystal %d;Deposited energy [MeV];Counts", crystal),
                             100, 0.0, upperEnergy);
        hist->SetDirectory(nullptr);
        TTreeReader reader(trees[crystal - 1]);
        TTreeReaderValue<double> edep(reader, "edep_MeV");
        while (reader.Next()) {
            if (*edep == 0.) continue;
            hist->Fill(*edep);
        }

        canvas->cd(crystal);
        gPad->SetLogy();
        gPad->SetLeftMargin(0.18);
        gPad->SetRightMargin(0.12);
        //gPad->SetBottomMargin(0.05);
        //gPad->SetTopMargin(0.15);
        hist->SetLineColor(kBlue);
        hist->SetLineWidth(3);
        hist->Draw("HIST");
    }
    canvas->Update();
}
