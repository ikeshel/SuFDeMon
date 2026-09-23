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
#include <TH1D.h>
#include <TTimer.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

std::string TSuFDeMonGui::SelectedHistogramName() const
{
    return SuFDeMon::MusicAdcHistogramName(
        fFieldCageCombo->GetSelected(), fAdcCombo->GetSelected());
}

void TSuFDeMonGui::UpdateHistogramName()
{
    fHistogramEntry->SetText(SelectedHistogramName().c_str());
}

void TSuFDeMonGui::SetConnectedUi(bool connected)
{
    fConnectButton->SetEnabled(!connected);
    fDisconnectButton->SetEnabled(connected);
    fDrawButton->SetEnabled(connected);
    fClearButton->SetEnabled(connected);
    fClearAllButton->SetEnabled(connected);
    fCloseServerButton->SetEnabled(connected);
    fStatusLabel->SetText(connected ? "Connected" : "Disconnected");

    Pixel_t statusColor = 0;
    gClient->GetColorByName(connected ? "green" : "red", statusColor);
    fStatusLabel->SetTextColor(statusColor);

    if (!connected && fUpdateTimer) fUpdateTimer->TurnOff();
    if (fConnectionTimer) {
        fConnectionTimer->TurnOff();
        if (connected) fConnectionTimer->Start(1000, kTRUE);
    }

    SyncConfiguredServerStates();
    Layout();
}

void TSuFDeMonGui::ConnectServer()
{
    if (fClient) fClient->Disconnect();
    fClient = std::make_unique<TSuFDeMonClient>(
        fHostEntry->GetText(),
        static_cast<int>(fPortEntry->GetNumber()));

    const bool connected = fClient->Connect();
    if (!connected) fClient.reset();

    SetConnectedUi(connected);
    UpdateTimerState();
}

void TSuFDeMonGui::DisconnectServer()
{
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    if (fClient) {
        fClient->Disconnect();
        fClient.reset();
    }
    SetConnectedUi(false);
}

void TSuFDeMonGui::ConnectConfiguredServer(std::size_t index)
{
    if (index >= fServerButtons.size()) return;

    const bool thisServerIsActive =
        fClient && fClient->IsConnected() &&
        fServerHosts[index] == fHostEntry->GetText() &&
        fServerPorts[index] == static_cast<int>(fPortEntry->GetNumber());

    if (thisServerIsActive) {
        DisconnectServer();
        return;
    }

    fHostEntry->SetText(fServerHosts[index].c_str());
    fPortEntry->SetNumber(fServerPorts[index], kFALSE);

    DisconnectServer();
    ConnectServer();
    SyncConfiguredServerStates();
}

void TSuFDeMonGui::RefreshServerConnections()
{
    // This view reports the client connections owned by this GUI. It does not
    // create extra probe sockets, which is important while servers use a
    // single-client request loop.
    if (fClient && !fClient->IsConnected()) {
        DisconnectServer();
        return;
    }

    SyncConfiguredServerStates();
    Layout();
}

void TSuFDeMonGui::UpdateConfiguredServerButton(std::size_t index)
{
    if (index >= fServerButtons.size()) return;

    const bool connected = fServerStates[index] > 0;
    const char* stateText =
        connected ? "Connected - Disconnect" : "Disconnected - Connect";
    const char* colorName = connected ? "green" : "red";

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
    const bool connected = fClient && fClient->IsConnected();
    const std::string host = fHostEntry ? fHostEntry->GetText() : "";
    const int port = fPortEntry
        ? static_cast<int>(fPortEntry->GetNumber())
        : 0;

    for (std::size_t index = 0; index < fServerButtons.size(); ++index) {
        fServerStates[index] =
            connected &&
            fServerHosts[index] == host &&
            fServerPorts[index] == port ? 1 : 0;
        UpdateConfiguredServerButton(index);
    }

    UpdateServerConnectionsSummary();
}

