// Author: Irakli Keshelashvili, 2026
//
// SupFDetMon - Super-FRS Detector Monitoring Software
//
// Copyright (C) 2026 Irakli Keshelashvili
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// See the LICENSE file in the project root for the full license text.

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
