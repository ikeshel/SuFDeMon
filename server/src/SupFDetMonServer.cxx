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

#include "TSupFDetMonServer.h"

#include "SupFDetMonProtocol.h"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    int port = SupFDetMon::Protocol::kDefaultPort;

    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [port]" << std::endl;
        return 1;
    }

    if (argc == 2) {
        try {
            port = std::stoi(argv[1]);
        } catch (const std::exception&) {
            std::cerr << "Invalid port: " << argv[1] << std::endl;
            return 1;
        }

        if (port < 1 || port > 65535) {
            std::cerr << "Port must be in the range 1..65535." << std::endl;
            return 1;
        }
    }

    TSupFDetMonServer server(port);
    return server.Run();
}
