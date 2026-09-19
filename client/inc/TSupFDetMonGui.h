#ifndef TSUPFDETMONGUI_H
#define TSUPFDETMONGUI_H

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
class TSupFDetMonClient;

class TSupFDetMonGui : public TGMainFrame
{
public:
    TSupFDetMonGui(const TGWindow* parent, UInt_t width, UInt_t height,
                   std::string host = "localhost", int port = 9090);
    ~TSupFDetMonGui() override;

    void ConnectServer();
    void DisconnectServer();
    void SelectionChanged(Int_t id);
    void DrawSelected();
    void ClearSelected();
    void ClearAllHistograms();
    void AutoUpdateToggled();
    void UpdateIntervalChanged();
    void IncreaseUpdateInterval();
    void DecreaseUpdateInterval();
    void AutoUpdate();
    void CloseClient();
    void CloseServer();
    void CloseAll();
    void CloseWindow() override;

    TSupFDetMonClient* GetClient() const { return fClient.get(); }
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

    std::unique_ptr<TSupFDetMonClient> fClient;
    std::unique_ptr<TH1D> fHistogram;
    TCanvas* fCanvas = nullptr;
    TTimer* fUpdateTimer = nullptr;
    bool fHasDrawnHistogram = false;

    ClassDefOverride(TSupFDetMonGui, 0);
};

#endif
