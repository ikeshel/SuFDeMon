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

#ifndef SUFDEMON_DRAW_DETECTOR_ALL_H
#define SUFDEMON_DRAW_DETECTOR_ALL_H

#include <TCanvas.h>
#include <TError.h>
#include <TH1.h>
#include <TString.h>
#include "../common/inc/SuFDeMonPlsciLayout.h"

TH1* SuFDeMonGet(const char* name);

// PLSCI has one channel per PMT.
inline void DrawDetectorAll(const char* instance, const char* quantity)
{
    const TString canvasName = TString::Format("c%s_%s_ALL", instance, quantity);
    const TString title = TString::Format("%s %s - all channels", instance, quantity);
    const Bool_t plsci = TString(instance).BeginsWith("PLSCI");
    const Int_t channels = plsci ? SuFDeMon::PlsciPmtCount(instance) : 32;
    auto* canvas = new TCanvas(canvasName.Data(), title.Data(), 40, 40, 1600, 900);
    canvas->Divide(plsci ? channels / 2 : 8, plsci ? 2 : 4, 0.001, 0.001);
    for (Int_t channel = 0; channel < channels; ++channel) {
        const TString name = TString::Format("h%s_%s%d", instance, quantity, channel);
        TH1* histogram = SuFDeMonGet(name.Data());
        canvas->cd(channel + 1);
        if (histogram)
            histogram->Draw();
        else
            Warning("DrawDetectorAll", "Could not draw %s", name.Data());
    }
    canvas->Modified();
    canvas->Update();
}

#endif
