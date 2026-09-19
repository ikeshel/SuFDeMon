#include "TSupFDetMonGui.h"

#include "SupFDetMonNames.h"
#include "TSupFDetMonClient.h"

#include <TCanvas.h>
#include <TGButton.h>
#include <TGComboBox.h>
#include <TGFrame.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGTextEntry.h>
#include <TH1D.h>
#include <TTimer.h>

#include <algorithm>
#include <cmath>
#include <string>

ClassImp(TSupFDetMonGui)

TSupFDetMonGui::TSupFDetMonGui(const TGWindow* parent, UInt_t width, UInt_t height,
                               std::string host, int port)
    : TGMainFrame(parent, width, height)
{
    SetCleanup(kDeepCleanup);
    SetWindowName("SupFDetMon Controls");

    // Connection controls. Put the widgets in a vertical wrapper so the
    // complete Host/Port/button/status row is centered inside the group box.
    auto* connection = new TGGroupFrame(this, "Connection", kVerticalFrame);
    auto* connectionRow = new TGHorizontalFrame(connection);

    fHostEntry = new TGTextEntry(connectionRow, host.c_str());
    fHostEntry->Resize(180, 28);
    fPortEntry = new TGNumberEntry(connectionRow, port, 6, -1, TGNumberFormat::kNESInteger,
                                   TGNumberFormat::kNEANonNegative,
                                   TGNumberFormat::kNELLimitMinMax, 1, 65535);
    fConnectButton = new TGTextButton(connectionRow, "&Connect");
    fDisconnectButton = new TGTextButton(connectionRow, "&Disconnect");
    fStatusLabel = new TGLabel(connectionRow, "Disconnected");

    connectionRow->AddFrame(new TGLabel(connectionRow, "Host:"),
                            new TGLayoutHints(kLHintsCenterY, 5, 4, 0, 0));
    connectionRow->AddFrame(fHostEntry,
                            new TGLayoutHints(kLHintsCenterY, 0, 5, 0, 0));
    connectionRow->AddFrame(new TGLabel(connectionRow, "Port:"),
                            new TGLayoutHints(kLHintsCenterY, 0, 4, 0, 0));
    connectionRow->AddFrame(fPortEntry,
                            new TGLayoutHints(kLHintsCenterY, 0, 15, 0, 0));
    connectionRow->AddFrame(fConnectButton,
                            new TGLayoutHints(kLHintsCenterY, 0, 8, 0, 0));
    connectionRow->AddFrame(fDisconnectButton,
                            new TGLayoutHints(kLHintsCenterY, 0, 20, 0, 0));
    connectionRow->AddFrame(fStatusLabel,
                            new TGLayoutHints(kLHintsCenterY, 0, 5, 0, 0));

    connection->AddFrame(connectionRow,
                         new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 0, 0, 12, 12));
    AddFrame(connection, new TGLayoutHints(kLHintsExpandX, 8, 8, 8, 4));

    // Histogram selection controls
    auto* controls = new TGGroupFrame(this, "Select Histogram", kVerticalFrame);

    auto* fcRow = new TGHorizontalFrame(controls);
    fcRow->AddFrame(new TGLabel(fcRow, "Field Cage:"), new TGLayoutHints(kLHintsCenterY, 2, 8, 4, 4));
    fFieldCageCombo = new TGComboBox(fcRow);
    for (int fc = 1; fc <= SupFDetMon::kNFieldCages; ++fc)
        fFieldCageCombo->AddEntry(("FC" + std::to_string(fc)).c_str(), fc);
    fFieldCageCombo->Select(1);
    fFieldCageCombo->Resize(160, 24);
    fcRow->AddFrame(fFieldCageCombo, new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));
    controls->AddFrame(fcRow, new TGLayoutHints(kLHintsExpandX));

    auto* adcRow = new TGHorizontalFrame(controls);
    adcRow->AddFrame(new TGLabel(adcRow, "ADC Channel:"), new TGLayoutHints(kLHintsCenterY, 2, 8, 4, 4));
    fAdcCombo = new TGComboBox(adcRow);
    for (int adc = 0; adc < SupFDetMon::kNAdcChannels; ++adc)
        fAdcCombo->AddEntry(("ADC" + std::to_string(adc)).c_str(), adc);
    fAdcCombo->Select(0);
    fAdcCombo->Resize(160, 24);
    adcRow->AddFrame(fAdcCombo, new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));
    controls->AddFrame(adcRow, new TGLayoutHints(kLHintsExpandX));

    controls->AddFrame(new TGLabel(controls, "Histogram name:"), new TGLayoutHints(kLHintsLeft, 2, 2, 12, 2));
    fHistogramEntry = new TGTextEntry(controls);
    fHistogramEntry->SetEnabled(kFALSE);
    controls->AddFrame(fHistogramEntry, new TGLayoutHints(kLHintsExpandX, 2, 2, 2, 10));

    auto* updateRow = new TGHorizontalFrame(controls);
    fAutoUpdateCheck = new TGCheckButton(updateRow, "Auto update");
    fAutoUpdateCheck->SetState(kButtonDown);
    fUpdateIntervalEntry = new TGNumberEntry(updateRow, 1.0, 6, -1,
                                             TGNumberFormat::kNESRealTwo,
                                             TGNumberFormat::kNEAPositive,
                                             TGNumberFormat::kNELLimitMin,
                                             0.05);
    updateRow->AddFrame(fAutoUpdateCheck, new TGLayoutHints(kLHintsCenterY, 2, 16, 5, 5));
    updateRow->AddFrame(new TGLabel(updateRow, "Interval [s]:"), new TGLayoutHints(kLHintsCenterY, 2, 6, 5, 5));
    updateRow->AddFrame(fUpdateIntervalEntry, new TGLayoutHints(kLHintsCenterY, 2, 2, 5, 5));
    controls->AddFrame(updateRow, new TGLayoutHints(kLHintsExpandX));

    fDrawButton = new TGTextButton(controls, "&Draw");
    fClearButton = new TGTextButton(controls, "C&lear");
    fClearAllButton = new TGTextButton(controls, "Clear &All");
    controls->AddFrame(fDrawButton, new TGLayoutHints(kLHintsExpandX, 2, 2, 8, 4));
    controls->AddFrame(fClearButton, new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));
    controls->AddFrame(fClearAllButton, new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 8));
    AddFrame(controls, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 8, 8, 4, 8));

    // TTimer emits Timeout() in the ROOT event loop. It is single-shot here and
    // restarted after every refresh, so changing the interval takes effect cleanly.
    fUpdateTimer = new TTimer(1000, kTRUE);

    fConnectButton->Connect("Clicked()", "TSupFDetMonGui", this, "ConnectServer()");
    fDisconnectButton->Connect("Clicked()", "TSupFDetMonGui", this, "DisconnectServer()");
    fFieldCageCombo->Connect("Selected(Int_t)", "TSupFDetMonGui", this, "SelectionChanged(Int_t)");
    fAdcCombo->Connect("Selected(Int_t)", "TSupFDetMonGui", this, "SelectionChanged(Int_t)");
    fDrawButton->Connect("Clicked()", "TSupFDetMonGui", this, "DrawSelected()");
    fClearButton->Connect("Clicked()", "TSupFDetMonGui", this, "ClearSelected()");
    fClearAllButton->Connect("Clicked()", "TSupFDetMonGui", this, "ClearAllHistograms()");
    fAutoUpdateCheck->Connect("Toggled(Bool_t)", "TSupFDetMonGui", this, "AutoUpdateToggled()");
    fUpdateIntervalEntry->Connect("ValueSet(Long_t)", "TSupFDetMonGui", this, "UpdateIntervalChanged()");
    fUpdateTimer->Connect("Timeout()", "TSupFDetMonGui", this, "AutoUpdate()");

    UpdateHistogramName();
    SetConnectedUi(false);

    MapSubwindows();
    Resize(GetDefaultSize());
    MapWindow();

    // Try the host/port supplied on the command line immediately at startup.
    // If the server is unavailable, the GUI remains open and the user can
    // retry later with the Connect button.
    ConnectServer();
}

