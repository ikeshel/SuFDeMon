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
#include <TGFrame.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGTab.h>
#include <TGTextEntry.h>
#include <TTimer.h>

#include <algorithm>
#include <string>

ClassImp(TSuFDeMonGui)

TSuFDeMonGui::TSuFDeMonGui(const TGWindow* parent, UInt_t width, UInt_t height,
                           std::string host, int port)
    : TGMainFrame(parent, width, height)
{
    SetCleanup(kDeepCleanup);
    SetWindowName("SuFDeMon Controls");

    fTabs = new TGTab(this, width, height);
    auto* generalTab = fTabs->AddTab("General");
    auto* serverConnectionsTab = fTabs->AddTab("Server Connections");

    // General tab: top-level status overview.
    auto* generalStatus = new TGGroupFrame(generalTab, "General Status", kVerticalFrame);
    auto* generalStatusRow = new TGHorizontalFrame(generalStatus);
    fGeneralStatusButton = new TGTextButton(generalStatusRow, "Display General Status");
    fGeneralStatusLabel = new TGLabel(generalStatusRow, "Status has not been requested yet.");
    generalStatusRow->AddFrame(
        fGeneralStatusButton,
        new TGLayoutHints(kLHintsCenterY, 4, 12, 8, 8));
    generalStatusRow->AddFrame(
        fGeneralStatusLabel,
        new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 4, 8, 8));
    generalStatus->AddFrame(
        generalStatusRow,
        new TGLayoutHints(kLHintsExpandX, 0, 0, 2, 2));
    generalTab->AddFrame(
        generalStatus,
        new TGLayoutHints(kLHintsExpandX, 8, 8, 8, 4));

    // Histogram selection controls remain on the General tab.
    auto* controls = new TGGroupFrame(generalTab, "Select Histogram", kVerticalFrame);

    auto* fcRow = new TGHorizontalFrame(controls);
    fcRow->AddFrame(
        new TGLabel(fcRow, "Field Cage:"),
        new TGLayoutHints(kLHintsCenterY, 2, 8, 4, 4));
    fFieldCageCombo = new TGComboBox(fcRow);
    for (int fc = 1; fc <= SuFDeMon::kNFieldCages; ++fc)
        fFieldCageCombo->AddEntry(("FC" + std::to_string(fc)).c_str(), fc);
    fFieldCageCombo->Select(1);
    fFieldCageCombo->Resize(160, 24);
    fcRow->AddFrame(
        fFieldCageCombo,
        new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));
    controls->AddFrame(fcRow, new TGLayoutHints(kLHintsExpandX));

    auto* adcRow = new TGHorizontalFrame(controls);
    adcRow->AddFrame(
        new TGLabel(adcRow, "ADC Channel:"),
        new TGLayoutHints(kLHintsCenterY, 2, 8, 4, 4));
    fAdcCombo = new TGComboBox(adcRow);
    for (int adc = 0; adc < SuFDeMon::kNAdcChannels; ++adc)
        fAdcCombo->AddEntry(("ADC" + std::to_string(adc)).c_str(), adc);
    fAdcCombo->Select(0);
    fAdcCombo->Resize(160, 24);
    adcRow->AddFrame(
        fAdcCombo,
        new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));
    controls->AddFrame(adcRow, new TGLayoutHints(kLHintsExpandX));

    controls->AddFrame(
        new TGLabel(controls, "Histogram name:"),
        new TGLayoutHints(kLHintsLeft, 2, 2, 12, 2));
    fHistogramEntry = new TGTextEntry(controls);
    fHistogramEntry->SetEnabled(kFALSE);
    controls->AddFrame(
        fHistogramEntry,
        new TGLayoutHints(kLHintsExpandX, 2, 2, 2, 10));

    auto* updateRow = new TGHorizontalFrame(controls);
    fAutoUpdateCheck = new TGCheckButton(updateRow, "Auto update");
    fAutoUpdateCheck->SetState(kButtonDown);
    fUpdateIntervalEntry = new TGNumberEntry(
        updateRow, 1.0, 6, -1,
        TGNumberFormat::kNESRealOne,
        TGNumberFormat::kNEAPositive,
        TGNumberFormat::kNELLimitMin,
        0.2);
    updateRow->AddFrame(
        fAutoUpdateCheck,
        new TGLayoutHints(kLHintsCenterY, 2, 16, 5, 5));
    updateRow->AddFrame(
        new TGLabel(updateRow, "Interval [s]:"),
        new TGLayoutHints(kLHintsCenterY, 2, 6, 5, 5));
    updateRow->AddFrame(
        fUpdateIntervalEntry,
        new TGLayoutHints(kLHintsCenterY, 2, 2, 5, 5));
    controls->AddFrame(updateRow, new TGLayoutHints(kLHintsExpandX));

    auto* drawRow = new TGHorizontalFrame(controls);
    fDrawButton = new TGTextButton(drawRow, "&Draw");
    fClearButton = new TGTextButton(drawRow, "C&lear");
    fClearAllButton = new TGTextButton(drawRow, "Clear &All");
    drawRow->AddFrame(
        fDrawButton,
        new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 2, 4, 6, 6));
    drawRow->AddFrame(
        fClearButton,
        new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 4, 6, 6));
    drawRow->AddFrame(
        fClearAllButton,
        new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 2, 6, 6));
    controls->AddFrame(
        drawRow,
        new TGLayoutHints(kLHintsExpandX, 0, 0, 2, 4));

    generalTab->AddFrame(
        controls,
        new TGLayoutHints(kLHintsExpandX, 8, 8, 4, 4));

    // Process controls stay on the General tab.
    auto* processControls =
        new TGGroupFrame(generalTab, "Process Control", kVerticalFrame);
    auto* processRow = new TGHorizontalFrame(processControls);
    fCloseClientButton = new TGTextButton(processRow, "Close client");
    fCloseServerButton = new TGTextButton(processRow, "Close server");
    fCloseAllButton = new TGTextButton(processRow, "Close All");
    processRow->AddFrame(
        fCloseClientButton,
        new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 4, 0, 0));
    processRow->AddFrame(
        fCloseServerButton,
        new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 4, 0, 0));
    processRow->AddFrame(
        fCloseAllButton,
        new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 4, 0, 0));
    processControls->AddFrame(
        processRow,
        new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 0, 0, 12, 12));
    generalTab->AddFrame(
        processControls,
        new TGLayoutHints(kLHintsExpandX, 8, 8, 4, 8));

    // Server Connections tab: manual endpoint plus configured server overview.
    auto* connection =
        new TGGroupFrame(serverConnectionsTab, "Active Client Connection", kVerticalFrame);
    auto* connectionRow = new TGHorizontalFrame(connection);

    fHostEntry = new TGTextEntry(connectionRow, host.c_str());
    fHostEntry->Resize(180, 28);
    fPortEntry = new TGNumberEntry(
        connectionRow, port, 6, -1,
        TGNumberFormat::kNESInteger,
        TGNumberFormat::kNEANonNegative,
        TGNumberFormat::kNELLimitMinMax,
        1, 65535);
    fConnectButton = new TGTextButton(connectionRow, "&Connect");
    fDisconnectButton = new TGTextButton(connectionRow, "&Disconnect");
    fStatusLabel = new TGLabel(connectionRow, "Disconnected");

    connectionRow->AddFrame(
        new TGLabel(connectionRow, "Host:"),
        new TGLayoutHints(kLHintsCenterY, 5, 4, 0, 0));
    connectionRow->AddFrame(
        fHostEntry,
        new TGLayoutHints(kLHintsCenterY, 0, 5, 0, 0));
    connectionRow->AddFrame(
        new TGLabel(connectionRow, "Port:"),
        new TGLayoutHints(kLHintsCenterY, 0, 4, 0, 0));
    connectionRow->AddFrame(
        fPortEntry,
        new TGLayoutHints(kLHintsCenterY, 0, 15, 0, 0));
    connectionRow->AddFrame(
        fConnectButton,
        new TGLayoutHints(kLHintsCenterY, 0, 8, 0, 0));
    connectionRow->AddFrame(
        fDisconnectButton,
        new TGLayoutHints(kLHintsCenterY, 0, 20, 0, 0));
    connectionRow->AddFrame(
        fStatusLabel,
        new TGLayoutHints(kLHintsCenterY, 0, 5, 0, 0));

    connection->AddFrame(
        connectionRow,
        new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 0, 0, 12, 12));
    serverConnectionsTab->AddFrame(
        connection,
        new TGLayoutHints(kLHintsExpandX, 8, 8, 8, 4));

    BuildServerConnectionsTab(serverConnectionsTab);

    AddFrame(
        fTabs,
        new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 6, 6, 6, 6));

    // Timers.
    fUpdateTimer = new TTimer(1000, kTRUE);
    fConnectionTimer = new TTimer(1000, kTRUE);
    fConnectionTimer->Connect(
        "Timeout()", "TSuFDeMonGui", this, "CheckConnection()");

    // Signals and slots.
    fGeneralStatusButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "DisplayGeneralStatus()");
    fConnectButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "ConnectServer()");
    fDisconnectButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "DisconnectServer()");
    fFieldCageCombo->Connect(
        "Selected(Int_t)", "TSuFDeMonGui", this, "SelectionChanged(Int_t)");
    fAdcCombo->Connect(
        "Selected(Int_t)", "TSuFDeMonGui", this, "SelectionChanged(Int_t)");
    fDrawButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "DrawSelected()");
    fClearButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "ClearSelected()");
    fClearAllButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "ClearAllHistograms()");
    fCloseClientButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "CloseClient()");
    fCloseServerButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "CloseServer()");
    fCloseAllButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "CloseAll()");
    fAutoUpdateCheck->Connect(
        "Toggled(Bool_t)", "TSuFDeMonGui", this, "AutoUpdateToggled()");

    // Route the number-entry arrow events through our 0.2 s stepping logic.
    fUpdateIntervalEntry->SetButtonToNum(kTRUE);
    fUpdateIntervalEntry->Connect(
        "ValueChanged(Long_t)", "TSuFDeMonGui", this,
        "UpdateIntervalArrow(Long_t)");
    fUpdateIntervalEntry->Connect(
        "ValueSet(Long_t)", "TSuFDeMonGui", this,
        "UpdateIntervalChanged()");
    fUpdateTimer->Connect(
        "Timeout()", "TSuFDeMonGui", this, "AutoUpdate()");

    UpdateHistogramName();
    SetConnectedUi(false);

    MapSubwindows();
    const TGDimension defaultSize = GetDefaultSize();
    const UInt_t windowWidth = std::max<UInt_t>(960, defaultSize.fWidth);
    const UInt_t windowHeight = std::max<UInt_t>(700, defaultSize.fHeight);
    Resize(windowWidth, windowHeight);
    MapWindow();

    // Keep General as the startup tab.
    fTabs->SetTab(0, kFALSE);

    // Try the endpoint supplied on the command line immediately.
    ConnectServer();

    if (fClient && fClient->IsConnected())
        DrawSelected();
}

TSuFDeMonGui::~TSuFDeMonGui()
{
    if (fConnectionTimer) {
        fConnectionTimer->TurnOff();
        delete fConnectionTimer;
        fConnectionTimer = nullptr;
    }
    if (fUpdateTimer) {
        fUpdateTimer->TurnOff();
        delete fUpdateTimer;
        fUpdateTimer = nullptr;
    }
    if (fClient) fClient->Disconnect();
}