void TSuFDeMonGui::UpdateServerConnectionsSummary()
{
    if (!fServerConnectionsSummaryLabel) return;

    const auto connected =
        std::count(fServerStates.begin(), fServerStates.end(), 1);

    std::ostringstream text;
    text << fServerButtons.size() << " configured servers";

    if (connected > 0) {
        text << " | " << connected << " active client connection";
    } else if (fClient && fClient->IsConnected()) {
        text << " | active manual connection: "
             << fHostEntry->GetText() << ":"
             << static_cast<int>(fPortEntry->GetNumber());
    } else {
        text << " | no active connection";
    }

    fServerConnectionsSummaryLabel->SetText(text.str().c_str());
}

void TSuFDeMonGui::DisplayGeneralStatus()
{
    if (!fGeneralStatusLabel) return;

    const bool connected = fClient && fClient->IsConnected();
    const bool autoUpdate =
        fAutoUpdateCheck && fAutoUpdateCheck->IsOn();

    const auto configuredConnections =
        std::count(fServerStates.begin(), fServerStates.end(), 1);

    std::ostringstream status;
    status << "Client: " << (connected ? "Connected" : "Disconnected")
           << " | Endpoint: " << fHostEntry->GetText()
           << ":" << static_cast<int>(fPortEntry->GetNumber())
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
    if (!fClient || !fClient->IsConnected()) return;

    auto histogram = fClient->GetHistogram(SelectedHistogramName());
    if (!histogram) return;

    fHistogram = std::move(histogram);

    if (!fCanvas)
        fCanvas = new TCanvas(
            "SuFDeMonCanvas", "SuFDeMon Histogram", 1000, 700);

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
    if (!fClient || !fClient->IsConnected()) return;
    if (fClient->ClearHistogram(SelectedHistogramName()))
        FetchAndDraw();
    UpdateTimerState();
}

void TSuFDeMonGui::ClearAllHistograms()
{
    if (!fClient || !fClient->IsConnected()) return;
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
    if (!fUpdateIntervalEntry) return;

    if (fUpdateIntervalEntry->GetNumber() < 0.2)
        fUpdateIntervalEntry->SetNumber(0.2, kFALSE);
    UpdateTimerState();
}

void TSuFDeMonGui::UpdateIntervalArrow(Long_t value)
{
    if (!fUpdateIntervalEntry || value == 0) return;

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
    if (!fUpdateTimer) return;
    fUpdateTimer->TurnOff();

    const bool enabled =
        fAutoUpdateCheck && fAutoUpdateCheck->IsOn();
    const bool connected =
        fClient && fClient->IsConnected();

    if (!connected) {
        SetConnectedUi(false);
        return;
    }
    if (!enabled || !fHasDrawnHistogram) return;

    const double seconds =
        std::max(0.2, fUpdateIntervalEntry->GetNumber());
    const Long_t milliseconds =
        static_cast<Long_t>(std::lround(seconds * 1000.0));
    fUpdateTimer->Start(milliseconds, kTRUE);
}

void TSuFDeMonGui::AutoUpdate()
{
    if (!fAutoUpdateCheck || !fAutoUpdateCheck->IsOn()) return;
    if (!fClient || !fClient->IsConnected() || !fHasDrawnHistogram) {
        UpdateTimerState();
        return;
    }

    FetchAndDraw();
    UpdateTimerState();
}

void TSuFDeMonGui::CheckConnection()
{
    if (!fClient || !fClient->IsConnected()) {
        DisconnectServer();
        return;
    }
    fConnectionTimer->Start(1000, kTRUE);
}

void TSuFDeMonGui::CloseClient()
{
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    DisconnectServer();
    if (fCanvas) {
        fCanvas->Close();
        fCanvas = nullptr;
    }
    DeleteWindow();
    if (gApplication) gApplication->Terminate(0);
}

void TSuFDeMonGui::CloseServer()
{
    if (!fClient || !fClient->IsConnected()) return;
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    fClient->ShutdownServer();
    fClient.reset();
    SetConnectedUi(false);
}

void TSuFDeMonGui::CloseAll()
{
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    if (fClient && fClient->IsConnected())
        fClient->ShutdownServer();
    fClient.reset();

    if (fCanvas) {
        fCanvas->Close();
        fCanvas = nullptr;
    }

    DeleteWindow();
    if (gApplication) gApplication->Terminate(0);
}

void TSuFDeMonGui::CloseWindow()
{
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    DisconnectServer();
    DeleteWindow();
}