TSupFDetMonGui::~TSupFDetMonGui()
{
    if (fUpdateTimer) {
        fUpdateTimer->TurnOff();
        delete fUpdateTimer;
        fUpdateTimer = nullptr;
    }
    if (fClient) fClient->Disconnect();
}

std::string TSupFDetMonGui::SelectedHistogramName() const
{
    return SupFDetMon::MusicAdcHistogramName(fFieldCageCombo->GetSelected(), fAdcCombo->GetSelected());
}

void TSupFDetMonGui::UpdateHistogramName()
{
    fHistogramEntry->SetText(SelectedHistogramName().c_str());
}

void TSupFDetMonGui::SetConnectedUi(bool connected)
{
    fConnectButton->SetEnabled(!connected);
    fDisconnectButton->SetEnabled(connected);
    fDrawButton->SetEnabled(connected);
    fClearButton->SetEnabled(connected);
    fClearAllButton->SetEnabled(connected);
    fStatusLabel->SetText(connected ? "Connected" : "Disconnected");
    if (!connected && fUpdateTimer) fUpdateTimer->TurnOff();
    Layout();
}

void TSupFDetMonGui::ConnectServer()
{
    if (fClient) fClient->Disconnect();
    fClient = std::make_unique<TSupFDetMonClient>(fHostEntry->GetText(),
                                                  static_cast<int>(fPortEntry->GetNumber()));
    const bool connected = fClient->Connect();
    if (!connected) fClient.reset();
    SetConnectedUi(connected);
    UpdateTimerState();
}

