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

#include "SuFDeMonNames.h"
#include "TSuFDeMonClient.h"

#include <TApplication.h>
#include <TCanvas.h>
#include <TGButton.h>
#include <TGClient.h>
#include <TGComboBox.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGTextEntry.h>
#include <TGTab.h>
#include <TH1D.h>
#include <TTimer.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
std::string InfoValue(const std::string& info, const std::string& key)
{
    const std::string prefix = key + "=";
    std::istringstream input(info);
    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind(prefix, 0) == 0)
            return line.substr(prefix.size());
    }
    return {};
}
}

std::string TSuFDeMonGui::SelectedHistogramName() const
{
    const auto name = SuFDeMon::DetectorHistogramName(
        fConnectedInstance, fQuantityCombo->GetSelected() == 1 ? "TDC" : "ADC",
        fAdcCombo->GetSelected(),
        fConnectedDetectorType == "MUSIC" ? fFieldCageCombo->GetSelected() : 0);
    return (fLegacyHistogramPrefix.empty() || fQuantityCombo->GetSelected() == 1) ? name
        : fLegacyHistogramPrefix + name.substr(fConnectedInstance.size() + 2);
}

void TSuFDeMonGui::UpdateHistogramName()
{
    fHistogramEntry->SetText(SelectedHistogramName().c_str());
}

void TSuFDeMonGui::UpdateServerIdentity(const std::string& info)
{
    const std::string type = InfoValue(info, "type");
    const std::string instance = InfoValue(info, "instance");

    if (!type.empty())
        fConnectedDetectorType = type;
    if (!instance.empty())
        fConnectedInstance = instance;

    UpdateHistogramName();
}

void TSuFDeMonGui::SetConnectedUi(bool connected)
{
    for (const auto& controls : fDetectorControls) {
        const int index = controls.server->GetSelected();
        const bool available = index >= 0 && static_cast<std::size_t>(index) < fServerClients.size()
            && fServerClients[index] && fServerClients[index]->IsConnected();
        controls.draw->SetEnabled(available);
        controls.clear->SetEnabled(available);
        controls.clearAll->SetEnabled(available);
    }

    fCloseServerButton->SetEnabled(connected);
    if (!connected && fUpdateTimer) fUpdateTimer->TurnOff();

    SyncConfiguredServerStates();
    Layout();
}

void TSuFDeMonGui::SelectServer(Int_t index)
{
    if (index < 0 || static_cast<std::size_t>(index) >= fServerClients.size()) return;
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    fHasDrawnHistogram = false;
    const int group = fServerTypes[index] == "MUSIC" ? 0 : fServerTypes[index] == "PLSCI" ? 1 : 2;
    ActivateDetectorControls(group);
    if (fTabs->GetCurrent() >= 2) fTabs->SetTab(group + 2, kFALSE);
    fSelectedServer = index;
    fServerCombo->Select(index, kFALSE);
    fClient = fServerClients[index];
    fConnectedDetectorType = fServerTypes[index];
    fConnectedInstance = fServerInstances[index];
    fLegacyHistogramPrefix.clear();
    if (fClient && fClient->IsConnected()) {
        UpdateServerIdentity(fClient->Info());
        if (fConnectedDetectorType == "MUSIC") {
            const auto expected = SuFDeMon::MusicAdcHistogramName(
                fConnectedInstance, fFieldCageCombo->GetSelected(), fAdcCombo->GetSelected());
            const auto suffix = expected.substr(fConnectedInstance.size() + 2);
            const auto legacy = "TH1D_MUSIC_ADC_" + suffix;
            const auto previous = fConnectedInstance + "_" + legacy;
            std::istringstream names(fClient->ListHistograms());
            std::string name;
            bool hasExpected = false;
            bool hasLegacy = false;
            bool hasPrevious = false;
            while (std::getline(names, name)) {
                hasExpected = hasExpected || name == expected;
                hasLegacy = hasLegacy || name == legacy;
                hasPrevious = hasPrevious || name == previous;
            }
            if (!hasExpected) {
                if (hasPrevious)
                    fLegacyHistogramPrefix = fConnectedInstance + "_TH1D_MUSIC_ADC_";
                else if (hasLegacy)
                    fLegacyHistogramPrefix = "TH1D_MUSIC_ADC_";
            }
            UpdateHistogramName();
        }
    }
    UpdateHistogramName();
    SetConnectedUi(fClient && fClient->IsConnected());
}

void TSuFDeMonGui::DetectorTabSelected(Int_t tab)
{
    if (tab < 2 || tab > 4) return;
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    ActivateDetectorControls(tab - 2);
    const int index = fServerCombo->GetSelected();
    if (index >= 0) {
        SelectServer(index);
    } else {
        fSelectedServer = -1;
        fClient.reset();
        fHasDrawnHistogram = false;
        fHistogramEntry->SetText("No configured servers for this detector");
        SetConnectedUi(false);
    }
}

