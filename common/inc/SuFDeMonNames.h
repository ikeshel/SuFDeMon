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

#ifndef SUFDEMON_NAMES_H
#define SUFDEMON_NAMES_H

#include <stdexcept>
#include <string>

namespace SuFDeMon {

inline constexpr int kNFieldCages = 3;
inline constexpr int kNAdcChannels = 32;

inline std::string MusicAdcHistogramName(int fieldCage, int adcChannel)
{
    if (fieldCage < 1 || fieldCage > kNFieldCages) {
        throw std::out_of_range("MUSIC field cage must be in the range 1..3");
    }

    if (adcChannel < 0 || adcChannel >= kNAdcChannels) {
        throw std::out_of_range("MUSIC ADC channel must be in the range 0..31");
    }

    return "TH1D_MUSIC_ADC_FC" + std::to_string(fieldCage)
         + "_ADC" + std::to_string(adcChannel);
}

} // namespace SuFDeMon

#endif
