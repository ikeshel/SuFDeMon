// Author: Irakli Keshelashvili, 2026
//
// SuFDeMon - Super-FRS Detector Monitoring Software
//
// Copyright (C) 2026 Irakli Keshelashvili
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// See the LICENSE file in the project root for the full license text.

#ifndef SUFDEMON_DRAW_SCIFI_ALL_H
#define SUFDEMON_DRAW_SCIFI_ALL_H

#include <TCanvas.h>
#include <TError.h>
#include <TH1.h>
#include <TPad.h>
#include <TROOT.h>
#include <TROOT.h>
#include <TString.h>

TH1* SuFDeMonGet(const char* name);

inline void DrawSCIFIAll(const char* instance)
{
    const TString canvasName = TString::Format("c%s_ALL", instance);
    // Reuse a canvas on repeated draws; fetched snapshots remain owned by the GUI.
    auto* canvas = dynamic_cast<TCanvas*>(gROOT->FindObject(canvasName.Data()));
    if (!canvas)
        canvas = new TCanvas(canvasName.Data(), instance, 40, 40, 1600, 700);
    canvas->Clear();
    canvas->Divide(2, 1);
    Int_t pad = 1;
    for (const char* quantity : {"ToT", "TDC"}) {
        const TString name = TString::Format("h%s_%s", instance, quantity);
        TH1* histogram = SuFDeMonGet(name.Data());
        auto* channelPad = canvas->cd(pad++);
        channelPad->SetLeftMargin(0.16);
        channelPad->SetRightMargin(0.15);
        if (histogram && histogram->GetDimension() == 2)
            histogram->Draw("COLZ");
        else
            Warning("DrawSCIFIAll", "Could not load 2D histogram %s; rebuild and restart the SCIFI server", name.Data());
    }
    canvas->Modified();
    canvas->Update();
}

#endif