void TSuFDeMonGui::SetServerConnected(std::size_t index, bool connected)
{
    if (index >= fServerClients.size()) return;
    auto& client = fServerClients[index];
    if (connected) {
        if (!client || !client->IsConnected()) {
            client = std::make_shared<TSuFDeMonClient>(fServerHosts[index], fServerPorts[index]);
            if (!client->Connect()) client.reset();
        }
    } else {
        if (client) client->Disconnect();
        client.reset();
    }
    if (fSelectedServer == static_cast<int>(index)) SelectServer(fSelectedServer);
}

void TSuFDeMonGui::ConnectServer()
{
    if (fSelectedServer >= 0) SetServerConnected(fSelectedServer, true);
    RefreshServerConnections();
}

void TSuFDeMonGui::DisconnectServer()
{
    if (fSelectedServer >= 0) SetServerConnected(fSelectedServer, false);
    RefreshServerConnections();
}

void TSuFDeMonGui::ConnectConfiguredServer(std::size_t index)
{
    if (index >= fServerClients.size()) return;
    const bool connected = fServerClients[index] && fServerClients[index]->IsConnected();
    SetServerConnected(index, !connected);
    if (!connected && fServerClients[index]) SelectServer(static_cast<Int_t>(index));
    RefreshServerConnections();
}

void TSuFDeMonGui::ConnectGroup(Int_t group)
{
    const std::vector<std::string> types = {"MUSIC", "PLSCI", "SCIFI"};
    if (group < 0 || group >= 3) return;
    for (std::size_t i = 0; i < fServerClients.size(); ++i)
        if (fServerTypes[i] == types[group]) SetServerConnected(i, true);
    if (!fClient || !fClient->IsConnected()) {
        for (std::size_t i = 0; i < fServerClients.size(); ++i)
            if (fServerClients[i] && fServerClients[i]->IsConnected()) {
                SelectServer(static_cast<Int_t>(i));
                break;
            }
    }
    RefreshServerConnections();
}

void TSuFDeMonGui::DisconnectGroup(Int_t group)
{
    const std::vector<std::string> types = {"MUSIC", "PLSCI", "SCIFI"};
    if (group < 0 || group >= 3) return;
    for (std::size_t i = 0; i < fServerClients.size(); ++i)
        if (fServerTypes[i] == types[group]) SetServerConnected(i, false);
    RefreshServerConnections();
}

int TSuFDeMonGui::ConnectedServerCount() const
{
    return std::count_if(fServerClients.begin(), fServerClients.end(),
        [](const auto& client) { return client && client->IsConnected(); });
}

void TSuFDeMonGui::ListOfHistograms()
{
    bool anyConnected = false;
    for (const auto& client : fServerClients) {
        if (!client || !client->IsConnected()) continue;
        anyConnected = true;
        std::cout << client->ListHistograms();
    }
    if (!anyConnected) std::cout << "No connected servers.\n";
    RefreshServerConnections();
}

void TSuFDeMonGui::RefreshServerConnections()
{
    SetConnectedUi(fClient && fClient->IsConnected());
    Layout();
}

void TSuFDeMonGui::UpdateConfiguredServerButton(std::size_t index)
{
    if (index >= fServerButtons.size())
        return;

    const bool connected = fServerStates[index] > 0;
    const char* stateText =
        connected ? "Connected - Disconnect" : "Disconnected - Connect";
    const char* colorName = connected ? "#176c37" : "red";

    const std::string label =
        fServerInstances[index] + "  " +
        fServerHosts[index] + ":" +
        std::to_string(fServerPorts[index]) +
        "  [" + stateText + "]";

    fServerButtons[index]->SetTitle(label.c_str());

    Pixel_t color = 0;
    gClient->GetColorByName(colorName, color);
    fServerButtons[index]->SetTextColor(color);
}

void TSuFDeMonGui::SyncConfiguredServerStates()
{
    for (std::size_t index = 0; index < fServerButtons.size(); ++index) {
        fServerStates[index] = fServerClients[index] && fServerClients[index]->IsConnected();
        UpdateConfiguredServerButton(index);
    }

    UpdateServerConnectionsSummary();
}

void TSuFDeMonGui::UpdateServerConnectionsSummary()
{
    if (!fServerConnectionsSummaryLabel)
        return;

    const auto connected =
        std::count(fServerStates.begin(), fServerStates.end(), 1);

    std::ostringstream text;
    text << fServerButtons.size() << " configured servers";

    text << " | " << connected << " connected";

    fServerConnectionsSummaryLabel->SetText(text.str().c_str());
}

void TSuFDeMonGui::DisplayGeneralStatus()
{
    if (!fGeneralStatusLabel)
        return;

    const bool connected = fClient && fClient->IsConnected();
    const bool autoUpdate =
        fAutoUpdateCheck && fAutoUpdateCheck->IsOn();

    const auto configuredConnections =
        std::count(fServerStates.begin(), fServerStates.end(), 1);

    std::ostringstream status;
    status << "Client: " << (connected ? "Connected" : "Disconnected")
           << " | Server: " << fConnectedDetectorType
           << " / " << fConnectedInstance
           << " | Configured connections: " << configuredConnections
           << " | Histogram: " << SelectedHistogramName()
           << " | Auto update: " << (autoUpdate ? "On" : "Off")
           << " (" << std::fixed << std::setprecision(1)
           << fUpdateIntervalEntry->GetNumber() << " s)";

    fGeneralStatusLabel->SetText(status.str().c_str());
    Layout();
}

