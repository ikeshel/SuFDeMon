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

#ifndef TSUFDEMONGUI_H
#define TSUFDEMONGUI_H

#include <TGFrame.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

class TGTextEntry;
class TGNumberEntry;
class TGComboBox;
class TGTextButton;
class TGCheckButton;
class TGLabel;
class TGTab;
class TGCompositeFrame;
class TCanvas;
class TH1D;
class TTimer;
class TSuFDeMonClient;

class TSuFDeMonGui : public TGMainFrame
{
public:
    TSuFDeMonGui(const TGWindow* parent, UInt_t width, UInt_t height,
                 std::string host = "localhost", int port = 9090);
    ~TSuFDeMonGui() override;

    void ConnectServer();
    void DisconnectServer();
    void SelectionChanged(Int_t id);
    void SelectServer(Int_t index);
    void DetectorTabSelected(Int_t tab);
    void ConnectGroup(Int_t group);
    void DisconnectGroup(Int_t group);
    int ConnectedServerCount() const;
    void ListOfHistograms();
    TH1D* FetchHistogram(const char* name);
    void DrawSelectedMacro();
    void DrawSelected();
    void ClearSelected();
    void ClearAllHistograms();
    void AutoUpdateToggled();
    void UpdateIntervalChanged();
    void UpdateIntervalArrow(Long_t value);
    void AutoUpdate();
    void CheckConnection();
    void DisplayGeneralStatus();
    void RefreshServerConnections();
    void CloseClient();
    void CloseServer();
    void CloseAll();
    void CloseWindow() override;
    Bool_t ProcessMessage(Longptr_t msg, Longptr_t parm1, Longptr_t parm2) override;

    TSuFDeMonClient* GetClient() const { return fClient.get(); }
    TH1D* GetHistogram() const { return fHistogram.get(); }

private:
    std::string SelectedHistogramName() const;
    void UpdateHistogramName();
    void UpdateServerIdentity(const std::string& info);
    void SetConnectedUi(bool connected);
    void UpdateTimerState();
    void FetchAndDraw();
    void BuildDetectorTab(TGCompositeFrame* tab, int group);
    void ActivateDetectorControls(int group);
    void BuildServerConnectionsTab(TGCompositeFrame* tab);
    void ConnectConfiguredServer(std::size_t index);
    void SetServerConnected(std::size_t index, bool connected);
    void UpdateConfiguredServerButton(std::size_t index);
    void UpdateServerConnectionsSummary();
    void SyncConfiguredServerStates();

    struct DetectorControls {
        TGComboBox* server = nullptr;
        TGComboBox* fieldCage = nullptr;
        TGComboBox* channel = nullptr;
        TGComboBox* quantity = nullptr;
        TGTextEntry* histogram = nullptr;
        TGTextButton* draw = nullptr;
        TGTextButton* clear = nullptr;
        TGTextButton* clearAll = nullptr;
        TGCheckButton* autoUpdate = nullptr;
        TGNumberEntry* interval = nullptr;
    };
    std::array<DetectorControls, 3> fDetectorControls; //!
    TGTab* fTabs = nullptr;

    TGComboBox* fServerCombo = nullptr;
    int fSelectedServer = -1;
    TGComboBox* fFieldCageCombo = nullptr;
    TGComboBox* fAdcCombo = nullptr;
    TGComboBox* fQuantityCombo = nullptr;
    TGTextEntry* fHistogramEntry = nullptr;
    TGTextButton* fDrawButton = nullptr;
    TGTextButton* fClearButton = nullptr;
    TGTextButton* fClearAllButton = nullptr;
    TGTextButton* fCloseClientButton = nullptr;
    TGTextButton* fCloseServerButton = nullptr;
    TGTextButton* fCloseAllButton = nullptr;
    TGTextButton* fGeneralStatusButton = nullptr;
    TGTextButton* fRefreshServerConnectionsButton = nullptr;
    std::array<TGComboBox*, 3> fMacroCombos{}; //!
    TGCheckButton* fAutoUpdateCheck = nullptr;
    TGNumberEntry* fUpdateIntervalEntry = nullptr;
    TGLabel* fGeneralStatusLabel = nullptr;
    TGLabel* fServerConnectionsSummaryLabel = nullptr;

    std::vector<std::string> fServerTypes;
    std::vector<std::string> fServerInstances;
    std::vector<std::string> fServerHosts;
    std::vector<int> fServerPorts;
    std::vector<int> fServerStates;
    std::vector<TGTextButton*> fServerButtons;
    std::array<std::vector<std::string>, 3> fMacroPaths;

    std::string fConnectedDetectorType = "MUSIC";
    std::string fConnectedInstance = "MUSIC1";
    std::vector<std::shared_ptr<TSuFDeMonClient>> fServerClients; //!
    std::shared_ptr<TSuFDeMonClient> fClient; //!
    std::unique_ptr<TH1D> fHistogram;
    std::vector<std::unique_ptr<TH1D>> fPromptHistograms; //!
    TCanvas* fCanvas = nullptr;
    TTimer* fUpdateTimer = nullptr;
    TTimer* fConnectionTimer = nullptr;
    bool fHasDrawnHistogram = false;
    std::string fLegacyHistogramPrefix;

    ClassDefOverride(TSuFDeMonGui, 0);
};

#endif
