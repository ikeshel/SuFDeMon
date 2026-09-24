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
#include <TH1D.h>
#include <TTimer.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>

extern TSuFDeMonGui* gSuFDeMonGui;

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

    BuildDetectorTab(fTabs->AddTab("MUSIC"), 0);
    BuildDetectorTab(fTabs->AddTab("PLSCI"), 1);
    BuildDetectorTab(fTabs->AddTab("SCIFI"), 2);
    ActivateDetectorControls(0);

    // Process controls stay on the General tab.
    auto* processControls =
        new TGGroupFrame(generalTab, "Process Control", kVerticalFrame);
    auto* processRow = new TGHorizontalFrame(processControls);
    fCloseClientButton = new TGTextButton(processRow, "Close client");
    fCloseServerButton = new TGTextButton(processRow, "Close server");
    fCloseAllButton = new TGTextButton(processRow, "Close All");
    fCloseServerButton->SetToolTipText("Shut down the selected histogram server");
    fCloseAllButton->SetToolTipText("Shut down all connected servers and close the client");
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
    fTabs->Connect("Selected(Int_t)", "TSuFDeMonGui", this, "DetectorTabSelected(Int_t)");
    fCloseClientButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "CloseClient()");
    fCloseServerButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "CloseServer()");
    fCloseAllButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "CloseAll()");
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

    // Try every configured server; failed connections remain available for retry.
    for (int group = 0; group < 3; ++group) ConnectGroup(group);

    // Prefer the requested endpoint for drawing when it is connected.
    for (std::size_t i = 0; i < fServerHosts.size(); ++i) {
        if (fServerHosts[i] == host && fServerPorts[i] == port &&
            fServerClients[i] && fServerClients[i]->IsConnected()) {
            SelectServer(static_cast<Int_t>(i));
            break;
        }
    }
    fConnectionTimer->Start(1000, kTRUE);

    if (fClient && fClient->IsConnected())
        DrawSelected();
}

TSuFDeMonGui::~TSuFDeMonGui()
{
    if (gSuFDeMonGui == this)
        gSuFDeMonGui = nullptr;
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
    for (auto& client : fServerClients)
        if (client) client->Disconnect();
}

