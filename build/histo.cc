void histo() {

	TFile *f1 = TFile::Open("output.root");
	TTree *t1 = (TTree*)f1->Get("Trigger activated");
	TTree *t2 = (TTree*)f1->Get("Trigger activated");
	TTree *t3 = (TTree*)f1->Get("Trigger activated");
	Double_t edep;
	Int_t S13, S14;
	t1->SetBranchAddress("crystalEdep_keV", &edep);
	t2->SetBranchAddress("S13", &S13);
	t3->SetBranchAddress("S14", &S14);

	auto c1 = new TCanvas("c1", "cnvs", 1200, 600);
	auto c2 = new TCanvas("c2", "cnvs",1200, 600);
	auto h1 = new TH1D("edep", ";keV; entries", 100, 100, 2000);
	auto h2 = new TH1D("S13", ";Npe; entries", 35, 0, 35);
	auto h3 = new TH1D("S14", ";Npe; entries", 100, 0, 100);
	Long64_t nentries = t1->GetEntries();
	for (Long64_t i = 0; i < nentries; i++) {

		t1->GetEntry(i);
		t2->GetEntry(i);
		t3->GetEntry(i);
		h1->Fill(edep);
		h2->Fill(S13);
		h3->Fill(S14);
	}
	h2->SetLineColor(kYellow+1);
	h2->SetFillColor(kYellow+1);
	h3->SetLineColor(kBlue+1);
	h3->SetFillColor(kBlue+1);
	c1->Divide(2,1);
	c1->cd(1);
	h2->Draw();
	gPad->SetLogy();
	c1->cd(2);
	h3->Draw();
	gPad->SetLogy();
	c2->cd();
	h1->Draw();
	c1->Update();
	c2->Update();
}
