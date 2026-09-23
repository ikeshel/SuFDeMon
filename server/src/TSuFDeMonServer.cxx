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

#include "SuFDeMonNames.h"
#include "SuFDeMonProtocol.h"

#include <TMessage.h>
#include <TServerSocket.h>
#include <TSocket.h>
#include <TH1D.h>
#include <TRandom3.h>

#include <iostream>
#include <sstream>
#include <string>
#include <chrono>

namespace {

bool StartsWith(const std::string& text, const std::string& prefix)
{
    return text.size() >= prefix.size()
        && text.compare(0, prefix.size(), prefix) == 0;
}

void SendText(TSocket& socket, const std::string& text)
{
    socket.Send(text.c_str());
}

} // namespace

TSuFDeMonServer::TSuFDeMonServer(int port)
    : fPort(port),
      fRandom(std::make_unique<TRandom3>(0))
{
    CreateHistograms();
}

TSuFDeMonServer::~TSuFDeMonServer()
{
    fFillRunning = false;
    if (fFillThread.joinable())
        fFillThread.join();
}

void TSuFDeMonServer::CreateHistograms()
{
    for (int fc = 1; fc <= SuFDeMon::kNFieldCages; ++fc) {
        for (int adc = 0; adc < SuFDeMon::kNAdcChannels; ++adc) {
            const std::string name = SuFDeMon::MusicAdcHistogramName(fc, adc);
            const std::string title = "MUSIC ADC FC" + std::to_string(fc)
                                    + " ADC" + std::to_string(adc)
                                    + ";ADC value;Counts";

            fMusicAdc[fc - 1][adc] =
                std::make_unique<TH1D>(name.c_str(), title.c_str(), 4096, 0.0, 4096.0);

            fMusicAdc[fc - 1][adc]->SetDirectory(nullptr);
        }
    }
}

void TSuFDeMonServer::FillHistograms()
{
    for (int fc = 0; fc < kNFieldCages; ++fc) {
        for (int adc = 0; adc < kNAdcChannels; ++adc) {
            const double mean = 1500.0 + 250.0 * fc + 5.0 * adc;
            const double sigma = 120.0 + 10.0 * fc;
            fMusicAdc[fc][adc]->Fill(fRandom->Gaus(mean, sigma));
        }
    }
}

void TSuFDeMonServer::FillLoop()
{
    // Simulate a continuously running detector independently of client traffic.
    // One event per histogram is generated every 10 ms (~100 Hz).
    while (fFillRunning) {
        FillHistograms();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

TH1D* TSuFDeMonServer::FindHistogram(const std::string& name)
{
    for (auto& fieldCage : fMusicAdc) {
        for (auto& histogram : fieldCage) {
            if (name == histogram->GetName()) {
                return histogram.get();
            }
        }
    }

    return nullptr;
}

std::string TSuFDeMonServer::HistogramList() const
{
    std::ostringstream output;

    for (const auto& fieldCage : fMusicAdc) {
        for (const auto& histogram : fieldCage) {
            output << histogram->GetName() << '\n';
        }
    }

    return output.str();
}

bool TSuFDeMonServer::HandleCommand(TSocket& socket, const std::string& command)
{
    if (command == SuFDeMon::Protocol::kPing) {
        SendText(socket, "PONG");
        return true;
    }

    if (command == SuFDeMon::Protocol::kList) {
        SendText(socket, HistogramList());
        return true;
    }

    if (command == SuFDeMon::Protocol::kClearAll) {
        for (auto& fieldCage : fMusicAdc) {
            for (auto& histogram : fieldCage) {
                histogram->Reset();
            }
        }

        SendText(socket, "OK");
        return true;
    }

    const std::string getPrefix = std::string(SuFDeMon::Protocol::kGet) + " ";
    if (StartsWith(command, getPrefix)) {
        const std::string name = command.substr(getPrefix.size());
        TH1D* histogram = FindHistogram(name);

        if (!histogram) {
            SendText(socket, "ERROR histogram not found");
            return true;
        }

        TMessage message(kMESS_OBJECT);
        message.WriteObject(histogram);
        socket.Send(message);
        return true;
    }

    const std::string clearPrefix = std::string(SuFDeMon::Protocol::kClear) + " ";
    if (StartsWith(command, clearPrefix)) {
        const std::string name = command.substr(clearPrefix.size());
        TH1D* histogram = FindHistogram(name);

        if (!histogram) {
            SendText(socket, "ERROR histogram not found");
            return true;
        }

        histogram->Reset();
        SendText(socket, "OK");
        return true;
    }

    if (command == SuFDeMon::Protocol::kShutdown) {
        SendText(socket, "BYE");
        fServerRunning = false;
        fFillRunning = false;
        return false;
    }

    if (command == SuFDeMon::Protocol::kQuit) {
        SendText(socket, "BYE");
        return false;
    }

    SendText(socket, "ERROR unknown command");
    return true;
}

bool TSuFDeMonServer::HandleClient(TSocket& socket)
{
    std::cout << "Client connected." << std::endl;

    while (socket.IsValid()) {
        char commandBuffer[4096] = {};
        const int received = socket.Recv(commandBuffer, sizeof(commandBuffer));

        if (received <= 0) {
            break;
        }

        const std::string command(commandBuffer);
        std::cout << "Command: " << command << std::endl;

        if (!HandleCommand(socket, command)) {
            break;
        }
    }

    socket.Close();
    std::cout << "Client disconnected." << std::endl;
    return true;
}

int TSuFDeMonServer::Run()
{
    fServerSocket = std::make_unique<TServerSocket>(fPort, true);

    if (!fServerSocket->IsValid()) {
        std::cerr << "Failed to open server socket on port " << fPort << std::endl;
        return 1;
    }

    std::cout << "SuFDeMon server listening on port " << fPort << std::endl;
    std::cout << "Created " << kNFieldCages * kNAdcChannels
              << " MUSIC ADC histograms." << std::endl;

    fFillRunning = true;
    fFillThread = std::thread(&TSuFDeMonServer::FillLoop, this);
    std::cout << "Continuous simulated data filling started at ~100 Hz." << std::endl;

    fServerRunning = true;
    while (fServerRunning) {
        std::unique_ptr<TSocket> socket(fServerSocket->Accept());

        if (!socket || !socket->IsValid()) {
            if (!fServerRunning)
                break;
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        HandleClient(*socket);
    }

    fFillRunning = false;
    if (fFillThread.joinable())
        fFillThread.join();

    fServerSocket->Close();
    std::cout << "SuFDeMon server stopped." << std::endl;
    return 0;
}
