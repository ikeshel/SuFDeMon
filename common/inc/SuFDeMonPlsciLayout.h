#ifndef SUFDEMON_PLSCI_LAYOUT_H
#define SUFDEMON_PLSCI_LAYOUT_H

#include <cstring>
#include <string>
#include <vector>
#include <sstream>

namespace SuFDeMon {
inline std::vector<std::string> PlsciChambers(const std::string& text)
{
    std::vector<std::string> result;
    std::istringstream input(text);
    std::string chamber;
    while (std::getline(input, chamber, ',')) {
        const auto first = chamber.find_first_not_of(" \t\r\n");
        result.push_back(first == std::string::npos ? "" :
            chamber.substr(first, chamber.find_last_not_of(" \t\r\n") - first + 1));
    }
    if (!text.empty() && text.back() == ',') result.emplace_back();
    return result;
}

inline std::string PlsciInstance(const std::string& chamber)
{
    const char* chambers[] = {"FPF2", "FPF4", "FMF1", "FMF2", "FMF3", "FHF1"};
    for (int i = 0; i < 6; ++i)
        if (chamber == chambers[i]) return "PLSCI" + std::to_string(i + 1);
    return {};
}

// Six PMTs is the standard layout; detectors 4 and 6 have eight.
inline int PlsciPmtCount(const char* instance)
{
    return std::strcmp(instance, "PLSCI4") == 0 || std::strcmp(instance, "PLSCI6") == 0 ? 8 : 6;
}
}

#endif
