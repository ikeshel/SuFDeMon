#include "TSupFDetMonGui.h"

#include "SupFDetMonNames.h"
#include "TSupFDetMonClient.h"

#include <TCanvas.h>
#include <TApplication.h>
#include <TGButton.h>
#include <TGComboBox.h>
#include <TGFrame.h>
#include <TGLabel.h>
#include <TGClient.h>
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
                                             TGNumberFormat::kNESRealOne,
                                             TGNumberFormat::kNEAPositive,
                                             TGNumberFormat::kNELLimitMin,
                                             0.2);
    updateRow->AddFrame(fAutoUpdateCheck, new TGLayoutHints(kLHintsCenterY, 2, 16, 5, 5));
    updateRow->AddFrame(new TGLabel(updateRow, "Interval [s]:"), new TGLayoutHints(kLHintsCenterY, 2, 6, 5, 5));
    updateRow->AddFrame(fUpdateIntervalEntry, new TGLayoutHints(kLHintsCenterY, 2, 2, 5, 5));
    controls->AddFrame(updateRow, new TGLayoutHints(kLHintsExpandX));

    auto* drawRow = new TGHorizontalFrame(controls);
    // ROOT's default TGTextButton font is not reliably UTF-8 capable.
    // Keep button labels ASCII-only so they render correctly on all desktops.
    fDrawButton = new TGTextButton(drawRow, "&Draw");
    fClearButton = new TGTextButton(drawRow, "C&lear");
    fClearAllButton = new TGTextButton(drawRow, "Clear &All");
    drawRow->AddFrame(fDrawButton, new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 2, 4, 6, 6));
    drawRow->AddFrame(fClearButton, new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 4, 6, 6));
    drawRow->AddFrame(fClearAllButton, new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 2, 6, 6));
    controls->AddFrame(drawRow, new TGLayoutHints(kLHintsExpandX, 0, 0, 2, 4));

    AddFrame(controls, new TGLayoutHints(kLHintsExpandX, 8, 8, 4, 4));

    // Process controls: use a vertical group with one horizontal row so the
    // buttons are vertically centered inside the group, like Connection.
    auto* processControls = new TGGroupFrame(this, "Process Control", kVerticalFrame);
    auto* processRow = new TGHorizontalFrame(processControls);
    fCloseClientButton = new TGTextButton(processRow, "Close client");
    fCloseServerButton = new TGTextButton(processRow, "Close server");
    fCloseAllButton = new TGTextButton(processRow, "Close All");
    processRow->AddFrame(fCloseClientButton, new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 4, 0, 0));
    processRow->AddFrame(fCloseServerButton, new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 4, 0, 0));
    processRow->AddFrame(fCloseAllButton, new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 4, 0, 0));
    processControls->AddFrame(processRow,
                              new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 0, 0, 12, 12));
    AddFrame(processControls, new TGLayoutHints(kLHintsExpandX, 8, 8, 4, 8));

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
    fCloseClientButton->Connect("Clicked()", "TSupFDetMonGui", this, "CloseClient()");
    fCloseServerButton->Connect("Clicked()", "TSupFDetMonGui", this, "CloseServer()");
    fCloseAllButton->Connect("Clicked()", "TSupFDetMonGui", this, "CloseAll()");
    fAutoUpdateCheck->Connect("Toggled(Bool_t)", "TSupFDetMonGui", this, "AutoUpdateToggled()");
    // Route the arrow buttons to the parent instead of letting TGNumberEntry
    // apply its built-in 0.01 step. ProcessMessage() receives +1/-1 and the
    // ValueChanged signal below turns that into an exact 0.2 s step.
    fUpdateIntervalEntry->SetButtonToNum(kTRUE);
    fUpdateIntervalEntry->Connect("ValueChanged(Long_t)", "TSupFDetMonGui", this, "UpdateIntervalArrow(Long_t)");
    fUpdateIntervalEntry->Connect("ValueSet(Long_t)", "TSupFDetMonGui", this, "UpdateIntervalChanged()");
    fUpdateTimer->Connect("Timeout()", "TSupFDetMonGui", this, "AutoUpdate()");

    UpdateHistogramName();
    SetConnectedUi(false);

    MapSubwindows();
    // Size after every group has been mapped so ROOT includes Connection,
    // Select Histogram (including Draw/Clear/Clear All), and Process Control.
    const TGDimension defaultSize = GetDefaultSize();
    const UInt_t windowWidth = std::max<UInt_t>(620, defaultSize.fWidth);
    const UInt_t windowHeight = std::max<UInt_t>(500, defaultSize.fHeight);
    Resize(windowWidth, windowHeight);
    MapWindow();

    // Try the host/port supplied on the command line immediately at startup.
    // If the server is unavailable, the GUI remains open and the user can
    // retry later with the Connect button.
    ConnectServer();

    // Start with the currently selected histogram already drawn.  With
    // Auto update enabled by default, this also starts periodic refreshes.
    if (fClient && fClient->IsConnected())
        DrawSelected();
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

    // Make the connection state immediately visible: green when connected,
    // red when disconnected.
    Pixel_t statusColor = 0;
    gClient->GetColorByName(connected ? "green" : "red", statusColor);
    fStatusLabel->SetTextColor(statusColor);

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
    if (!fUpdateIntervalEntry) return;

    // Manual input is allowed, but keep the timer at or above 0.2 s.
    if (fUpdateIntervalEntry->GetNumber() < 0.2)
        fUpdateIntervalEntry->SetNumber(0.2, kFALSE);
    UpdateTimerState();
}

void TSupFDetMonGui::UpdateIntervalArrow(Long_t value)
{
    if (!fUpdateIntervalEntry || value == 0) return;

    const double current = fUpdateIntervalEntry->GetNumber();
    const double direction = value > 0 ? 1.0 : -1.0;
    const double stepped = std::max(0.2, current + direction * 0.2);
    fUpdateIntervalEntry->SetNumber(std::round(stepped * 5.0) / 5.0, kFALSE);
    UpdateTimerState();
}

void TSupFDetMonGui::UpdateTimerState()
{
    if (!fUpdateTimer) return;
    fUpdateTimer->TurnOff();

    const bool enabled = fAutoUpdateCheck && fAutoUpdateCheck->IsOn();
    const bool connected = fClient && fClient->IsConnected();
    if (!enabled || !connected || !fHasDrawnHistogram) return;

    const double seconds = std::max(0.2, fUpdateIntervalEntry->GetNumber());
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

void TSupFDetMonGui::CloseClient()
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

void TSupFDetMonGui::CloseServer()
{
    if (!fClient || !fClient->IsConnected()) return;
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    fClient->ShutdownServer();
    fClient.reset();
    SetConnectedUi(false);
}

void TSupFDetMonGui::CloseAll()
{
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    if (fClient && fClient->IsConnected()) fClient->ShutdownServer();
    fClient.reset();
    if (fCanvas) {
        fCanvas->Close();
        fCanvas = nullptr;
    }
    DeleteWindow();
    if (gApplication) gApplication->Terminate(0);
}

void TSupFDetMonGui::CloseWindow()
{
    if (fUpdateTimer) fUpdateTimer->TurnOff();
    DisconnectServer();
    DeleteWindow();
}
