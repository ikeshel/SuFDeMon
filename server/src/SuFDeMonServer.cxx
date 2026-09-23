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

#include "TSuFDeMonServer.h"
#include "SuFDeMonServerConfig.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#ifndef SUFDEMON_DEFAULT_TYPE
#define SUFDEMON_DEFAULT_TYPE "MUSIC"
#endif

namespace {
void Usage(const char* program)
{
    std::cout << "Usage: " << program
              << " [port] [--config FILE] [--type MUSIC|PLSCI|SCIFI]"
                 " [--instance NAME] [--hostname HOST] [--port PORT]\n"
                 "Options override the config file regardless of order.\n"
                 "Hostname is advertised to clients; the socket listens on all local interfaces.\n";
}
}

int main(int argc, char** argv)
{
    try {
        SuFDeMon::ServerConfig config;
        config.type = SuFDeMon::ParseDetectorType(SUFDEMON_DEFAULT_TYPE);
        config.instance = std::string(SUFDEMON_DEFAULT_TYPE) + "1";
        std::string configPath;
        // Locate the config first so explicit command-line options always win.
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") { Usage(argv[0]); return 0; }
            if (arg == "--config") {
                if (++i == argc) throw std::invalid_argument("Missing --config value");
                if (!configPath.empty()) throw std::invalid_argument("Only one config file is allowed");
                configPath = argv[i];
            } else if (arg == "--type" || arg == "--instance" || arg == "--hostname" || arg == "--port") {
                if (++i == argc) throw std::invalid_argument("Missing value for " + arg);
            }
        }
        if (!configPath.empty()) config = SuFDeMon::ReadServerConfig(configPath);
        bool explicitInstance = false;
        bool explicitType = false;
        bool portGiven = false;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--config") { ++i; continue; }
            if (arg == "--type") {
                config.type = SuFDeMon::ParseDetectorType(argv[++i]);
                explicitType = true;
            } else if (arg == "--instance") {
                config.instance = argv[++i];
                explicitInstance = true;
            } else if (arg == "--hostname") config.hostname = argv[++i];
            else if (arg == "--port" || (!arg.empty() && arg[0] != '-')) {
                if (portGiven) throw std::invalid_argument("Port specified more than once");
                config.port = SuFDeMon::ParseServerPort(arg == "--port" ? argv[++i] : arg);
                portGiven = true;
            } else throw std::invalid_argument("Unknown option: " + arg);
        }
        if (explicitType && !explicitInstance && configPath.empty())
            config.instance = std::string(SuFDeMon::DetectorTypeName(config.type)) + "1";
#ifdef SUFDEMON_FIXED_TYPE
        if (config.type != SuFDeMon::ParseDetectorType(SUFDEMON_DEFAULT_TYPE))
            throw std::invalid_argument("This executable only serves " SUFDEMON_DEFAULT_TYPE);
#endif
        SuFDeMon::ValidateServerConfig(config);
        TSuFDeMonServer server(config);
        return server.Run();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        Usage(argv[0]);
        return 1;
    }
}
