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

#include <memory>
#include <string>

class TGTextEntry;
class TGNumberEntry;
class TGComboBox;
class TGTextButton;
class TGCheckButton;
class TGLabel;
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
    void DrawSelected();
    void ClearSelected();
    void ClearAllHistograms();
    void AutoUpdateToggled();
    void UpdateIntervalChanged();
    void UpdateIntervalArrow(Long_t value);
    void AutoUpdate();
    void CheckConnection();
    void CloseClient();
    void CloseServer();
    void CloseAll();
    void CloseWindow() override;

    TSuFDeMonClient* GetClient() const { return fClient.get(); }
    TH1D* GetHistogram() const { return fHistogram.get(); }

private:
    std::string SelectedHistogramName() const;
    void UpdateHistogramName();
    void SetConnectedUi(bool connected);
    void UpdateTimerState();
    void FetchAndDraw();

    TGTextEntry* fHostEntry = nullptr;
    TGNumberEntry* fPortEntry = nullptr;
    TGComboBox* fFieldCageCombo = nullptr;
    TGComboBox* fAdcCombo = nullptr;
    TGTextEntry* fHistogramEntry = nullptr;
    TGTextButton* fConnectButton = nullptr;
    TGTextButton* fDisconnectButton = nullptr;
    TGTextButton* fDrawButton = nullptr;
    TGTextButton* fClearButton = nullptr;
    TGTextButton* fClearAllButton = nullptr;
    TGTextButton* fCloseClientButton = nullptr;
    TGTextButton* fCloseServerButton = nullptr;
    TGTextButton* fCloseAllButton = nullptr;
    TGCheckButton* fAutoUpdateCheck = nullptr;
    TGNumberEntry* fUpdateIntervalEntry = nullptr;
    TGLabel* fStatusLabel = nullptr;

    std::unique_ptr<TSuFDeMonClient> fClient;
    std::unique_ptr<TH1D> fHistogram;
    TCanvas* fCanvas = nullptr;
    TTimer* fUpdateTimer = nullptr;
    TTimer* fConnectionTimer = nullptr;
    bool fHasDrawnHistogram = false;

    ClassDefOverride(TSuFDeMonGui, 0);
};

#endif

