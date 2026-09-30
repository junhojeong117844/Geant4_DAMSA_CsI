
#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TLegend.h>
#include <TMath.h>

#include <iostream>
#include <cmath>

double QuadraticRightZero(double a, double b, double c)
{
    if (std::abs(c) < 1e-15) {
        if (std::abs(b) < 1e-15) return NAN;
        return -a / b;
    }

    double disc = b*b - 4.0*a*c;
    if (disc < 0.0) return NAN;

    double r1 = (-b + std::sqrt(disc)) / (2.0*c);
    double r2 = (-b - std::sqrt(disc)) / (2.0*c);

    return (r1 > r2) ? r1 : r2;
}

void fit_linear_quad()
{
    const char* fileName   = "output.root";
    const char* treeName   = "auto-selected trigger tree";
    const char* branchName = "crystalEdep_keV";

    double fitMin = 950.0;
    double fitMax = 1850.0;

    double histMin = 10.0;
    double histMax = 2010.0;
    int nBins = 300;

    TFile *file = TFile::Open(fileName);
    if (!file || file->IsZombie()) return;
    TTree* tree = nullptr;
    for (const auto* candidate : {"CrystalTrigger", "ExternalTrigger", "AllEvents", "Trigger activated"}) {
        tree = dynamic_cast<TTree*>(file->Get(candidate));
        if (tree) { treeName = candidate; break; }
    }
    if (!tree) { std::cerr << "No event tree in " << fileName << std::endl; return; }

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

    TF1 *fLinear = new TF1(
        "fLinear",
        "[0] + [1]*x",
        fitMin,
        fitMax
    );

    fLinear->SetParNames("a", "b");

    double yAtMin = h->GetBinContent(h->FindBin(fitMin));
    double yAtMax = h->GetBinContent(h->FindBin(fitMax));

    double slopeGuess =
        (yAtMax - yAtMin) / (fitMax - fitMin);

    double interceptGuess =
        yAtMin - slopeGuess * fitMin;

    fLinear->SetParameters(interceptGuess, slopeGuess);

    h->Fit(fLinear, "R0Q");

    double aLin = fLinear->GetParameter(0);
    double bLin = fLinear->GetParameter(1);

    double endpointLinear = NAN;
    if (std::abs(bLin) > 1e-15)
        endpointLinear = -aLin / bLin;

    double chi2ndfLinear = NAN;
    if (fLinear->GetNDF() > 0)
        chi2ndfLinear =
            fLinear->GetChisquare() / fLinear->GetNDF();

    TF1 *fQuad = new TF1(
        "fQuad",
        "[0] + [1]*x + [2]*x*x",
        fitMin,
        fitMax
    );

    fQuad->SetParNames("a", "b", "c");
    fQuad->SetParameters(
        interceptGuess,
        slopeGuess,
        0.0
    );

    h->Fit(fQuad, "R0Q");

    double aQ = fQuad->GetParameter(0);
    double bQ = fQuad->GetParameter(1);
    double cQ = fQuad->GetParameter(2);

    double endpointQuad =
        QuadraticRightZero(aQ, bQ, cQ);

    double chi2ndfQuad = NAN;
    if (fQuad->GetNDF() > 0)
        chi2ndfQuad =
            fQuad->GetChisquare() / fQuad->GetNDF();

    std::cout << "\n============================================\n";
    std::cout << "Fit range : "
              << fitMin << " - "
              << fitMax << " keV\n";

    std::cout << "\n[Linear]\n";
    std::cout << "f(x) = a + b*x\n";
    std::cout << "a = " << aLin << "\n";
    std::cout << "b = " << bLin << "\n";
    std::cout << "chi2/ndf = "
              << fLinear->GetChisquare()
              << " / "
              << fLinear->GetNDF()
              << " = "
              << chi2ndfLinear
              << "\n";
    std::cout << "zero crossing = "
              << endpointLinear
              << " keV\n";

    std::cout << "\n[Quadratic]\n";
    std::cout << "f(x) = a + b*x + c*x^2\n";
    std::cout << "a = " << aQ << "\n";
    std::cout << "b = " << bQ << "\n";
    std::cout << "c = " << cQ << "\n";
    std::cout << "chi2/ndf = "
              << fQuad->GetChisquare()
              << " / "
              << fQuad->GetNDF()
              << " = "
              << chi2ndfQuad
              << "\n";
    std::cout << "right-side zero crossing = "
              << endpointQuad
              << " keV\n";
    std::cout << "============================================\n";

    TCanvas *c1 =
        new TCanvas(
            "c1",
            "Linear vs Quadratic Fit",
            700,
            500
        );

    h->SetLineWidth(2);
    h->Draw("hist");

    double drawMax = histMax;

    if (std::isfinite(endpointLinear) &&
        endpointLinear > drawMax)
        drawMax = endpointLinear * 1.05;

    if (std::isfinite(endpointQuad) &&
        endpointQuad > drawMax)
        drawMax = endpointQuad * 1.05;

    h->GetXaxis()->SetRangeUser(histMin, drawMax);

    fLinear->SetRange(fitMin, drawMax);
    fQuad->SetRange(fitMin, drawMax);

    fLinear->SetLineColor(kRed);
    fLinear->SetLineWidth(3);

    fQuad->SetLineColor(kBlue);
    fQuad->SetLineWidth(3);

    fLinear->Draw("same");
    fQuad->Draw("same");
    c1->Update();
}
