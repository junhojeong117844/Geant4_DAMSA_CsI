#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TPad.h>

void PlotGeneratedPhotons()
{
    TFile* file = TFile::Open("../datas/Teflon_8SiPM_0um_6mm.root");

    TCanvas* canvas = new TCanvas(
        "c1",
        "Generated Photons",
        1600, 200
    );

    canvas->Divide(8, 1, 0.001, 0.001);

    for (int i = 1; i <= 8; i++) {

        TTree* tree = (TTree*)file->Get(Form("SiPM%d", i));

        TH1D* hist = new TH1D(
            Form("h%d", i),
            Form("Crystal %d;Generated photons;Counts", i),
            100, 0, 30000
        );

        TTreeReader reader(tree);
        TTreeReaderValue<int> generated(reader, "Generated_photons");

        while (reader.Next()) {
            hist->Fill(*generated);
        }

        canvas->cd(i);

        //gPad->SetLogy();
		gStyle->SetOptStat(0);
        gPad->SetLeftMargin(0.18);
        gPad->SetRightMargin(0.12);

		hist->SetLineColor(kBlue);
		hist->SetLineWidth(3);

		hist->Draw("HIST");

		TF1* gaus = new TF1(
				Form("gaus%d", i),
				"gaus",
				10000, 25000
				);

		hist->Fit(gaus, "RQ0");
		gaus->Draw("SAME");

		double mean = gaus->GetParameter(1);

		TLatex* text = new TLatex();
		text->SetNDC();
		text->SetTextSize(0.05);
		text->DrawLatex(0.60, 0.80, Form("#mu = %.1f", mean));
	}

	canvas->Update();
}
