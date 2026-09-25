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

#include "TSuFDeMonClient.h"
#include "SuFDeMonProtocol.h"

#include <TH1.h>
#include <TInterpreter.h>
#include <TRint.h>

#include <iostream>
#include <memory>
#include <string>

namespace {
std::unique_ptr<TSuFDeMonClient> gClient;
std::unique_ptr<TH1> gHistogram;
}

bool SuFDeMonPing()
{
    return gClient && gClient->Ping();
}

void ListOfHistograms()
{
    if (gClient) std::cout << gClient->ListHistograms();
}

TH1* SuFDeMonGet(const char* name)
{
    if (!gClient) return nullptr;
    gHistogram = gClient->GetHistogram(name);
    return gHistogram.get();
}

TH1* SuFDeMonDraw(const char* name)
{
    TH1* histogram = SuFDeMonGet(name);
    if (histogram) histogram->Draw();
    return histogram;
}

bool SuFDeMonClear(const char* name)
{
    return gClient && gClient->ClearHistogram(name);
}

bool SuFDeMonClearAll()
{
    return gClient && gClient->ClearAll();
}

int main(int argc, char** argv)
{
    std::string host = "localhost";
    int port = SuFDeMon::Protocol::kDefaultPort;

    // Consume our host/port arguments before TRint sees its own ROOT options.
    if (argc >= 2 && argv[1][0] != '-') host = argv[1];
    if (argc >= 3 && argv[2][0] != '-') {
        try {
            port = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Invalid port: " << argv[2] << std::endl;
            return 1;
        }
    }

    int rootArgc = 1;
    char* rootArgv[] = {argv[0], nullptr};
    TRint application("SuFDeMonClient", &rootArgc, rootArgv);

    gClient = std::make_unique<TSuFDeMonClient>(host, port);
    if (!gClient->Connect()) return 1;

    gInterpreter->Declare(R"(
        class TH1;
        bool SuFDeMonPing();
        void ListOfHistograms();
        TH1* SuFDeMonGet(const char*);
        TH1* SuFDeMonDraw(const char*);
        bool SuFDeMonClear(const char*);
        bool SuFDeMonClearAll();
    )");

    std::cout << "\nSuFDeMon connected to " << host << ':' << port << "\n"
              << "ROOT prompt is active. Normal ROOT/C++ commands work here.\n"
              << "Histogram names use h followed by the server instance name.\n"
              << "Use ListOfHistograms() to see the exact names for this server.\n"
              << "SuFDeMon helpers:\n"
              << "  SuFDeMonPing()\n"
              << "  ListOfHistograms()\n"
              << "  TH1* h = SuFDeMonGet(\"hMUSIC1_FC1_ADC0\")\n"
              << "  SuFDeMonDraw(\"hMUSIC1_FC1_ADC0\")\n"
              << "  SuFDeMonClear(\"hMUSIC1_FC1_ADC0\")\n"
              << "  SuFDeMonClearAll()\n\n";

    application.Run();

    gClient->Disconnect();
    gHistogram.reset();
    gClient.reset();
    return 0;
}