void TSupFDetMonGui::DisconnectServer()
{
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    if (fClient) {
        fClient->Disconnect();
        fClient.reset();
    }
    SetConnectedUi(false);
}

void TSupFDetMonGui::SelectionChanged(Int_t)
{
    UpdateHistogramName();
}

void TSupFDetMonGui::FetchAndDraw()
{
    if (!fClient || !fClient->IsConnected()) return;

    auto histogram = fClient->GetHistogram(SelectedHistogramName());
    if (!histogram) return;

    // Keep the existing canvas window; only replace the received snapshot.
    fHistogram = std::move(histogram);

    if (!fCanvas)
        fCanvas = new TCanvas("SupFDetMonCanvas", "SupFDetMon Histogram", 1000, 700);

    fCanvas->cd();
    fCanvas->Clear();
    fHistogram->Draw();
    fCanvas->Modified();
    fCanvas->Update();
    fHasDrawnHistogram = true;
}

void TSupFDetMonGui::DrawSelected()
{
    FetchAndDraw();
    UpdateTimerState();
}

void TSupFDetMonGui::ClearSelected()
{
    if (!fClient || !fClient->IsConnected()) return;
    if (fClient->ClearHistogram(SelectedHistogramName()))
        FetchAndDraw();
    UpdateTimerState();
}

void TSupFDetMonGui::ClearAllHistograms()
{
    if (!fClient || !fClient->IsConnected()) return;
    if (fClient->ClearAll())
        FetchAndDraw();
    UpdateTimerState();
}

void TSupFDetMonGui::AutoUpdateToggled()
{
    UpdateTimerState();
}

void TSupFDetMonGui::UpdateIntervalChanged()
{
    UpdateTimerState();
}

void TSupFDetMonGui::UpdateTimerState()
{
    if (!fUpdateTimer) return;
    fUpdateTimer->TurnOff();

    const bool enabled = fAutoUpdateCheck && fAutoUpdateCheck->IsOn();
    const bool connected = fClient && fClient->IsConnected();
    if (!enabled || !connected || !fHasDrawnHistogram) return;

    const double seconds = std::max(0.05, fUpdateIntervalEntry->GetNumber());
    const Long_t milliseconds = static_cast<Long_t>(std::lround(seconds * 1000.0));
    fUpdateTimer->Start(milliseconds, kTRUE);
}

void TSupFDetMonGui::AutoUpdate()
{
    if (!fAutoUpdateCheck || !fAutoUpdateCheck->IsOn()) return;
    if (!fClient || !fClient->IsConnected() || !fHasDrawnHistogram) return;

    FetchAndDraw();
    UpdateTimerState();
}

void TSupFDetMonGui::CloseWindow()
{
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    DisconnectServer();
    DeleteWindow();
}
