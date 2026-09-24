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

#ifndef SUFDEMON_DRAW_MUSIC_ALL_H
#define SUFDEMON_DRAW_MUSIC_ALL_H

#include <TCanvas.h>
#include <TError.h>
#include <TH1D.h>
#include <TPad.h>
#include <TString.h>

TH1D* SuFDeMonGet(const char* name);

inline void DrawMUSICAll(const char* instance, const char* quantity)
{
    const TString canvasName = TString::Format("c%s_%s_ALL", instance, quantity);
    const TString canvasTitle = TString::Format("%s %s---all FC channels", instance, quantity);
    auto* canvas = new TCanvas(
        canvasName.Data(), canvasTitle.Data(), 40, 40, 2200, 1200);
    canvas->Divide(16, 6, 0.001, 0.001);

    Int_t pad = 1;
    for (Int_t fieldCage = 1; fieldCage <= 3; ++fieldCage) {
        for (Int_t channel = 0; channel < 32; ++channel, ++pad) {
            const TString name = TString::Format("h%s_FC%d_%s%d", instance, fieldCage, quantity, channel);
            TH1D* histogram = SuFDeMonGet(name.Data());
            auto* channelPad = canvas->cd(pad);
            const Color_t background = fieldCage % 2 == 0 ? kGray : kWhite;
            channelPad->SetFillColor(background);
            channelPad->SetFrameFillColor(background);
            if (histogram)
                histogram->Draw();
            else
                Warning("DrawMUSICAll", "Could not draw %s", name.Data());
        }
    }
    canvas->Modified();
    canvas->Update();
}

#endif