void TSuFDeMonGui::SelectionChanged(Int_t)
{
    UpdateHistogramName();
}

void TSuFDeMonGui::FetchAndDraw()
{
    if (!fClient || !fClient->IsConnected())
        return;

    auto histogram = fClient->GetHistogram(SelectedHistogramName());
    if (!histogram) {
        std::cerr << "Could not load histogram '" << SelectedHistogramName()
                  << "'. Use ListOfHistograms() at the ROOT prompt to check the "
                     "names provided by the connected server.\n";
        return;
    }

    fHistogram = std::move(histogram);

    if (!fCanvas)
        fCanvas = new TCanvas(
            "SuFDeMonCanvas", "SuFDeMon Histogram", 10, 10, 1000, 700);

    fCanvas->cd();
    fCanvas->Clear();
    fHistogram->Draw();
    fCanvas->Modified();
    fCanvas->Update();
    fHasDrawnHistogram = true;
}

void TSuFDeMonGui::DrawSelected()
{
    FetchAndDraw();
    UpdateTimerState();
}

void TSuFDeMonGui::ClearSelected()
{
    if (!fClient || !fClient->IsConnected())
        return;

    if (fClient->ClearHistogram(SelectedHistogramName()))
        FetchAndDraw();
    UpdateTimerState();
}

void TSuFDeMonGui::ClearAllHistograms()
{
    if (!fClient || !fClient->IsConnected())
        return;

    if (fClient->ClearAll())
        FetchAndDraw();
    UpdateTimerState();
}

void TSuFDeMonGui::AutoUpdateToggled()
{
    UpdateTimerState();
}

void TSuFDeMonGui::UpdateIntervalChanged()
{
    if (!fUpdateIntervalEntry)
        return;

    if (fUpdateIntervalEntry->GetNumber() < 0.2)
        fUpdateIntervalEntry->SetNumber(0.2, kFALSE);
    UpdateTimerState();
}

void TSuFDeMonGui::UpdateIntervalArrow(Long_t value)
{
    if (!fUpdateIntervalEntry || value == 0)
        return;

    const double current = fUpdateIntervalEntry->GetNumber();
    const double direction = value > 0 ? 1.0 : -1.0;
    const double stepped =
        std::max(0.2, current + direction * 0.2);
    fUpdateIntervalEntry->SetNumber(
        std::round(stepped * 5.0) / 5.0, kFALSE);
    UpdateTimerState();
}

void TSuFDeMonGui::UpdateTimerState()
{
    if (!fUpdateTimer)
        return;

    fUpdateTimer->TurnOff();

    const bool enabled =
        fAutoUpdateCheck && fAutoUpdateCheck->IsOn();
    const bool connected =
        fClient && fClient->IsConnected();

    if (!connected) {
        SetConnectedUi(false);
        return;
    }

    if (!enabled || !fHasDrawnHistogram)
        return;

    const double seconds =
        std::max(0.2, fUpdateIntervalEntry->GetNumber());
    const Long_t milliseconds =
        static_cast<Long_t>(std::lround(seconds * 1000.0));
    fUpdateTimer->Start(milliseconds, kTRUE);
}

void TSuFDeMonGui::AutoUpdate()
{
    if (!fAutoUpdateCheck || !fAutoUpdateCheck->IsOn())
        return;

    if (!fClient || !fClient->IsConnected() ||
        !fHasDrawnHistogram) {
        UpdateTimerState();
        return;
    }

    FetchAndDraw();
    UpdateTimerState();
}

void TSuFDeMonGui::CheckConnection()
{
    RefreshServerConnections();
    fConnectionTimer->Start(1000, kTRUE);
}

void TSuFDeMonGui::CloseClient()
{
    if (fUpdateTimer)
        fUpdateTimer->TurnOff();
    DisconnectServer();
    if (fCanvas) {
        fCanvas->Close();
        fCanvas = nullptr;
    }
    DeleteWindow();
    if (gApplication)
        gApplication->Terminate(0);
}

void TSuFDeMonGui::CloseServer()
{
    if (!fClient || !fClient->IsConnected())
        return;

    if (fUpdateTimer)
        fUpdateTimer->TurnOff();
    fClient->ShutdownServer();
    DisconnectServer();
    SetConnectedUi(false);
}

void TSuFDeMonGui::CloseAll()
{
    if (fUpdateTimer)
        fUpdateTimer->TurnOff();
    for (auto& client : fServerClients)
        if (client && client->IsConnected()) client->ShutdownServer();
    fClient.reset();
    fServerClients.clear();

    if (fCanvas) {
        fCanvas->Close();
        fCanvas = nullptr;
    }

    DeleteWindow();
    if (gApplication)
        gApplication->Terminate(0);
}

void TSuFDeMonGui::CloseWindow()
{
    if (fUpdateTimer)
        fUpdateTimer->TurnOff();
    DisconnectServer();
    DeleteWindow();
}
