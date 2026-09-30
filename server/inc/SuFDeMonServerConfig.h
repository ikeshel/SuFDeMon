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

#ifndef SUFDEMON_SERVER_CONFIG_H
#define SUFDEMON_SERVER_CONFIG_H

#include <string>

namespace SuFDeMon {

enum class DetectorType { MUSIC, PLSCI, SCIFI };

const char* DetectorTypeName(DetectorType type);
DetectorType ParseDetectorType(const std::string& name);

struct ServerConfig {
    DetectorType type = DetectorType::MUSIC;
    std::string instance = "MUSIC1";
    // Advertised client hostname; the listening socket uses all local interfaces.
    std::string hostname = "localhost";
    int port = 10001;
};

// A config file contains type, instance, hostname and port as key=value lines.
ServerConfig ReadServerConfig(const std::string& path);
void ValidateServerConfig(const ServerConfig& config);
int ParseServerPort(const std::string& text);

} // namespace SuFDeMon

#endif