void TSuFDeMonGui::BuildDetectorTab(TGCompositeFrame* tab, int group)
{
    auto* controls = new TGGroupFrame(tab, "Select Histogram", kVerticalFrame);

    auto* serverRow = new TGHorizontalFrame(controls);
    serverRow->AddFrame(new TGLabel(serverRow, "Histogram server:"),
        new TGLayoutHints(kLHintsCenterY, 2, 8, 4, 4));
    fServerCombo = new TGComboBox(serverRow);
    fServerCombo->Resize(240, 24);
    serverRow->AddFrame(fServerCombo, new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));
    controls->AddFrame(serverRow, new TGLayoutHints(kLHintsExpandX));

    fFieldCageCombo = nullptr;
    if (group == 0) {
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

    }

    auto* quantityRow = new TGHorizontalFrame(controls);
    quantityRow->AddFrame(new TGLabel(quantityRow, "Quantity:"),
        new TGLayoutHints(kLHintsCenterY, 2, 8, 4, 4));
    fQuantityCombo = new TGComboBox(quantityRow);
    fQuantityCombo->AddEntry("ADC", 0);
    fQuantityCombo->AddEntry("TDC", 1);
    fQuantityCombo->Select(0);
    fQuantityCombo->Resize(160, 24);
    quantityRow->AddFrame(fQuantityCombo, new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));
    controls->AddFrame(quantityRow, new TGLayoutHints(kLHintsExpandX));

    auto* adcRow = new TGHorizontalFrame(controls);
    adcRow->AddFrame(
        new TGLabel(adcRow, "Channel:"),
        new TGLayoutHints(kLHintsCenterY, 2, 8, 4, 4));
    fAdcCombo = new TGComboBox(adcRow);
    for (int adc = 0; adc < SuFDeMon::kNAdcChannels; ++adc)
        fAdcCombo->AddEntry(("CH" + std::to_string(adc)).c_str(), adc);
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

    tab->AddFrame(
        controls,
        new TGLayoutHints(kLHintsExpandX, 8, 8, 4, 4));

    {
        const std::string detector = std::array<std::string, 3>{"MUSIC", "PLSCI", "SCIFI"}[group];
        auto* macros = new TGGroupFrame(tab, ("Custom " + detector + " Drawings").c_str(), kVerticalFrame);
        auto* macroRow = new TGHorizontalFrame(macros);
        auto* macroCombo = new TGComboBox(macroRow);
        fMacroCombos[group] = macroCombo;
        macroCombo->Resize(360, 24);
        auto* drawMacroButton = new TGTextButton(macroRow, "Draw macro");
        auto& paths = fMacroPaths[group];

        std::filesystem::path macroDirectory;
        if (const char* configured = std::getenv("SUFDEMON_MACRO_DIR"))
            macroDirectory = std::filesystem::path(configured) / detector;
        if (macroDirectory.empty() || !std::filesystem::is_directory(macroDirectory)) {
            for (const auto& candidate : {
                     std::filesystem::path("macros") / detector,
                     std::filesystem::path("../macros") / detector,
                     std::filesystem::path("../../macros") / detector}) {
                if (std::filesystem::is_directory(candidate)) {
                    macroDirectory = std::filesystem::absolute(candidate);
                    break;
                }
            }
        }

        std::vector<std::string> macroNames;
        const int instances = std::array<int, 3>{2, 6, 14}[group];
        for (int instance = 1; instance <= instances; ++instance)
            for (const std::string quantity : {"ADC", "TDC"})
                macroNames.push_back("Draw_" + detector + std::to_string(instance)
                    + "_" + quantity + "_ALL.C");
        for (const auto& macroName : macroNames) {
            const auto path = macroDirectory / macroName;
            if (!std::filesystem::is_regular_file(path))
                continue;
            const int id = static_cast<int>(paths.size());
            paths.push_back(path.string());
            macroCombo->AddEntry(macroName.c_str(), id);
        }
        if (!paths.empty())
            macroCombo->Select(0);
        else
            drawMacroButton->SetEnabled(kFALSE);

        macroRow->AddFrame(macroCombo,
            new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 2, 8, 4, 4));
        macroRow->AddFrame(drawMacroButton,
            new TGLayoutHints(kLHintsCenterY, 2, 2, 4, 4));
        macros->AddFrame(macroRow, new TGLayoutHints(kLHintsExpandX));
        tab->AddFrame(macros,
            new TGLayoutHints(kLHintsExpandX, 8, 8, 4, 4));
        drawMacroButton->Connect(
            "Clicked()", "TSuFDeMonGui", this, "DrawSelectedMacro()");
    }

    fServerCombo->Connect(
        "Selected(Int_t)", "TSuFDeMonGui", this, "SelectServer(Int_t)");
    if (fFieldCageCombo) fFieldCageCombo->Connect(
        "Selected(Int_t)", "TSuFDeMonGui", this, "SelectionChanged(Int_t)");
    fQuantityCombo->Connect(
        "Selected(Int_t)", "TSuFDeMonGui", this, "SelectionChanged(Int_t)");
    fAdcCombo->Connect(
        "Selected(Int_t)", "TSuFDeMonGui", this, "SelectionChanged(Int_t)");
    fDrawButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "DrawSelected()");
    fClearButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "ClearSelected()");
    fClearAllButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "ClearAllHistograms()");
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
    fDetectorControls[group] = {fServerCombo, fFieldCageCombo, fAdcCombo,
        fQuantityCombo, fHistogramEntry, fDrawButton, fClearButton, fClearAllButton,
        fAutoUpdateCheck, fUpdateIntervalEntry};
}

void TSuFDeMonGui::ActivateDetectorControls(int group)
{
    const auto& controls = fDetectorControls[group];
    fServerCombo = controls.server;
    fFieldCageCombo = controls.fieldCage;
    fAdcCombo = controls.channel;
    fQuantityCombo = controls.quantity;
    fHistogramEntry = controls.histogram;
    fDrawButton = controls.draw;
    fClearButton = controls.clear;
    fClearAllButton = controls.clearAll;
    fAutoUpdateCheck = controls.autoUpdate;
    fUpdateIntervalEntry = controls.interval;
}
