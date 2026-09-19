#ifndef SUPFDETMON_PROTOCOL_H
#define SUPFDETMON_PROTOCOL_H

#include <string_view>

namespace SupFDetMon::Protocol {

inline constexpr int kDefaultPort = 10001;

inline constexpr std::string_view kPing = "PING";
inline constexpr std::string_view kList = "LIST";
inline constexpr std::string_view kGet = "GET";
inline constexpr std::string_view kClear = "CLEAR";
inline constexpr std::string_view kClearAll = "CLEAR ALL";
inline constexpr std::string_view kQuit = "QUIT";
inline constexpr std::string_view kShutdown = "SHUTDOWN";

} // namespace SupFDetMon::Protocol

#endif
