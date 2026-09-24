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
#include "SuFDeMonPlsciLayout.h"

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

// ADC and TDC currently share 32 raw-count channels and 12-bit binning.
inline std::string DetectorHistogramName(const std::string& instance,
                                        const std::string& quantity,
                                        int channel, int fieldCage = 0)
{
    if (quantity != "ADC" && quantity != "TDC")
        throw std::invalid_argument("Quantity must be ADC or TDC");
    if (channel < 0 || channel >= kNAdcChannels)
        throw std::out_of_range("Channel must be in the range 0..31");
    if (fieldCage < 0 || fieldCage > kNFieldCages)
        throw std::out_of_range("Field cage must be 0 (none) or 1..3");
    const auto prefix = fieldCage ? "FC" + std::to_string(fieldCage) + "_" : "";
    return "h" + InstanceHistogramName(instance, prefix + quantity + std::to_string(channel));
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

    return "h" + InstanceHistogramName(
        instance,
        "FC" + std::to_string(fieldCage)
            + "_ADC" + std::to_string(adcChannel));
}

} // namespace SuFDeMon

#endif
