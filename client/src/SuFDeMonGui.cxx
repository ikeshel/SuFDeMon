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

#include "TSuFDeMonGui.h"
#include "SuFDeMonProtocol.h"

#include <TGClient.h>
#include <TRint.h>

#include <exception>
#include <iostream>
#include <string>

TSuFDeMonGui* gSuFDeMonGui = nullptr;

int main(int argc, char** argv)
{
    std::string host = "localhost";
    int port = SuFDeMon::Protocol::kDefaultPort;

    if (argc > 3) {
        std::cerr << "Usage: " << argv[0] << " [host] [port]\n";
        return 1;
    }
    if (argc >= 2) host = argv[1];
    if (argc == 3) {
        try {
            port = std::stoi(argv[2]);
        } catch (const std::exception&) {
            std::cerr << "Invalid port: " << argv[2] << '\n';
            return 1;
        }
        if (port < 1 || port > 65535) {
            std::cerr << "Port must be in the range 1..65535.\n";
            return 1;
        }
    }

    // TRint gives the GUI process the normal interactive ROOT command line too.
    int rootArgc = 1;
    char* rootArgv[] = {argv[0], nullptr};
    TRint application("SuFDeMonGui", &rootArgc, rootArgv);

    gSuFDeMonGui = new TSuFDeMonGui(gClient->GetRoot(), 430, 360, host, port);

    std::cout << "\nSuFDeMon GUI started. ROOT command line is active.\n"
              << "The control window and TCanvas are separate windows.\n"
              << "Global GUI pointer: gSuFDeMonGui\n\n";

    application.Run();

    gSuFDeMonGui = nullptr;
    return 0;
}
