#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TStyle.h>
#include <TPad.h>

void PlotEdep()
{
    gStyle->SetOptStat(110);

    TFile* file = TFile::Open("../datas/Teflon_8SiPM_0um_6mm.root");

    TCanvas* canvas = new TCanvas("edep_canvas", "Energy deposit: crystals 1-8", 1600, 200);
    canvas->Divide(8, 1, 0.001, 0.001);

    for (int i = 1; i <= 8; i++) {
        TTree* tree = (TTree*)file->Get(Form("SiPM%d", i));

        TH1D* hist = new TH1D(
            Form("edep_crystal%d", i),
            Form("Crystal %d;Deposited energy [MeV];Counts", i),
            100, 0, 20
        );

        TTreeReader reader(tree);
        TTreeReaderValue<double> edep(reader, "edep_MeV");

        while (reader.Next()) {
            hist->Fill(*edep);
        }

        canvas->cd(i);
        //gPad->SetLogy();
        gPad->SetLeftMargin(0.18);
        gPad->SetRightMargin(0.12);

        hist->SetLineColor(kBlue);
        hist->SetLineWidth(3);
        hist->Draw("HIST");
    }

    canvas->Update();
}
