#ifndef SUPFDETMON_NAMES_H
#define SUPFDETMON_NAMES_H

#include <stdexcept>
#include <string>

namespace SupFDetMon {

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

} // namespace SupFDetMon

#endif
