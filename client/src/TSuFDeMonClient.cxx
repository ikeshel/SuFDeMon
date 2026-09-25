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

#include <TCanvas.h>
#include <TClass.h>
#include <TH1.h>
#include <TMessage.h>
#include <TSocket.h>
#include <TSystem.h>

#include <iostream>
#include <memory>
#include <string>
#include <utility>

TSuFDeMonClient::TSuFDeMonClient(std::string host, int port)
    : fHost(std::move(host)), fPort(port) {}

TSuFDeMonClient::~TSuFDeMonClient() { Disconnect(); }

bool TSuFDeMonClient::Connect()
{
    Disconnect();
    fSocket = std::make_unique<TSocket>(fHost.c_str(), fPort);
    if (!fSocket->IsValid()) {
        std::cerr << "Failed to connect to " << fHost << ':' << fPort << std::endl;
        fSocket.reset();
        return false;
    }
    return true;
}

void TSuFDeMonClient::Disconnect()
{
    if (!fSocket) return;
    if (IsConnected() && SendCommand(std::string(SuFDeMon::Protocol::kQuit))) {
        ReceiveText();
    }
    fSocket->Close();
    fSocket.reset();
}

bool TSuFDeMonClient::IsConnected() const
{
    if (!fSocket || !fSocket->IsValid()) return false;
    // IsValid only checks the local descriptor. A peer FIN/reset leaves it valid.
    // Poll without waiting and peek one byte without consuming a ROOT message.
    const int ready = fSocket->Select(TSocket::kRead, 0);
    if (ready == 0) return true;
    if (ready > 0) {
        char byte = 0;
        const int received = fSocket->RecvRaw(&byte, 1, kPeek);
        if (received > 0) return true;
    }
    fSocket->Close();
    return false;
}

bool TSuFDeMonClient::SendCommand(const std::string& command)
{
    if (!IsConnected()) {
        std::cerr << "Client is not connected." << std::endl;
        return false;
    }
    if (fSocket->Send(command.c_str()) > 0) return true;
    fSocket->Close();
    return false;
}

std::string TSuFDeMonClient::ReceiveText()
{
    if (!IsConnected()) return {};
    char buffer[65536] = {};
    const int received = fSocket->Recv(buffer, sizeof(buffer));
    if (received > 0) return std::string(buffer);
    fSocket->Close();
    return {};
}

std::string TSuFDeMonClient::Info()
{
    if (!SendCommand(std::string(SuFDeMon::Protocol::kInfo))) return {};
    return ReceiveText();
}

bool TSuFDeMonClient::Ping()
{
    return SendCommand(std::string(SuFDeMon::Protocol::kPing)) && ReceiveText() == "PONG";
}

std::string TSuFDeMonClient::ListHistograms()
{
    if (!SendCommand(std::string(SuFDeMon::Protocol::kList))) return {};
    return ReceiveText();
}

std::unique_ptr<TH1> TSuFDeMonClient::GetHistogram(const std::string& name)
{
    if (!SendCommand(std::string(SuFDeMon::Protocol::kGet) + " " + name)) return nullptr;

    TMessage* rawMessage = nullptr;
    const int received = fSocket->Recv(rawMessage);
    std::unique_ptr<TMessage> message(rawMessage);

    if (received <= 0 || !message) {
        fSocket->Close();
        std::cerr << "Failed to receive histogram." << std::endl;
        return nullptr;
    }
    if (message->What() == kMESS_STRING) {
        char error[4096] = {};
        message->ReadString(error, sizeof(error));
        std::cerr << error << std::endl;
        return nullptr;
    }
    if (message->What() != kMESS_OBJECT) {
        std::cerr << "Unexpected message type received from server." << std::endl;
        return nullptr;
    }

    TObject* object = message->ReadObject(message->GetClass());
    auto* histogram = dynamic_cast<TH1*>(object);
    if (!histogram) {
        delete object;
        std::cerr << "Received object is not a TH1." << std::endl;
        return nullptr;
    }
    histogram->SetDirectory(nullptr);
    // Apply display colors locally, including snapshots from older servers.
    const std::string histogramName = histogram->GetName();
    const auto quantity = histogramName.substr(histogramName.find_last_of('_') + 1);
    if (histogram->GetDimension() == 1 &&
        (quantity.compare(0, 3, "ADC") == 0 || quantity.compare(0, 3, "TDC") == 0)) {
        const Color_t color = quantity.compare(0, 3, "ADC") == 0 ? kRed : kBlue;
        histogram->SetFillStyle(1001);
        histogram->SetFillColorAlpha(color, 0.35f);
        histogram->SetLineColor(color);
    }
    if (histogram->GetDimension() == 2) {
        histogram->SetOption("COLZ");
        histogram->SetStats(kFALSE);
    }
    return std::unique_ptr<TH1>(histogram);
}

bool TSuFDeMonClient::ClearHistogram(const std::string& name)
{
    return SendCommand(std::string(SuFDeMon::Protocol::kClear) + " " + name)
        && ReceiveText() == "OK";
}

bool TSuFDeMonClient::ClearAll()
{
    return SendCommand(std::string(SuFDeMon::Protocol::kClearAll))
        && ReceiveText() == "OK";
}

bool TSuFDeMonClient::ShutdownServer()
{
    if (!SendCommand(std::string(SuFDeMon::Protocol::kShutdown)))
        return false;

    const bool acknowledged = ReceiveText() == "BYE";
    if (fSocket) {
        fSocket->Close();
        fSocket.reset();
    }
    return acknowledged;
}

bool TSuFDeMonClient::DrawHistogram(const std::string& name)
{
    auto histogram = GetHistogram(name);
    if (!histogram) return false;

    auto canvas = std::make_unique<TCanvas>("SuFDeMonCanvas", name.c_str(), 1000, 700);
    histogram->Draw();
    canvas->Modified();
    canvas->Update();
    gSystem->ProcessEvents();

    std::cout << "Histogram displayed. Press Enter to close it..." << std::endl;
    std::cin.get();

    canvas->Close();
    gSystem->ProcessEvents();
    return true;
}
