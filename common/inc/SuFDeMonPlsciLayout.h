#ifndef SUFDEMON_PLSCI_LAYOUT_H
#define SUFDEMON_PLSCI_LAYOUT_H

#include <cstring>

namespace SuFDeMon {
// Six PMTs is the standard layout; detectors 4 and 6 have eight.
inline int PlsciPmtCount(const char* instance)
{
    return std::strcmp(instance, "PLSCI4") == 0 || std::strcmp(instance, "PLSCI6") == 0 ? 8 : 6;
}
}

#endif
