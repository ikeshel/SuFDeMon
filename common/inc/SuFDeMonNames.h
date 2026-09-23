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

inline std::string InstanceHistogramName(const std::string& instance,
                                         const std::string& histogramName)
{
    if (instance.empty()) {
        throw std::invalid_argument("Histogram instance name must not be empty");
    }
    if (histogramName.empty()) {
        throw std::invalid_argument("Histogram name must not be empty");
    }

    return instance + "_" + histogramName;
}

inline std::string MusicAdcHistogramName(const std::string& instance,
                                         int fieldCage,
                                         int adcChannel)
{
    if (fieldCage < 1 || fieldCage > kNFieldCages) {
        throw std::out_of_range("MUSIC field cage must be in the range 1..3");
    }

    if (adcChannel < 0 || adcChannel >= kNAdcChannels) {
        throw std::out_of_range("MUSIC ADC channel must be in the range 0..31");
    }

    return InstanceHistogramName(
        instance,
        "TH1D_MUSIC_ADC_FC" + std::to_string(fieldCage)
            + "_ADC" + std::to_string(adcChannel));
}

} // namespace SuFDeMon

#endif
