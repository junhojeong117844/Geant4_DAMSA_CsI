
#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TF1.h>

#include <iostream>

void fit_edep_endpoint_constrained()
{
    const char* fileName   = "output.root";
    const char* treeName   = "Trigger activated";
    const char* branchName = "crystalEdep_keV";

    // ===== 직접 조절할 부분 =====
    double fitMin = 950.0;
    double fitMax = 1950.0;

    double histMin = 100.0;
    double histMax = 2000.0;
    int nBins = 100;

    double endpointGuess = 1950.0;
    // ===========================

    TFile *file = TFile::Open(fileName);

    if (!file || file->IsZombie()) {
        std::cerr << "Cannot open " << fileName << std::endl;
        return;
    }

    TTree *tree = dynamic_cast<TTree*>(file->Get(treeName));

    if (!tree) {
        std::cerr << "Cannot find tree: " << treeName << std::endl;
        return;
    }

    TH1D *h = new TH1D(
        "hEdep",
        "Crystal Energy Deposit;Crystal Edep [keV];Entries",
        nBins,
        histMin,
        histMax
    );

    tree->Draw(
        Form("%s >> hEdep", branchName),
        "",
        "goff"
    );

    // ============================================================
    // Endpoint-constrained quadratic fit
    //
    // f(E) = A * (E0 - E)^2
    //
    // par[0] = A
    // par[1] = E0 = endpoint
    // ============================================================
    TF1 *fEndpoint = new TF1(
        "fEndpoint",
        "[0]*([1]-x)*([1]-x)",
        fitMin,
        fitMax
    );

    fEndpoint->SetParNames("A", "Endpoint");
    fEndpoint->SetParameters(1.0, endpointGuess);

    h->Fit(fEndpoint, "R0Q");

    double endpoint =
        fEndpoint->GetParameter(1);

    double endpointError =
        fEndpoint->GetParError(1);

    double chi2ndf = 0.0;

    if (fEndpoint->GetNDF() > 0)
        chi2ndf =
            fEndpoint->GetChisquare()
            / fEndpoint->GetNDF();

    std::cout << "\n============================================\n";
    std::cout << "Fit range = "
              << fitMin << " - " << fitMax << " keV\n";

    std::cout << "Endpoint = "
              << endpoint
              << " +/- "
              << endpointError
              << " keV\n";

    std::cout << "chi2/ndf = "
              << fEndpoint->GetChisquare()
              << " / "
              << fEndpoint->GetNDF()
              << " = "
              << chi2ndf
              << "\n";

    std::cout << "============================================\n";

    TCanvas *c1 =
        new TCanvas(
            "c1",
            "Energy Endpoint Fit",
            1000,
            500
        );

    h->SetLineWidth(2);
    h->Draw("hist");

    fEndpoint->SetLineColor(kRed);
    fEndpoint->SetLineWidth(3);
    fEndpoint->Draw("same");

    c1->Update();
}
